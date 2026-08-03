#include "ParameterWidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "controller/CommandController.h"
#include "model/AppData.h"
#include "model/Command.h"

namespace {
// Readable label for a parameter type shown in multi-type mode selectors.
QString typeLabel(ParamType::Type t)
{
    switch (t) {
    case ParamType::Player: return QObject::tr("Player");
    case ParamType::Location: return QObject::tr("Location");
    case ParamType::Enum: return QObject::tr("Preset");
    case ParamType::Guid: return QObject::tr("Custom GUID");
    case ParamType::Item: return QObject::tr("Item");
    case ParamType::Vehicle: return QObject::tr("Vehicle");
    default: return ParamType::toString(t);
    }
}

QString tableNameFor(ParamType::Type t)
{
    switch (t) {
    case ParamType::Item: return QStringLiteral("item");
    case ParamType::Vehicle: return QStringLiteral("vehicle");
    case ParamType::Animal: return QStringLiteral("animal");
    case ParamType::Effect: return QStringLiteral("effect");
    case ParamType::Quest: return QStringLiteral("quest");
    case ParamType::Achievement: return QStringLiteral("achievement");
    case ParamType::Skillset: return QStringLiteral("skillset");
    case ParamType::Player: return QStringLiteral("player");
    default: return QString();
    }
}
} // namespace

ParameterWidget::ParameterWidget(AppData* data, CommandController* controller, QWidget* parent)
    : QWidget(parent)
    , m_data(data)
    , m_controller(controller)
{
    auto* vbox = new QVBoxLayout(this);

    m_warning = new QLabel(this);
    m_warning->setStyleSheet(QStringLiteral("color:#c0392b;"));
    m_warning->setWordWrap(true);
    m_warning->hide();
    vbox->addWidget(m_warning);

    m_form = new QFormLayout;
    m_form->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    m_form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    vbox->addLayout(m_form);
    vbox->addStretch(1);

    connect(m_controller, &CommandController::commandChanged, this, &ParameterWidget::rebuild);
    connect(m_controller, &CommandController::generatedStringChanged, this,
            [this] { refreshWarning(); });
    // Keep lookup dropdowns in sync with table edits made elsewhere.
    connect(m_data, &AppData::tableChanged, this, &ParameterWidget::rebuild);
}

bool ParameterWidget::isComplete() const
{
    if (!m_cmd)
        return true;
    for (int i = 0; i < m_cmd->params.size(); ++i) {
        const Parameter& p = m_cmd->params[i];
        if (!p.optional && m_controller->paramValue(i).trimmed().isEmpty())
            return false;
    }
    return true;
}

void ParameterWidget::rebuild()
{
    while (m_form->rowCount() > 0)
        m_form->removeRow(0);

    m_cmd = m_controller->currentCommand();
    if (!m_cmd) {
        refreshWarning();
        return;
    }

    if (!m_cmd->description.isEmpty()) {
        auto* desc = new QLabel(m_cmd->description, this);
        desc->setWordWrap(true);
        desc->setStyleSheet(QStringLiteral("color:#7f8c8d;"));
        m_form->addRow(desc);
    }

    for (int i = 0; i < m_cmd->params.size(); ++i)
        addEditorRow(m_cmd->params[i], i, createEditor(m_cmd->params[i], i));

    refreshWarning();
}

ParameterWidget::Editor ParameterWidget::createEditor(const Parameter& param, int idx)
{
    if (param.types.size() > 1)
        return createMultiTypeEditor(param, idx);

    const ParamType::Type t = param.primaryType();
    switch (t) {
    case ParamType::Player:
    case ParamType::Item:
    case ParamType::Vehicle:
    case ParamType::Animal:
    case ParamType::Effect:
    case ParamType::Quest:
    case ParamType::Achievement:
    case ParamType::Skillset:
        return createLookupEditor(param, tableNameFor(t), idx);
    case ParamType::Location:
        return createLocationEditor(param, idx);
    case ParamType::Map:
        return createMapEditor(param, idx);
    case ParamType::Enum:
        return createEnumEditor(param, idx);
    case ParamType::Boolean:
        return createBooleanEditor(param, idx);
    case ParamType::Integer:
        return createIntegerEditor(param, false, idx);
    case ParamType::Float:
        return createIntegerEditor(param, true, idx);
    case ParamType::Color:
        return createColorEditor(param, idx);
    case ParamType::Duration:
        return createDurationEditor(param, idx);
    case ParamType::String:
    case ParamType::Guid:
    case ParamType::Flag:
        return createTextEditor(param, QString(), idx);
    default:
        return createTextEditor(param, QString(), idx);
    }
}

