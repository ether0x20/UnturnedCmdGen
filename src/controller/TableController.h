#pragma once

#include <QObject>
#include <QString>

class AppData;

// Handles data-table operations: importing user-provided JSON files into the
// lookup tables and exporting tables back to JSON.
class TableController : public QObject
{
    Q_OBJECT
public:
    explicit TableController(AppData* data, QObject* parent = nullptr);

    // tableType is one of: item/vehicle/animal/effect/quest/achievement/skillset/player
    // modOverride: empty = keep the "mod" field as written in the file;
    // kForceVanilla = clear it; any other value = tag every entry with it.
    bool importJson(const QString& path, const QString& tableType, bool merge,
                    const QString& modOverride = QString());
    bool exportJson(const QString& path, const QString& tableType) const;
    // Persists the current table contents back to its source file
    // (players.json for the player table, the assets JSON otherwise).
    bool saveTable(const QString& tableType) const;

    // Sentinel modOverride value that imports entries as vanilla (mod cleared).
    static const QString kForceVanilla;

    // Player management (persisted by AppData).
    void addPlayer(const QString& steamId, const QString& name, const QString& note = QString());
    bool updatePlayer(int row, const QString& steamId, const QString& name, const QString& note);
    bool removePlayer(int row);

signals:
    void playersChanged();

private:
    AppData* m_data = nullptr;
};
