#include "MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>

#include "controller/AppController.h"
#include "controller/CommandController.h"
#include "controller/RconClient.h"
#include "controller/TableController.h"
#include "model/AppData.h"
#include "ui/CommandListWidget.h"
#include "ui/ImportDialog.h"
#include "ui/OutputWidget.h"
#include "ui/ParameterWidget.h"
#include "ui/SettingsDialog.h"
#include "ui/TableManagerDialog.h"
#include "util/Translator.h"

MainWindow::MainWindow(AppData* data, CommandController* commands,
                       TableController* tables, RconClient* rcon,
                       TranslationManager* lang, QWidget* parent)
    : QMainWindow(parent)
    , m_data(data)
    , m_commands(commands)
    , m_tables(tables)
    , m_rcon(rcon)
    , m_lang(lang)
{
    setWindowTitle(QStringLiteral("Unturned Command Generator"));
    resize(960, 640);

    // --- Central splitter ------------------------------------------------
    m_splitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_splitter);

    m_commandList = new CommandListWidget(m_data, m_commands, this);
    m_commandList->setMinimumWidth(180);
    m_splitter->addWidget(m_commandList);

    auto* right = new QWidget(this);
    auto* rvbox = new QVBoxLayout(right);
    rvbox->setContentsMargins(0, 0, 0, 0);

    auto* scroll = new QScrollArea(right);
    scroll->setWidgetResizable(true);
    m_paramPanel = new ParameterWidget(m_data, m_commands, scroll);
    scroll->setWidget(m_paramPanel);
    rvbox->addWidget(scroll, 1);

    m_output = new OutputWidget(m_commands, m_rcon, right);
    rvbox->addWidget(m_output);
    m_splitter->addWidget(right);

    m_splitter->setSizes({260, 700});

    // --- Menus -----------------------------------------------------------
    auto* fileMenu = menuBar()->addMenu(tr("File"));
    QAction* importAct = fileMenu->addAction(tr("Import JSON..."));
    QAction* exportAct = fileMenu->addAction(tr("Export JSON..."));
    fileMenu->addSeparator();
    QAction* exitAct = fileMenu->addAction(tr("Exit"));

    auto* dataMenu = menuBar()->addMenu(tr("Data"));
    // Canonical order for the table management entries.
    const QStringList kManageOrder = {
        QStringLiteral("item"), QStringLiteral("vehicle"), QStringLiteral("animal"),
        QStringLiteral("effect"), QStringLiteral("quest"), QStringLiteral("achievement"),
        QStringLiteral("skillset"), QStringLiteral("player")
    };
    QVector<QAction*> manageActs;
    for (const QString& key : kManageOrder) {
        const QString label = TableManagerDialog::displayName(key);
        manageActs.append(dataMenu->addAction(tr("Manage %1...").arg(label)));
    }

    auto* settingsMenu = menuBar()->addMenu(tr("Settings"));
    QAction* settingsAct = settingsMenu->addAction(tr("Preferences..."));

    auto* langMenu = menuBar()->addMenu(tr("Language"));
    // Native names, never translated.
    QAction* enAct = langMenu->addAction(QStringLiteral("English"));
    QAction* zhAct = langMenu->addAction(QStringLiteral("中文"));
    enAct->setCheckable(true);
    zhAct->setCheckable(true);
    const QString current = m_lang ? m_lang->currentLocale() : QStringLiteral("en");
    enAct->setChecked(current == QLatin1String("en"));
    zhAct->setChecked(current == QLatin1String("zh_CN"));

    auto* helpMenu = menuBar()->addMenu(tr("Help"));
    QAction* aboutAct = helpMenu->addAction(tr("About"));

    connect(importAct, &QAction::triggered, this, &MainWindow::openImportDialog);
    connect(exportAct, &QAction::triggered, this, &MainWindow::openExportDialog);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);
    for (int i = 0; i < kManageOrder.size(); ++i) {
        const QString key = kManageOrder[i];
        connect(manageActs[i], &QAction::triggered, this, [this, key] { openTableManager(key); });
    }
    connect(settingsAct, &QAction::triggered, this, &MainWindow::openSettingsDialog);
    connect(enAct, &QAction::triggered, this, [this] { emit languageRequested(QStringLiteral("en")); });
    connect(zhAct, &QAction::triggered, this, [this] { emit languageRequested(QStringLiteral("zh_CN")); });
    connect(aboutAct, &QAction::triggered, this, &MainWindow::showAbout);

    connect(m_commandList, &CommandListWidget::commandSelected, this,
            [this](const QString&) { refreshStatus(); });
    connect(m_commands, &CommandController::generatedStringChanged, this,
            [this](const QString&) { refreshStatus(); });
    connect(m_data, &AppData::tableChanged, this, [this] { refreshStatus(); });
    connect(m_data, &AppData::playersChanged, this, [this] { refreshStatus(); });

    m_commandList->applyFilter();
    m_commandList->selectFirst();
    if (!m_commandList->currentCommand().isEmpty())
        m_commands->selectCommand(m_commandList->currentCommand());

    statusBar()->show();
    refreshStatus();
}

void MainWindow::reloadSettings()
{
    QSettings s(QStringLiteral("UnturnedCmdGen"), QStringLiteral("UnturnedCmdGen"));
    m_commands->setFormat(static_cast<CommandController::Format>(
        s.value(QStringLiteral("format"), static_cast<int>(CommandController::Format::ChatSlash)).toInt()));
}

void MainWindow::openImportDialog()
{
    ImportDialog dlg(m_data, m_tables, this);
    dlg.exec();
}

void MainWindow::openExportDialog()
{
    const QStringList types = m_data->tableTypes();
    const QString tableType = QInputDialog::getItem(
        this, QStringLiteral("Export"), QStringLiteral("Source table:"), types, 0, false);
    if (tableType.isEmpty())
        return;

    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Export JSON table"),
        QDir::home().filePath(tableType + QStringLiteral(".json")),
        QStringLiteral("JSON files (*.json)"));
    if (path.isEmpty())
        return;

    m_tables->exportJson(path, tableType);
}

void MainWindow::openTableManager(const QString& tableType)
{
    TableManagerDialog dlg(m_data, m_tables, tableType, this);
    dlg.exec();
    refreshStatus();
}

void MainWindow::openSettingsDialog()
{
    SettingsDialog dlg(m_commands, m_rcon, this);
    dlg.exec();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, tr("About"),
                       tr("Unturned Command Generator v0.1.0\n\n"
                          "Generates Unturned server commands in terminal "
                          "or in-game chat format.\n\n"
                          "Data tables are editable JSON files under the "
                          "assets/ directory next to this executable."));
}

void MainWindow::refreshStatus()
{
    QString msg;
    msg += tr("Commands: %1").arg(m_data->commands().size());
    msg += QStringLiteral("  |  ") + tr("Items: %1").arg(m_data->table(QStringLiteral("item")).size());
    msg += QStringLiteral("  |  ") + tr("Vehicles: %1").arg(m_data->table(QStringLiteral("vehicle")).size());
    msg += QStringLiteral("  |  ") + tr("Players: %1").arg(m_data->table(QStringLiteral("player")).size());
    msg += QStringLiteral("  |  ") + tr("Maps: %1").arg(m_data->maps().size());
    msg += QStringLiteral("  |  ") + tr("Format: %1").arg(m_commands->formatLabel());
    statusBar()->showMessage(msg);
}
