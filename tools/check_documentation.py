"""Check declaration documentation in project-owned Vala and Python sources."""

import argparse
import ast
from pathlib import Path
import re


TOKEN = re.compile(
    r'/\*\*.*?\*/|/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|'
    r"'(?:\\.|[^'\\])*'|[A-Za-z_]\w*|[^\s]",
    re.DOTALL,
)


def scan_vala(path: Path) -> list[tuple[str, int, bool]]:
    """Scan declaration scopes, ignoring strings, comments, attributes and method bodies.

    This small lexical checker covers the declarations used by this repository;
    compiler/Valadoc validation remains necessary for syntax and comment quality.
    """
    source = path.read_text(encoding="utf-8")
    tokens = list(TOKEN.finditer(source))
    results = []
    scopes = ["namespace"]
    declaration = []
    documented = False
    body_depth = 0
    bracket_depth = 0
    paren_depth = 0
    for match in tokens:
        token = match.group()
        if body_depth:
            if token == "{":
                body_depth += 1
            elif token == "}":
                body_depth -= 1
            continue
        if token.startswith("/**"):
            documented = bool(token[3:-2].strip("* \n\t"))
            continue
        if token.startswith(("//", "/*")):
            continue
        if token == "[" and not declaration:
            bracket_depth = 1
            continue
        if bracket_depth:
            if token == "[":
                bracket_depth += 1
            elif token == "]":
                bracket_depth -= 1
            continue
        if token == "(" and declaration:
            paren_depth += 1
        elif token == ")" and paren_depth:
            paren_depth -= 1
        if token in ("{", ";") and not paren_depth:
            if declaration:
                words = [item.group() for item in declaration]
                line = source.count("\n", 0, declaration[0].start()) + 1
                if words[0] == "namespace":
                    scopes.append("namespace")
                elif words[0] != "using":
                    results.append((" ".join(words), line, documented))
                    if token == "{":
                        if any(word in words for word in (
                            "class", "struct", "interface", "enum", "errordomain"
                        )) and "(" not in words:
                            scopes.append("type")
                        else:
                            body_depth = 1
                declaration = []
            documented = False
        elif token == "}" and not paren_depth:
            if declaration or len(scopes) == 1:
                raise ValueError(f"Unsupported declaration syntax in {path}")
            scopes.pop()
            documented = False
        else:
            declaration.append(match)
    if declaration or len(scopes) != 1 or body_depth or bracket_depth or paren_depth:
        raise ValueError(f"Unbalanced declaration scopes in {path}")
    return results


def scan_python(path: Path) -> list[tuple[str, int, bool]]:
    """Use the Python syntax tree to locate modules, types and callable docstrings."""
    tree = ast.parse(path.read_text(encoding="utf-8"), filename=str(path))
    nodes = (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)
    return [
        (getattr(node, "name", "module"), getattr(node, "lineno", 1),
         bool(ast.get_docstring(node)))
        for node in ast.walk(tree) if isinstance(node, nodes)
    ]


def main() -> int:
    """Report documented declarations and exit nonzero for missing summaries."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("root", type=Path)
    root = parser.parse_args().root.resolve()
    files = sorted((root / "src").rglob("*.vala"))
    files += sorted((root / "src").rglob("*.vala.in"))
    for directory in ("tools", "tests"):
        files += sorted((root / directory).rglob("*.py"))
    total = missing = 0
    for path in files:
        scanner = scan_python if path.suffix == ".py" else scan_vala
        declarations = scanner(path)
        if not declarations:
            raise ValueError(f"No declarations found in {path}")
        total += len(declarations)
        absent = [(name, line) for name, line, present in declarations if not present]
        missing += len(absent)
        print(f"{path.relative_to(root)}: {len(declarations) - len(absent)}/"
              f"{len(declarations)} documented")
        for name, line in absent:
            print(f"  Missing line {line}: {name}")
    print(f"Documentation: {total - missing}/{total} declarations")
    return int(missing != 0 or total == 0)


if __name__ == "__main__":
    raise SystemExit(main())
