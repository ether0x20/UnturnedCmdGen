#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtTest/QtTest>

#include "controller/CommandController.h"
#include "model/AppData.h"

class TestCommandGen : public QObject
{
    Q_OBJECT
private slots:
    void loadCommands();
    void giveGeneration();
    void teleportGeneration();
    void formatVariants();
    void chatAtPrefix();
    void optionalParams();
    void missingRequiredIsIncomplete();
    void tableSearch();
    void chineseAnnotations();
    void tableStoragePaths();
    void modDiscoveryAndFiltering();
    void modLabelAndRemove();
};

void TestCommandGen::loadCommands()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    QVERIFY(data.loadAll());
    QCOMPARE(data.commands().size(), 59);
    QVERIFY(data.findCommand(QStringLiteral("Teleport")) != nullptr);
    QVERIFY(data.table(QStringLiteral("item")).size() > 0);
    QVERIFY(data.maps().size() > 0);
    QVERIFY(data.locationsForMap(QStringLiteral("pei")).contains(QStringLiteral("Charlottetown")));
}

void TestCommandGen::giveGeneration()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.setFormat(CommandController::Format::ChatSlash);
    c.selectCommand(QStringLiteral("Give"));
    // Give [Player]/[Item]/[Amount]
    c.setParamValue(0, QStringLiteral("Ethan"));
    c.setParamValue(1, QStringLiteral("Maplestrike"));
    c.setParamValue(2, QStringLiteral("5"));
    QCOMPARE(c.generatedString(), QStringLiteral("/give Ethan/Maplestrike/5"));
}

void TestCommandGen::teleportGeneration()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.selectCommand(QStringLiteral("Teleport"));
    // Source optional + Target location
    c.setParamValue(1, QStringLiteral("Seattle"));
    QCOMPARE(c.generatedString(), QStringLiteral("/teleport Seattle"));
    // With source player
    c.setParamValue(0, QStringLiteral("Ethan"));
    QCOMPARE(c.generatedString(), QStringLiteral("/teleport Ethan/Seattle"));
}

void TestCommandGen::formatVariants()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.selectCommand(QStringLiteral("Save"));
    QCOMPARE(c.generatedString(), QStringLiteral("/save"));

    c.setFormat(CommandController::Format::Terminal);
    QCOMPARE(c.generatedString(), QStringLiteral("save"));

    c.setFormat(CommandController::Format::ChatAt);
    QCOMPARE(c.generatedString(), QStringLiteral("@save"));
}

void TestCommandGen::chatAtPrefix()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.setFormat(CommandController::Format::ChatAt);
    c.selectCommand(QStringLiteral("Teleport"));
    c.setParamValue(1, QStringLiteral("Seattle"));
    QCOMPARE(c.generatedString(), QStringLiteral("@teleport Seattle"));
}

void TestCommandGen::optionalParams()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.selectCommand(QStringLiteral("Give"));
    // Leave optional Amount empty -> only player+item
    c.setParamValue(0, QStringLiteral("Ethan"));
    c.setParamValue(1, QStringLiteral("Maplestrike"));
    QCOMPARE(c.generatedString(), QStringLiteral("/give Ethan/Maplestrike"));
}

void TestCommandGen::missingRequiredIsIncomplete()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    CommandController c(&data);
    c.selectCommand(QStringLiteral("Kill")); // requires player
    // No value set -> still "required" semantics, but empty token is skipped
    QCOMPARE(c.generatedString(), QStringLiteral("/kill"));
    QVERIFY(c.paramValue(0).isEmpty());
}

void TestCommandGen::tableSearch()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();
    const auto hits = data.search(QStringLiteral("item"), QStringLiteral("knife"));
    QVERIFY(hits.size() > 0);
    QVERIFY(std::any_of(hits.cbegin(), hits.cend(),
                        [](const TableEntry& e) { return e.name.contains(QStringLiteral("Knife")); }));
}

void TestCommandGen::chineseAnnotations()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();

    // Use the first bundled entry that carries a Chinese annotation.
    const auto& items = data.table(QStringLiteral("item"));
    const TableEntry* zh = nullptr;
    for (const auto& e : items) {
        if (!e.nameZh.isEmpty()) {
            zh = &e;
            break;
        }
    }
    QVERIFY(zh != nullptr);

    // The label is bilingual when a Chinese annotation exists.
    QVERIFY(data.entryLabel(*zh).contains(zh->nameZh));
    QVERIFY(data.entryLabel(*zh).startsWith(QStringLiteral("[%1] %2").arg(zh->id, zh->name)));

    // Search works on the Chinese name.
    const auto hits = data.search(QStringLiteral("item"), zh->nameZh);
    QVERIFY(hits.size() > 0);

    // Vehicle and animal tables also carry Chinese annotations.
    bool vh = false;
    for (const auto& e : data.table(QStringLiteral("vehicle")))
        if (!e.nameZh.isEmpty()) { vh = true; break; }
    QVERIFY(vh);
    QVERIFY(data.search(QStringLiteral("vehicle"), QStringLiteral("直升机")).size() > 0);
    QVERIFY(data.search(QStringLiteral("animal"), QStringLiteral("狼")).size() > 0);
}

