"""Поиск, фильтрация и сортировка каталога (задание 3)."""

from enum import StrEnum

from chudo_obuv.models import Product

ALL_CATEGORIES = "Все категории"


class SortOrder(StrEnum):
    """Варианты сортировки в выпадающем списке."""

    NONE = "Без сортировки"
    PRICE_ASCENDING = "Цена по возрастанию"
    PRICE_DESCENDING = "Цена по убыванию"


def normalize(text: str) -> str:
    """Текст для поиска без учёта регистра и различия «е» и «ё»."""
    return text.casefold().replace("ё", "е")


def filter_products(products: list[Product], search_text: str, category: str,
                    sort_order: SortOrder) -> list[Product]:
    """Товары выбранной категории, в наименовании или описании которых есть строка поиска.

    Фильтр и поиск применяются совместно, а сортировка — к их результату,
    поэтому выбранный порядок сохраняется при любом изменении условий.
    Сортировка устойчивая: товары с одинаковой ценой остаются в порядке каталога.
    """
    needle = normalize(search_text.strip())
    result = [
        product for product in products
        if category in (ALL_CATEGORIES, product.category)
        and (needle in normalize(product.name) or needle in normalize(product.description))
    ]
    if sort_order is SortOrder.PRICE_ASCENDING:
        result.sort(key=lambda product: product.price)
    elif sort_order is SortOrder.PRICE_DESCENDING:
        result.sort(key=lambda product: product.price, reverse=True)
    return result
