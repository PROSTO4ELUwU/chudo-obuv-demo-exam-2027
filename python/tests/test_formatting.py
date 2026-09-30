"""Тесты форматирования значений для интерфейса."""

from datetime import date
from decimal import Decimal

import pytest

from chudo_obuv.formatting import NBSP, format_date, format_money, format_size, pairs


def test_format_money():
    assert format_money(Decimal("1642.5")) == f"1{NBSP}642,50{NBSP}₽"
    assert format_money(Decimal("990")) == f"990,00{NBSP}₽"


@pytest.mark.parametrize(
    ("size", "text"),
    [(Decimal("38.0"), "38"), (Decimal("36.5"), "36,5")],
    ids=["целый", "половинчатый"],
)
def test_format_size(size, text):
    assert format_size(size) == text


def test_format_date():
    assert format_date(date(2026, 4, 2)) == "02.04.2026"


@pytest.mark.parametrize(
    ("count", "text"),
    [(1, "1 пара"), (3, "3 пары"), (5, "5 пар"), (11, "11 пар"), (12, "12 пар"),
     (21, "21 пара"), (22, "22 пары"), (0, "0 пар")],
)
def test_pairs(count, text):
    assert pairs(count) == text.replace(" ", NBSP)
