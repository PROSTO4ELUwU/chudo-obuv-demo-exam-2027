"""Представление значений в интерфейсе: деньги, размеры, даты и количество пар."""

from datetime import date
from decimal import Decimal

NBSP = " "


def format_money(value: Decimal) -> str:
    """Сумма в рублях: 1 642,50 ₽.

    Неразрывные пробелы не дают сумме разорваться при переносе строки.
    """
    text = f"{value:,.2f}".replace(",", NBSP).replace(".", ",")
    return f"{text}{NBSP}₽"


def format_size(value: Decimal) -> str:
    """Размер обуви: целый без дробной части (38), половинчатый с запятой (38,5)."""
    if value == value.to_integral_value():
        return str(int(value))
    return str(value.normalize()).replace(".", ",")


def format_date(value: date) -> str:
    """Дата в формате ДД.ММ.ГГГГ."""
    return value.strftime("%d.%m.%Y")


def pairs(count: int) -> str:
    """Количество пар с согласованным существительным: 1 пара, 3 пары, 5 пар."""
    if count % 10 == 1 and count % 100 != 11:
        word = "пара"
    elif 2 <= count % 10 <= 4 and not 12 <= count % 100 <= 14:
        word = "пары"
    else:
        word = "пар"
    return f"{count}{NBSP}{word}"
