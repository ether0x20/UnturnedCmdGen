#include "AppData.h"

#include <QCoreApplication>
#include <QDir>
#include <QHash>

#include "util/JsonHelper.h"

QString TableEntry::label() const
{
    QString base = id.isEmpty() ? name : QStringLiteral("[%1] %2").arg(id, name);
    if (!nameZh.isEmpty())
        base += QStringLiteral(" / ") + nameZh;
    return base;
}

static const char* kItemTypes[] = {
    "item", "vehicle", "animal", "effect", "quest", "achievement", "skillset", "player"
};

AppData::AppData(QObject* parent)
    : QObject(parent)
{
    for (const char* t : kItemTypes)
        ensureTable(QString::fromLatin1(t));
}

void AppData::setAssetsDir(const QString& dir)
{
    m_assetsDir = dir;
}

void AppData::setUserDataDir(const QString& dir)
{
    m_userDataDir = dir;
    m_playersPath = QDir(dir).filePath(QStringLiteral("players.json"));
    QDir().mkpath(dir);
}

bool AppData::loadAll()
{
    if (m_assetsDir.isEmpty()) {
        // Default: look next to the executable.
        m_assetsDir = QCoreApplication::applicationDirPath() + QStringLiteral("/assets");
    }

    loadCommands(m_assetsDir + QStringLiteral("/commands.json"));

    for (const char* t : kItemTypes) {
        const QString key = QString::fromLatin1(t);
        // player is user-maintained, skip loading from assets
        if (key == QLatin1String("player"))
            continue;
        loadTable(m_assetsDir + QStringLiteral("/%1s.json").arg(key), key);
    }

    loadMapsAndLocations(m_assetsDir + QStringLiteral("/maps.json"),
                         m_assetsDir + QStringLiteral("/locations.json"));
    return !m_commands.isEmpty();
}

void AppData::loadCommands(const QString& path)
{
    m_commands.clear();
    const QJsonObject root = JsonHelper::readObjectFile(path);
    const QJsonArray arr = root.value(QStringLiteral("commands")).toArray();
    for (const auto& val : arr) {
        const QJsonObject o = val.toObject();
        Command cmd;
        cmd.name = o.value(QStringLiteral("name")).toString();
        if (cmd.name.isEmpty())
            continue;
        cmd.syntax = o.value(QStringLiteral("syntax")).toString();
        cmd.classifications = JsonHelper::stringList(o.value(QStringLiteral("classifications")).toArray());
        cmd.description = o.value(QStringLiteral("description")).toString();

        const QJsonArray parr = o.value(QStringLiteral("params")).toArray();
        for (const auto& pv : parr) {
            const QJsonObject po = pv.toObject();
            Parameter p;
            p.name = po.value(QStringLiteral("name")).toString();
            p.types = JsonHelper::stringList(po.value(QStringLiteral("types")).toArray());
            p.optional = po.value(QStringLiteral("optional")).toBool(false);
            p.defaultValue = po.value(QStringLiteral("default")).toString();
            if (JsonHelper::has(po, QStringLiteral("min"))) {
                p.min = po.value(QStringLiteral("min")).toDouble();
                p.hasMin = true;
            }
            if (JsonHelper::has(po, QStringLiteral("max"))) {
                p.max = po.value(QStringLiteral("max")).toDouble();
                p.hasMax = true;
            }
            p.values = JsonHelper::stringList(po.value(QStringLiteral("values")).toArray());
            cmd.params.append(p);
        }
        m_commands.append(cmd);
    }
}

void AppData::loadTable(const QString& path, const QString& tableType)
{
    ensureTable(tableType);
    m_tables[tableType] = parseEntries(JsonHelper::readArrayFile(path));
    m_tablePaths[tableType] = path;
    emit tableChanged(tableType);
}

QVector<TableEntry> AppData::parseEntries(const QJsonArray& arr)
{
    QVector<TableEntry> out;
    out.reserve(arr.size());
    for (const auto& val : arr) {
        const QJsonObject o = val.toObject();
        TableEntry e;
        // Accept several common field names for the primary key.
        e.id = JsonHelper::v(o, QStringLiteral("id"),
                             JsonHelper::v(o, QStringLiteral("steamId"),
                                           JsonHelper::v(o, QStringLiteral("key"), QString()))).toString();
        e.name = o.value(QStringLiteral("name")).toString();
        e.nameZh = o.value(QStringLiteral("nameZh")).toString();
        e.note = JsonHelper::v(o, QStringLiteral("note"), QString()).toString();
        if (e.name.isEmpty() && e.nameZh.isEmpty() && e.id.isEmpty())
            continue;
        out.append(e);
    }
    return out;
}

