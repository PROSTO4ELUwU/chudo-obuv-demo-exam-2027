#pragma once

#include "Money.h"

#include <QDate>

// Алгоритм расчёта цены товара с учётом скидки (задание 2).
// Скидка 25 % действует на товары, по которым нет заказов в предыдущем
// календарном месяце относительно даты расчёта.
// Блок-схема алгоритма: docs/algorithm_discount.pdf.
namespace Pricing {

inline constexpr int discountPercent = 25;

// Предыдущий календарный месяц как полуинтервал [start; end): при сравнении
// «дата < end» последний день предыдущего месяца попадает в период целиком
struct Period
{
    QDate start;
    QDate end;
};

Period previousMonth(QDate calculationDate);

// Размер скидки в процентах
int discountFor(bool hasOrdersInPreviousMonth);

// Цена со скидкой, округлённая до копеек по правилам арифметики
Money priceWithDiscount(Money basePrice, int discount);

} // namespace Pricing
