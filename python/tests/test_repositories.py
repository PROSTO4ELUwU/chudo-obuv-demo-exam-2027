"""Интеграционные тесты запросов к базе данных.

Тесты работают с базой из config.ini внутри транзакции, которая всегда
откатывается, поэтому данные не меняются (номера заказов при этом могут
пропускаться — последовательности PostgreSQL не откатываются).
Если база недоступна, тесты пропускаются.
"""

from datetime import date
from decimal import Decimal

import psycopg
import pytest

from chudo_obuv.config import load_database_settings
from chudo_obuv.database import Database
from chudo_obuv.models import Role
from chudo_obuv.order_draft import DraftLine
from chudo_obuv.repositories import (
    InsufficientStockError,
    OrderError,
    OrderRepository,
    ProductRepository,
    UserRepository,
)

ZVEZDOCHKA = "Кроссовки детские «Звёздочка» экокожа"
RADUGA = "Кроссовки детские «Радуга» экокожа перфорированная"


@pytest.fixture(scope="module")
def database():
    database = Database(load_database_settings())
    try:
        database.connection
    except psycopg.OperationalError as error:
        pytest.skip(f"База данных недоступна: {error}")
    yield database
    database.close()


@pytest.fixture
def db(database):
    with database.connection.transaction(force_rollback=True):
        yield database


def product_by_name(db, name, calculation_date=date(2026, 9, 30)):
    products = ProductRepository(db).list_products(calculation_date)
    return next(product for product in products if product.name == name)


def first_position(db, name):
    product = product_by_name(db, name)
    return product, ProductRepository(db).list_positions(product.product_id)[0]


def draft_line(product, position, quantity):
    return DraftLine(position.stock_item_id, product.name, product.manufacturer,
                     position.size, product.price, quantity, position.quantity)


def client_id(db, login="asidorova"):
    return UserRepository(db).find_by_login(login).user_id


def test_login_returns_user_with_role(db):
    user = UserRepository(db).find_by_login("isivanov")
    assert user.full_name == "Иванов Иван Сергеевич"
    assert user.role is Role.ADMIN
    assert UserRepository(db).find_by_login("нет_такого") is None


def test_clients_are_only_authorized_users(db):
    clients = UserRepository(db).list_clients()
    assert len(clients) == 9
    assert {client.role for client in clients} == {Role.CLIENT}


def test_discount_depends_on_orders_in_previous_month(db):
    # «Звёздочку» заказывали в апреле 2026, «Радугу» — нет
    may = date(2026, 5, 15)
    assert product_by_name(db, ZVEZDOCHKA, may).discount == 0
    assert product_by_name(db, RADUGA, may).discount == 25
    assert product_by_name(db, ZVEZDOCHKA, may).price == Decimal("2190.00")
    assert product_by_name(db, ZVEZDOCHKA, date(2026, 9, 30)).price == Decimal("1642.50")


def test_total_quantity_sums_all_sizes(db):
    assert product_by_name(db, ZVEZDOCHKA).total_quantity == 6 * 20


def test_get_product_matches_catalog(db):
    product = product_by_name(db, ZVEZDOCHKA)
    assert ProductRepository(db).get_product(product.product_id, date(2026, 9, 30)) == product
    assert ProductRepository(db).get_product(-1, date(2026, 9, 30)) is None


def test_create_order_writes_off_stock(db):
    orders = OrderRepository(db)
    product, position = first_position(db, ZVEZDOCHKA)
    order_id = orders.create_order(client_id(db), date(2026, 9, 30),
                                   [draft_line(product, position, 2)])
    summary = orders.get_order(order_id)
    assert summary.client_name == "Сидорова Анна Дмитриевна"
    assert summary.total == product.price * 2
    assert ProductRepository(db).list_positions(product.product_id)[0].quantity \
        == position.quantity - 2


def test_order_is_not_saved_when_stock_is_short(db):
    orders = OrderRepository(db)
    product, position = first_position(db, ZVEZDOCHKA)
    orders_before = len(orders.list_orders())
    with pytest.raises(InsufficientStockError, match="в наличии 20"):
        orders.create_order(client_id(db), date(2026, 9, 30),
                            [draft_line(product, position, position.quantity + 1)])
    assert len(orders.list_orders()) == orders_before
    assert ProductRepository(db).list_positions(product.product_id)[0].quantity \
        == position.quantity


def test_delete_order_returns_pairs_to_stock(db):
    orders = OrderRepository(db)
    product, position = first_position(db, ZVEZDOCHKA)
    order_id = orders.create_order(client_id(db), date(2026, 9, 30),
                                   [draft_line(product, position, 3)])
    orders.delete_order(order_id)
    assert orders.get_order(order_id) is None
    assert ProductRepository(db).list_positions(product.product_id)[0].quantity \
        == position.quantity


def test_delete_line_returns_pairs_and_keeps_last_line(db):
    orders = OrderRepository(db)
    lines = orders.list_lines(1)
    assert len(lines) == 2
    orders.delete_line(lines[0].order_item_id)
    assert [line.order_item_id for line in orders.list_lines(1)] == [lines[1].order_item_id]
    with pytest.raises(OrderError, match="единственная позиция"):
        orders.delete_line(lines[1].order_item_id)


def test_change_order_date(db):
    orders = OrderRepository(db)
    orders.change_order_date(1, date(2026, 4, 3))
    assert orders.get_order(1).order_date == date(2026, 4, 3)
