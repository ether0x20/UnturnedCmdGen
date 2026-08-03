#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace JsonHelper {

// Reads a UTF-8 JSON file, returns null object/array on failure.
QJsonObject readObjectFile(const QString& path);
QJsonArray readArrayFile(const QString& path);
bool writeObjectFile(const QString& path, const QJsonObject& obj);
bool writeArrayFile(const QString& path, const QJsonArray& arr);

QStringList stringList(const QJsonArray& arr);
QVariant v(const QJsonObject& o, const QString& key, const QVariant& def = QVariant());
bool has(const QJsonObject& o, const QString& key);

}
