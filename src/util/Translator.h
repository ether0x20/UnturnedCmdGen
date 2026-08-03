#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QTranslator>

// JSON-backed QTranslator. Qt's own .qm files need the lrelease tool, so we
// load translations from a simple JSON map (English source -> translation)
// instead. Keys are matched on the source text alone, ignoring the Qt context.
class JsonTranslator : public QTranslator
{
public:
    explicit JsonTranslator(QObject* parent = nullptr);

    bool loadFromFile(const QString& path);
    bool loadFromObject(const QJsonObject& obj);

    QString translate(const char* context, const char* sourceText,
                      const char* disambiguation = nullptr, int n = -1) const override;
    bool isEmpty() const override;

private:
    QHash<QString, QString> m_map;
};

// Owns the active language and installs/uninstalls the translator on the
// running application. English ("en") is the built-in base language.
class TranslationManager : public QObject
{
    Q_OBJECT
public:
    explicit TranslationManager(QObject* parent = nullptr);

    void setTranslationsDir(const QString& dir);
    QString translationsDir() const { return m_dir; }

    // Locale codes we ship. English needs no file.
    QStringList availableLocales() const { return {QStringLiteral("en"), QStringLiteral("zh_CN")}; }
    // Native name shown in the language menu.
    QString displayName(const QString& locale) const;

    QString currentLocale() const { return m_locale; }
    // Installs the translator for `locale` and persists nothing itself.
    // Returns false only if the translation file could not be loaded.
    bool setLanguage(const QString& locale);

signals:
    void languageChanged(const QString& locale);

private:
    QString m_dir;
    QString m_locale = QStringLiteral("en");
    JsonTranslator* m_translator = nullptr;
};
