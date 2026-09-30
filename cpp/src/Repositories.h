#pragma once

#include "Database.h"
#include "Errors.h"
#include "Models.h"
#include "OrderDraft.h"

#include <QStringList>

#include <optional>
#include <vector>

// Запросы к базе данных: пользователи, каталог и заказы.
// Параметры передаются только через именованные плейсхолдеры (:name),
// что защищает от SQL-инъекций.

// Текст ошибки при попытке удалить единственную позицию заказа
QString lastLineMessage();

// Пока заказ формировался, нужного количества пар не осталось
class InsufficientStockError : public OrderError
{
public:
    struct Shortage
    {
        DraftLine line;
        int available = 0;
    };

    explicit InsufficientStockError(std::vector<Shortage> shortages);

    const std::vector<Shortage> &shortages() const { return m_shortages; }

private:
    static QString describe(const std::vector<Shortage> &shortages);

    std::vector<Shortage> m_shortages;
};

class UserRepository
{
public:
    explicit UserRepository(Database &database);

    // Пользователь с указанным логином, если он есть
    std::optional<User> findByLogin(const QString &login);

    // Клиенты, на которых менеджер или администратор оформляет заказ
    std::vector<User> listClients();

private:
    static User toUser(const QSqlQuery &query);

    Database &m_database;
};

class ProductRepository
{
public:
    explicit ProductRepository(Database &database);

    // Каталог с ценами, рассчитанными на указанную дату
    std::vector<Product> listProducts(QDate calculationDate);

    // Модель с ценой на указанную дату, если она есть в каталоге
    std::optional<Product> getProduct(int productId, QDate calculationDate);

    // Названия всех категорий для фильтра
    QStringList listCategories();

    // Размерный ряд модели с остатками
    std::vector<StockPosition> listPositions(int productId);

private:
    QSqlQuery selectProducts(const QString &condition, QDate calculationDate);
    static Product toProduct(const QSqlQuery &query);

    Database &m_database;
};

class OrderRepository
{
public:
    explicit OrderRepository(Database &database);

    // Все заказы, новые сверху
    std::vector<OrderSummary> listOrders();

    // Заказ по номеру, если его ещё не удалили
    std::optional<OrderSummary> getOrder(int orderId);

    // Состав заказа
    std::vector<OrderLine> listLines(int orderId);

    // Сохраняет заказ и списывает остатки в одной транзакции. Строки остатков
    // блокируются (FOR UPDATE) в порядке номеров, поэтому два одновременных
    // заказа не спишут одни и те же пары и не заблокируют друг друга.
    // При нехватке товара бросает InsufficientStockError.
    int createOrder(int clientId, QDate orderDate, const std::vector<DraftLine> &lines);

    // Удаляет заказ, возвращая его пары в остатки; состав удаляется каскадно
    void deleteOrder(int orderId);

    void changeOrderDate(int orderId, QDate orderDate);

    // Удаляет позицию заказа и возвращает её пары в остатки. Единственную
    // позицию удалить нельзя: в этом случае удаляется заказ целиком.
    void deleteLine(int orderItemId);

private:
    static OrderSummary toSummary(const QSqlQuery &query);

    Database &m_database;
};
