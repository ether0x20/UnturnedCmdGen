#pragma once

#include <QHash>
#include <QObject>
#include <QVector>

#include "Command.h"

// A generic row used by every lookup table. `id` is the primary key shown /
// substituted into commands (item id, steam id, effect id, ...), `name` the
// human-readable label, `nameZh` an optional Chinese annotation, `note` extra
// info.
struct TableEntry {
    QString id;
    QString name;
    QString nameZh;
    QString note;

    // Bilingual label used in dropdowns: "[id] name / 中文" when a Chinese
    // annotation exists, otherwise "[id] name".
    QString label() const;
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
    // user data directory (players.json, settings)
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
    const QVector<TableEntry>& table(const QString& tableType) const;
    QVector<TableEntry>& mutableTable(const QString& tableType);
    QStringList tableTypes() const;
    // Source JSON file this table was loaded from (empty for the player table,
    // which is stored in the user data directory).
    QString tableStoragePath(const QString& tableType) const;
    // Case-insensitive substring search over id/name/note.
    QVector<TableEntry> search(const QString& tableType, const QString& query, int maxResults = 50) const;

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

private:
    static QVector<TableEntry> parseEntries(const QJsonArray& arr);
    void ensureTable(const QString& tableType);

    QString m_assetsDir;
    QString m_userDataDir;
    QString m_playersPath;

    QVector<Command> m_commands;
    QHash<QString, QVector<TableEntry>> m_tables;
    QHash<QString, QString> m_tablePaths;
    QVector<MapInfo> m_maps;
};
