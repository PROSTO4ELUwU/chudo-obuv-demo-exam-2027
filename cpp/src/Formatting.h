#pragma once

#include "Money.h"

#include <QDate>
#include <QString>

// Представление значений в интерфейсе: деньги, размеры, даты и количество пар
namespace Formatting {

// Неразрывный пробел не даёт сумме «1 642,50 ₽» разорваться при переносе строки
inline constexpr QChar nbsp = u' ';

// Сумма в рублях: «1 642,50 ₽»
QString money(Money value);

// Размер обуви из десятых долей: 380 → «38», 365 → «36,5»
QString size(int sizeTenths);

// Дата в формате ДД.ММ.ГГГГ
QString date(QDate value);

// Количество пар с согласованным существительным: «1 пара», «3 пары», «5 пар»
QString pairs(int count);

} // namespace Formatting
