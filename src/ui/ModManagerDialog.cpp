#include "ModManagerDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QVBoxLayout>

#include "controller/TableController.h"
#include "model/AppData.h"
#include "ui/ImportDialog.h"
#include "ui/TableManagerDialog.h"

// Table types (excluding player) whose entries are counted per mod.
static const char* kContentTables[] = {
    "item", "vehicle", "animal", "effect", "quest", "achievement", "skillset"
};

ModManagerDialog::ModManagerDialog(AppData* data, TableController* tables, QWidget* parent)
    : QDialog(parent)
    , m_data(data)
    , m_tables(tables)
{
    setWindowTitle(tr("Mod Management"));
    resize(860, 420);

    auto* vbox = new QVBoxLayout(this);

    m_table = new QTableWidget(0, 2 + 1 + 7, this); // name + id + enabled + 7 counts
    {
        QStringList headers;
        headers << tr("Mod Name") << tr("ID") << tr("Enabled");
        for (const char* t : kContentTables)
            headers << TableManagerDialog::displayName(QString::fromLatin1(t));
        m_table->setHorizontalHeaderLabels(headers);
    }
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    for (int i = 3; i < m_table->columnCount(); ++i)
        m_table->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    vbox->addWidget(m_table, 1);

    auto* btnRow = new QHBoxLayout;
    auto* addBtn = new QPushButton(tr("Add Mod"), this);
    auto* delBtn = new QPushButton(tr("Remove"), this);
    auto* importBtn = new QPushButton(tr("Import Mod Data..."), this);
    auto* manageBtn = new QPushButton(tr("Manage Entries..."), this);
    btnRow->addWidget(addBtn);
    btnRow->addWidget(delBtn);
    btnRow->addWidget(importBtn);
    btnRow->addWidget(manageBtn);
    btnRow->addStretch(1);
    vbox->addLayout(btnRow);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    buttons->button(QDialogButtonBox::Close)->setText(tr("Close"));
    vbox->addWidget(buttons);

    connect(addBtn, &QPushButton::clicked, this, &ModManagerDialog::addMod);
    connect(delBtn, &QPushButton::clicked, this, &ModManagerDialog::removeSelectedMod);
    connect(importBtn, &QPushButton::clicked, this, &ModManagerDialog::importModData);
    connect(manageBtn, &QPushButton::clicked, this, &ModManagerDialog::manageEntries);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    connect(m_data, &AppData::modsChanged, this, &ModManagerDialog::refresh);

    refresh();
}

void ModManagerDialog::refresh()
{
    m_table->setRowCount(0);

    const int nameCol = 0;
    const int idCol = 1;
    const int enabledCol = 2;
    const int countCol0 = 3;

    // Virtual vanilla mod (empty id), always enabled and locked.
    {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto* nameItem = new QTableWidgetItem(tr("Vanilla (Base Game)"));
        nameItem->setData(Qt::UserRole, QString());
        m_table->setItem(row, nameCol, nameItem);
        m_table->setItem(row, idCol, new QTableWidgetItem());
        auto* check = new QCheckBox();
        check->setChecked(true);
        check->setEnabled(false);
        m_table->setCellWidget(row, enabledCol, check);
        for (int i = 0; i < 7; ++i) {
            const QString type = QString::fromLatin1(kContentTables[i]);
            m_table->setItem(row, countCol0 + i,
                             new QTableWidgetItem(QString::number(m_data->modEntryCount(QString(), type))));
        }
    }

    // Registered mods, sorted by display name.
    const QHash<QString, ModInfo> mods = m_data->mods();
    QList<QString> ids = mods.keys();
    ids.removeAll(QString());
    std::sort(ids.begin(), ids.end(), [&mods](const QString& a, const QString& b) {
        return mods[a].name.compare(mods[b].name, Qt::CaseInsensitive) < 0;
    });
    for (const QString& id : ids) {
        const ModInfo& mi = mods[id];
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto* nameItem = new QTableWidgetItem(mi.name);
        nameItem->setData(Qt::UserRole, mi.id);
        m_table->setItem(row, nameCol, nameItem);
        m_table->setItem(row, idCol, new QTableWidgetItem(mi.id));

        auto* check = new QCheckBox();
        check->setChecked(mi.enabled);
        if (mi.locked)
            check->setEnabled(false);
        m_table->setCellWidget(row, enabledCol, check);
        connect(check, &QCheckBox::toggled, this, [this, id](bool on) { applyModToggle(id, on); });

        for (int i = 0; i < 7; ++i) {
            const QString type = QString::fromLatin1(kContentTables[i]);
            m_table->setItem(row, countCol0 + i,
                             new QTableWidgetItem(QString::number(m_data->modEntryCount(id, type))));
        }
    }
}

