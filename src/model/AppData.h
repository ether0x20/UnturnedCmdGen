#pragma once

#include <QHash>
#include <QObject>
#include <QVector>

#include "Command.h"
#include "ModInfo.h"

// A generic row used by every lookup table. `id` is the primary key shown /
// substituted into commands (item id, steam id, effect id, ...), `name` the
// human-readable label, `nameZh` an optional Chinese annotation, `mod` the id
// of the mod this entry belongs to (empty = vanilla), `note` extra info.
struct TableEntry {
    QString id;
    QString name;
    QString nameZh;
    QString mod;
    QString note;
};

struct MapInfo {
    QString id;
    QString name;
    QStringList locations; // teleportable location nodes on this map
};

class AppData : public QObject
{
    Q_OBJECT
public:
    explicit AppData(QObject* parent = nullptr);

    // assets/ root directory (next to the executable)
    void setAssetsDir(const QString& dir);
    QString assetsDir() const { return m_assetsDir; }
    // user data directory (players.json, mods.json)
    void setUserDataDir(const QString& dir);

    // --- Loading ---------------------------------------------------------
    // Returns false if the mandatory assets (commands.json) failed to load.
    bool loadAll();
    void loadCommands(const QString& path);
    void loadTable(const QString& path, const QString& tableType);
    void loadMapsAndLocations(const QString& mapsPath, const QString& locationsPath);

    // --- Commands --------------------------------------------------------
    const QVector<Command>& commands() const { return m_commands; }
    const Command* findCommand(const QString& name) const;
    QStringList commandNames() const;

    // --- Tables ----------------------------------------------------------
    // tableType is one of: item/vehicle/animal/effect/quest/achievement/skillset/player
    // `table()` returns only entries whose mod is enabled (what the selection
    // UI should show); `rawTable()` returns everything (management dialogs).
    const QVector<TableEntry>& table(const QString& tableType) const;
    const QVector<TableEntry>& rawTable(const QString& tableType) const;
    // Mutable access to the raw table; call rebuildTableFilter() afterwards.
    QVector<TableEntry>& mutableTable(const QString& tableType);
    QStringList tableTypes() const;
    // Source JSON file this table was loaded from (empty for the player table,
    // which is stored in the user data directory).
    QString tableStoragePath(const QString& tableType) const;
    // Case-insensitive substring search over id/name/nameZh/mod/note.
    QVector<TableEntry> search(const QString& tableType, const QString& query, int maxResults = 50) const;

    // Rebuilds the filtered view of a table from its raw entries + mod states.
    void rebuildTableFilter(const QString& tableType);
    void rebuildAllFilters();

    // Dropdown label: "[id] name / 中文 [Mod]" (mod tag omitted for vanilla).
    QString entryLabel(const TableEntry& e) const;

    // --- Mods ------------------------------------------------------------
    const QHash<QString, ModInfo>& mods() const { return m_mods; }
    ModInfo modInfo(const QString& modId) const;
    bool isModEnabled(const QString& modId) const; // vanilla (empty) => true
    bool setModEnabled(const QString& modId, bool enabled);
    bool addMod(const QString& id, const QString& name);
    // Removes the mod AND every entry tagged with it (vanilla/locked refuse).
    bool removeMod(const QString& modId);
    int modEntryCount(const QString& modId, const QString& tableType) const;
    // Scans all raw tables for mod ids and registers unknown ones (enabled).
    void discoverMods();
    void loadMods();
    void saveMods() const;
    QString modsStoragePath() const { return m_modsPath; }

    // --- Players (user-maintained, persisted) ---------------------------
    void loadPlayers();
    void savePlayers() const;
    void addPlayer(const QString& steamId, const QString& name, const QString& note = QString());
    bool updatePlayer(int row, const QString& steamId, const QString& name, const QString& note);
    bool removePlayer(int row);
    QString playerStoragePath() const { return m_playersPath; }

    // --- Maps & locations -------------------------------------------------
    const QVector<MapInfo>& maps() const { return m_maps; }
    QStringList mapNames() const;
    QStringList locationsForMap(const QString& mapId) const;
    QString mapIdByName(const QString& mapName) const;

signals:
    void tableChanged(const QString& tableType);
    void playersChanged();
    void modsChanged();

private:
    static QVector<TableEntry> parseEntries(const QJsonArray& arr);
    void ensureTable(const QString& tableType);

    QString m_assetsDir;
    QString m_userDataDir;
    QString m_playersPath;
    QString m_modsPath;

    QVector<Command> m_commands;
    QHash<QString, QVector<TableEntry>> m_rawTables; // all entries
    QHash<QString, QVector<TableEntry>> m_tables;    // filtered (enabled mods)
    QHash<QString, QString> m_tablePaths;
    QHash<QString, ModInfo> m_mods;
    QVector<MapInfo> m_maps;
};
