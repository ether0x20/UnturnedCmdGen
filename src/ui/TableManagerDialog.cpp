#include "TableManagerDialog.h"

#include <QColor>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

#include "controller/TableController.h"
#include "model/AppData.h"

QString TableManagerDialog::displayName(const QString& tableType)
{
    if (tableType == QLatin1String("item")) return QObject::tr("Items");
    if (tableType == QLatin1String("vehicle")) return QObject::tr("Vehicles");
    if (tableType == QLatin1String("animal")) return QObject::tr("Animals");
    if (tableType == QLatin1String("effect")) return QObject::tr("Effects");
    if (tableType == QLatin1String("quest")) return QObject::tr("Quests");
    if (tableType == QLatin1String("achievement")) return QObject::tr("Achievements");
    if (tableType == QLatin1String("skillset")) return QObject::tr("Skillsets");
    if (tableType == QLatin1String("player")) return QObject::tr("Players");
    return tableType;
}

TableManagerDialog::TableManagerDialog(AppData* data, TableController* tables,
                                       const QString& tableType, const QString& filterMod,
                                       QWidget* parent)
    : QDialog(parent)
    , m_data(data)
    , m_tables(tables)
    , m_tableType(tableType)
    , m_filterMod(filterMod)
{
    // The player table has no Chinese/mod columns; it is not mod content.
    const bool hasZh = tableType != QLatin1String("player");
    const bool hasMod = tableType != QLatin1String("player");
    const int columns = (hasZh ? 1 : 0) + (hasMod ? 1 : 0) + 3; // id/name/note + extras

    const QString idHeader = tableType == QLatin1String("player")
        ? tr("SteamID")
        : tr("ID");
    setWindowTitle(tr("Manage %1").arg(displayName(tableType)));
    resize(600, 400);

    auto* vbox = new QVBoxLayout(this);

    m_table = new QTableWidget(0, columns, this);
    {
        QStringList headers;
        headers << idHeader << tr("Name");
        if (hasZh)
            headers << tr("Chinese Name");
        if (hasMod)
            headers << tr("Mod");
        headers << tr("Note");
        m_table->setHorizontalHeaderLabels(headers);
    }
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    vbox->addWidget(m_table, 1);

    const int modCol = hasMod ? (hasZh ? 3 : 2) : -1;
    const int noteCol = columns - 1;

    for (const auto& e : m_data->rawTable(tableType)) {
        if (!m_filterMod.isEmpty() && e.mod != m_filterMod)
            continue;

        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(e.id));
        m_table->setItem(row, 1, new QTableWidgetItem(e.name));
        int col = 2;
        if (hasZh)
            m_table->setItem(row, col++, new QTableWidgetItem(e.nameZh));
        if (hasMod)
            m_table->setItem(row, col++, new QTableWidgetItem(e.mod));
        m_table->setItem(row, noteCol, new QTableWidgetItem(e.note));

        // Grey out entries whose mod is disabled (still fully editable).
        if (hasMod && !m_data->isModEnabled(e.mod)) {
            for (int c = 0; c < columns; ++c) {
                if (auto* it = m_table->item(row, c))
                    it->setForeground(QColor(160, 160, 160));
            }
        }
    }

    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton(tr("Add"), this);
    auto* delBtn = new QPushButton(tr("Remove Selected"), this);
    btnRow->addWidget(addBtn);
    btnRow->addWidget(delBtn);
    btnRow->addStretch(1);
    vbox->addLayout(btnRow);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    vbox->addWidget(buttons);

    connect(addBtn, &QPushButton::clicked, this, &TableManagerDialog::addRow);
    connect(delBtn, &QPushButton::clicked, this, &TableManagerDialog::removeSelected);
    connect(buttons, &QDialogButtonBox::accepted, this, &TableManagerDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void TableManagerDialog::addRow()
{
    const int row = m_table->rowCount();
    m_table->insertRow(row);
    m_table->setCurrentCell(row, 0);
}

void TableManagerDialog::removeSelected()
{
    const QList<QTableWidgetItem*> sel = m_table->selectedItems();
    QSet<int> rows;
    for (auto* item : sel)
        rows.insert(item->row());
    QList<int> sorted = rows.values();
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());
    for (int r : sorted)
        m_table->removeRow(r);
}

void TableManagerDialog::save()
{
    const bool hasZh = m_tableType != QLatin1String("player");
    const bool hasMod = m_tableType != QLatin1String("player");
    const int modCol = hasMod ? (hasZh ? 3 : 2) : -1;
    const int noteCol = (hasZh ? 1 : 0) + (hasMod ? 1 : 0) + 2;

    QVector<TableEntry> entries;
    entries.reserve(m_table->rowCount());
    for (int r = 0; r < m_table->rowCount(); ++r) {
        TableEntry e;
        e.id = m_table->item(r, 0) ? m_table->item(r, 0)->text().trimmed() : QString();
        e.name = m_table->item(r, 1) ? m_table->item(r, 1)->text().trimmed() : QString();
        int col = 2;
        if (hasZh)
            e.nameZh = m_table->item(r, col++) ? m_table->item(r, col - 1)->text().trimmed() : QString();
        if (hasMod)
            e.mod = m_table->item(r, col++) ? m_table->item(r, col - 1)->text().trimmed() : QString();
        e.note = m_table->item(r, noteCol) ? m_table->item(r, noteCol)->text().trimmed() : QString();
        if (e.id.isEmpty() && e.name.isEmpty() && e.nameZh.isEmpty())
            continue;
        entries.append(e);
    }

    m_data->mutableTable(m_tableType) = entries;
    m_data->rebuildTableFilter(m_tableType);
    m_data->discoverMods();
    emit m_data->tableChanged(m_tableType);
    if (m_tables->saveTable(m_tableType)) {
        if (m_tableType == QLatin1String("player"))
            emit m_data->playersChanged();
        accept();
    } else {
        QMessageBox::warning(this, tr("Save"),
                             tr("Failed to write the table file."));
    }
}
