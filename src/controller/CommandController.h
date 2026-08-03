#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

#include "model/Command.h"

class AppData;

// Command generation engine: holds the currently selected command, the output
// format (terminal / chat slash / chat at) and the resolved parameter tokens,
// and assembles the final command string.
class CommandController : public QObject
{
    Q_OBJECT
public:
    enum class Format { Terminal = 0, ChatSlash, ChatAt };

    explicit CommandController(AppData* data, QObject* parent = nullptr);

    const Command* currentCommand() const { return m_current; }
    Format format() const { return m_format; }
    QString formatLabel() const;

    // Token currently entered for param index (empty = not filled).
    QString paramValue(int idx) const;
    // Optional params can be skipped explicitly.
    bool paramFilled(int idx) const;

    QString generatedString() const { return m_generated; }

public slots:
    void selectCommand(const QString& name);
    void setFormat(Format f);
    void setParamValue(int idx, const QString& token);
    void setParamFilled(int idx, bool filled);

signals:
    void commandChanged(const QString& name);
    void generatedStringChanged(const QString& text);

private:
    void generate();

    AppData* m_data = nullptr;
    const Command* m_current = nullptr;
    Format m_format = Format::ChatSlash;
    QHash<int, QString> m_values;
    QSet<int> m_filled;       // optional params the user chose to include
    QString m_generated;
};
