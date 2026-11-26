"""Verify local collections across restarts and concurrent picker processes."""

import fcntl
import os
from pathlib import Path
import stat
import subprocess
import sys
import tempfile
import time


PROCESS_TIMEOUT = 30
WORKER_COUNT = 4


def run(executable: Path, option: str) -> None:
    """Run one checked repository subprocess using the isolated environment."""
    subprocess.run([str(executable), option], check=True, timeout=PROCESS_TIMEOUT)


def wait_ready(processes: list[subprocess.Popen], markers: list[Path]) -> None:
    """Wait until every worker has opened and read its original settings snapshot."""
    deadline = time.monotonic() + 10
    while not all(marker.exists() for marker in markers):
        if any(process.poll() is not None for process in processes):
            raise RuntimeError("A collection worker exited before the barrier")
        if time.monotonic() >= deadline:
            raise TimeoutError("Collection workers did not reach the barrier")
        time.sleep(0.01)


def finish(processes: list[subprocess.Popen]) -> None:
    """Require successful workers and terminate any survivors if a test fails."""
    for process in processes:
        if process.wait(timeout=PROCESS_TIMEOUT) != 0:
            raise RuntimeError(f"Collection worker failed: {process.returncode}")


def stop(processes: list[subprocess.Popen]) -> None:
    """Always reap children, even if their barrier or an assertion fails."""
    for process in processes:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=5)


def concurrent_writes(executable: Path, directory: Path) -> None:
    """Force stale initial readers to race additions, removals and shared increments."""
    run(executable, "--prepare-concurrent")
    markers = [directory / f"ready-{index}" for index in range(WORKER_COUNT)]
    start = directory / "start"
    processes = []
    try:
        for index, marker in enumerate(markers):
            processes.append(subprocess.Popen([
                str(executable), "--worker", str(index), str(marker), str(start)
            ]))
        wait_ready(processes, markers)
        start.touch()
        finish(processes)
    finally:
        stop(processes)
    run(executable, "--verify-concurrent")


def privacy_race(executable: Path, directory: Path) -> None:
    """Disable recording before releasing a writer with an old enabled snapshot."""
    ready = directory / "privacy-ready"
    start = directory / "privacy-start"
    process = subprocess.Popen([
        str(executable), "--stale-writer", str(ready), str(start)
    ])
    try:
        wait_ready([process], [ready])
        run(executable, "--disable")
        start.touch()
        finish([process])
    finally:
        stop([process])
    run(executable, "--verify-disabled")
    run(executable, "--reenable")
    run(executable, "--verify-trimmed")


def hold_lock(lock_file: Path, ready: Path) -> None:
    """Simulate a process dying while holding the same advisory descriptor."""
    with lock_file.open("r+b") as descriptor:
        fcntl.flock(descriptor, fcntl.LOCK_EX)
        ready.touch()
        time.sleep(60)


def lock_contention(executable: Path, directory: Path) -> None:
    """Check bounded rejection, then automatic lock release without deleting its inode."""
    lock_file = directory / "majoris-characters" / "collections.lock"
    assert stat.S_IMODE(lock_file.stat().st_mode) == 0o600
    with lock_file.open("r+b") as descriptor:
        fcntl.flock(descriptor, fcntl.LOCK_EX)
        run(executable, "--expect-lock-failure")
    # A killed lock owner must not leave a permanently locked collection.
    ready = directory / "lock-owner-ready"
    process = subprocess.Popen([
        sys.executable, str(Path(__file__).resolve()), "--hold-lock",
        str(lock_file), str(ready),
    ])
    try:
        wait_ready([process], [ready])
    finally:
        stop([process])
    run(executable, "--verify-trimmed")
    # A new mutation must succeed after the competing descriptor is closed.
    run(executable, "--disable")
    run(executable, "--verify-disabled")


def scenario(executable: Path, directory: Path) -> None:
    """Run the same production repository checks against one actual backend."""
    for option in ("--seed", "--verify"):
        run(executable, option)
    concurrent_writes(executable, directory)
    privacy_race(executable, directory)
    lock_contention(executable, directory)


def main() -> int:
    """Isolate both keyfile and dconf tests, including the dconf activation environment."""
    if len(sys.argv) == 4 and sys.argv[1] == "--hold-lock":
        hold_lock(Path(sys.argv[2]), Path(sys.argv[3]))
        return 0
    executable = Path(sys.argv[1]).resolve()
    if len(sys.argv) == 4 and sys.argv[2] == "--scenario":
        scenario(executable, Path(sys.argv[3]))
        return 0
    for backend in ("keyfile", "dconf"):
        with tempfile.TemporaryDirectory(prefix="characters-library-") as temporary:
            directory = Path(temporary)
            runtime = directory / "runtime"
            runtime.mkdir(mode=0o700)
            environment = os.environ.copy()
            environment.update(
                HOME=temporary,
                XDG_CONFIG_HOME=temporary,
                XDG_RUNTIME_DIR=str(runtime),
                GSETTINGS_BACKEND=backend,
                G_DEBUG="fatal-warnings",
            )
            environment.pop("DCONF_PROFILE", None)
            command = [sys.executable, str(Path(__file__).resolve()),
                       str(executable), "--scenario", temporary]
            if backend == "dconf":
                command = ["dbus-run-session", "--", *command]
            subprocess.run(command, env=environment, check=True, timeout=90)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
