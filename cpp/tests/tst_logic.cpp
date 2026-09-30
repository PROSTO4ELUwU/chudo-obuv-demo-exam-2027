// Тесты логики без базы данных и окон (Qt Test)

#include "CatalogFilter.h"
#include "Formatting.h"
#include "Money.h"
#include "OrderDraft.h"
#include "Pricing.h"

#include <QTest>

using namespace Qt::StringLiterals;

namespace {

Product makeProduct(int totalQuantity = 9)
{
    Product product;
    product.id = 1;
    product.name = u"Кроссовки"_s;
    product.category = u"Мужская обувь"_s;
    product.manufacturer = u"Топ-Топ"_s;
    product.basePrice = Money::fromKopecks(956700);
    product.discount = 25;
    product.price = Money::fromKopecks(717525);
    product.totalQuantity = totalQuantity;
    return product;
}

const StockPosition size41{10, 410, 3};
const StockPosition size42{11, 420, 3};

Product catalogItem(int id, const QString &name, const QString &category, qint64 kopecks,
                    const QString &description = {})
{
    Product product;
    product.id = id;
    product.name = name;
    product.category = category;
    product.description = description;
    product.price = Money::fromKopecks(kopecks);
    return product;
}

const std::vector<Product> catalog = {
    catalogItem(1, u"Кроссовки детские «Звёздочка»"_s, u"Детская обувь"_s, 164250,
                u"Производитель: ООО «Малыш-Спорт», г. Смоленск"_s),
    catalogItem(2, u"Сапоги зимние"_s, u"Женская обувь"_s, 1875000),
    catalogItem(3, u"Кроссовки кожаные"_s, u"Женская обувь"_s, 657375),
    catalogItem(4, u"Ботинки зимние классические"_s, u"Мужская обувь"_s, 1175250),
};

QList<int> ids(const std::vector<Product> &products)
{
    QList<int> result;
    for (const Product &product : products)
        result << product.id;
    return result;
}

} // namespace

class LogicTest : public QObject
{
    Q_OBJECT

private slots:
    void previousMonth_data();
    void previousMonth();
    void discountOnlyWithoutOrdersInPreviousMonth();
    void priceWithDiscount_data();
    void priceWithDiscount();
    void moneyFromDatabaseText();
    void formatMoneySizeDate();
    void pairs_data();
    void pairs();

    void sameSizeIsMergedIntoOneLine();
    void totalUsesPriceWithDiscount();
    void cannotAddMoreThanInStock();
    void setQuantityKeepsValueOnError();
    void updateAvailableReducesQuantity();
    void removeAndClear();
    void catalogQuantityRules_data();
    void catalogQuantityRules();

    void filterWithoutConditions();
    void searchIgnoresCaseAndYo();
    void searchLooksInDescription();
    void categoryAndSearchWorkTogether();
    void sortingIsKeptWithFilter();
    void nothingFound();
};

void LogicTest::previousMonth_data()
{
    QTest::addColumn<QDate>("calculationDate");
    QTest::addColumn<QDate>("start");
    QTest::addColumn<QDate>("end");
    QTest::newRow("конец месяца") << QDate(2026, 9, 30) << QDate(2026, 8, 1) << QDate(2026, 9, 1);
    QTest::newRow("первое число") << QDate(2026, 5, 1) << QDate(2026, 4, 1) << QDate(2026, 5, 1);
    QTest::newRow("январь — декабрь прошлого года")
        << QDate(2026, 1, 15) << QDate(2025, 12, 1) << QDate(2026, 1, 1);
    QTest::newRow("високосный год") << QDate(2024, 3, 31) << QDate(2024, 2, 1) << QDate(2024, 3, 1);
}

void LogicTest::previousMonth()
{
    QFETCH(QDate, calculationDate);
    QFETCH(QDate, start);
    QFETCH(QDate, end);
    const Pricing::Period period = Pricing::previousMonth(calculationDate);
    QCOMPARE(period.start, start);
    QCOMPARE(period.end, end);
}

