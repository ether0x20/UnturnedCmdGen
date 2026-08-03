#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

namespace ParamType {
enum Type {
    Player,      // reference to Player table (name or SteamID)
    Item,        // reference to Item table
    Vehicle,     // reference to Vehicle table
    Animal,      // reference to Animal table
    Effect,      // reference to Effect table
    Quest,       // reference to Quest table
    Achievement, // reference to Achievement table
    Skillset,    // reference to Skillset table
    Location,    // map location node or "wp"
    Map,         // map id from maps.json
    Enum,        // fixed list of values
    Boolean,     // Y / N
    Integer,     // numeric range
    Float,       // numeric range
    String,      // free text
    Color,       // RGB triple
    Guid,        // asset GUID string
    Duration,    // time in seconds
    Flag,        // player flag key
    Invalid
};

Type fromString(const QString& s);
QString toString(Type t);
}

// A single input slot of a command. "/" separates parameters, "|" separates
// the alternative input styles accepted by one parameter.
struct Parameter {
    QString name;
    QStringList types;    // ParamType names, e.g. ["player","location"]
    bool optional = false;
    QString defaultValue; // human text, e.g. "executing player" or "1"
    double min = 0.0;
    double max = 0.0;
    bool hasMin = false;
    bool hasMax = false;
    QStringList values;   // enum values

    QVector<ParamType::Type> typeList() const;
    ParamType::Type primaryType() const;
    bool accepts(ParamType::Type t) const;
};

struct Command {
    QString name;
    QString syntax;
    QStringList classifications; // runtime / cheat / config / any
    QString description;
    QVector<Parameter> params;

    bool isCheat() const;
    bool emptyParams() const;
};
