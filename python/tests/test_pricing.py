"""Тесты алгоритма расчёта цены со скидкой."""

from datetime import date
from decimal import Decimal

import pytest

from chudo_obuv.pricing import discount_percent, previous_month_bounds, price_with_discount


@pytest.mark.parametrize(
    ("calculation_date", "expected"),
    [
        (date(2026, 9, 30), (date(2026, 8, 1), date(2026, 9, 1))),
        (date(2026, 5, 1), (date(2026, 4, 1), date(2026, 5, 1))),
        (date(2026, 1, 15), (date(2025, 12, 1), date(2026, 1, 1))),
        (date(2024, 3, 31), (date(2024, 2, 1), date(2024, 3, 1))),
    ],
    ids=["конец месяца", "первое число", "январь — декабрь прошлого года", "високосный год"],
)
def test_previous_month_bounds(calculation_date, expected):
    assert previous_month_bounds(calculation_date) == expected


def test_discount_only_without_orders_in_previous_month():
    assert discount_percent(has_orders_in_previous_month=False) == Decimal(25)
    assert discount_percent(has_orders_in_previous_month=True) == Decimal(0)


@pytest.mark.parametrize(
    ("base_price", "discount", "expected"),
    [
        (Decimal("2190.00"), Decimal(25), Decimal("1642.50")),
        (Decimal("7757.00"), Decimal(25), Decimal("5817.75")),
        (Decimal("15670.00"), Decimal(0), Decimal("15670.00")),
        (Decimal("0.06"), Decimal(25), Decimal("0.05")),
    ],
    ids=["скидка 25 %", "копейки", "без скидки", "половина копейки округляется вверх"],
)
def test_price_with_discount(base_price, discount, expected):
    assert price_with_discount(base_price, discount) == expected
