"""Алгоритм расчёта цены товара с учётом скидки (задание 2).

Скидка 25 % действует на товары, по которым нет заказов в предыдущем
календарном месяце относительно даты расчёта.
Блок-схема алгоритма: docs/algorithm_discount.pdf.
"""

from datetime import date, timedelta
from decimal import ROUND_HALF_UP, Decimal

DISCOUNT_PERCENT = Decimal(25)
KOPECK = Decimal("0.01")


def previous_month_bounds(calculation_date: date) -> tuple[date, date]:
    """Границы предыдущего календарного месяца в виде полуинтервала [начало, конец).

    Конец — первое число месяца расчёта: при сравнении «дата < конец»
    последний день предыдущего месяца попадает в период целиком.
    """
    period_end = calculation_date.replace(day=1)
    period_start = (period_end - timedelta(days=1)).replace(day=1)
    return period_start, period_end


def discount_percent(has_orders_in_previous_month: bool) -> Decimal:
    """Размер скидки в процентах."""
    if has_orders_in_previous_month:
        return Decimal(0)
    return DISCOUNT_PERCENT


def price_with_discount(base_price: Decimal, discount: Decimal) -> Decimal:
    """Цена со скидкой, округлённая до копеек по правилам арифметики."""
    price = base_price * (100 - discount) / 100
    return price.quantize(KOPECK, rounding=ROUND_HALF_UP)
