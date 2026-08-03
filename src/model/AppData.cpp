#include "AppData.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>

#include <algorithm>

#include "util/JsonHelper.h"

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
    m_modsPath = QDir(dir).filePath(QStringLiteral("mods.json"));
    QDir().mkpath(dir);
}

bool AppData::loadAll()
{
    if (m_assetsDir.isEmpty()) {
        // Probe likely locations in order of preference: the dev build copies
        // assets next to the executable; an installed .deb places them under
        // /usr/share/unturnedCmdGen/assets.
        const QString exeDir = QCoreApplication::applicationDirPath();
        const QStringList candidates = {
            exeDir + QStringLiteral("/assets"),
            exeDir + QStringLiteral("/../share/unturnedCmdGen/assets"),
            QStringLiteral("/usr/share/unturnedCmdGen/assets")
        };
        for (const QString& c : candidates) {
            if (QFileInfo::exists(c + QStringLiteral("/commands.json"))) {
                m_assetsDir = c;
                break;
            }
        }
        if (m_assetsDir.isEmpty())
            m_assetsDir = candidates.first();
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

    // Restore saved mod states first, then register any mod ids found in the
    // tables that are not yet known. Stored display names win over discovery.
    loadMods();
    discoverMods();
    rebuildAllFilters();
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
    m_rawTables[tableType] = parseEntries(JsonHelper::readArrayFile(path));
    m_tablePaths[tableType] = path;
    rebuildTableFilter(tableType);
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
        e.mod = o.value(QStringLiteral("mod")).toString();
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
    if (!m_rawTables.contains(tableType))
        m_rawTables.insert(tableType, QVector<TableEntry>());
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

const QVector<TableEntry>& AppData::rawTable(const QString& tableType) const
{
    static const QVector<TableEntry> kEmpty;
    const auto it = m_rawTables.constFind(tableType);
    if (it == m_rawTables.constEnd())
        return kEmpty;
    return it.value();
}

QVector<TableEntry>& AppData::mutableTable(const QString& tableType)
{
    ensureTable(tableType);
    return m_rawTables[tableType];
}

QStringList AppData::tableTypes() const
{
    return m_rawTables.keys();
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
            || e.mod.contains(query, Qt::CaseInsensitive)
            || modInfo(e.mod).name.contains(query, Qt::CaseInsensitive)
            || e.note.contains(query, Qt::CaseInsensitive)) {
            out.append(e);
            if (out.size() >= maxResults)
                break;
        }
    }
    return out;
}

void AppData::rebuildTableFilter(const QString& tableType)
{
    if (!m_rawTables.contains(tableType))
        return;
    QVector<TableEntry> filtered;
    filtered.reserve(m_rawTables[tableType].size());
    for (const auto& e : m_rawTables[tableType]) {
        if (isModEnabled(e.mod))
            filtered.append(e);
    }
    m_tables[tableType] = filtered;
}

void AppData::rebuildAllFilters()
{
    for (const QString& type : m_rawTables.keys())
        rebuildTableFilter(type);
}

QString AppData::entryLabel(const TableEntry& e) const
{
    QString base = e.id.isEmpty() ? e.name : QStringLiteral("[%1] %2").arg(e.id, e.name);
    if (!e.nameZh.isEmpty())
        base += QStringLiteral(" / ") + e.nameZh;
    if (!e.mod.isEmpty()) {
        const QString modName = modInfo(e.mod).name;
        if (!modName.isEmpty())
            base += QStringLiteral(" [") + modName + QStringLiteral("]");
    }
    return base;
}

ModInfo AppData::modInfo(const QString& modId) const
{
    static const ModInfo kVanilla; // empty id, enabled, no name
    if (modId.isEmpty())
        return kVanilla;
    return m_mods.value(modId, kVanilla);
}

bool AppData::isModEnabled(const QString& modId) const
{
    if (modId.isEmpty())
        return true;
    const auto it = m_mods.constFind(modId);
    if (it == m_mods.constEnd())
        return true; // unknown mod treated as enabled
    return it->enabled;
}

bool AppData::setModEnabled(const QString& modId, bool enabled)
{
    if (modId.isEmpty())
        return false; // vanilla cannot be toggled
    auto it = m_mods.find(modId);
    if (it == m_mods.end() || it->enabled == enabled)
        return false;
    it->enabled = enabled;
    rebuildAllFilters();
    emit modsChanged();
    for (const QString& type : m_tables.keys())
        emit tableChanged(type);
    return true;
}

bool AppData::addMod(const QString& id, const QString& name)
{
    const QString cleanId = id.trimmed();
    if (cleanId.isEmpty() || m_mods.contains(cleanId))
        return false;
    ModInfo mi;
    mi.id = cleanId;
    mi.name = name.trimmed().isEmpty() ? cleanId : name.trimmed();
    m_mods.insert(cleanId, mi);
    emit modsChanged();
    return true;
}

