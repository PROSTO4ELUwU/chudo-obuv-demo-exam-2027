#include "Repositories.h"

#include "Formatting.h"
#include "Pricing.h"

#include <QHash>

namespace {

// Размер из numeric(3,1) («36.5») в десятых долях (365)
int toSizeTenths(const QVariant &value)
{
    return qRound(value.toString().toDouble() * 10);
}

Money toMoney(const QVariant &value)
{
    return Money::fromDecimalString(value.toString());
}

const QString userSelect = QStringLiteral(R"(
    SELECT u.user_id, u.last_name, u.first_name, u.patronymic, r.role_name
    FROM users AS u
    JOIN roles AS r ON r.role_id = u.role_id
)");

// Скидка зависит от заказов модели в любом размере за предыдущий месяц
const QString productSelect = QStringLiteral(R"(
    SELECT p.product_id, p.product_name, c.category_name, m.manufacturer_name,
           p.description, p.composition, p.price, p.image_file,
           COALESCE(SUM(si.quantity), 0) AS total_quantity,
           EXISTS (
               SELECT 1
               FROM order_items AS oi
               JOIN orders AS o ON o.order_id = oi.order_id
               JOIN stock_items AS ordered ON ordered.stock_item_id = oi.stock_item_id
               WHERE ordered.product_id = p.product_id
                 AND o.order_date >= :period_start
                 AND o.order_date < :period_end
           ) AS has_recent_orders
    FROM products AS p
    JOIN categories AS c ON c.category_id = p.category_id
    JOIN manufacturers AS m ON m.manufacturer_id = p.manufacturer_id
    LEFT JOIN stock_items AS si ON si.product_id = p.product_id
)");

const QString productGroupBy =
    QStringLiteral(" GROUP BY p.product_id, c.category_name, m.manufacturer_name");

const QString orderSummarySelect = QStringLiteral(R"(
    SELECT o.order_id, o.order_date,
           concat_ws(' ', u.last_name, u.first_name, u.patronymic) AS client_name,
           COALESCE(SUM(oi.quantity * oi.unit_price), 0) AS total
    FROM orders AS o
    JOIN users AS u ON u.user_id = o.client_id
    LEFT JOIN order_items AS oi ON oi.order_id = o.order_id
)");

const QString orderSummaryGroupBy = QStringLiteral(" GROUP BY o.order_id, u.user_id");

} // namespace

QString lastLineMessage()
{
    return QStringLiteral("Это единственная позиция заказа, поэтому удалить её нельзя.\n"
                          "Чтобы отменить заказ полностью, удалите его в списке заказов.");
}

InsufficientStockError::InsufficientStockError(std::vector<Shortage> shortages)
    : OrderError(describe(shortages))
    , m_shortages(std::move(shortages))
{
}

QString InsufficientStockError::describe(const std::vector<Shortage> &shortages)
{
    QStringList details;
    for (const Shortage &shortage : shortages) {
        details << QStringLiteral("• %1, размер %2: в заказе %3, в наличии %4")
                       .arg(shortage.line.productName, Formatting::size(shortage.line.sizeTenths),
                            Formatting::pairs(shortage.line.quantity),
                            Formatting::pairs(shortage.available));
    }
    return QStringLiteral("Пока заказ формировался, часть товара закончилась:\n")
           + details.join(u'\n');
}

UserRepository::UserRepository(Database &database)
    : m_database(database)
{
}

std::optional<User> UserRepository::findByLogin(const QString &login)
{
    QSqlQuery query = m_database.prepare(userSelect + QStringLiteral(" WHERE u.login = :login"));
    query.bindValue(QStringLiteral(":login"), login);
    m_database.exec(query);
    if (!query.next())
        return std::nullopt;
    return toUser(query);
}

std::vector<User> UserRepository::listClients()
{
    QSqlQuery query = m_database.prepare(
        userSelect
        + QStringLiteral(" WHERE r.role_name = :role ORDER BY u.last_name, u.first_name"));
    query.bindValue(QStringLiteral(":role"), roleName(Role::Client));
    m_database.exec(query);
    std::vector<User> clients;
    while (query.next())
        clients.push_back(toUser(query));
    return clients;
}

