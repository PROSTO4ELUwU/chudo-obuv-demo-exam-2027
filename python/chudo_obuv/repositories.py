"""Запросы к базе данных: пользователи, каталог и заказы."""

from datetime import date

from psycopg.rows import class_row

from chudo_obuv.database import Database
from chudo_obuv.formatting import format_size, pairs
from chudo_obuv.models import OrderLine, OrderSummary, Product, Role, StockPosition, User
from chudo_obuv.order_draft import DraftLine
from chudo_obuv.pricing import discount_percent, previous_month_bounds, price_with_discount


class OrderError(Exception):
    """Операция с заказом недопустима; текст показывается пользователю."""


class InsufficientStockError(OrderError):
    """Пока заказ формировался, нужного количества пар не осталось."""

    def __init__(self, shortages: list[tuple[DraftLine, int]]) -> None:
        self.shortages = shortages
        details = "\n".join(
            f"• {line.product_name}, размер {format_size(line.size)}: "
            f"в заказе {pairs(line.quantity)}, в наличии {pairs(available)}"
            for line, available in shortages
        )
        super().__init__(f"Пока заказ формировался, часть товара закончилась:\n{details}")


class UserRepository:
    """Пользователи системы."""

    _SELECT = """
        SELECT u.user_id, u.last_name, u.first_name, u.patronymic, r.role_name
        FROM users AS u
        JOIN roles AS r ON r.role_id = u.role_id
    """

    def __init__(self, database: Database) -> None:
        self._database = database

    def find_by_login(self, login: str) -> User | None:
        """Пользователь с указанным логином или None, если такого нет."""
        row = self._database.connection.execute(
            self._SELECT + " WHERE u.login = %s", (login,)
        ).fetchone()
        return self._to_user(row) if row else None

    def list_clients(self) -> list[User]:
        """Клиенты, на которых менеджер или администратор оформляет заказ."""
        rows = self._database.connection.execute(
            self._SELECT + " WHERE r.role_name = %s ORDER BY u.last_name, u.first_name",
            (Role.CLIENT.value,),
        ).fetchall()
        return [self._to_user(row) for row in rows]

    @staticmethod
    def _to_user(row: tuple) -> User:
        user_id, last_name, first_name, patronymic, role_name = row
        return User(user_id, last_name, first_name, patronymic, Role(role_name))


class ProductRepository:
    """Каталог товаров."""

    # Скидка зависит от заказов модели в любом размере за предыдущий месяц
    _PRODUCTS_QUERY = """
        SELECT p.product_id, p.product_name, c.category_name, m.manufacturer_name,
               p.description, p.composition, p.price, p.image_file,
               COALESCE(SUM(si.quantity), 0) AS total_quantity,
               EXISTS (
                   SELECT 1
                   FROM order_items AS oi
                   JOIN orders AS o ON o.order_id = oi.order_id
                   JOIN stock_items AS ordered ON ordered.stock_item_id = oi.stock_item_id
                   WHERE ordered.product_id = p.product_id
                     AND o.order_date >= %(period_start)s
                     AND o.order_date < %(period_end)s
               ) AS has_recent_orders
        FROM products AS p
        JOIN categories AS c ON c.category_id = p.category_id
        JOIN manufacturers AS m ON m.manufacturer_id = p.manufacturer_id
        LEFT JOIN stock_items AS si ON si.product_id = p.product_id
        GROUP BY p.product_id, c.category_name, m.manufacturer_name
        ORDER BY p.product_id
    """

    def __init__(self, database: Database) -> None:
        self._database = database

    def list_products(self, calculation_date: date) -> list[Product]:
        """Каталог с ценами, рассчитанными на указанную дату."""
        period_start, period_end = previous_month_bounds(calculation_date)
        rows = self._database.connection.execute(
            self._PRODUCTS_QUERY, {"period_start": period_start, "period_end": period_end}
        ).fetchall()
        products = []
        for (product_id, name, category, manufacturer, description, composition,
             base_price, image_file, total_quantity, has_recent_orders) in rows:
            discount = discount_percent(has_recent_orders)
            products.append(Product(
                product_id=product_id,
                name=name,
                category=category,
                manufacturer=manufacturer,
                description=description,
                composition=composition,
                base_price=base_price,
                discount=discount,
                price=price_with_discount(base_price, discount),
                total_quantity=total_quantity,
                image_file=image_file,
            ))
        return products

    def list_categories(self) -> list[str]:
        """Названия всех категорий для фильтра."""
        rows = self._database.connection.execute(
            "SELECT category_name FROM categories ORDER BY category_name"
        ).fetchall()
        return [category for (category,) in rows]

    def list_positions(self, product_id: int) -> list[StockPosition]:
        """Размерный ряд модели с остатками."""
        with self._database.connection.cursor(row_factory=class_row(StockPosition)) as cursor:
            return cursor.execute(
                """
                SELECT si.stock_item_id, s.size_value AS size, si.quantity
                FROM stock_items AS si
                JOIN sizes AS s ON s.size_id = si.size_id
                WHERE si.product_id = %s
                ORDER BY s.size_value
                """,
                (product_id,),
            ).fetchall()


