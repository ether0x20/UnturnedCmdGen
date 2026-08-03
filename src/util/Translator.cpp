#include "Translator.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

JsonTranslator::JsonTranslator(QObject* parent)
    : QTranslator(parent)
{
}

bool JsonTranslator::loadFromObject(const QJsonObject& obj)
{
    m_map.clear();
    for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        m_map.insert(it.key(), it.value().toString());
    return !m_map.isEmpty();
}

bool JsonTranslator::loadFromFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    return loadFromObject(doc.object());
}

QString JsonTranslator::translate(const char* context, const char* sourceText,
                                  const char* disambiguation, int n) const
{
    Q_UNUSED(context)
    Q_UNUSED(disambiguation)
    Q_UNUSED(n)
    if (!sourceText)
        return QString();
    const auto it = m_map.constFind(QLatin1String(sourceText));
    if (it != m_map.constEnd())
        return it.value();
    // Return empty so QCoreApplication::translate falls back to the source text.
    return QString();
}

bool JsonTranslator::isEmpty() const
{
    return m_map.isEmpty();
}

// ---------------------------------------------------------------------------

TranslationManager::TranslationManager(QObject* parent)
    : QObject(parent)
{
}

void TranslationManager::setTranslationsDir(const QString& dir)
{
    m_dir = dir;
}

QString TranslationManager::displayName(const QString& locale) const
{
    if (locale == QLatin1String("zh_CN"))
        return QStringLiteral("中文");
    return QStringLiteral("English");
}

bool TranslationManager::setLanguage(const QString& locale)
{
    if (locale == m_locale)
        return true;

    // Remove any previously installed translator.
    if (m_translator) {
        QCoreApplication::removeTranslator(m_translator);
        delete m_translator;
        m_translator = nullptr;
    }

    if (locale == QLatin1String("en")) {
        m_locale = locale;
        emit languageChanged(m_locale);
        return true;
    }

    auto* translator = new JsonTranslator(this);
    const QString path = m_dir + QStringLiteral("/%1.json").arg(locale);
    if (!translator->loadFromFile(path)) {
        delete translator;
        return false;
    }
    m_translator = translator;
    QCoreApplication::installTranslator(translator);
    m_locale = locale;
    emit languageChanged(m_locale);
    return true;
}
