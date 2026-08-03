#pragma once

#include <QMainWindow>

class CommandController;
class RconClient;
class AppData;
class TableController;
class TranslationManager;
class CommandListWidget;
class OutputWidget;
class ParameterWidget;
class QSplitter;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(AppData* data, CommandController* commands,
               TableController* tables, RconClient* rcon,
               TranslationManager* lang,
               QWidget* parent = nullptr);

    void reloadSettings();

signals:
    void languageRequested(const QString& locale);

private slots:
    void openImportDialog();
    void openExportDialog();
    void openTableManager(const QString& tableType);
    void openModManager();
    void openSettingsDialog();
    void showAbout();
    void refreshStatus();

private:
    AppData* m_data = nullptr;
    CommandController* m_commands = nullptr;
    TableController* m_tables = nullptr;
    RconClient* m_rcon = nullptr;
    TranslationManager* m_lang = nullptr;
    CommandListWidget* m_commandList = nullptr;
    ParameterWidget* m_paramPanel = nullptr;
    OutputWidget* m_output = nullptr;
    QSplitter* m_splitter = nullptr;
};