void TestCommandGen::tableStoragePaths()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.loadAll();
    // Non-player tables point back to their assets JSON file.
    QVERIFY(data.tableStoragePath(QStringLiteral("item")).endsWith(QStringLiteral("assets/items.json")));
    QVERIFY(data.tableStoragePath(QStringLiteral("vehicle")).endsWith(QStringLiteral("assets/vehicles.json")));
    // The player table has no bundled file (stored in user data dir).
    QVERIFY(data.tableStoragePath(QStringLiteral("player")).isEmpty());
}

void TestCommandGen::modDiscoveryAndFiltering()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.setUserDataDir(QStringLiteral("/tmp/unturnedCmdGen_test_mods"));
    QVERIFY(data.loadAll());

    // Add a mod and tag an item with it.
    QVERIFY(data.addMod(QStringLiteral("weapons_pack"), QStringLiteral("Weapons Pack")));
    TableEntry extra;
    extra.id = QStringLiteral("9001");
    extra.name = QStringLiteral("Plasma Rifle");
    extra.nameZh = QStringLiteral("等离子步枪");
    extra.mod = QStringLiteral("weapons_pack");
    data.mutableTable(QStringLiteral("item")).append(extra);
    data.rebuildTableFilter(QStringLiteral("item"));

    // discoverMods() finds mod ids not yet registered.
    data.addMod(QStringLiteral("extra_mod"), QStringLiteral("Extra"));
    TableEntry extra2;
    extra2.id = QStringLiteral("9002");
    extra2.name = QStringLiteral("Railgun");
    extra2.mod = QStringLiteral("extra_mod");
    data.mutableTable(QStringLiteral("item")).append(extra2);
    data.rebuildTableFilter(QStringLiteral("item"));
    data.discoverMods();
    QVERIFY(data.mods().contains(QStringLiteral("extra_mod")));

    // Both mod-tagged items are visible in the filtered table.
    QVERIFY(data.table(QStringLiteral("item")).size() == data.rawTable(QStringLiteral("item")).size());

    // Disabling weapons_pack hides its item from the filtered view only.
    QVERIFY(data.setModEnabled(QStringLiteral("weapons_pack"), false));
    QCOMPARE(data.table(QStringLiteral("item")).size(), data.rawTable(QStringLiteral("item")).size() - 1);
    QVERIFY(std::none_of(data.table(QStringLiteral("item")).cbegin(),
                         data.table(QStringLiteral("item")).cend(),
                         [](const TableEntry& e) { return e.mod == QLatin1String("weapons_pack"); }));

    // Vanilla (empty mod) is always enabled and cannot be toggled.
    QVERIFY(data.isModEnabled(QString()));
    QVERIFY(!data.setModEnabled(QString(), false));

    data.setModEnabled(QStringLiteral("weapons_pack"), true);
    data.setModEnabled(QStringLiteral("extra_mod"), false);
    data.saveMods();

    // Reload into a fresh AppData and confirm the enabled state persisted.
    AppData reload;
    reload.setAssetsDir(QStringLiteral("assets"));
    reload.setUserDataDir(QStringLiteral("/tmp/unturnedCmdGen_test_mods"));
    reload.loadAll();
    reload.mutableTable(QStringLiteral("item")).append(extra2);
    reload.rebuildTableFilter(QStringLiteral("item"));
    reload.discoverMods();
    QVERIFY(!reload.isModEnabled(QStringLiteral("extra_mod")));
    QVERIFY(reload.modInfo(QStringLiteral("weapons_pack")).name == QStringLiteral("Weapons Pack"));

    QDir(QStringLiteral("/tmp/unturnedCmdGen_test_mods")).removeRecursively();
}

void TestCommandGen::modLabelAndRemove()
{
    AppData data;
    data.setAssetsDir(QStringLiteral("assets"));
    data.setUserDataDir(QStringLiteral("/tmp/unturnedCmdGen_test_mods2"));
    QVERIFY(data.loadAll());

    QVERIFY(data.addMod(QStringLiteral("wpn"), QStringLiteral("Weapons")));
    TableEntry a, b;
    a.id = QStringLiteral("9001"); a.name = QStringLiteral("Plasma"); a.mod = QStringLiteral("wpn");
    b.id = QStringLiteral("9002"); b.name = QStringLiteral("Vanilla knife"); b.mod.clear();
    data.mutableTable(QStringLiteral("item")).append(a);
    data.mutableTable(QStringLiteral("item")).append(b);
    data.rebuildTableFilter(QStringLiteral("item"));

    // entryLabel() tags mod content (display name) but not vanilla.
    QVERIFY(data.entryLabel(a).contains(QStringLiteral("Weapons")));
    QVERIFY(!data.entryLabel(b).contains(QStringLiteral("Weapons")));

    // Removing the mod also removes its entries from the raw table.
    QVERIFY(data.removeMod(QStringLiteral("wpn")));
    QVERIFY(std::none_of(data.rawTable(QStringLiteral("item")).cbegin(),
                         data.rawTable(QStringLiteral("item")).cend(),
                         [](const TableEntry& e) { return e.mod == QLatin1String("wpn"); }));
    QVERIFY(!data.mods().contains(QStringLiteral("wpn")));
    // The vanilla entry survived.
    QVERIFY(std::any_of(data.rawTable(QStringLiteral("item")).cbegin(),
                        data.rawTable(QStringLiteral("item")).cend(),
                        [](const TableEntry& e) { return e.id == QLatin1String("9002"); }));

    QDir(QStringLiteral("/tmp/unturnedCmdGen_test_mods2")).removeRecursively();
}

QTEST_GUILESS_MAIN(TestCommandGen)
#include "test_command_gen.moc"
