#include "AppConfig.h"

#include "Errors.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>

namespace AppConfig {

QString findConfigFile()
{
    QDir folder(QCoreApplication::applicationDirPath());
    for (int level = 0; level < 3; ++level) {
        const QString candidate = folder.filePath(QStringLiteral("config.ini"));
        if (QFileInfo::exists(candidate))
            return candidate;
        if (!folder.cdUp())
            break;
    }
    throw ConfigError(QStringLiteral("Файл настроек config.ini не найден рядом с приложением"));
}

DatabaseSettings loadDatabaseSettings(const QString &fileName)
{
    QSettings config(fileName, QSettings::IniFormat);
    config.beginGroup(QStringLiteral("database"));
    for (const auto key : {"dbname", "user", "password"}) {
        if (!config.contains(QString::fromLatin1(key)))
            throw ConfigError(QStringLiteral("В config.ini не указан параметр %1").arg(key));
    }
    DatabaseSettings settings;
    settings.host = config.value(QStringLiteral("host"), QStringLiteral("localhost")).toString();
    bool portIsNumber = false;
    settings.port = config.value(QStringLiteral("port"), 5432).toInt(&portIsNumber);
    if (!portIsNumber)
        throw ConfigError(QStringLiteral("В config.ini параметр port должен быть целым числом"));
    settings.databaseName = config.value(QStringLiteral("dbname")).toString();
    settings.userName = config.value(QStringLiteral("user")).toString();
    settings.password = config.value(QStringLiteral("password")).toString();
    return settings;
}

} // namespace AppConfig
