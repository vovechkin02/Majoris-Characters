# Architecture

## Stage 1 boundaries

`main.vala` initializes gettext and delegates to `Characters.Application`.
The application owns the GTK/GApplication lifecycle, actions and current window.
It uses the framework's single-instance session bus registration, not an
application-defined singleton or service locator. `MainWindow` owns its GSettings
instance, restores the normal requested logical dimensions on construction, and
saves state when closing. All current application and UI operations run on the
GTK main context. There are no worker threads or asynchronous jobs in Stage 1.

The main window template and icon are compiled into the executable as GResources.
The icon, desktop entry, AppStream metadata, GSettings schema and D-Bus service
are also installed into the corresponding standard data directories. GApplication
uses the reverse-DNS application ID for its default resource base path and D-Bus
identity. The Meson project version is the source of the runtime version string.
Desktop/AppStream and Debian release metadata are maintained together per release.

No domain engine is included yet. Stage 2 owns the Unicode engine and database
adoption. The existing application must not acquire dependencies on database
internals; future views consume domain services as they become necessary.

## GoF applicability and lifecycle

| Area | Decision | Ownership and rationale |
| --- | --- | --- |
| Application activation | Framework lifecycle, not GoF Singleton | GApplication provides one primary process per session bus. No global mutable instance accessor. |
| Actions | Native GAction, command-like idiom | Application owns actions; simple immediate callbacks, no custom Command hierarchy or undo queue. |
| Window construction | Direct constructor | One fixed XML template; no factory or builder abstraction is warranted. |
| Settings | Direct GSettings composition | Window owns its settings object. No alternate storage backends are required at this stage. |
| Signals | Framework observer-like idiom | Actions and receivers share application lifetime and execute on the GTK main context. No independent event bus. |

There are no claimed custom GoF pattern instances at this stage. The architecture
favors composition and native framework extension points. Revisit pattern
applicability when real backend or algorithm variation is introduced.

## Compatibility surfaces

- Application and D-Bus name: `com.majoris.characters`.
- Executable and package: `majoris-characters`.
- GSettings path: `/com/majoris/characters/`.
- Keys: `window-width` and `window-height` (integer logical pixels, range
  320–8192, default 960×720), `window-maximized` (boolean, default false).
- Actions: `app.about` and `app.quit`.
- Resource path: `/com/majoris/characters/ui/main-window.ui`.

Window geometry is a GTK logical-size request, not a guarantee of compositor
allocation. The window manager controls placement. The standard normal-close
path persists settings; an abrupt process termination need not save changes.
The GUI integration test uses 800×600 under Xvfb; it verifies stored logical
settings, not physical allocations or native Wayland behavior.