User UserRepository::toUser(const QSqlQuery &query)
{
    const QString name = query.value(QStringLiteral("role_name")).toString();
    const std::optional<Role> role = roleFromName(name);
    if (!role)
        throw DatabaseError(QStringLiteral("В базе указана неизвестная роль «%1»").arg(name));
    User user;
    user.id = query.value(QStringLiteral("user_id")).toInt();
    user.lastName = query.value(QStringLiteral("last_name")).toString();
    user.firstName = query.value(QStringLiteral("first_name")).toString();
    user.patronymic = query.value(QStringLiteral("patronymic")).toString();
    user.role = *role;
    return user;
}

ProductRepository::ProductRepository(Database &database)
    : m_database(database)
{
}

QSqlQuery ProductRepository::selectProducts(const QString &condition, QDate calculationDate)
{
    QSqlQuery query = m_database.prepare(productSelect + condition + productGroupBy
                                         + QStringLiteral(" ORDER BY p.product_id"));
    const Pricing::Period period = Pricing::previousMonth(calculationDate);
    query.bindValue(QStringLiteral(":period_start"), period.start);
    query.bindValue(QStringLiteral(":period_end"), period.end);
    return query;
}

std::vector<Product> ProductRepository::listProducts(QDate calculationDate)
{
    QSqlQuery query = selectProducts(QString(), calculationDate);
    m_database.exec(query);
    std::vector<Product> products;
    while (query.next())
        products.push_back(toProduct(query));
    return products;
}

std::optional<Product> ProductRepository::getProduct(int productId, QDate calculationDate)
{
    QSqlQuery query =
        selectProducts(QStringLiteral(" WHERE p.product_id = :product_id"), calculationDate);
    query.bindValue(QStringLiteral(":product_id"), productId);
    m_database.exec(query);
    if (!query.next())
        return std::nullopt;
    return toProduct(query);
}

Product ProductRepository::toProduct(const QSqlQuery &query)
{
    Product product;
    product.id = query.value(QStringLiteral("product_id")).toInt();
    product.name = query.value(QStringLiteral("product_name")).toString();
    product.category = query.value(QStringLiteral("category_name")).toString();
    product.manufacturer = query.value(QStringLiteral("manufacturer_name")).toString();
    product.description = query.value(QStringLiteral("description")).toString();
    product.composition = query.value(QStringLiteral("composition")).toString();
    product.basePrice = toMoney(query.value(QStringLiteral("price")));
    product.discount = Pricing::discountFor(query.value(QStringLiteral("has_recent_orders")).toBool());
    product.price = Pricing::priceWithDiscount(product.basePrice, product.discount);
    product.totalQuantity = query.value(QStringLiteral("total_quantity")).toInt();
    product.imageFile = query.value(QStringLiteral("image_file")).toString();
    return product;
}

QStringList ProductRepository::listCategories()
{
    QSqlQuery query = m_database.prepare(
        QStringLiteral("SELECT category_name FROM categories ORDER BY category_name"));
    m_database.exec(query);
    QStringList categories;
    while (query.next())
        categories << query.value(0).toString();
    return categories;
}

