#include <QtCore/QCoreApplication>
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
    QCOMPARE(c.generatedString(), QStringLiteral("/give Ethan Maplestrike 5"));
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
    QCOMPARE(c.generatedString(), QStringLiteral("/teleport Ethan Seattle"));
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
    QCOMPARE(c.generatedString(), QStringLiteral("/give Ethan Maplestrike"));
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

    // nameZh parsed from the bundled data.
    const auto& items = data.table(QStringLiteral("item"));
    const TableEntry* knife = nullptr;
    for (const auto& e : items) {
        if (e.name == QLatin1String("Military Knife"))
            knife = &e;
    }
    QVERIFY(knife != nullptr);
    QCOMPARE(knife->nameZh, QStringLiteral("军用刀"));

    // The label is bilingual when a Chinese annotation exists.
    QVERIFY(knife->label().contains(QStringLiteral("军用刀")));
    QVERIFY(knife->label().startsWith(QStringLiteral("[27] Military Knife")));

    // Search works on the Chinese name.
    const auto hits = data.search(QStringLiteral("item"), QStringLiteral("军用刀"));
    QVERIFY(hits.size() > 0);
    QCOMPARE(hits.first().name, QStringLiteral("Military Knife"));

    // Chinese search for vehicles and animals too.
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

QTEST_GUILESS_MAIN(TestCommandGen)
#include "test_command_gen.moc"
