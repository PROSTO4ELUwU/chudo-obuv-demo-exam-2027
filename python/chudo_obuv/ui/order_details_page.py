"""Состав заказа (задание 4)."""

import psycopg
from PySide6.QtWidgets import QFormLayout, QLabel, QVBoxLayout

from chudo_obuv.formatting import format_date, format_money, format_size, pairs
from chudo_obuv.models import OrderLine, OrderSummary
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.widgets import ALIGN_CENTER, ALIGN_RIGHT, make_table, table_item

COLUMNS = ["Товарная позиция (модель — производство)", "Размер", "Количество",
           "Цена за единицу", "Сумма"]


class OrderDetailsPage(Page):
    """Товарные позиции заказа с количеством, ценой за единицу и итоговой суммой."""

    title = "Состав заказа"

    def __init__(self, context: AppContext, order_id: int) -> None:
        super().__init__(context)
        self._order_id = order_id
        self._order: OrderSummary | None = None
        self._lines: list[OrderLine] = []

        self._number_label = QLabel(objectName="productTitle")
        self._client_label = QLabel()
        self._date_label = QLabel()
        info = QFormLayout()
        info.addRow(self._number_label)
        info.addRow("Клиент:", self._client_label)
        info.addRow("Дата заказа:", self._date_label)

        self._table = make_table(COLUMNS)
        self._total_label = QLabel(objectName="total")

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addLayout(info)
        layout.addWidget(self._table, stretch=1)
        layout.addWidget(self._total_label)

    def on_activated(self) -> None:
        """Загружает заказ и его состав из базы."""
        try:
            order = self.context.orders.get_order(self._order_id)
            lines = self.context.orders.list_lines(self._order_id) if order else []
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        if order is None:
            messages.show_warning(self, f"Заказ №{self._order_id} не найден — возможно, его удалили.")
            self.navigator.go_back()
            return
        self._order = order
        self._lines = lines
        self._number_label.setText(f"Заказ №{order.order_id}")
        self._client_label.setText(order.client_name)
        self._date_label.setText(format_date(order.order_date))
        self._table.setRowCount(len(lines))
        for row, line in enumerate(lines):
            self._table.setItem(row, 0, table_item(f"{line.product_name} — {line.manufacturer}"))
            self._table.setItem(row, 1, table_item(format_size(line.size), ALIGN_CENTER))
            self._table.setItem(row, 2, table_item(pairs(line.quantity), ALIGN_CENTER))
            self._table.setItem(row, 3, table_item(format_money(line.unit_price), ALIGN_RIGHT))
            self._table.setItem(row, 4, table_item(format_money(line.total), ALIGN_RIGHT))
        self._total_label.setText(f"Итоговая сумма по заказу: {format_money(order.total)}")
