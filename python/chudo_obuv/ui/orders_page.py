"""Список заказов для менеджера и администратора (задание 4)."""

import psycopg
from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout

from chudo_obuv.formatting import format_date, format_money
from chudo_obuv.models import OrderSummary
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.widgets import ALIGN_CENTER, ALIGN_RIGHT, accent_button, make_table, table_item

COLUMNS = ["№ заказа", "Дата заказа", "ФИО клиента", "Сумма"]


class OrdersPage(Page):
    """Заказы с датой, ФИО клиента и суммой: добавление, удаление, просмотр состава."""

    title = "Заказы"

    def __init__(self, context: AppContext) -> None:
        super().__init__(context)
        self._orders: list[OrderSummary] = []

        add_button = accent_button("Добавить заказ")
        add_button.setToolTip("Выбрать товары из каталога для нового заказа")
        add_button.clicked.connect(self.navigator.show_catalog_for_order)
        details_button = QPushButton("Состав заказа")
        details_button.clicked.connect(self._open_details)
        delete_button = QPushButton("Удалить заказ")
        delete_button.clicked.connect(self._delete_order)
        buttons = QHBoxLayout()
        buttons.addWidget(add_button)
        buttons.addWidget(details_button)
        buttons.addWidget(delete_button)
        buttons.addStretch()
        buttons.addWidget(QLabel("Двойной щелчок по заказу открывает его состав",
                                 objectName="hint"))

        self._table = make_table(COLUMNS, stretch_column=2)
        self._table.doubleClicked.connect(self._open_details)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addLayout(buttons)
        layout.addWidget(self._table, stretch=1)

    def on_activated(self) -> None:
        """Список обновляется после добавления, изменения и удаления заказов."""
        selected = self._selected_order()
        try:
            self._orders = self.context.orders.list_orders()
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self._table.setRowCount(len(self._orders))
        for row, order in enumerate(self._orders):
            self._table.setItem(row, 0, table_item(str(order.order_id), ALIGN_CENTER))
            self._table.setItem(row, 1, table_item(format_date(order.order_date), ALIGN_CENTER))
            self._table.setItem(row, 2, table_item(order.client_name))
            self._table.setItem(row, 3, table_item(format_money(order.total), ALIGN_RIGHT))
            if selected is not None and order.order_id == selected.order_id:
                self._table.selectRow(row)

    def _selected_order(self) -> OrderSummary | None:
        rows = self._table.selectionModel().selectedRows()
        return self._orders[rows[0].row()] if rows else None

    def _open_details(self) -> None:
        order = self._selected_order()
        if order is None:
            messages.show_warning(self, "Выберите заказ в списке.")
            return
        self.navigator.show_order_details(order.order_id)

    def _delete_order(self) -> None:
        order = self._selected_order()
        if order is None:
            messages.show_warning(self, "Выберите заказ, который нужно удалить.")
            return
        if not messages.ask_confirmation(
            self,
            f"Удалить заказ №{order.order_id} от {format_date(order.order_date)}, "
            f"клиент {order.client_name}?\n"
            "Товары заказа вернутся в остатки. Действие нельзя отменить.",
        ):
            return
        try:
            self.context.orders.delete_order(order.order_id)
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self.on_activated()
