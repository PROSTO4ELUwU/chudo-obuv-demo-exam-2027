// Интеграционные тесты запросов к базе данных (Qt Test).
// Каждый тест работает внутри транзакции, которая всегда откатывается,
// поэтому данные не меняются (номера заказов при этом могут пропускаться —
// последовательности PostgreSQL не откатываются). Если база недоступна,
// тесты пропускаются.

#include "AppConfig.h"
#include "Database.h"
#include "Errors.h"
#include "Repositories.h"

#include <QTest>

#include <memory>
#include <optional>

using namespace Qt::StringLiterals;

namespace {

const QString zvezdochka = u"Кроссовки детские «Звёздочка» экокожа"_s;
const QString raduga = u"Кроссовки детские «Радуга» экокожа перфорированная"_s;
const QDate september(2026, 9, 30);

} // namespace

class RepositoriesTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanup();

    void loginReturnsUserWithRole();
    void clientsAreOnlyAuthorizedUsers();
    void discountDependsOnOrdersInPreviousMonth();
    void totalQuantitySumsAllSizes();
    void getProductMatchesCatalog();
    void createOrderWritesOffStock();
    void orderIsNotSavedWhenStockIsShort();
    void deleteOrderReturnsPairsToStock();
    void deleteLineReturnsPairsAndKeepsLastLine();
    void deletingAlreadyDeletedLineChangesNothing();
    void changeOrderDate();

private:
    Product productByName(const QString &name, QDate calculationDate = september);
    StockPosition firstPosition(const Product &product);
    static DraftLine draftLine(const Product &product, const StockPosition &position, int quantity);
    int clientId(const QString &login = u"asidorova"_s);

    std::unique_ptr<Database> m_database;
    std::unique_ptr<UserRepository> m_users;
    std::unique_ptr<ProductRepository> m_products;
    std::unique_ptr<OrderRepository> m_orders;
    std::optional<Database::Transaction> m_transaction;
};

void RepositoriesTest::initTestCase()
{
    try {
        m_database = std::make_unique<Database>(
            AppConfig::loadDatabaseSettings(AppConfig::findConfigFile()));
        m_database->checkConnection();
    } catch (const AppError &error) {
        QSKIP(qPrintable(u"База данных недоступна: "_s + error.message()));
    }
    m_users = std::make_unique<UserRepository>(*m_database);
    m_products = std::make_unique<ProductRepository>(*m_database);
    m_orders = std::make_unique<OrderRepository>(*m_database);
}

void RepositoriesTest::init()
{
    m_transaction.emplace(*m_database);
}

void RepositoriesTest::cleanup()
{
    m_transaction.reset();
}

Product RepositoriesTest::productByName(const QString &name, QDate calculationDate)
{
    for (const Product &product : m_products->listProducts(calculationDate)) {
        if (product.name == name)
            return product;
    }
    throw AppError(u"Нет товара "_s + name);
}

StockPosition RepositoriesTest::firstPosition(const Product &product)
{
    return m_products->listPositions(product.id).front();
}

DraftLine RepositoriesTest::draftLine(const Product &product, const StockPosition &position,
                                      int quantity)
{
    return {position.stockItemId, product.name, product.manufacturer, position.sizeTenths,
            product.price, quantity, position.quantity};
}

int RepositoriesTest::clientId(const QString &login)
{
    return m_users->findByLogin(login)->id;
}

void RepositoriesTest::loginReturnsUserWithRole()
{
    const std::optional<User> user = m_users->findByLogin(u"isivanov"_s);
    QVERIFY(user.has_value());
    QCOMPARE(user->fullName(), u"Иванов Иван Сергеевич"_s);
    QCOMPARE(user->role, Role::Admin);
    QVERIFY(!m_users->findByLogin(u"нет_такого"_s).has_value());
}

void RepositoriesTest::clientsAreOnlyAuthorizedUsers()
{
    const std::vector<User> clients = m_users->listClients();
    QCOMPARE(clients.size(), 9);
    for (const User &client : clients)
        QCOMPARE(client.role, Role::Client);
}

void RepositoriesTest::discountDependsOnOrdersInPreviousMonth()
{
    // «Звёздочку» заказывали в апреле 2026, «Радугу» — нет
    const QDate may(2026, 5, 15);
    QCOMPARE(productByName(zvezdochka, may).discount, 0);
    QCOMPARE(productByName(raduga, may).discount, 25);
    QCOMPARE(productByName(zvezdochka, may).price.kopecks(), 219000);
    QCOMPARE(productByName(zvezdochka).price.kopecks(), 164250);
}