bool AppData::removeMod(const QString& modId)
{
    if (modId.isEmpty())
        return false;
    const auto it = m_mods.constFind(modId);
    if (it == m_mods.constEnd())
        return false;
    if (it->locked)
        return false;
    m_mods.remove(modId);

    // Remove every entry tagged with this mod from all raw tables.
    for (auto tbl = m_rawTables.begin(); tbl != m_rawTables.end(); ++tbl) {
        QVector<TableEntry>& vec = tbl.value();
        const int before = vec.size();
        vec.erase(std::remove_if(vec.begin(), vec.end(),
                                 [&modId](const TableEntry& e) { return e.mod == modId; }),
                  vec.end());
        if (vec.size() != before) {
            rebuildTableFilter(tbl.key());
            emit tableChanged(tbl.key());
        }
    }
    emit modsChanged();
    return true;
}

int AppData::modEntryCount(const QString& modId, const QString& tableType) const
{
    int count = 0;
    for (const auto& e : rawTable(tableType)) {
        if (e.mod == modId)
            ++count;
    }
    return count;
}

void AppData::discoverMods()
{
    bool changed = false;
    for (auto tbl = m_rawTables.constBegin(); tbl != m_rawTables.constEnd(); ++tbl) {
        for (const auto& e : tbl.value()) {
            if (e.mod.isEmpty() || m_mods.contains(e.mod))
                continue;
            ModInfo mi;
            mi.id = e.mod;
            mi.name = e.mod; // placeholder; overwritten by loadMods if stored
            m_mods.insert(e.mod, mi);
            changed = true;
        }
    }
    if (changed)
        emit modsChanged();
}

void AppData::loadMods()
{
    if (m_modsPath.isEmpty())
        return;
    const QJsonObject root = JsonHelper::readObjectFile(m_modsPath);
    const QJsonArray arr = root.value(QStringLiteral("mods")).toArray();
    for (const auto& val : arr) {
        const QJsonObject o = val.toObject();
        ModInfo mi;
        mi.id = o.value(QStringLiteral("id")).toString();
        if (mi.id.isEmpty())
            continue;
        mi.name = o.value(QStringLiteral("name")).toString();
        mi.desc = o.value(QStringLiteral("desc")).toString();
        mi.enabled = o.value(QStringLiteral("enabled")).toBool(true);
        mi.locked = o.value(QStringLiteral("locked")).toBool(false);
        m_mods.insert(mi.id, mi);
    }
}

void AppData::saveMods() const
{
    if (m_modsPath.isEmpty())
        return;
    QJsonArray arr;
    for (auto it = m_mods.constBegin(); it != m_mods.constEnd(); ++it) {
        const ModInfo& mi = it.value();
        if (mi.id.isEmpty())
            continue; // vanilla is implicit, never persisted
        QJsonObject o;
        o.insert(QStringLiteral("id"), mi.id);
        o.insert(QStringLiteral("name"), mi.name);
        o.insert(QStringLiteral("enabled"), mi.enabled);
        if (mi.locked)
            o.insert(QStringLiteral("locked"), true);
        if (!mi.desc.isEmpty())
            o.insert(QStringLiteral("desc"), mi.desc);
        arr.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("mods"), arr);
    JsonHelper::writeObjectFile(m_modsPath, root);
}

void AppData::loadPlayers()
{
    if (m_playersPath.isEmpty())
        return;
    m_rawTables[QStringLiteral("player")] = parseEntries(JsonHelper::readArrayFile(m_playersPath));
    rebuildTableFilter(QStringLiteral("player"));
    emit playersChanged();
}

void AppData::savePlayers() const
{
    if (m_playersPath.isEmpty())
        return;
    QJsonArray arr;
    for (const auto& e : rawTable(QStringLiteral("player"))) {
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
    m_rawTables[QStringLiteral("player")].append(e);
    rebuildTableFilter(QStringLiteral("player"));
    emit playersChanged();
}

bool AppData::updatePlayer(int row, const QString& steamId, const QString& name, const QString& note)
{
    auto& t = m_rawTables[QStringLiteral("player")];
    if (row < 0 || row >= t.size())
        return false;
    t[row].id = steamId.trimmed();
    t[row].name = name.trimmed();
    t[row].note = note.trimmed();
    rebuildTableFilter(QStringLiteral("player"));
    emit playersChanged();
    return true;
}

bool AppData::removePlayer(int row)
{
    auto& t = m_rawTables[QStringLiteral("player")];
    if (row < 0 || row >= t.size())
        return false;
    t.remove(row);
    rebuildTableFilter(QStringLiteral("player"));
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