QString ModManagerDialog::selectedModId() const
{
    const auto* item = m_table->item(m_table->currentRow(), 0);
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ModManagerDialog::applyModToggle(const QString& modId, bool enabled)
{
    if (m_data->setModEnabled(modId, enabled)) {
        m_data->saveMods();
        // Counts are unchanged by toggling; no need to refresh the table.
    }
}

void ModManagerDialog::addMod()
{
    const QString name = QInputDialog::getText(
        this, tr("New Mod"), tr("Mod name:"));
    if (name.trimmed().isEmpty())
        return;
    QString id = QInputDialog::getText(
        this, tr("New Mod"),
        tr("Mod ID (unique, used in data files):"));
    id = id.trimmed();
    if (id.isEmpty()) {
        id = name.trimmed();
        id.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_]+")), QStringLiteral("_"));
        id = id.toLower();
    }
    if (!m_data->addMod(id, name.trimmed())) {
        QMessageBox::warning(this, tr("New Mod"),
                             tr("A mod with this ID already exists."));
        return;
    }
    m_data->saveMods();
}

void ModManagerDialog::removeSelectedMod()
{
    const QString id = selectedModId();
    if (id.isEmpty()) {
        QMessageBox::information(this, tr("Remove"),
                                 tr("Vanilla cannot be removed."));
        return;
    }
    const int count = m_data->modEntryCount(id, QStringLiteral("item"))
                    + m_data->modEntryCount(id, QStringLiteral("vehicle"))
                    + m_data->modEntryCount(id, QStringLiteral("animal"))
                    + m_data->modEntryCount(id, QStringLiteral("effect"))
                    + m_data->modEntryCount(id, QStringLiteral("quest"))
                    + m_data->modEntryCount(id, QStringLiteral("achievement"))
                    + m_data->modEntryCount(id, QStringLiteral("skillset"));
    const auto answer = QMessageBox::question(
        this, tr("Remove Mod"),
        tr("Remove mod \"%1\" and all %2 of its entries?")
            .arg(m_data->modInfo(id).name)
            .arg(count));
    if (answer != QMessageBox::Yes)
        return;
    if (m_data->removeMod(id)) {
        m_data->saveMods();
        refresh();
    }
}

void ModManagerDialog::importModData()
{
    const QString modId = selectedModId();
    ImportDialog dlg(m_data, m_tables, modId, this);
    dlg.exec();
}

void ModManagerDialog::manageEntries()
{
    const QString modId = selectedModId();
    const QStringList types = {QStringLiteral("item"), QStringLiteral("vehicle"),
                               QStringLiteral("animal"), QStringLiteral("effect"),
                               QStringLiteral("quest"), QStringLiteral("achievement"),
                               QStringLiteral("skillset")};
    QStringList labels;
    for (const QString& t : types)
        labels << TableManagerDialog::displayName(t);
    const QString label = QInputDialog::getItem(
        this, tr("Manage Entries"), tr("Table:"), labels, 0, false);
    if (label.isEmpty())
        return;
    const QString type = types[labels.indexOf(label)];
    TableManagerDialog dlg(m_data, m_tables, type, modId, this);
    dlg.exec();
}