void RepositoriesTest::totalQuantitySumsAllSizes()
{
    QCOMPARE(productByName(zvezdochka).totalQuantity, 6 * 20);
}

void RepositoriesTest::getProductMatchesCatalog()
{
    const Product product = productByName(zvezdochka);
    const std::optional<Product> loaded = m_products->getProduct(product.id, september);
    QVERIFY(loaded.has_value());
    QCOMPARE(loaded->name, product.name);
    QCOMPARE(loaded->price, product.price);
    QCOMPARE(loaded->totalQuantity, product.totalQuantity);
    QVERIFY(!m_products->getProduct(-1, september).has_value());
}

void RepositoriesTest::createOrderWritesOffStock()
{
    const Product product = productByName(zvezdochka);
    const StockPosition position = firstPosition(product);
    const int orderId =
        m_orders->createOrder(clientId(), september, {draftLine(product, position, 2)});
    const std::optional<OrderSummary> summary = m_orders->getOrder(orderId);
    QVERIFY(summary.has_value());
    QCOMPARE(summary->clientName, u"Сидорова Анна Дмитриевна"_s);
    QCOMPARE(summary->total, product.price * 2);
    QCOMPARE(firstPosition(product).quantity, position.quantity - 2);
}

void RepositoriesTest::orderIsNotSavedWhenStockIsShort()
{
    const Product product = productByName(zvezdochka);
    const StockPosition position = firstPosition(product);
    const std::size_t ordersBefore = m_orders->listOrders().size();
    try {
        m_orders->createOrder(clientId(), september,
                              {draftLine(product, position, position.quantity + 1)});
        QFAIL("Ожидалась ошибка InsufficientStockError");
    } catch (const InsufficientStockError &error) {
        QVERIFY(error.message().contains(u"в наличии 20"_s));
        QCOMPARE(error.shortages().front().available, position.quantity);
    }
    QCOMPARE(m_orders->listOrders().size(), ordersBefore);
    QCOMPARE(firstPosition(product).quantity, position.quantity);
}

void RepositoriesTest::deleteOrderReturnsPairsToStock()
{
    const Product product = productByName(zvezdochka);
    const StockPosition position = firstPosition(product);
    const int orderId =
        m_orders->createOrder(clientId(), september, {draftLine(product, position, 3)});
    m_orders->deleteOrder(orderId);
    QVERIFY(!m_orders->getOrder(orderId).has_value());
    QCOMPARE(firstPosition(product).quantity, position.quantity);
}

void RepositoriesTest::deleteLineReturnsPairsAndKeepsLastLine()
{
    const Product product = productByName(zvezdochka);
    const auto size20InStock = [&] {
        for (const StockPosition &position : m_products->listPositions(product.id)) {
            if (position.sizeTenths == 200)
                return position.quantity;
        }
        return -1;
    };

    const std::vector<OrderLine> lines = m_orders->listLines(1);
    QCOMPARE(lines.size(), 2);
    QCOMPARE(lines[0].productName, zvezdochka);
    QCOMPARE(lines[0].sizeTenths, 200);
    const int inStock = size20InStock();
    m_orders->deleteLine(lines[0].orderItemId);
    QCOMPARE(size20InStock(), inStock + lines[0].quantity);
    const std::vector<OrderLine> remaining = m_orders->listLines(1);
    QCOMPARE(remaining.size(), 1);
    QCOMPARE(remaining.front().orderItemId, lines[1].orderItemId);
    QVERIFY_THROWS_EXCEPTION(OrderError, m_orders->deleteLine(lines[1].orderItemId));
}

void RepositoriesTest::deletingAlreadyDeletedLineChangesNothing()
{
    const std::vector<OrderLine> lines = m_orders->listLines(4);
    m_orders->deleteLine(lines[0].orderItemId);
    const Money total = m_orders->getOrder(4)->total;
    m_orders->deleteLine(lines[0].orderItemId);
    QCOMPARE(m_orders->getOrder(4)->total, total);
    QCOMPARE(m_orders->listLines(4).size(), lines.size() - 1);
}

void RepositoriesTest::changeOrderDate()
{
    m_orders->changeOrderDate(1, QDate(2026, 4, 3));
    QCOMPARE(m_orders->getOrder(1)->date, QDate(2026, 4, 3));
}

QTEST_GUILESS_MAIN(RepositoriesTest)

#include "tst_repositories.moc"