void LogicTest::discountOnlyWithoutOrdersInPreviousMonth()
{
    QCOMPARE(Pricing::discountFor(false), 25);
    QCOMPARE(Pricing::discountFor(true), 0);
}

void LogicTest::priceWithDiscount_data()
{
    QTest::addColumn<qint64>("basePrice");
    QTest::addColumn<int>("discount");
    QTest::addColumn<qint64>("expected");
    QTest::newRow("скидка 25 %") << qint64(219000) << 25 << qint64(164250);
    QTest::newRow("копейки") << qint64(775700) << 25 << qint64(581775);
    QTest::newRow("без скидки") << qint64(1567000) << 0 << qint64(1567000);
    QTest::newRow("половина копейки округляется вверх") << qint64(6) << 25 << qint64(5);
}

void LogicTest::priceWithDiscount()
{
    QFETCH(qint64, basePrice);
    QFETCH(int, discount);
    QFETCH(qint64, expected);
    const Money price = Pricing::priceWithDiscount(Money::fromKopecks(basePrice), discount);
    QCOMPARE(price.kopecks(), expected);
}

void LogicTest::moneyFromDatabaseText()
{
    QCOMPARE(Money::fromDecimalString(u"2190.00"_s).kopecks(), 219000);
    QCOMPARE(Money::fromDecimalString(u"1642.5"_s).kopecks(), 164250);
    QCOMPARE(Money::fromDecimalString(u"-5.25"_s).kopecks(), -525);
    QCOMPARE(Money::fromKopecks(164250).toDecimalString(), u"1642.50"_s);
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Money::fromDecimalString(u"12.345"_s));
}

void LogicTest::formatMoneySizeDate()
{
    const QString nbsp(Formatting::nbsp);
    QCOMPARE(Formatting::money(Money::fromKopecks(164250)), u"1"_s + nbsp + u"642,50"_s + nbsp + u"₽"_s);
    QCOMPARE(Formatting::money(Money::fromKopecks(99000)), u"990,00"_s + nbsp + u"₽"_s);
    QCOMPARE(Formatting::size(380), u"38"_s);
    QCOMPARE(Formatting::size(365), u"36,5"_s);
    QCOMPARE(Formatting::date(QDate(2026, 4, 2)), u"02.04.2026"_s);
}

void LogicTest::pairs_data()
{
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("expected");
    const QStringList cases = {u"1 пара"_s, u"3 пары"_s, u"5 пар"_s, u"11 пар"_s,
                               u"12 пар"_s, u"21 пара"_s, u"22 пары"_s, u"0 пар"_s};
    // Имя случая передаётся в UTF-8: qPrintable перекодировал бы его в ANSI Windows
    for (const QString &text : cases)
        QTest::newRow(text.toUtf8().constData()) << text.section(u' ', 0, 0).toInt() << text;
}

void LogicTest::pairs()
{
    QFETCH(int, count);
    QFETCH(QString, expected);
    QCOMPARE(Formatting::pairs(count), expected.replace(u' ', Formatting::nbsp));
}

void LogicTest::sameSizeIsMergedIntoOneLine()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 1);
    draft.add(makeProduct(), size41, 2);
    QCOMPARE(draft.lines().size(), 1);
    QCOMPARE(draft.reserved(size41.stockItemId), 3);
}

void LogicTest::totalUsesPriceWithDiscount()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 2);
    draft.add(makeProduct(), size42, 1);
    QCOMPARE(draft.total().kopecks(), 2152575);
    QCOMPARE(draft.pairsCount(), 3);
}

void LogicTest::cannotAddMoreThanInStock()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 2);
    try {
        draft.add(makeProduct(), size41, 2);
        QFAIL("Ожидалась ошибка OrderDraftError");
    } catch (const OrderDraftError &error) {
        QVERIFY(error.message().contains(u"можно добавить: 1"_s));
    }
    QCOMPARE(draft.reserved(size41.stockItemId), 2);
}