std::vector<StockPosition> ProductRepository::listPositions(int productId)
{
    QSqlQuery query = m_database.prepare(QStringLiteral(R"(
        SELECT si.stock_item_id, s.size_value, si.quantity
        FROM stock_items AS si
        JOIN sizes AS s ON s.size_id = si.size_id
        WHERE si.product_id = :product_id
        ORDER BY s.size_value
    )"));
    query.bindValue(QStringLiteral(":product_id"), productId);
    m_database.exec(query);
    std::vector<StockPosition> positions;
    while (query.next()) {
        positions.push_back({query.value(0).toInt(), toSizeTenths(query.value(1)),
                             query.value(2).toInt()});
    }
    return positions;
}

OrderRepository::OrderRepository(Database &database)
    : m_database(database)
{
}

std::vector<OrderSummary> OrderRepository::listOrders()
{
    QSqlQuery query = m_database.prepare(
        orderSummarySelect + orderSummaryGroupBy
        + QStringLiteral(" ORDER BY o.order_date DESC, o.order_id DESC"));
    m_database.exec(query);
    std::vector<OrderSummary> orders;
    while (query.next())
        orders.push_back(toSummary(query));
    return orders;
}

std::optional<OrderSummary> OrderRepository::getOrder(int orderId)
{
    QSqlQuery query = m_database.prepare(
        orderSummarySelect + QStringLiteral(" WHERE o.order_id = :order_id") + orderSummaryGroupBy);
    query.bindValue(QStringLiteral(":order_id"), orderId);
    m_database.exec(query);
    if (!query.next())
        return std::nullopt;
    return toSummary(query);
}

OrderSummary OrderRepository::toSummary(const QSqlQuery &query)
{
    return {query.value(QStringLiteral("order_id")).toInt(),
            query.value(QStringLiteral("order_date")).toDate(),
            query.value(QStringLiteral("client_name")).toString(),
            toMoney(query.value(QStringLiteral("total")))};
}

std::vector<OrderLine> OrderRepository::listLines(int orderId)
{
    QSqlQuery query = m_database.prepare(QStringLiteral(R"(
        SELECT oi.order_item_id, p.product_name, m.manufacturer_name,
               s.size_value, oi.quantity, oi.unit_price
        FROM order_items AS oi
        JOIN stock_items AS si ON si.stock_item_id = oi.stock_item_id
        JOIN products AS p ON p.product_id = si.product_id
        JOIN manufacturers AS m ON m.manufacturer_id = p.manufacturer_id
        JOIN sizes AS s ON s.size_id = si.size_id
        WHERE oi.order_id = :order_id
        ORDER BY oi.order_item_id
    )"));
    query.bindValue(QStringLiteral(":order_id"), orderId);
    m_database.exec(query);
    std::vector<OrderLine> lines;
    while (query.next()) {
        lines.push_back({query.value(0).toInt(), query.value(1).toString(),
                         query.value(2).toString(), toSizeTenths(query.value(3)),
                         query.value(4).toInt(), toMoney(query.value(5))});
    }
    return lines;
}

int OrderRepository::createOrder(int clientId, QDate orderDate,
                                 const std::vector<DraftLine> &lines)
{
    Database::Transaction transaction(m_database);

    // Qt не передаёт массивы в запрос, поэтому номера позиций передаются
    // строкой вида {1,2,3} и приводятся к integer[] на стороне PostgreSQL
    QStringList stockItemIds;
    for (const DraftLine &line : lines)
        stockItemIds << QString::number(line.stockItemId);
    QSqlQuery lock = m_database.prepare(QStringLiteral(R"(
        SELECT stock_item_id, quantity
        FROM stock_items
        WHERE stock_item_id = ANY(CAST(:ids AS integer[]))
        ORDER BY stock_item_id
        FOR UPDATE
    )"));
    lock.bindValue(QStringLiteral(":ids"), u'{' + stockItemIds.join(u',') + u'}');
    m_database.exec(lock);
    QHash<int, int> available;
    while (lock.next())
        available.insert(lock.value(0).toInt(), lock.value(1).toInt());

    std::vector<InsufficientStockError::Shortage> shortages;
    for (const DraftLine &line : lines) {
        const int inStock = available.value(line.stockItemId, 0);
        if (line.quantity > inStock)
            shortages.push_back({line, inStock});
    }
    if (!shortages.empty())
        throw InsufficientStockError(std::move(shortages));

    QSqlQuery insertOrder = m_database.prepare(QStringLiteral(
        "INSERT INTO orders (order_date, client_id) VALUES (:order_date, :client_id) "
        "RETURNING order_id"));
    insertOrder.bindValue(QStringLiteral(":order_date"), orderDate);
    insertOrder.bindValue(QStringLiteral(":client_id"), clientId);
    m_database.exec(insertOrder);
    insertOrder.next();
    const int orderId = insertOrder.value(0).toInt();

    QSqlQuery insertLine = m_database.prepare(QStringLiteral(
        "INSERT INTO order_items (order_id, stock_item_id, quantity, unit_price) "
        "VALUES (:order_id, :stock_item_id, :quantity, CAST(:unit_price AS numeric))"));
    QSqlQuery writeOff = m_database.prepare(QStringLiteral(
        "UPDATE stock_items SET quantity = quantity - :quantity "
        "WHERE stock_item_id = :stock_item_id"));
    for (const DraftLine &line : lines) {
        insertLine.bindValue(QStringLiteral(":order_id"), orderId);
        insertLine.bindValue(QStringLiteral(":stock_item_id"), line.stockItemId);
        insertLine.bindValue(QStringLiteral(":quantity"), line.quantity);
        insertLine.bindValue(QStringLiteral(":unit_price"), line.unitPrice.toDecimalString());
        m_database.exec(insertLine);
        writeOff.bindValue(QStringLiteral(":quantity"), line.quantity);
        writeOff.bindValue(QStringLiteral(":stock_item_id"), line.stockItemId);
        m_database.exec(writeOff);
    }
    transaction.commit();
    return orderId;
}

void OrderRepository::deleteOrder(int orderId)
{
    Database::Transaction transaction(m_database);
    QSqlQuery returnToStock = m_database.prepare(QStringLiteral(R"(
        UPDATE stock_items AS si
        SET quantity = si.quantity + oi.quantity
        FROM order_items AS oi
        WHERE si.stock_item_id = oi.stock_item_id AND oi.order_id = :order_id
    )"));
    returnToStock.bindValue(QStringLiteral(":order_id"), orderId);
    m_database.exec(returnToStock);
    QSqlQuery remove =
        m_database.prepare(QStringLiteral("DELETE FROM orders WHERE order_id = :order_id"));
    remove.bindValue(QStringLiteral(":order_id"), orderId);
    m_database.exec(remove);
    transaction.commit();
}

void OrderRepository::changeOrderDate(int orderId, QDate orderDate)
{
    QSqlQuery query = m_database.prepare(
        QStringLiteral("UPDATE orders SET order_date = :order_date WHERE order_id = :order_id"));
    query.bindValue(QStringLiteral(":order_date"), orderDate);
    query.bindValue(QStringLiteral(":order_id"), orderId);
    m_database.exec(query);
}

void OrderRepository::deleteLine(int orderItemId)
{
    Database::Transaction transaction(m_database);
    QSqlQuery lock = m_database.prepare(QStringLiteral(R"(
        SELECT o.order_id
        FROM orders AS o
        JOIN order_items AS oi ON oi.order_id = o.order_id
        WHERE oi.order_item_id = :order_item_id
        FOR UPDATE OF o
    )"));
    lock.bindValue(QStringLiteral(":order_item_id"), orderItemId);
    m_database.exec(lock);
    // Позицию уже удалили, например в другом окне, — удалять нечего
    if (!lock.next())
        return;

    QSqlQuery count = m_database.prepare(
        QStringLiteral("SELECT count(*) FROM order_items WHERE order_id = :order_id"));
    count.bindValue(QStringLiteral(":order_id"), lock.value(0));
    m_database.exec(count);
    count.next();
    if (count.value(0).toInt() <= 1)
        throw OrderError(lastLineMessage());

    QSqlQuery returnToStock = m_database.prepare(QStringLiteral(R"(
        UPDATE stock_items AS si
        SET quantity = si.quantity + oi.quantity
        FROM order_items AS oi
        WHERE si.stock_item_id = oi.stock_item_id AND oi.order_item_id = :order_item_id
    )"));
    returnToStock.bindValue(QStringLiteral(":order_item_id"), orderItemId);
    m_database.exec(returnToStock);
    QSqlQuery remove = m_database.prepare(
        QStringLiteral("DELETE FROM order_items WHERE order_item_id = :order_item_id"));
    remove.bindValue(QStringLiteral(":order_item_id"), orderItemId);
    m_database.exec(remove);
    transaction.commit();
}
