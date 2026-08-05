# Unturned Command Generator

A cross-platform desktop tool that builds Unturned server commands in either
**server-terminal** or **in-game chat** format, with searchable lookup tables, item annotations, and an optional RCON/TCP bridge
to send commands directly to your server.

Built with **C++17 + Qt 6** and licensed under the **MIT License**.

---

## Features

- **59 commands** defined in `assets/commands.json`, driving a dynamic parameter
  form (player, item, vehicle, location, enum, boolean, integer, float, color,
  duration, map, GUID, ...).
- **Three output formats** with live preview and one-click copy:
  - Server terminal: `teleport Steve/Seattle`
  - In-game chat: `/teleport Steve/Seattle`
  - In-game chat: `@teleport Steve/Seattle`
- **Multi-type parameters** (e.g. Teleport target = player *or* map location;
  Weather = preset *or* custom GUID) via a mode selector.
- **Lookup tables** with searchable, editable dropdowns for items, vehicles,
  animals, effects, quests, achievements and skillsets, plus a user-maintained
  player table.
- **In-app CRUD** for every table from the `Data` menu; edits persist back to
  the source JSON files.
- **Import / Export JSON** with merge-or-replace and a built-in format tutorial.
- **Language switching**  from the `Language` menu, persisted
  across restarts.
- **Optional RCON/TCP bridge (Experimental) ** to send generated commands to the server.

---

## Requirements

- CMake >= 3.16
- A C++17 compiler (GCC/Clang/MSVC)
- Qt 6 development libraries: `Core`, `Widgets`, `Network`
  (`qt6-base-dev` on Debian/Ubuntu)

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/unturnedCmdGen
```

The `assets/` and `translations/` directories are copied next to the
executable automatically.

## Windows packaging

Build a portable executable and an **NSIS installer** (`.exe`) with
`build.ps1`:

```powershell
.\build.ps1
```

Run it from any PowerShell — the script auto-detects and sets up the MSVC
environment (`vcvars64.bat`), locates CMake/Ninja/NSIS and Qt under
`C:\Qt`, so no Developer PowerShell is required.

Prerequisites (Windows 10/11):

- [CMake](https://cmake.org/download/) >= 3.16
- MSVC toolchain — [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022)
  with the "Desktop development with C++" workload
- [Ninja](https://ninja-build.org/) (optional; falls back to the Visual
  Studio generator)
- [NSIS](https://nsis.sourceforge.io/) 3.x — required for the installer
- Qt 6 MSVC kit installed under `C:\Qt` (e.g. via the Qt Online Installer:
  **Qt 6.7.x → MSVC 2019 64-bit**, which ships `windeployqt.exe`)

What it produces in `build-win/`:

- `UnturnedCmdGen.exe` — runnable anywhere (Qt runtime DLLs bundled by
  `windeployqt` during the install step)
- `unturnedcmdgen-<version>-win64.exe` — the NSIS installer (Start-menu +
  desktop shortcuts, uninstaller, license page)

The app auto-detects `assets/` and `translations/` next to the executable, so
the install layout is flat and requires no code changes. Use
`.\build.ps1 -SkipInstaller` to build only the executable.

## Windows portable version packaging

Build a portable executable and environment required with
`build_portable.ps1`:

```powershell
.\build.ps1
```

## Tests

The logic tests and a headless GUI smoke test are enabled with
`-DBUILD_GUI_TEST=ON`:

```bash
cmake -S . -B build -DBUILD_GUI_TEST=ON
cmake --build build
ctest --test-dir build --output-on-failure   # run with QT_QPA_PLATFORM=offscreen headlessly
```

## Usage

1. Select a command from the left panel (search + classification filters).
   Commands marked with **⚠** are *cheat* commands — they require `/cheats` to
   be enabled on the server before they take effect.
2. Fill the parameters; required fields are highlighted if missing.
3. Choose the output format (Terminal / Chat (/) / Chat (@)).
4. **Copy** the result, or **Send** it to the server if an RCON/TCP bridge is
   configured in `Settings → Preferences...`.

Add your own data (items, mod content, players) via
`File → Import JSON...`. See
[docs/import-format-en.md](docs/import-format-en.md) for the exact format
(Chinese: [docs/import-format-zh-CN.md](docs/import-format-zh-CN.md)).

---

## Project Layout

```
assets/          Bundled data tables & command definitions (JSON)
translations/    UI translations (JSON, English source -> Chinese)
docs/            Import format documentation
src/model/       Data models (Command, Parameter, AppData)
src/controller/  Command generation, tables, RCON, app wiring
src/ui/          Qt widgets (main window, dynamic parameter form, dialogs)
src/util/        JSON helpers, JSON-backed translator
tests/           Logic unit tests + headless GUI smoke test
```

## Notes

- In the command list, a **⚠** (warning triangle) after a command name marks it
  as a *cheat* command (its `classifications` in `assets/commands.json` include
  `cheat`, e.g. `Give`, `Vehicle`, `Animal`, `Experience`). Such commands only
  work once `Cheats` has been enabled on the server; use the **Cheat** filter
  checkbox in the left panel to show or hide them.
- Item/vehicle/animal data shipped with the program is a starter set; import
  your own (official or mod) tables for full coverage.
- The player table is stored in the user data directory (e.g.
  `~/.local/share/UnturnedCmdGen/players.json` on Linux).
- Unturned has no built-in RCON; the optional sender is a generic line-based
  TCP client for use with RCON-style mods or a console bridge.
- Editing tables writes back to `assets/`; use `File → Export JSON...` if that
  directory is read-only.

## License

[MIT](LICENSE)
