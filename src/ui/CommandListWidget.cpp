#include "CommandListWidget.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>

#include "controller/CommandController.h"
#include "model/AppData.h"

CommandListWidget::CommandListWidget(AppData* data, CommandController* controller, QWidget* parent)
    : QWidget(parent)
    , m_data(data)
    , m_controller(controller)
{
    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(6, 6, 6, 6);

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText(tr("Search commands..."));
    m_search->setClearButtonEnabled(true);
    vbox->addWidget(m_search);

    m_runtime = new QCheckBox(tr("Runtime"), this);
    m_cheat = new QCheckBox(tr("Cheat"), this);
    m_config = new QCheckBox(tr("Config"), this);
    m_any = new QCheckBox(tr("Any"), this);

    auto* filt = new QWidget(this);
    auto* grid = new QGridLayout(filt);
    grid->setContentsMargins(0, 0, 0, 0);
    grid->addWidget(m_runtime, 0, 0);
    grid->addWidget(m_cheat, 0, 1);
    grid->addWidget(m_config, 1, 0);
    grid->addWidget(m_any, 1, 1);
    vbox->addWidget(filt);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    vbox->addWidget(m_list, 1);

    connect(m_search, &QLineEdit::textChanged, this, &CommandListWidget::applyFilter);
    connect(m_runtime, &QCheckBox::toggled, this, &CommandListWidget::applyFilter);
    connect(m_cheat, &QCheckBox::toggled, this, &CommandListWidget::applyFilter);
    connect(m_config, &QCheckBox::toggled, this, &CommandListWidget::applyFilter);
    connect(m_any, &QCheckBox::toggled, this, &CommandListWidget::applyFilter);
    connect(m_list, &QListWidget::itemActivated, this, &CommandListWidget::onItemActivated);
    connect(m_list, &QListWidget::itemClicked, this, &CommandListWidget::onItemActivated);
}

QString CommandListWidget::currentCommand() const
{
    const auto* item = m_list->currentItem();
    return item ? item->text() : QString();
}

void CommandListWidget::selectFirst()
{
    if (m_list->count() > 0)
        m_list->setCurrentRow(0);
}

void CommandListWidget::applyFilter()
{
    const QString q = m_search->text().trimmed();
    const bool fRuntime = m_runtime->isChecked();
    const bool fCheat = m_cheat->isChecked();
    const bool fConfig = m_config->isChecked();
    const bool fAny = m_any->isChecked();
    const bool anyFilter = fRuntime || fCheat || fConfig || fAny;

    m_list->clear();
    for (const auto& c : m_data->commands()) {
        if (!q.isEmpty() && !c.name.contains(q, Qt::CaseInsensitive))
            continue;
        if (anyFilter) {
            bool match = false;
            if (fRuntime && c.classifications.contains(QStringLiteral("runtime"))) match = true;
            if (fCheat && c.classifications.contains(QStringLiteral("cheat"))) match = true;
            if (fConfig && c.classifications.contains(QStringLiteral("config"))) match = true;
            if (fAny && c.classifications.contains(QStringLiteral("any"))) match = true;
            if (!match)
                continue;
        }
        auto* item = new QListWidgetItem(c.name, m_list);
        item->setData(Qt::UserRole, c.name);
        if (c.isCheat())
            item->setText(c.name + QStringLiteral("  ⚠"));
    }
}

void CommandListWidget::onItemActivated(QListWidgetItem* item)
{
    if (!item)
        return;
    m_controller->selectCommand(item->data(Qt::UserRole).toString());
    emit commandSelected(item->data(Qt::UserRole).toString());
}
