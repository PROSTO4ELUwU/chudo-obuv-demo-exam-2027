#include "Database.h"

#include "Errors.h"

#include <QSqlError>

Database::Database(const DatabaseSettings &settings)
{
    QSqlDatabase connection = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"));
    if (!connection.isValid()) {
        throw DatabaseError(QStringLiteral(
            "Не загружен драйвер PostgreSQL для Qt: рядом с программой должны быть "
            "sqldrivers/qsqlpsql.dll и библиотека libpq.dll с зависимостями"));
    }
    connection.setHostName(settings.host);
    connection.setPort(settings.port);
    connection.setDatabaseName(settings.databaseName);
    connection.setUserName(settings.userName);
    connection.setPassword(settings.password);
    connection.setConnectOptions(
        QStringLiteral("connect_timeout=5;application_name=chudo_obuv_cpp"));
    // numeric приходит строкой, поэтому цены читаются без потери точности
    connection.setNumericalPrecisionPolicy(QSql::HighPrecision);
}

Database::~Database()
{
    const QString name = QSqlDatabase::database(QSqlDatabase::defaultConnection, false)
                             .connectionName();
    QSqlDatabase::database(name, false).close();
    QSqlDatabase::removeDatabase(name);
}

QSqlDatabase Database::openConnection()
{
    QSqlDatabase connection = QSqlDatabase::database(QSqlDatabase::defaultConnection, false);
    if (!connection.isOpen() && !connection.open())
        throw DatabaseError(connection.lastError().text());
    return connection;
}

void Database::checkConnection()
{
    QSqlQuery query = prepare(QStringLiteral("SELECT 1"));
    exec(query);
}

QSqlQuery Database::prepare(const QString &sql)
{
    QSqlQuery query(openConnection());
    if (!query.prepare(sql))
        throw DatabaseError(query.lastError().text());
    return query;
}

void Database::exec(QSqlQuery &query)
{
    if (query.exec())
        return;
    const QSqlError error = query.lastError();
    if (error.type() == QSqlError::ConnectionError)
        QSqlDatabase::database(QSqlDatabase::defaultConnection, false).close();
    throw DatabaseError(error.text());
}

Database::Transaction::Transaction(Database &database)
    : m_database(database)
    , m_connection(database.openConnection())
    , m_level(database.m_transactionDepth + 1)
{
    if (m_level == 1) {
        if (!m_connection.transaction())
            throw DatabaseError(m_connection.lastError().text());
    } else {
        execute(QStringLiteral("SAVEPOINT ") + savepointName());
    }
    m_database.m_transactionDepth = m_level;
}

Database::Transaction::~Transaction()
{
    if (m_finished)
        return;
    // Деструктор не должен бросать исключений, поэтому ошибка отката
    // игнорируется: при разрыве соединения сервер откатит транзакцию сам
    try {
        if (m_level == 1)
            m_connection.rollback();
        else
            execute(QStringLiteral("ROLLBACK TO SAVEPOINT ") + savepointName());
    } catch (const DatabaseError &) {
    }
    m_database.m_transactionDepth = m_level - 1;
}

void Database::Transaction::commit()
{
    if (m_level == 1) {
        if (!m_connection.commit())
            throw DatabaseError(m_connection.lastError().text());
    } else {
        execute(QStringLiteral("RELEASE SAVEPOINT ") + savepointName());
    }
    m_finished = true;
    m_database.m_transactionDepth = m_level - 1;
}

void Database::Transaction::execute(const QString &sql)
{
    QSqlQuery query(m_connection);
    if (!query.exec(sql))
        throw DatabaseError(query.lastError().text());
}

QString Database::Transaction::savepointName() const
{
    return QStringLiteral("level_%1").arg(m_level);
}
