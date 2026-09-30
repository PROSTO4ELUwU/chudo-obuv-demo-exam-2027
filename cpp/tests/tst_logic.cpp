// Тесты логики без базы данных и окон (Qt Test)

#include "Formatting.h"
#include "Money.h"
#include "Pricing.h"

#include <QTest>

using namespace Qt::StringLiterals;

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

QTEST_APPLESS_MAIN(LogicTest)

#include "tst_logic.moc"
