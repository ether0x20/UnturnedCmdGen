// Headless GUI smoke test: builds the real widgets, drives them like a user
// would, and asserts the generated command text. Run with QT_QPA_PLATFORM=offscreen.
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTest>

#include "controller/CommandController.h"
#include "controller/RconClient.h"
#include "controller/TableController.h"
#include "model/AppData.h"
#include "ui/CommandListWidget.h"
#include "ui/ImportDialog.h"
#include "ui/MainWindow.h"
#include "ui/OutputWidget.h"
#include "ui/ParameterWidget.h"
#include "ui/TableManagerDialog.h"
#include "util/Translator.h"

static int g_failures = 0;
#define CHECK(cond, msg) \
    do { \
        if (!(cond)) { \
            ++g_failures; \
            qCritical("FAIL: %s", msg); \
        } else { \
            qInfo("ok: %s", msg); \
        } \
    } while (0)

// Find an editable combo (lookup editor) inside the parameter panel.
static QList<QComboBox*> findCombos(const QWidget* root)
{
    QList<QComboBox*> out;
    for (QComboBox* c : root->findChildren<QComboBox*>())
        out.append(c);
    return out;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.setUserDataDir(QStringLiteral("/tmp/unturnedCmdGen_test"));
    if (!data.loadAll())
        return 2;
    data.loadPlayers();

    CommandController commands(&data);
    TableController tables(&data);
    RconClient rcon;
    TranslationManager lang;
    lang.setTranslationsDir(QStringLiteral("translations"));
    MainWindow win(&data, &commands, &tables, &rcon, &lang);
    win.resize(960, 640);
    win.show();
    app.processEvents();

    // 1) Default command is selected and a preview shows.
    CHECK(!commands.generatedString().isEmpty(), "default preview generated");

    // 2) Pick "Give" via the list widget (simulate the user clicking it).
    ParameterWidget* panel = win.findChild<ParameterWidget*>();
    CommandListWidget* list = win.findChild<CommandListWidget*>();
    CHECK(panel && list, "found param panel and command list");

    list->applyFilter();
    commands.selectCommand(QStringLiteral("Give"));
    app.processEvents();

    // Give -> Player(optional), Item, Amount. Expect a lookup combo (item table).
    const QList<QComboBox*> combos = findCombos(panel);
    QComboBox* itemCombo = nullptr;
    for (QComboBox* c : combos) {
        if (c->isEditable())
            itemCombo = c;
    }
    CHECK(itemCombo != nullptr, "Give shows an editable lookup combo");

    itemCombo->setCurrentText(QStringLiteral("Maplestrike"));
    app.processEvents();
    CHECK(commands.generatedString() == QStringLiteral("/give Maplestrike"),
          qPrintable(QStringLiteral("give preview: %1").arg(commands.generatedString())));

    // Chinese annotation resolves back to the English item name in commands.
    itemCombo->setCurrentText(QStringLiteral("军用刀"));
    app.processEvents();
    CHECK(commands.generatedString() == QStringLiteral("/give Military Knife"),
          qPrintable(QStringLiteral("chinese token: %1").arg(commands.generatedString())));

    // 3) Teleport: multi-type Target (player | location).
    commands.selectCommand(QStringLiteral("Teleport"));
    app.processEvents();

    // The multi-type Target has a mode selector (non-editable combo) whose
    // page 1 is the Location editor. Switch to it and enter a location.
    QComboBox* modeCombo = nullptr;
    QComboBox* locCombo = nullptr;
    for (QComboBox* c : findCombos(panel)) {
        if (c->isEditable() && c->isEnabled()) {
            if (c->count() == 0 || c->currentText().isEmpty())
                continue;
        }
    }
    for (QComboBox* c : findCombos(panel)) {
        if (!c->isEditable() && c->isEnabled() && c->count() >= 2)
            modeCombo = c; // the mode selector for Target
    }
    CHECK(modeCombo != nullptr, "Teleport builds a multi-type mode selector");
    if (modeCombo) {
        modeCombo->setCurrentIndex(1); // Location page
        app.processEvents();
        for (QComboBox* c : findCombos(panel)) {
            if (c->isEditable() && c->isEnabled() && !locCombo)
                locCombo = c;
        }
        if (locCombo) {
            locCombo->setCurrentText(QStringLiteral("Seattle"));
            app.processEvents();
            qInfo("teleport location preview: %s", qPrintable(commands.generatedString()));
            CHECK(commands.generatedString() == QStringLiteral("/teleport Seattle"),
                  qPrintable(QStringLiteral("location token: %1").arg(commands.generatedString())));
        }
    }

    // 4) Format switching via the output radio buttons.
    OutputWidget* out = win.findChild<OutputWidget*>();
    CHECK(out != nullptr, "output widget exists");

    // 5) Import dialog exposes the tutorial button.
    ImportDialog imp(&data, &tables);
    bool tutorialFound = false;
    const auto buttons = imp.findChildren<QPushButton*>();
    for (QPushButton* b : buttons) {
        if (b->text().contains(QStringLiteral("Tutorial")))
            tutorialFound = true;
    }
    CHECK(tutorialFound, "import dialog shows the tutorial button");
    imp.resize(480, 220);
    imp.show();
    app.processEvents();
    imp.grab().save(QStringLiteral("/tmp/unturnedCmdGen_import.png"));

    // 6) Generic table managers render the loaded data.
    TableManagerDialog items(&data, &tables, QStringLiteral("item"));
    CHECK(items.windowTitle().contains(QStringLiteral("Items")), "item manager dialog title");
    QTableWidget* itemTbl = items.findChild<QTableWidget*>();
    CHECK(itemTbl != nullptr, "item manager has a table");
    if (itemTbl) {
        CHECK(itemTbl->columnCount() == 4, "item manager has a Chinese Name column");
        bool zhCell = false;
        for (int r = 0; r < itemTbl->rowCount() && !zhCell; ++r) {
            const QTableWidgetItem* c = itemTbl->item(r, 2);
            if (c && !c->text().isEmpty())
                zhCell = true;
        }
        CHECK(zhCell, "Chinese Name column is populated");
    }
    items.resize(520, 380);
    items.show();
    app.processEvents();
    items.grab().save(QStringLiteral("/tmp/unturnedCmdGen_items.png"));

    TableManagerDialog vehicles(&data, &tables, QStringLiteral("vehicle"));
    vehicles.resize(520, 380);
    vehicles.show();
    app.processEvents();
    vehicles.grab().save(QStringLiteral("/tmp/unturnedCmdGen_vehicles.png"));

    // Render PNGs for the record.
    win.grab().save(QStringLiteral("/tmp/unturnedCmdGen_default.png"));
    commands.selectCommand(QStringLiteral("Give"));
    app.processEvents();
    win.grab().save(QStringLiteral("/tmp/unturnedCmdGen_give.png"));
    commands.selectCommand(QStringLiteral("Teleport"));
    app.processEvents();
    win.grab().save(QStringLiteral("/tmp/unturnedCmdGen_teleport.png"));

    // 7) Language switching via the JSON-backed translator.
    CHECK(QCoreApplication::translate("", "Copy") == QStringLiteral("Copy"),
          "english is the base language");
    CHECK(lang.setLanguage(QStringLiteral("zh_CN")), "zh_CN translator loads");
    CHECK(QCoreApplication::translate("", "Copy") == QStringLiteral("复制"),
          "zh_CN: Copy -> 复制");
    CHECK(QCoreApplication::translate("", "Terminal") == QStringLiteral("终端"),
          "zh_CN: Terminal -> 终端");
    CHECK(QCoreApplication::translate("", "Items") == QStringLiteral("物品"),
          "zh_CN: Items -> 物品");
    lang.setLanguage(QStringLiteral("en"));
    CHECK(QCoreApplication::translate("", "Copy") == QStringLiteral("Copy"),
          "switching back to english");

    // 8) The Language menu's native-name actions request the right locale.
    QString requested;
    QObject::connect(&win, &MainWindow::languageRequested,
                     [&requested](const QString& loc) { requested = loc; });
    QAction* zhAction = nullptr;
    for (QAction* a : win.findChildren<QAction*>()) {
        if (a->text() == QStringLiteral("中文"))
            zhAction = a;
    }
    CHECK(zhAction != nullptr, "language menu contains 中文");
    if (zhAction) {
        zhAction->trigger();
        app.processEvents();
    }
    CHECK(requested == QStringLiteral("zh_CN"), "中文 action requests zh_CN");

    qInfo("failures: %d", g_failures);
    return g_failures == 0 ? 0 : 1;
}
