#include "JsonHelper.h"

#include <QFile>
#include <QJsonDocument>

namespace JsonHelper {

QJsonObject readObjectFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonObject();
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return QJsonObject();
    return doc.object();
}

QJsonArray readArrayFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return QJsonArray();
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return QJsonArray();
    return doc.array();
}

bool writeObjectFile(const QString& path, const QJsonObject& obj)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return true;
}

bool writeArrayFile(const QString& path, const QJsonArray& arr)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    f.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
    return true;
}

QStringList stringList(const QJsonArray& arr)
{
    QStringList out;
    out.reserve(arr.size());
    for (const auto& v : arr)
        out.append(v.toString());
    return out;
}

QVariant v(const QJsonObject& o, const QString& key, const QVariant& def)
{
    if (!o.contains(key))
        return def;
    return o.value(key).toVariant();
}

bool has(const QJsonObject& o, const QString& key)
{
    return o.contains(key);
}

} // namespace JsonHelper
