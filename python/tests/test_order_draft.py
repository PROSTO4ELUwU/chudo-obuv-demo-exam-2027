"""Тесты формируемого заказа и правил отображения каталога."""

from decimal import Decimal

import pytest

from chudo_obuv.models import Product, StockPosition
from chudo_obuv.order_draft import OrderDraft, OrderDraftError


def make_product(total_quantity: int = 9) -> Product:
    return Product(
        product_id=1,
        name="Кроссовки",
        category="Мужская обувь",
        manufacturer="Топ-Топ",
        description="",
        composition="",
        base_price=Decimal("9567.00"),
        discount=Decimal(25),
        price=Decimal("7175.25"),
        total_quantity=total_quantity,
        image_file=None,
    )


SIZE_41 = StockPosition(stock_item_id=10, size=Decimal("41.0"), quantity=3)
SIZE_42 = StockPosition(stock_item_id=11, size=Decimal("42.0"), quantity=3)


def test_same_size_is_merged_into_one_line():
    draft = OrderDraft()
    draft.add(make_product(), SIZE_41, 1)
    draft.add(make_product(), SIZE_41, 2)
    assert len(draft.lines) == 1
    assert draft.reserved(SIZE_41.stock_item_id) == 3


def test_total_uses_price_with_discount():
    draft = OrderDraft()
    draft.add(make_product(), SIZE_41, 2)
    draft.add(make_product(), SIZE_42, 1)
    assert draft.total == Decimal("21525.75")
    assert draft.pairs_count == 3


def test_cannot_add_more_than_in_stock():
    draft = OrderDraft()
    draft.add(make_product(), SIZE_41, 2)
    with pytest.raises(OrderDraftError, match="можно добавить: 1"):
        draft.add(make_product(), SIZE_41, 2)
    assert draft.reserved(SIZE_41.stock_item_id) == 2


@pytest.mark.parametrize("quantity", [0, 4], ids=["ноль пар", "больше остатка"])
def test_set_quantity_keeps_value_on_error(quantity):
    draft = OrderDraft()
    draft.add(make_product(), SIZE_41, 1)
    with pytest.raises(OrderDraftError):
        draft.set_quantity(SIZE_41.stock_item_id, quantity)
    assert draft.reserved(SIZE_41.stock_item_id) == 1


def test_remove_and_clear():
    draft = OrderDraft()
    draft.add(make_product(), SIZE_41, 1)
    draft.add(make_product(), SIZE_42, 1)
    draft.remove(SIZE_41.stock_item_id)
    assert [line.stock_item_id for line in draft.lines] == [SIZE_42.stock_item_id]
    draft.clear()
    assert draft.is_empty


@pytest.mark.parametrize(
    ("total_quantity", "text", "running_out"),
    [(6, "много", False), (5, "мало", False), (3, "мало", True), (0, "мало", True)],
    ids=["больше пяти", "ровно пять", "три — подсветка", "нет в наличии"],
)
def test_catalog_quantity_rules(total_quantity, text, running_out):
    product = make_product(total_quantity)
    assert product.quantity_text == text
    assert product.is_running_out is running_out
