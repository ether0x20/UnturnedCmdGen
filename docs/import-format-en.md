# Data Table JSON Import Format

All data tables (Items, Vehicles, Animals, Effects, Quests, Achievements, Skillsets, Players) use the **same JSON structure** and can be imported via `File → Import JSON...`.

---

## 1. Overall Format

The file must be a top-level **JSON array**; each element is an **object** representing one entry.

```json
[
  { "id": "116", "name": "Military Knife", "nameZh": "军用刀", "note": "melee" },
  { "id": "363", "name": "Maple Rifle", "nameZh": "枫木步枪" }
]
```

## 2. Field Reference

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | no* | Primary key. Game ID for items/vehicles etc.; SteamID for players. Stored as a string (write numbers as strings). |
| `name` | string | no* | English display name. **One of the required fields.** |
| `nameZh` | string | no | Chinese annotation, shown in dropdowns as `[id] name / 中文` and used for Chinese search. |
| `mod` | string | no | ID of the mod this entry belongs to (empty = vanilla). Unknown mod IDs are auto-registered on import. |
| `note` | string | no | Extra info, display only. |

> *Rule: `id`, `name` and `nameZh` cannot all be empty, otherwise the entry is skipped.

### Primary-key aliases

| Table type | Recommended field | Compatible field |
|------------|-------------------|------------------|
| Items/Vehicles/Animals/Effects/Quests/Achievements/Skillsets | `id` | `key` |
| Players | `steamId` | `id` |

---

## 3. Per-Table Examples

### Items
```json
[
  { "id": "116", "name": "Military Knife", "nameZh": "军用刀", "note": "melee" },
  { "id": "363", "name": "Maple Rifle", "nameZh": "枫木步枪" },
  { "id": "9001", "name": "Plasma Rifle", "nameZh": "等离子步枪", "mod": "weapons_pack" }
]
```

### Vehicles
```json
[
  { "id": "1", "name": "Offroader", "nameZh": "越野车" },
  { "id": "17", "name": "Helicopter", "nameZh": "直升机", "note": "rare spawn" },
  { "id": "9001", "name": "Hovercraft", "nameZh": "气垫船", "mod": "future_vehicles" }
]
```

### Players
```json
[
  { "steamId": "76561198000000001", "name": "Ethan", "note": "owner" },
  { "steamId": "76561198000000002", "name": "Player2" }
]
```

> The player table usually omits `nameZh` (player names do not need translation), but the field is supported there too.

### Animals
```json
[
  { "id": "5", "name": "Wolf", "nameZh": "狼" },
  { "id": "6", "name": "Bear", "nameZh": "熊" }
]
```

### Enum-style tables (Effects / Quests / Achievements / Skillsets)
```json
[
  { "id": "1", "name": "Find Aid", "nameZh": "寻找援助" },
  { "id": "255", "name": "All Skillsets", "nameZh": "全部技能组" }
]
```

---

## 4. Import Behavior

Use `File → Import JSON...` to open the import dialog; the **"? Import Tutorial"** button shows a built-in format example.

The program asks you to choose between **Merge or Replace**:

- **Merge**: appends to the existing table; entries with a duplicate `id` are skipped.
- **Replace**: clears the whole table first, then keeps only the imported content.

You can also tag the imported entries with a **Mod**:

- **(Keep as in file)**: keep each entry's `mod` field as written in the JSON (default). Unknown mod IDs are registered automatically.
- **(Vanilla)**: clears the `mod` field on every imported entry.
- Any specific mod: overrides the `mod` field of every imported entry.

**Import failure** occurs when: the file is not valid JSON, the top level is not an array, or every entry is skipped (`id`/`name`/`nameZh` all empty). A hint points you to the import tutorial.

## 5. In-Application Table Management

Imported data (including the bundled seed data) can be managed per table from the **`Data` menu**:

- `Manage Items...` / `Manage Vehicles...` / `Manage Animals...` / `Manage Effects...` etc.
- The dialog supports **CRUD** on the `ID / Name / Chinese Name / Mod / Note` columns (the player table omits the Chinese and Mod columns).
- Rows whose mod is **disabled** are shown greyed out but remain fully editable.
- Clicking **Save** writes back to the table's source JSON file:
  - Player table → `players.json` in the user data directory
  - Other tables → the corresponding JSON in the program's `assets/` directory (e.g. `assets/items.json`)

> Note: if the `assets/` directory is read-only (e.g. the program is installed to a system directory), saving edits will fail. Use `File → Export JSON...` first, or place the data files somewhere writable.

## 5b. Mod Management

`Data → Manage Mods...` opens the mod resource manager:

- Lists every known mod (plus the implicit **Vanilla** mod) with per-table entry counts.
- **Enable/Disable** checkboxes filter that mod's entries out of the command selection dropdowns immediately (management dialogs still show them greyed out).
- **Add Mod** / **Remove** manage the registry; removing a mod also deletes all of its entries.
- **Import Mod Data...** opens the import dialog with the selected mod preselected.
- **Manage Entries...** opens a table manager filtered to the selected mod.

Mod enable/disable state is persisted to `mods.json` in the user data directory.

## 6. Which Field Goes Into a Command

| Field | Purpose |
|-------|---------|
| `name` | Shown in lookup dropdowns as `[id] name / 中文`; when selected it is used **as the parameter value** in the command (e.g. `/give Player Maplestrike`) |
| `nameZh` | Chinese display in dropdowns + Chinese search; display only, never enters a command |
| `mod` | Tags the entry in dropdowns (`[id] name / 中文 [Mod Name]`); display only, never enters a command |
| `id` | Shown in dropdowns; for players, a manually typed SteamID is used verbatim |

## 7. Quick Validation

Validate a file with any JSON tool before importing:

```bash
python3 -c "import json,sys; json.load(open(sys.argv[1])); print('valid')" my_table.json
```