void LogicTest::setQuantityKeepsValueOnError()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 1);
    for (const int quantity : {0, 4}) {
        QVERIFY_THROWS_EXCEPTION(OrderDraftError, draft.setQuantity(size41.stockItemId, quantity));
        QCOMPARE(draft.reserved(size41.stockItemId), 1);
    }
}

void LogicTest::updateAvailableReducesQuantity()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 3);
    draft.add(makeProduct(), size42, 2);
    draft.updateAvailable(size41.stockItemId, 1);
    draft.updateAvailable(size42.stockItemId, 0);
    QCOMPARE(draft.lines()[0].quantity, 1);
    QVERIFY(!draft.lines()[0].exceedsStock());
    QCOMPARE(draft.lines()[1].quantity, 2);
    QVERIFY(draft.lines()[1].exceedsStock());
}

void LogicTest::removeAndClear()
{
    OrderDraft draft;
    draft.add(makeProduct(), size41, 1);
    draft.add(makeProduct(), size42, 1);
    draft.remove(size41.stockItemId);
    QCOMPARE(draft.lines().size(), 1);
    QCOMPARE(draft.lines()[0].stockItemId, size42.stockItemId);
    draft.clear();
    QVERIFY(draft.isEmpty());
}

void LogicTest::catalogQuantityRules_data()
{
    QTest::addColumn<int>("totalQuantity");
    QTest::addColumn<QString>("text");
    QTest::addColumn<bool>("runningOut");
    QTest::newRow("больше пяти") << 6 << u"много"_s << false;
    QTest::newRow("ровно пять") << 5 << u"мало"_s << false;
    QTest::newRow("три — подсветка") << 3 << u"мало"_s << true;
    QTest::newRow("нет в наличии") << 0 << u"мало"_s << true;
}

void LogicTest::catalogQuantityRules()
{
    QFETCH(int, totalQuantity);
    QFETCH(QString, text);
    QFETCH(bool, runningOut);
    const Product product = makeProduct(totalQuantity);
    QCOMPARE(product.quantityText(), text);
    QCOMPARE(product.isRunningOut(), runningOut);
}

void LogicTest::filterWithoutConditions()
{
    const auto result = filterProducts(catalog, {}, allCategoriesName(), SortOrder::None);
    QCOMPARE(ids(result), QList<int>({1, 2, 3, 4}));
}

void LogicTest::searchIgnoresCaseAndYo()
{
    const auto result = filterProducts(catalog, u"ЗВЕЗДОЧКА"_s, allCategoriesName(), SortOrder::None);
    QCOMPARE(ids(result), QList<int>({1}));
}

void LogicTest::searchLooksInDescription()
{
    const auto result = filterProducts(catalog, u"смоленск"_s, allCategoriesName(), SortOrder::None);
    QCOMPARE(ids(result), QList<int>({1}));
}

void LogicTest::categoryAndSearchWorkTogether()
{
    QCOMPARE(ids(filterProducts(catalog, u"зимние"_s, u"Женская обувь"_s, SortOrder::None)),
             QList<int>({2}));
    QCOMPARE(ids(filterProducts(catalog, u"зимние"_s, allCategoriesName(), SortOrder::None)),
             QList<int>({2, 4}));
}

void LogicTest::sortingIsKeptWithFilter()
{
    QCOMPARE(ids(filterProducts(catalog, {}, allCategoriesName(), SortOrder::PriceAscending)),
             QList<int>({1, 3, 4, 2}));
    QCOMPARE(ids(filterProducts(catalog, {}, u"Женская обувь"_s, SortOrder::PriceDescending)),
             QList<int>({2, 3}));
}

void LogicTest::nothingFound()
{
    QVERIFY(filterProducts(catalog, u"сандалии"_s, allCategoriesName(), SortOrder::None).empty());
}

QTEST_APPLESS_MAIN(LogicTest)

#include "tst_logic.moc"
