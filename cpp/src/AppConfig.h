#pragma once

#include <QString>

// Параметры подключения к PostgreSQL
struct DatabaseSettings
{
    QString host;
    int port = 5432;
    QString databaseName;
    QString userName;
    QString password;
};

namespace AppConfig {

// Ищет config.ini рядом с программой и в двух родительских папках: так один
// файл в корне репозитория подходит и для сборки в cpp/build, и для bin/cpp
QString findConfigFile();

// Читает раздел [database] из config.ini; при ошибке бросает ConfigError
DatabaseSettings loadDatabaseSettings();

} // namespace AppConfig