void AppData::loadMapsAndLocations(const QString& mapsPath, const QString& locationsPath)
{
    m_maps.clear();
    const QJsonArray marr = JsonHelper::readArrayFile(mapsPath);
    for (const auto& val : marr) {
        const QJsonObject o = val.toObject();
        MapInfo m;
        m.id = o.value(QStringLiteral("id")).toString();
        m.name = o.value(QStringLiteral("name")).toString();
        if (m.id.isEmpty())
            continue;
        m_maps.append(m);
    }

    // locations.json is [{ "mapId": "...", "locations": [...] }, ...]
    const QJsonArray larr = JsonHelper::readArrayFile(locationsPath);
    for (const auto& val : larr) {
        const QJsonObject o = val.toObject();
        const QString mapId = o.value(QStringLiteral("mapId")).toString();
        if (mapId.isEmpty())
            continue;
        const QStringList locs = JsonHelper::stringList(o.value(QStringLiteral("locations")).toArray());
        for (auto& m : m_maps) {
            if (m.id == mapId) {
                m.locations = locs;
                break;
            }
        }
    }
}

const Command* AppData::findCommand(const QString& name) const
{
    for (const auto& c : m_commands) {
        if (c.name.compare(name, Qt::CaseInsensitive) == 0)
            return &c;
    }
    return nullptr;
}

QStringList AppData::commandNames() const
{
    QStringList out;
    out.reserve(m_commands.size());
    for (const auto& c : m_commands)
        out.append(c.name);
    return out;
}

void AppData::ensureTable(const QString& tableType)
{
    if (!m_tables.contains(tableType))
        m_tables.insert(tableType, QVector<TableEntry>());
}

const QVector<TableEntry>& AppData::table(const QString& tableType) const
{
    static const QVector<TableEntry> kEmpty;
    const auto it = m_tables.constFind(tableType);
    if (it == m_tables.constEnd())
        return kEmpty;
    return it.value();
}

QVector<TableEntry>& AppData::mutableTable(const QString& tableType)
{
    ensureTable(tableType);
    return m_tables[tableType];
}

QStringList AppData::tableTypes() const
{
    return m_tables.keys();
}

QString AppData::tableStoragePath(const QString& tableType) const
{
    return m_tablePaths.value(tableType);
}

QVector<TableEntry> AppData::search(const QString& tableType, const QString& query, int maxResults) const
{
    QVector<TableEntry> out;
    const auto& src = table(tableType);
    out.reserve(qMin(src.size(), maxResults));
    for (const auto& e : src) {
        if (e.id.contains(query, Qt::CaseInsensitive)
            || e.name.contains(query, Qt::CaseInsensitive)
            || e.nameZh.contains(query, Qt::CaseInsensitive)
            || e.note.contains(query, Qt::CaseInsensitive)) {
            out.append(e);
            if (out.size() >= maxResults)
                break;
        }
    }
    return out;
}

void AppData::loadPlayers()
{
    if (m_playersPath.isEmpty())
        return;
    m_tables[QStringLiteral("player")] = parseEntries(JsonHelper::readArrayFile(m_playersPath));
    emit playersChanged();
}

void AppData::savePlayers() const
{
    if (m_playersPath.isEmpty())
        return;
    QJsonArray arr;
    for (const auto& e : table(QStringLiteral("player"))) {
        QJsonObject o;
        o.insert(QStringLiteral("steamId"), e.id);
        o.insert(QStringLiteral("name"), e.name);
        if (!e.nameZh.isEmpty())
            o.insert(QStringLiteral("nameZh"), e.nameZh);
        if (!e.note.isEmpty())
            o.insert(QStringLiteral("note"), e.note);
        arr.append(o);
    }
    JsonHelper::writeArrayFile(m_playersPath, arr);
}

void AppData::addPlayer(const QString& steamId, const QString& name, const QString& note)
{
    TableEntry e;
    e.id = steamId.trimmed();
    e.name = name.trimmed();
    e.note = note.trimmed();
    if (e.id.isEmpty())
        return;
    m_tables[QStringLiteral("player")].append(e);
    emit playersChanged();
}

bool AppData::updatePlayer(int row, const QString& steamId, const QString& name, const QString& note)
{
    auto& t = m_tables[QStringLiteral("player")];
    if (row < 0 || row >= t.size())
        return false;
    t[row].id = steamId.trimmed();
    t[row].name = name.trimmed();
    t[row].note = note.trimmed();
    emit playersChanged();
    return true;
}

bool AppData::removePlayer(int row)
{
    auto& t = m_tables[QStringLiteral("player")];
    if (row < 0 || row >= t.size())
        return false;
    t.remove(row);
    emit playersChanged();
    return true;
}

QStringList AppData::mapNames() const
{
    QStringList out;
    for (const auto& m : m_maps)
        out.append(m.name);
    return out;
}

QStringList AppData::locationsForMap(const QString& mapId) const
{
    for (const auto& m : m_maps) {
        if (m.id == mapId)
            return m.locations;
    }
    return QStringList();
}

QString AppData::mapIdByName(const QString& mapName) const
{
    for (const auto& m : m_maps) {
        if (m.name.compare(mapName, Qt::CaseInsensitive) == 0)
            return m.id;
    }
    return QString();
}
