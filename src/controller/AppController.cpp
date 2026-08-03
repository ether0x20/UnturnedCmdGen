#include "AppController.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

#include "model/AppData.h"
#include "ui/MainWindow.h"
#include "util/Translator.h"

AppController::AppController(QObject* parent)
    : QObject(parent)
{
    m_data = new AppData(this);
    m_commands = new CommandController(m_data, this);
    m_tables = new TableController(m_data, this);
    m_rcon = new RconClient(this);
    m_lang = new TranslationManager(this);
}

QString AppController::userDataDir() const
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return base.isEmpty()
        ? QDir::home().filePath(QStringLiteral(".unturnedCmdGen"))
        : base;
}

bool AppController::initialize()
{
    m_data->setUserDataDir(userDataDir());
    if (!m_data->loadAll())
        return false;
    m_data->loadPlayers();

    // Translations live next to the executable, alongside assets/, or under
    // /usr/share/unturnedCmdGen/translations for an installed .deb.
    const QString exeDir = QCoreApplication::applicationDirPath();
    QString translationsDir = exeDir + QStringLiteral("/translations");
    if (!QFileInfo::exists(translationsDir + QStringLiteral("/zh_CN.json"))) {
        const QStringList candidates = {
            exeDir + QStringLiteral("/../share/unturnedCmdGen/translations"),
            QStringLiteral("/usr/share/unturnedCmdGen/translations")
        };
        for (const QString& c : candidates) {
            if (QFileInfo::exists(c + QStringLiteral("/zh_CN.json"))) {
                translationsDir = c;
                break;
            }
        }
    }
    m_lang->setTranslationsDir(translationsDir);

    // Apply the saved language before the first window is created.
    QSettings s(QStringLiteral("UnturnedCmdGen"), QStringLiteral("UnturnedCmdGen"));
    const QString locale = s.value(QStringLiteral("language"), QStringLiteral("en")).toString();
    m_lang->setLanguage(locale);

    rebuildWindow();
    return true;
}

void AppController::rebuildWindow()
{
    if (m_window) {
        delete m_window;
        m_window = nullptr;
    }
    m_window = new MainWindow(m_data, m_commands, m_tables, m_rcon, m_lang);
    connect(m_window, &MainWindow::languageRequested, this, &AppController::changeLanguage);
    m_window->show();
}

int AppController::exec()
{
    return QApplication::exec();
}

void AppController::changeLanguage(const QString& locale)
{
    if (!m_lang->setLanguage(locale))
        return; // translation file failed to load; keep current UI
    QSettings s(QStringLiteral("UnturnedCmdGen"), QStringLiteral("UnturnedCmdGen"));
    s.setValue(QStringLiteral("language"), locale);
    // Defer the window rebuild so the currently running signal emission (which
    // originates from inside the old window) completes before we delete it.
    QTimer::singleShot(0, this, [this] { rebuildWindow(); });
}

void AppController::loadSettings()
{
    QSettings s(QStringLiteral("UnturnedCmdGen"), QStringLiteral("UnturnedCmdGen"));
    const int fmt = s.value(QStringLiteral("format"), static_cast<int>(CommandController::Format::ChatSlash)).toInt();
    m_commands->setFormat(static_cast<CommandController::Format>(fmt));

    const QString host = s.value(QStringLiteral("rcon/host"), QStringLiteral("127.0.0.1")).toString();
    const quint16 port = s.value(QStringLiteral("rcon/port"), 27015).toUInt();
    const QString pass = s.value(QStringLiteral("rcon/password")).toString();
    m_rcon->setConnection(host, port, pass);
}

void AppController::saveSettings()
{
    QSettings s(QStringLiteral("UnturnedCmdGen"), QStringLiteral("UnturnedCmdGen"));
    s.setValue(QStringLiteral("format"), static_cast<int>(m_commands->format()));
}