class OrderRepository:
    """Заказы и их состав."""

    _SUMMARY_SELECT = """
        SELECT o.order_id, o.order_date,
               concat_ws(' ', u.last_name, u.first_name, u.patronymic) AS client_name,
               COALESCE(SUM(oi.quantity * oi.unit_price), 0) AS total
        FROM orders AS o
        JOIN users AS u ON u.user_id = o.client_id
        LEFT JOIN order_items AS oi ON oi.order_id = o.order_id
    """
    _SUMMARY_GROUP_BY = " GROUP BY o.order_id, u.user_id"

    # Пары удалённой позиции или заказа возвращаются в остатки
    _RETURN_TO_STOCK = """
        UPDATE stock_items AS si
        SET quantity = si.quantity + oi.quantity
        FROM order_items AS oi
        WHERE si.stock_item_id = oi.stock_item_id AND {condition}
    """

    def __init__(self, database: Database) -> None:
        self._database = database

    def list_orders(self) -> list[OrderSummary]:
        """Все заказы, новые сверху."""
        query = (self._SUMMARY_SELECT + self._SUMMARY_GROUP_BY
                 + " ORDER BY o.order_date DESC, o.order_id DESC")
        with self._database.connection.cursor(row_factory=class_row(OrderSummary)) as cursor:
            return cursor.execute(query).fetchall()

    def get_order(self, order_id: int) -> OrderSummary | None:
        """Заказ по номеру или None, если его уже удалили."""
        query = self._SUMMARY_SELECT + " WHERE o.order_id = %s" + self._SUMMARY_GROUP_BY
        with self._database.connection.cursor(row_factory=class_row(OrderSummary)) as cursor:
            return cursor.execute(query, (order_id,)).fetchone()

    def list_lines(self, order_id: int) -> list[OrderLine]:
        """Состав заказа."""
        with self._database.connection.cursor(row_factory=class_row(OrderLine)) as cursor:
            return cursor.execute(
                """
                SELECT oi.order_item_id, p.product_name, m.manufacturer_name AS manufacturer,
                       s.size_value AS size, oi.quantity, oi.unit_price
                FROM order_items AS oi
                JOIN stock_items AS si ON si.stock_item_id = oi.stock_item_id
                JOIN products AS p ON p.product_id = si.product_id
                JOIN manufacturers AS m ON m.manufacturer_id = p.manufacturer_id
                JOIN sizes AS s ON s.size_id = si.size_id
                WHERE oi.order_id = %s
                ORDER BY oi.order_item_id
                """,
                (order_id,),
            ).fetchall()

    def create_order(self, client_id: int, order_date: date, lines: list[DraftLine]) -> int:
        """Сохраняет заказ и списывает остатки в одной транзакции.

        Строки остатков блокируются (FOR UPDATE) в порядке номеров, поэтому
        два одновременных заказа не спишут одни и те же пары и не заблокируют
        друг друга.
        """
        connection = self._database.connection
        with connection.transaction():
            available = dict(connection.execute(
                """
                SELECT stock_item_id, quantity
                FROM stock_items
                WHERE stock_item_id = ANY(%s)
                ORDER BY stock_item_id
                FOR UPDATE
                """,
                ([line.stock_item_id for line in lines],),
            ).fetchall())
            shortages = [(line, available.get(line.stock_item_id, 0)) for line in lines
                         if line.quantity > available.get(line.stock_item_id, 0)]
            if shortages:
                raise InsufficientStockError(shortages)

            (order_id,) = connection.execute(
                "INSERT INTO orders (order_date, client_id) VALUES (%s, %s) RETURNING order_id",
                (order_date, client_id),
            ).fetchone()
            with connection.cursor() as cursor:
                cursor.executemany(
                    "INSERT INTO order_items (order_id, stock_item_id, quantity, unit_price) "
                    "VALUES (%s, %s, %s, %s)",
                    [(order_id, line.stock_item_id, line.quantity, line.unit_price)
                     for line in lines],
                )
                cursor.executemany(
                    "UPDATE stock_items SET quantity = quantity - %s WHERE stock_item_id = %s",
                    [(line.quantity, line.stock_item_id) for line in lines],
                )
        return order_id

    def delete_order(self, order_id: int) -> None:
        """Удаляет заказ, возвращая его пары в остатки; состав удаляется каскадно."""
        connection = self._database.connection
        with connection.transaction():
            connection.execute(self._RETURN_TO_STOCK.format(condition="oi.order_id = %s"),
                               (order_id,))
            connection.execute("DELETE FROM orders WHERE order_id = %s", (order_id,))

    def change_order_date(self, order_id: int, order_date: date) -> None:
        """Изменяет дату заказа."""
        self._database.connection.execute(
            "UPDATE orders SET order_date = %s WHERE order_id = %s", (order_date, order_id)
        )

    def delete_line(self, order_item_id: int) -> None:
        """Удаляет позицию заказа и возвращает её пары в остатки.

        Единственную позицию удалить нельзя: заказ без товаров не имеет
        смысла, в этом случае удаляется заказ целиком.
        """
        connection = self._database.connection
        with connection.transaction():
            (lines_count,) = connection.execute(
                """
                SELECT count(*)
                FROM order_items
                WHERE order_id = (SELECT order_id FROM order_items WHERE order_item_id = %s)
                """,
                (order_item_id,),
            ).fetchone()
            if lines_count <= 1:
                raise OrderError(
                    "Это единственная позиция заказа, поэтому удалить её нельзя.\n"
                    "Чтобы отменить заказ полностью, удалите его в списке заказов."
                )
            connection.execute(self._RETURN_TO_STOCK.format(condition="oi.order_item_id = %s"),
                               (order_item_id,))
            connection.execute("DELETE FROM order_items WHERE order_item_id = %s",
                               (order_item_id,))