ParameterWidget::Editor ParameterWidget::createLookupEditor(const Parameter& param, const QString& tableType, int idx)
{
    Editor ed;
    auto* combo = new QComboBox(this);
    combo->setEditable(true);
    combo->setInsertPolicy(QComboBox::NoInsert);

    const auto& entries = m_data->table(tableType);
    // The value substituted into the command: the numeric/game id for content
    // tables (e.g. /give <player> 363 5), but the display name for players
    // (the game accepts [SteamID | Player]).
    const bool playerTable = tableType == QLatin1String("player");
    QStringList names;
    {
        const QSignalBlocker block(combo);
        for (const auto& e : entries) {
            const QString cmdValue = playerTable ? e.name : e.id;
            combo->addItem(m_data->entryLabel(e), cmdValue);
            // Matching roles: +1 = English name, +2 = Chinese annotation, so
            // a name/Chinese selection still resolves back to the command value.
            combo->setItemData(combo->count() - 1, e.name, Qt::UserRole + 1);
            combo->setItemData(combo->count() - 1, e.nameZh, Qt::UserRole + 2);
            names.append(e.name);
            if (!e.nameZh.isEmpty())
                names.append(e.nameZh);
        }
        if (combo->count() > 0)
            combo->setCurrentIndex(-1);
    }

    auto* completer = new QCompleter(names, combo);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    combo->setCompleter(completer);

    if (playerTable)
        combo->setPlaceholderText(tr("(executing player) or SteamID"));

    ed.widget = combo;
    ed.filled = [combo] { return !combo->currentText().trimmed().isEmpty(); };
    ed.token = [combo] {
        const QString text = combo->currentText().trimmed();
        if (text.isEmpty())
            return QString();
        for (int i = 0; i < combo->count(); ++i) {
            if (combo->itemText(i) == text) // picked the full "[id] name / 中文" entry
                return combo->itemData(i, Qt::UserRole).toString();
            if (combo->itemData(i, Qt::UserRole + 1).toString().compare(text, Qt::CaseInsensitive) == 0)
                return combo->itemData(i, Qt::UserRole).toString();
            if (combo->itemData(i, Qt::UserRole + 2).toString() == text) // Chinese name
                return combo->itemData(i, Qt::UserRole).toString();
        }
        return text;
    };
    connect(combo, &QComboBox::currentTextChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createLocationEditor(const Parameter& param, int idx)
{
    Q_UNUSED(param)
    Editor ed;
    auto* widget = new QWidget(this);
    auto* hbox = new QHBoxLayout(widget);
    hbox->setContentsMargins(0, 0, 0, 0);

    auto* mapCombo = new QComboBox(widget);
    mapCombo->setToolTip(tr("Map the location belongs to"));
    {
        const QSignalBlocker block(mapCombo);
        mapCombo->addItem(tr("<map>"), QString());
        for (const auto& m : m_data->maps())
            mapCombo->addItem(m.name, m.id);
    }

    auto* locCombo = new QComboBox(widget);
    locCombo->setEditable(true);
    locCombo->setInsertPolicy(QComboBox::NoInsert);
    locCombo->setPlaceholderText(tr("location or \"wp\" (waypoint)"));
    locCombo->setToolTip(tr("Pick a location for the selected map, or type one directly"));

    hbox->addWidget(mapCombo);
    hbox->addWidget(locCombo, 1);

    auto populateLocations = [this, locCombo] {
        const QSignalBlocker block(locCombo);
        locCombo->clear();
        locCombo->addItem(QStringLiteral("wp"), QStringLiteral("wp"));
        const QString mapId = locCombo->property("_mapId").toString();
        const QStringList locs = mapId.isEmpty() ? QStringList() : m_data->locationsForMap(mapId);
        for (const QString& l : locs)
            locCombo->addItem(l, l);
    };
    connect(mapCombo, &QComboBox::currentIndexChanged, this,
            [this, mapCombo, locCombo, populateLocations](int) {
                locCombo->setProperty("_mapId", mapCombo->currentData().toString());
                populateLocations();
            });
    populateLocations();

    ed.widget = widget;
    ed.filled = [locCombo] { return !locCombo->currentText().trimmed().isEmpty(); };
    ed.token = [locCombo] { return locCombo->currentText().trimmed(); };
    connect(locCombo, &QComboBox::currentTextChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createMapEditor(const Parameter& param, int idx)
{
    Q_UNUSED(param)
    Editor ed;
    auto* combo = new QComboBox(this);
    {
        const QSignalBlocker block(combo);
        for (const auto& m : m_data->maps())
            combo->addItem(m.name, m.id);
    }
    ed.widget = combo;
    ed.filled = [] { return true; };
    ed.token = [combo] { return combo->currentData().toString(); };
    connect(combo, &QComboBox::currentIndexChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createEnumEditor(const Parameter& param, int idx)
{
    Editor ed;
    auto* combo = new QComboBox(this);
    {
        const QSignalBlocker block(combo);
        for (const QString& v : param.values)
            combo->addItem(v, v);
        if (!param.defaultValue.isEmpty()) {
            const int d = combo->findText(param.defaultValue);
            if (d >= 0)
                combo->setCurrentIndex(d);
        }
    }
    ed.widget = combo;
    ed.filled = [] { return true; };
    ed.token = [combo] { return combo->currentText().trimmed(); };
    connect(combo, &QComboBox::currentIndexChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createBooleanEditor(const Parameter& param, int idx)
{
    Q_UNUSED(param)
    Editor ed;
    auto* combo = new QComboBox(this);
    combo->addItem(QStringLiteral("Yes"), QStringLiteral("Y"));
    combo->addItem(QStringLiteral("No"), QStringLiteral("N"));
    ed.widget = combo;
    ed.filled = [] { return true; };
    ed.token = [combo] { return combo->currentData().toString(); };
    connect(combo, &QComboBox::currentIndexChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createIntegerEditor(const Parameter& param, bool isFloat, int idx)
{
    Editor ed;
    const double lo = param.hasMin ? param.min : -1000000000.0;
    const double hi = param.hasMax ? param.max : 1000000000.0;

    QWidget* spin = nullptr;
    if (isFloat) {
        auto* s = new QDoubleSpinBox(this);
        s->setRange(lo, hi);
        s->setDecimals(4);
        if (!param.defaultValue.isEmpty())
            s->setValue(param.defaultValue.toDouble());
        spin = s;
        ed.token = [s] { return QString::number(s->value(), 'g', 10); };
        connect(s, &QDoubleSpinBox::valueChanged, this, [this, idx, ed] {
            m_controller->setParamValue(idx, ed.token());
        });
    } else {
        auto* s = new QSpinBox(this);
        s->setRange(qRound(lo), qRound(hi));
        if (!param.defaultValue.isEmpty())
            s->setValue(param.defaultValue.toInt());
        spin = s;
        ed.token = [s] { return QString::number(s->value()); };
        connect(s, &QSpinBox::valueChanged, this, [this, idx, ed] {
            m_controller->setParamValue(idx, ed.token());
        });
    }
    ed.widget = spin;
    ed.filled = [] { return true; };
    return ed;
}

ParameterWidget::Editor ParameterWidget::createTextEditor(const Parameter& param, const QString& placeholder, int idx)
{
    Editor ed;
    auto* line = new QLineEdit(this);
    if (!placeholder.isEmpty())
        line->setPlaceholderText(placeholder);
    else if (param.optional)
        line->setPlaceholderText(tr("(optional)"));
    if (param.hasMin || param.hasMax) {
        line->setToolTip(tr("Length %1%2%3")
                             .arg(param.hasMin ? QString::number(qRound(param.min)) : QString())
                             .arg(param.hasMin && param.hasMax ? QStringLiteral("-") : QString())
                             .arg(param.hasMax ? QString::number(qRound(param.max)) : QString()));
    }
    if (!param.defaultValue.isEmpty() && param.defaultValue != QLatin1String("executing player"))
        line->setText(param.defaultValue);

    ed.widget = line;
    ed.filled = [line] { return !line->text().trimmed().isEmpty(); };
    ed.token = [line] { return line->text().trimmed(); };
    connect(line, &QLineEdit::textChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createColorEditor(const Parameter& param, int idx)
{
    Q_UNUSED(param)
    Editor ed;
    auto* widget = new QWidget(this);
    auto* hbox = new QHBoxLayout(widget);
    hbox->setContentsMargins(0, 0, 0, 0);

    auto* r = new QSpinBox(widget);
    auto* g = new QSpinBox(widget);
    auto* b = new QSpinBox(widget);
    for (auto* s : {r, g, b}) {
        s->setRange(0, 255);
        hbox->addWidget(s);
    }
    hbox->addStretch(1);

    ed.widget = widget;
    ed.filled = [r, g, b] { return !(r->value() == 0 && g->value() == 0 && b->value() == 0); };
    ed.token = [r, g, b] {
        if (r->value() == 0 && g->value() == 0 && b->value() == 0)
            return QString();
        return QStringLiteral("%1 %2 %3").arg(r->value()).arg(g->value()).arg(b->value());
    };
    auto notify = [this, idx, ed] { m_controller->setParamValue(idx, ed.token()); };
    connect(r, &QSpinBox::valueChanged, this, notify);
    connect(g, &QSpinBox::valueChanged, this, notify);
    connect(b, &QSpinBox::valueChanged, this, notify);
    return ed;
}

ParameterWidget::Editor ParameterWidget::createDurationEditor(const Parameter& param, int idx)
{
    Q_UNUSED(param)
    Editor ed;
    auto* widget = new QWidget(this);
    auto* hbox = new QHBoxLayout(widget);
    hbox->setContentsMargins(0, 0, 0, 0);

    auto* line = new QLineEdit(widget);
    line->setPlaceholderText(tr("seconds, e.g. 604800"));

    auto* preset = new QComboBox(widget);
    preset->addItem(tr("Preset"), QString());
    preset->addItem(tr("1 hour"), QStringLiteral("3600"));
    preset->addItem(tr("1 day"), QStringLiteral("86400"));
    preset->addItem(tr("7 days"), QStringLiteral("604800"));
    preset->addItem(tr("30 days"), QStringLiteral("2592000"));
    preset->addItem(tr("365 days"), QStringLiteral("31536000"));

    hbox->addWidget(line, 1);
    hbox->addWidget(preset);

    connect(preset, &QComboBox::currentIndexChanged, this, [line, preset] {
        const QString v = preset->currentData().toString();
        if (!v.isEmpty())
            line->setText(v);
    });

    ed.widget = widget;
    ed.filled = [line] { return !line->text().trimmed().isEmpty(); };
    ed.token = [line] { return line->text().trimmed(); };
    connect(line, &QLineEdit::textChanged, this, [this, idx, ed] {
        m_controller->setParamValue(idx, ed.token());
    });
    return ed;
}

ParameterWidget::Editor ParameterWidget::createMultiTypeEditor(const Parameter& param, int idx)
{
    Editor ed;
    auto* widget = new QWidget(this);
    auto* vbox = new QVBoxLayout(widget);
    vbox->setContentsMargins(0, 0, 0, 0);
    vbox->setSpacing(4);

    auto* modeCombo = new QComboBox(widget);
    auto* stack = new QStackedWidget(widget);

    const QVector<ParamType::Type> types = param.typeList();
    for (const ParamType::Type t : types)
        modeCombo->addItem(typeLabel(t));

    // One editor page per accepted type. Sub-pages must NOT notify the
    // controller directly (they are only meaningful when active); we re-read
    // the active page's token in the wrapper.
    struct Page { Editor e; };
    QVector<Page> pages;
    pages.reserve(types.size());
    for (int i = 0; i < types.size(); ++i) {
        Editor page;
        const ParamType::Type t = types[i];
        const QString tbl = tableNameFor(t);
        switch (t) {
        case ParamType::Player:
        case ParamType::Item:
        case ParamType::Vehicle:
        case ParamType::Animal:
        case ParamType::Effect:
        case ParamType::Quest:
        case ParamType::Achievement:
        case ParamType::Skillset:
            page = createLookupEditor(param, tbl, idx);
            break;
        case ParamType::Location:
            page = createLocationEditor(param, idx);
            break;
        case ParamType::Enum:
            page = createEnumEditor(param, idx);
            break;
        case ParamType::Guid:
            page = createTextEditor(param, tr("asset GUID"), idx);
            break;
        default:
            page = createTextEditor(param, QString(), idx);
            break;
        }
        pages.append(Page{page});
        stack->addWidget(page.widget);
    }

    ed.widget = widget;
    ed.filled = [pages, stack] {
        const int a = stack->currentIndex();
        return a >= 0 && a < pages.size() && pages[a].e.filled();
    };
    ed.token = [pages, stack] {
        const int a = stack->currentIndex();
        return (a >= 0 && a < pages.size()) ? pages[a].e.token() : QString();
    };
    // Keep sub-editors from pushing to the controller while a page is inactive.
    // We disable page widgets; only the active page is enabled so only its
    // changes can reach the controller. On mode switch we push the new token.

    for (int i = 0; i < stack->count(); ++i)
        stack->widget(i)->setEnabled(i == 0);

    connect(modeCombo, &QComboBox::currentIndexChanged, this, [this, idx, ed, stack](int i) {
        stack->setCurrentIndex(i);
        for (int j = 0; j < stack->count(); ++j)
            stack->widget(j)->setEnabled(j == i);
        m_controller->setParamValue(idx, ed.token());
        m_controller->setParamFilled(idx, ed.filled());
    });
    return ed;
}

void ParameterWidget::addEditorRow(const Parameter& param, int idx, const Editor& ed)
{
    QString label = param.name;
    if (param.optional)
        label += QLatin1Char(' ') + tr("(optional)");
    if (!param.defaultValue.isEmpty() && param.defaultValue != QLatin1String("executing player"))
        label += QStringLiteral(" ") + tr("[default: %1]").arg(param.defaultValue);

    if (param.optional) {
        auto* row = new QWidget(this);
        auto* hbox = new QHBoxLayout(row);
        hbox->setContentsMargins(0, 0, 0, 0);
        hbox->setSpacing(6);
        auto* check = new QCheckBox(row);
        hbox->addWidget(check);
        hbox->addWidget(ed.widget, 1);
        ed.widget->setEnabled(false);
        connect(check, &QCheckBox::toggled, this, [this, idx, ed, check](bool on) {
            ed.widget->setEnabled(on);
            m_controller->setParamFilled(idx, on);
            if (on)
                m_controller->setParamValue(idx, ed.token());
        });
        m_form->addRow(label, row);
        return;
    }

    m_form->addRow(label, ed.widget);
}

void ParameterWidget::refreshWarning()
{
    if (!m_cmd) {
        m_warning->hide();
        return;
    }
    QStringList missing;
    for (int i = 0; i < m_cmd->params.size(); ++i) {
        const Parameter& p = m_cmd->params[i];
        if (!p.optional && m_controller->paramValue(i).trimmed().isEmpty())
            missing.append(p.name);
    }
    if (!missing.isEmpty()) {
        m_warning->setText(tr("Missing required: %1").arg(missing.join(QStringLiteral(", "))));
        m_warning->show();
    } else {
        m_warning->hide();
    }
}
