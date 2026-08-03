#include "CommandController.h"

#include "model/AppData.h"

CommandController::CommandController(AppData* data, QObject* parent)
    : QObject(parent)
    , m_data(data)
{
}

QString CommandController::formatLabel() const
{
    switch (m_format) {
    case Format::Terminal: return tr("Terminal");
    case Format::ChatSlash: return tr("Chat (/)");
    case Format::ChatAt: return tr("Chat (@)");
    }
    return QString();
}

QString CommandController::paramValue(int idx) const
{
    return m_values.value(idx);
}

bool CommandController::paramFilled(int idx) const
{
    if (m_filled.contains(idx))
        return true;
    if (!m_current || idx < 0 || idx >= m_current->params.size())
        return false;
    return !m_current->params[idx].optional;
}

void CommandController::selectCommand(const QString& name)
{
    m_current = m_data->findCommand(name);
    m_values.clear();
    m_filled.clear();
    if (m_current) {
        // Required params are always filled; optional ones start unfilled.
        for (int i = 0; i < m_current->params.size(); ++i) {
            if (!m_current->params[i].optional)
                m_filled.insert(i);
        }
    }
    emit commandChanged(name);
    generate();
}

void CommandController::setFormat(Format f)
{
    if (m_format == f)
        return;
    m_format = f;
    generate();
}

void CommandController::setParamValue(int idx, const QString& token)
{
    m_values[idx] = token;
    // Providing a value implicitly includes the parameter in the output.
    if (!token.trimmed().isEmpty())
        m_filled.insert(idx);
    generate();
}

void CommandController::setParamFilled(int idx, bool filled)
{
    if (filled)
        m_filled.insert(idx);
    else
        m_filled.remove(idx);
    generate();
}

void CommandController::generate()
{
    if (!m_current) {
        m_generated.clear();
        emit generatedStringChanged(m_generated);
        return;
    }

    QString cmdName = m_current->name.toLower();
    QString prefix;
    if (m_format == Format::ChatSlash)
        prefix = QStringLiteral("/");
    else if (m_format == Format::ChatAt)
        prefix = QStringLiteral("@");

    // Parameters are separated by "/" (e.g. /give Player/ItemID/Amount); a
    // single space separates the command name from the first parameter. Only
    // color values ("R G B") carry several tokens, which are split into R/G/B.
    QStringList parts;
    for (int i = 0; i < m_current->params.size(); ++i) {
        if (!m_filled.contains(i))
            continue;
        const QString token = m_values.value(i).trimmed();
        if (token.isEmpty())
            continue;
        if (m_current->params[i].primaryType() == ParamType::Color)
            parts.append(token.split(QLatin1Char(' '), Qt::SkipEmptyParts));
        else
            parts.append(token);
    }

    m_generated = prefix + cmdName;
    if (!parts.isEmpty())
        m_generated += QLatin1Char(' ') + parts.join(QLatin1Char('/'));

    emit generatedStringChanged(m_generated);
}
