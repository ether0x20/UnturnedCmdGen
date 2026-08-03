# Unturned Command Generator

A cross-platform desktop tool that builds Unturned server commands in either
**server-terminal** or **in-game chat** format, with searchable lookup tables,
bilingual (English/Chinese) item annotations, and an optional RCON/TCP bridge
to send commands directly to your server.

Built with **C++17 + Qt 6** and licensed under the **MIT License**.

---

## Features

- **59 commands** defined in `assets/commands.json`, driving a dynamic parameter
  form (player, item, vehicle, location, enum, boolean, integer, float, color,
  duration, map, GUID, ...).
- **Three output formats** with live preview and one-click copy:
  - Server terminal: `teleport Ethan Seattle`
  - In-game chat: `/teleport Ethan Seattle`
  - In-game chat: `@teleport Ethan Seattle`
- **Multi-type parameters** (e.g. Teleport target = player *or* map location;
  Weather = preset *or* custom GUID) via a mode selector.
- **Lookup tables** with searchable, editable dropdowns for items, vehicles,
  animals, effects, quests, achievements and skillsets, plus a user-maintained
  player table.
- **Bilingual data**: optional `nameZh` Chinese annotations are shown as
  `[id] name / 中文` and searchable, while commands always use the English name.
- **In-app CRUD** for every table from the `Data` menu; edits persist back to
  the source JSON files.
- **Import / Export JSON** with merge-or-replace and a built-in format tutorial.
- **Language switching** (English / 中文) from the `Language` menu, persisted
  across restarts.
- **Optional RCON/TCP bridge** to send generated commands to the server.

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
docs/            Import format documentation (EN / 中文)
src/model/       Data models (Command, Parameter, AppData)
src/controller/  Command generation, tables, RCON, app wiring
src/ui/          Qt widgets (main window, dynamic parameter form, dialogs)
src/util/        JSON helpers, JSON-backed translator
tests/           Logic unit tests + headless GUI smoke test
```

## Notes

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
