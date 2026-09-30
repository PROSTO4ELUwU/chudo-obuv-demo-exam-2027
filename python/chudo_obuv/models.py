"""Сущности предметной области и правила отображения каталога."""

from dataclasses import dataclass
from datetime import date
from decimal import Decimal
from enum import StrEnum

MANY_THRESHOLD = 5
LOW_STOCK_THRESHOLD = 3


class Role(StrEnum):
    """Роли пользователей; значения совпадают с roles.role_name в базе."""

    ADMIN = "Администратор"
    MANAGER = "Менеджер"
    CLIENT = "Авторизованный пользователь"
    GUEST = "Гость"


@dataclass(frozen=True)
class User:
    """Пользователь, вошедший в систему."""

    user_id: int | None
    last_name: str
    first_name: str
    patronymic: str | None
    role: Role

    @property
    def full_name(self) -> str:
        """Фамилия, имя и отчество."""
        parts = (self.last_name, self.first_name, self.patronymic)
        return " ".join(part for part in parts if part)

    @property
    def can_order(self) -> bool:
        """Фильтровать каталог и оформлять заказы может любой вошедший по логину."""
        return self.role is not Role.GUEST

    @property
    def can_manage_orders(self) -> bool:
        """Список заказов, их состав, добавление и удаление."""
        return self.role in (Role.ADMIN, Role.MANAGER)

    @property
    def can_edit_orders(self) -> bool:
        """Изменение даты заказа и удаление позиций."""
        return self.role is Role.ADMIN


GUEST = User(user_id=None, last_name="Гость", first_name="", patronymic=None, role=Role.GUEST)


@dataclass(frozen=True)
class Product:
    """Модель обуви в каталоге с ценой, рассчитанной на дату загрузки."""

    product_id: int
    name: str
    category: str
    manufacturer: str
    description: str
    composition: str
    base_price: Decimal
    discount: Decimal
    price: Decimal
    total_quantity: int
    image_file: str | None

    @property
    def has_discount(self) -> bool:
        """Действует ли на товар скидка."""
        return self.discount > 0

    @property
    def quantity_text(self) -> str:
        """«Много» — больше пяти пар по всем размерам, иначе «мало»."""
        return "много" if self.total_quantity > MANY_THRESHOLD else "мало"

    @property
    def is_running_out(self) -> bool:
        """Товар подсвечивается в каталоге: осталось три пары или меньше."""
        return self.total_quantity <= LOW_STOCK_THRESHOLD


@dataclass(frozen=True)
class StockPosition:
    """Товарная позиция: размер модели и количество пар в наличии."""

    stock_item_id: int
    size: Decimal
    quantity: int


@dataclass(frozen=True)
class OrderSummary:
    """Строка списка заказов."""

    order_id: int
    order_date: date
    client_name: str
    total: Decimal


@dataclass(frozen=True)
class OrderLine:
    """Позиция сохранённого заказа."""

    order_item_id: int
    product_name: str
    manufacturer: str
    size: Decimal
    quantity: int
    unit_price: Decimal

    @property
    def total(self) -> Decimal:
        """Стоимость позиции."""
        return self.unit_price * self.quantity
