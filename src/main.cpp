#include <QApplication>
#include <QIcon>
#include <QMessageBox>
#include <QObject>
#include <QString>

#include "controller/AppController.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("UnturnedCmdGen"));
    QApplication::setOrganizationName(QStringLiteral("UnturnedCmdGen"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/unturned.png")));

    AppController controller;
    if (!controller.initialize()) {
        QMessageBox::critical(
            nullptr, QStringLiteral("Unturned Command Generator"),
            QObject::tr("Failed to load commands.json from the assets directory.\n\n"
                        "Make sure the assets/ folder sits next to this executable."));
        return 1;
    }

    controller.loadSettings();
    const int code = controller.exec();
    controller.saveSettings();
    return code;
}
