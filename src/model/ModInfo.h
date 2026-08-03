#pragma once

#include <QString>

// Registry entry for a mod whose content (items/vehicles/etc.) is tagged via
// TableEntry::mod. `id` is the machine key stored on entries; `name` is the
// human-readable label shown in the UI.
struct ModInfo {
    QString id;         // unique key, e.g. "weapons_pack"; empty = vanilla
    QString name;       // display name, e.g. "Weapons Pack"
    QString desc;       // optional description
    bool enabled = true;
    bool locked = false; // cannot be deleted (reserved for built-in mods)

    bool operator==(const ModInfo& o) const { return id == o.id; }
};

// Entries with an empty mod id belong to the virtual "vanilla" mod which is
// always enabled and cannot be deleted.
inline bool isVanillaMod(const QString& modId)
{
    return modId.isEmpty();
}
