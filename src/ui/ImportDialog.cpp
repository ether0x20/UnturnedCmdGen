#include "ImportDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>

#include "controller/TableController.h"
#include "model/AppData.h"

const char* ImportDialog::kKeepFromFile = "keep_from_file";
const char* ImportDialog::kVanilla = "vanilla";

ImportDialog::ImportDialog(AppData* data, TableController* tables,
                           const QString& preselectMod, QWidget* parent)
    : QDialog(parent)
    , m_data(data)
    , m_tables(tables)
{
    setWindowTitle(tr("Import JSON Data"));
    resize(460, 240);

    auto* vbox = new QVBoxLayout(this);

    auto* form = new QFormLayout;
    m_path = new QLineEdit(this);
    m_path->setPlaceholderText(tr("Path to a .json file"));
    auto* browseBtn = new QPushButton(tr("Browse..."), this);
    auto* pathRow = new QHBoxLayout;
    pathRow->addWidget(m_path, 1);
    pathRow->addWidget(browseBtn);
    form->addRow(tr("File"), pathRow);

    m_tableCombo = new QComboBox(this);
    for (const QString& t : m_data->tableTypes())
        m_tableCombo->addItem(t, t);
    form->addRow(tr("Target table"), m_tableCombo);

    m_modCombo = new QComboBox(this);
    m_modCombo->setToolTip(tr("Tag imported entries with a mod, or keep the mod field from the file"));
    refreshModCombo(preselectMod);
    form->addRow(tr("Mod"), m_modCombo);

    auto* modeRow = new QHBoxLayout;
    m_merge = new QRadioButton(tr("Merge (skip duplicate ids)"), this);
    m_replace = new QRadioButton(tr("Replace whole table"), this);
    m_merge->setChecked(true);
    modeRow->addWidget(m_merge);
    modeRow->addWidget(m_replace);
    form->addRow(tr("Mode"), modeRow);

    vbox->addLayout(form);

    auto* tutorialBtn = new QPushButton(tr("?  Import Tutorial"), this);
    tutorialBtn->setToolTip(tr("Show a simple format example"));
    vbox->addWidget(tutorialBtn, 0, Qt::AlignLeft);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Import"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    vbox->addWidget(buttons);

    connect(browseBtn, &QPushButton::clicked, this, &ImportDialog::browse);
    connect(tutorialBtn, &QPushButton::clicked, this, &ImportDialog::showTutorial);
    connect(buttons, &QDialogButtonBox::accepted, this, &ImportDialog::doImport);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ImportDialog::refreshModCombo(const QString& selectMod)
{
    const QSignalBlocker block(m_modCombo);
    m_modCombo->clear();

    m_modCombo->addItem(tr("(Keep as in file)"), QLatin1String(kKeepFromFile));
    m_modCombo->addItem(tr("(Vanilla)"), QLatin1String(kVanilla));

    // One entry per known mod: "Name (id)".
    const QHash<QString, ModInfo>& mods = m_data->mods();
    QList<QString> ids = mods.keys();
    std::sort(ids.begin(), ids.end(), [&mods](const QString& a, const QString& b) {
        return mods[a].name.compare(mods[b].name, Qt::CaseInsensitive) < 0;
    });
    for (const QString& id : ids) {
        if (id.isEmpty())
            continue;
        m_modCombo->addItem(QStringLiteral("%1 (%2)").arg(mods[id].name, id), id);
    }

    if (!selectMod.isEmpty()) {
        const int idx = m_modCombo->findData(selectMod);
        if (idx >= 0)
            m_modCombo->setCurrentIndex(idx);
    }
}

void ImportDialog::browse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Select JSON file"),
        QDir::homePath(), QStringLiteral("JSON files (*.json)"));
    if (!path.isEmpty())
        m_path->setText(path);
}

void ImportDialog::showTutorial()
{
    QDialog dlg(this);
    dlg.setWindowTitle(tr("Import Format Example"));
    dlg.resize(560, 400);
    auto* vbox = new QVBoxLayout(&dlg);

    auto* text = new QPlainTextEdit(&dlg);
    text->setReadOnly(true);
    text->setFont(QFont(QStringLiteral("Monospace")));

    // The example JSON itself is language-independent; only the prose is
    // translated.
    const QString exampleItems =
        QStringLiteral("[\n"
                       "  { \"id\": \"116\", \"name\": \"Military Knife\", \"note\": \"melee\" },\n"
                       "  { \"id\": \"363\", \"name\": \"Maple Rifle\" }\n"
                       "]\n");
    const QString examplePlayers =
        QStringLiteral("[\n"
                       "  { \"steamId\": \"76561198000000001\", \"name\": \"Ethan\", \"note\": \"owner\" }\n"
                       "]\n");

    QString body;
    body += tr("Unified format: top level must be a JSON array; each element is one entry.")
        + QStringLiteral("\n\n") + exampleItems + QStringLiteral("\n");
    body += tr("Player table (uses steamId):") + QStringLiteral("\n") + examplePlayers + QStringLiteral("\n");
    body += tr("Fields:") + QStringLiteral("\n")
        + tr("  id    primary key (item/vehicle id; for players use steamId)") + QStringLiteral("\n")
        + tr("  name  display name, used as the parameter value in commands") + QStringLiteral("\n")
        + tr("  note  optional extra info") + QStringLiteral("\n")
        + tr("  mod   id of the mod this entry belongs to (empty = vanilla)") + QStringLiteral("\n\n");
    body += tr("Rules:") + QStringLiteral("\n")
        + tr("  * At least one of id/name must be set, otherwise the entry is skipped.") + QStringLiteral("\n")
        + tr("  * Merge appends and skips duplicate ids; Replace clears the table first.") + QStringLiteral("\n");
    text->setPlainText(body);

    vbox->addWidget(text, 1);

    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, &dlg);
    close->button(QDialogButtonBox::Close)->setText(tr("Close"));
    connect(close, &QDialogButtonBox::rejected, &dlg, &QDialog::accept);
    vbox->addWidget(close);

    dlg.exec();
}

void ImportDialog::doImport()
{
    const QString path = m_path->text().trimmed();
    if (path.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("Import"),
                             tr("Please choose a JSON file first."));
        return;
    }
    if (!QFile::exists(path)) {
        QMessageBox::warning(this, QStringLiteral("Import"),
                             tr("File does not exist:\n%1").arg(path));
        return;
    }

    // "(Keep as in file)" -> empty override; "(Vanilla)" -> kForceVanilla;
    // otherwise a specific mod id overrides every entry.
    QString modOverride;
    const QString sel = m_modCombo->currentData().toString();
    if (sel == QLatin1String(kVanilla))
        modOverride = TableController::kForceVanilla;
    else if (sel != QLatin1String(kKeepFromFile))
        modOverride = sel;

    const bool ok = m_tables->importJson(path, m_tableCombo->currentData().toString(),
                                         m_merge->isChecked(), modOverride);
    if (ok) {
        QMessageBox::information(this, QStringLiteral("Import"),
                                 tr("Imported into \"%1\" successfully.")
                                     .arg(m_tableCombo->currentData().toString()));
        accept();
    } else {
        QMessageBox::warning(this, QStringLiteral("Import"),
                             tr("Import failed: the file is invalid, "
                                "empty, or contains no usable entries.\n"
                                "Click \"? Import Tutorial\" for the format."));
    }
}
