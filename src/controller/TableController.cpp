#include "TableController.h"

#include <QJsonArray>
#include <QJsonObject>

#include "model/AppData.h"
#include "util/JsonHelper.h"

TableController::TableController(AppData* data, QObject* parent)
    : QObject(parent)
    , m_data(data)
{
}

bool TableController::importJson(const QString& path, const QString& tableType, bool merge)
{
    const QJsonArray arr = JsonHelper::readArrayFile(path);
    if (arr.isEmpty())
        return false;

    QVector<TableEntry> entries;
    entries.reserve(arr.size());
    for (const auto& val : arr) {
        const QJsonObject o = val.toObject();
        TableEntry e;
        e.id = JsonHelper::v(o, QStringLiteral("id"),
                             JsonHelper::v(o, QStringLiteral("steamId"), QString())).toString();
        e.name = o.value(QStringLiteral("name")).toString();
        e.nameZh = o.value(QStringLiteral("nameZh")).toString();
        e.note = JsonHelper::v(o, QStringLiteral("note"), QString()).toString();
        if (e.name.isEmpty() && e.nameZh.isEmpty() && e.id.isEmpty())
            continue;
        entries.append(e);
    }
    if (entries.isEmpty())
        return false;

    if (merge) {
        auto& dst = m_data->mutableTable(tableType);
        // Skip duplicate ids when merging.
        for (const auto& in : entries) {
            bool dup = false;
            for (const auto& e : dst) {
                if (!in.id.isEmpty() && e.id == in.id) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                dst.append(in);
        }
    } else {
        m_data->mutableTable(tableType) = entries;
    }
    emit m_data->tableChanged(tableType);
    return true;
}

bool TableController::exportJson(const QString& path, const QString& tableType) const
{
    QJsonArray arr;
    for (const auto& e : m_data->table(tableType)) {
        QJsonObject o;
        o.insert(QStringLiteral("id"), e.id);
        o.insert(QStringLiteral("name"), e.name);
        if (!e.nameZh.isEmpty())
            o.insert(QStringLiteral("nameZh"), e.nameZh);
        if (!e.note.isEmpty())
            o.insert(QStringLiteral("note"), e.note);
        arr.append(o);
    }
    return JsonHelper::writeArrayFile(path, arr);
}

bool TableController::saveTable(const QString& tableType) const
{
    if (tableType == QLatin1String("player")) {
        m_data->savePlayers();
        return true;
    }
    const QString path = m_data->tableStoragePath(tableType);
    if (path.isEmpty())
        return false;
    return exportJson(path, tableType);
}

void TableController::addPlayer(const QString& steamId, const QString& name, const QString& note)
{
    m_data->addPlayer(steamId, name, note);
    m_data->savePlayers();
    emit playersChanged();
}

bool TableController::updatePlayer(int row, const QString& steamId, const QString& name, const QString& note)
{
    const bool ok = m_data->updatePlayer(row, steamId, name, note);
    if (ok) {
        m_data->savePlayers();
        emit playersChanged();
    }
    return ok;
}

bool TableController::removePlayer(int row)
{
    const bool ok = m_data->removePlayer(row);
    if (ok) {
        m_data->savePlayers();
        emit playersChanged();
    }
    return ok;
}
