"""Тесты поиска, фильтрации и сортировки каталога."""

from decimal import Decimal

from chudo_obuv.catalog_filter import ALL_CATEGORIES, SortOrder, filter_products
from chudo_obuv.models import Product


def make_product(product_id: int, name: str, category: str, price: str,
                 description: str = "") -> Product:
    return Product(product_id, name, category, "Производство", description, "",
                   Decimal(price), Decimal(0), Decimal(price), 10, None)


CATALOG = [
    make_product(1, "Кроссовки детские «Звёздочка»", "Детская обувь", "1642.50",
                 "Производитель: ООО «Малыш-Спорт», г. Смоленск"),
    make_product(2, "Сапоги зимние", "Женская обувь", "18750.00"),
    make_product(3, "Кроссовки кожаные", "Женская обувь", "6573.75"),
    make_product(4, "Ботинки зимние классические", "Мужская обувь", "11752.50"),
]


def ids(products: list[Product]) -> list[int]:
    return [product.product_id for product in products]


def test_without_conditions_all_products_in_catalog_order():
    assert ids(filter_products(CATALOG, "", ALL_CATEGORIES, SortOrder.NONE)) == [1, 2, 3, 4]


def test_search_ignores_case_and_yo():
    assert ids(filter_products(CATALOG, "ЗВЕЗДОЧКА", ALL_CATEGORIES, SortOrder.NONE)) == [1]


def test_search_looks_in_description():
    assert ids(filter_products(CATALOG, "смоленск", ALL_CATEGORIES, SortOrder.NONE)) == [1]


def test_category_and_search_work_together():
    assert ids(filter_products(CATALOG, "зимние", "Женская обувь", SortOrder.NONE)) == [2]
    assert ids(filter_products(CATALOG, "зимние", ALL_CATEGORIES, SortOrder.NONE)) == [2, 4]


def test_sorting_is_kept_with_filter():
    assert ids(filter_products(CATALOG, "", ALL_CATEGORIES, SortOrder.PRICE_ASCENDING)) \
        == [1, 3, 4, 2]
    assert ids(filter_products(CATALOG, "", "Женская обувь", SortOrder.PRICE_DESCENDING)) \
        == [2, 3]


def test_nothing_found():
    assert filter_products(CATALOG, "сандалии", ALL_CATEGORIES, SortOrder.NONE) == []
