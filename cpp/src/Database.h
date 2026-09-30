#pragma once

#include "AppConfig.h"

#include <QSqlDatabase>
#include <QSqlQuery>

// Единое подключение приложения к PostgreSQL через драйвер Qt QPSQL.
// Разорванное соединение (например, после перезапуска сервера)
// открывается заново при следующем запросе.
class Database
{
public:
    explicit Database(const DatabaseSettings &settings);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    // Проверяет, что сервер доступен и отвечает; иначе бросает DatabaseError
    void checkConnection();

    // Подготавливает запрос с именованными параметрами (:name)
    QSqlQuery prepare(const QString &sql);

    // Выполняет подготовленный запрос; при ошибке бросает DatabaseError
    void exec(QSqlQuery &query);

    // Транзакция по принципу RAII: если commit() не вызван (например,
    // из-за исключения), изменения откатываются в деструкторе.
    // Вложенная транзакция становится точкой сохранения (SAVEPOINT) —
    // так тесты оборачивают работу репозиториев во внешнюю транзакцию,
    // которая всегда откатывается.
    class Transaction
    {
    public:
        explicit Transaction(Database &database);
        ~Transaction();

        Transaction(const Transaction &) = delete;
        Transaction &operator=(const Transaction &) = delete;

        void commit();

    private:
        void execute(const QString &sql);
        QString savepointName() const;

        Database &m_database;
        QSqlDatabase m_connection;
        int m_level = 0;
        bool m_finished = false;
    };

private:
    QSqlDatabase openConnection();

    int m_transactionDepth = 0;
};
