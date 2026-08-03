#pragma once

#include <QObject>
#include <QString>

#include "controller/CommandController.h"
#include "controller/RconClient.h"
#include "controller/TableController.h"

class AppData;
class MainWindow;
class TranslationManager;

// Application-level coordinator: owns the data store and sub-controllers,
// wires them to the main window, and manages user data directories.
class AppController : public QObject
{
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);

    bool initialize();
    int exec();

    AppData* data() const { return m_data; }
    CommandController* commands() const { return m_commands; }
    TableController* tables() const { return m_tables; }
    RconClient* rcon() const { return m_rcon; }
    TranslationManager* language() const { return m_lang; }

    // Settings helpers (QSettings-backed).
    void loadSettings();
    void saveSettings();

public slots:
    // Installs the translator for `locale`, persists the choice and rebuilds
    // the main window so all visible text is re-created in the new language.
    void changeLanguage(const QString& locale);

private:
    QString userDataDir() const;
    void rebuildWindow();

    AppData* m_data = nullptr;
    CommandController* m_commands = nullptr;
    TableController* m_tables = nullptr;
    RconClient* m_rcon = nullptr;
    TranslationManager* m_lang = nullptr;
    MainWindow* m_window = nullptr;
};
