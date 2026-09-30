"""Формируемый заказ: позиции, выбранные пользователем до подтверждения."""

from dataclasses import dataclass
from decimal import Decimal

from chudo_obuv.formatting import format_size, pairs
from chudo_obuv.models import Product, StockPosition


class OrderDraftError(Exception):
    """Недопустимое изменение формируемого заказа; текст показывается пользователю."""


@dataclass
class DraftLine:
    """Позиция формируемого заказа."""

    stock_item_id: int
    product_name: str
    manufacturer: str
    size: Decimal
    unit_price: Decimal
    quantity: int
    available: int

    @property
    def total(self) -> Decimal:
        """Стоимость позиции."""
        return self.unit_price * self.quantity

    @property
    def exceeds_stock(self) -> bool:
        """В позиции больше пар, чем осталось (после проверки при подтверждении)."""
        return self.quantity > self.available


class OrderDraft:
    """Позиции, которые пользователь добавил в заказ, но ещё не подтвердил.

    Остатки в базе не меняются, пока заказ не подтверждён: проверка
    и списание выполняются одной транзакцией при сохранении заказа.
    """

    def __init__(self) -> None:
        self._lines: dict[int, DraftLine] = {}

    @property
    def lines(self) -> list[DraftLine]:
        """Позиции в порядке добавления."""
        return list(self._lines.values())

    @property
    def is_empty(self) -> bool:
        """В заказе нет ни одной позиции."""
        return not self._lines

    @property
    def total(self) -> Decimal:
        """Итоговая сумма заказа."""
        return sum((line.total for line in self._lines.values()), Decimal(0))

    @property
    def pairs_count(self) -> int:
        """Общее количество пар в заказе."""
        return sum(line.quantity for line in self._lines.values())

    def reserved(self, stock_item_id: int) -> int:
        """Сколько пар этой товарной позиции уже в заказе."""
        line = self._lines.get(stock_item_id)
        return line.quantity if line else 0

    def add(self, product: Product, position: StockPosition, quantity: int) -> None:
        """Добавляет пары в заказ; повторный выбор того же размера увеличивает количество."""
        if quantity < 1:
            raise OrderDraftError("Количество пар должно быть не меньше одной.")
        reserved = self.reserved(position.stock_item_id)
        if reserved + quantity > position.quantity:
            raise OrderDraftError(
                f"Размер {format_size(position.size)}: в наличии {pairs(position.quantity)}, "
                f"в заказе уже {pairs(reserved)}, "
                f"можно добавить: {pairs(position.quantity - reserved)}."
            )
        line = self._lines.get(position.stock_item_id)
        if line is None:
            self._lines[position.stock_item_id] = DraftLine(
                stock_item_id=position.stock_item_id,
                product_name=product.name,
                manufacturer=product.manufacturer,
                size=position.size,
                unit_price=product.price,
                quantity=quantity,
                available=position.quantity,
            )
        else:
            line.quantity = reserved + quantity
            line.available = position.quantity

    def set_quantity(self, stock_item_id: int, quantity: int) -> None:
        """Изменяет количество пар в позиции в пределах остатка."""
        line = self._lines[stock_item_id]
        if not 1 <= quantity <= line.available:
            raise OrderDraftError(
                f"Размер {format_size(line.size)}: можно заказать от 1 до "
                f"{pairs(line.available)}. Чтобы убрать позицию, удалите её из заказа."
            )
        line.quantity = quantity

    def update_available(self, stock_item_id: int, available: int) -> None:
        """Запоминает остаток из базы и уменьшает количество в позиции до него.

        Позиция, которой не осталось совсем, не удаляется молча:
        пользователь увидит её выделенной и удалит сам.
        """
        line = self._lines[stock_item_id]
        line.available = available
        if available > 0:
            line.quantity = min(line.quantity, available)

    def remove(self, stock_item_id: int) -> None:
        """Удаляет позицию из заказа."""
        del self._lines[stock_item_id]

    def clear(self) -> None:
        """Отказ от заказа или очистка после сохранения."""
        self._lines.clear()
