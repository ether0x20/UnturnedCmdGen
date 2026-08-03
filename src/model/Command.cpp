#include "Command.h"

namespace ParamType {

Type fromString(const QString& s)
{
    const QString t = s.toLower();
    if (t == QStringLiteral("player")) return Player;
    if (t == QStringLiteral("item")) return Item;
    if (t == QStringLiteral("vehicle")) return Vehicle;
    if (t == QStringLiteral("animal")) return Animal;
    if (t == QStringLiteral("effect")) return Effect;
    if (t == QStringLiteral("quest")) return Quest;
    if (t == QStringLiteral("achievement")) return Achievement;
    if (t == QStringLiteral("skillset")) return Skillset;
    if (t == QStringLiteral("location")) return Location;
    if (t == QStringLiteral("map")) return Map;
    if (t == QStringLiteral("enum")) return Enum;
    if (t == QStringLiteral("boolean")) return Boolean;
    if (t == QStringLiteral("integer")) return Integer;
    if (t == QStringLiteral("float")) return Float;
    if (t == QStringLiteral("string")) return String;
    if (t == QStringLiteral("color")) return Color;
    if (t == QStringLiteral("guid")) return Guid;
    if (t == QStringLiteral("duration")) return Duration;
    if (t == QStringLiteral("flag")) return Flag;
    return Invalid;
}

QString toString(Type t)
{
    switch (t) {
    case Player: return QStringLiteral("player");
    case Item: return QStringLiteral("item");
    case Vehicle: return QStringLiteral("vehicle");
    case Animal: return QStringLiteral("animal");
    case Effect: return QStringLiteral("effect");
    case Quest: return QStringLiteral("quest");
    case Achievement: return QStringLiteral("achievement");
    case Skillset: return QStringLiteral("skillset");
    case Location: return QStringLiteral("location");
    case Map: return QStringLiteral("map");
    case Enum: return QStringLiteral("enum");
    case Boolean: return QStringLiteral("boolean");
    case Integer: return QStringLiteral("integer");
    case Float: return QStringLiteral("float");
    case String: return QStringLiteral("string");
    case Color: return QStringLiteral("color");
    case Guid: return QStringLiteral("guid");
    case Duration: return QStringLiteral("duration");
    case Flag: return QStringLiteral("flag");
    default: return QString();
    }
}

} // namespace ParamType

QVector<ParamType::Type> Parameter::typeList() const
{
    QVector<ParamType::Type> out;
    out.reserve(types.size());
    for (const QString& t : types)
        out.append(ParamType::fromString(t));
    return out;
}

ParamType::Type Parameter::primaryType() const
{
    const auto lst = typeList();
    return lst.isEmpty() ? ParamType::Invalid : lst.first();
}

bool Parameter::accepts(ParamType::Type t) const
{
    return typeList().contains(t);
}

bool Command::isCheat() const
{
    return classifications.contains(QStringLiteral("cheat"));
}

bool Command::emptyParams() const
{
    return params.isEmpty();
}
