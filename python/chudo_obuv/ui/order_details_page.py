"""Состав заказа и его редактирование администратором (задание 4)."""

import psycopg
from PySide6.QtCore import QDate
from PySide6.QtWidgets import QDateEdit, QFormLayout, QHBoxLayout, QLabel, QPushButton, QVBoxLayout

from chudo_obuv.formatting import format_date, format_money, format_size, pairs
from chudo_obuv.models import OrderLine, OrderSummary
from chudo_obuv.repositories import LAST_LINE_MESSAGE, OrderError
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.widgets import (
    ALIGN_CENTER,
    ALIGN_RIGHT,
    accent_button,
    hide_unless,
    make_table,
    table_item,
)

COLUMNS = ["Товарная позиция (модель — производство)", "Размер", "Количество",
           "Цена за единицу", "Сумма"]


class OrderDetailsPage(Page):
    """Товарные позиции заказа с количеством, ценой за единицу и итоговой суммой.

    Администратор может изменить дату заказа и удалить позиции,
    менеджер видит состав только для чтения.
    """

    title = "Состав заказа"

    def __init__(self, context: AppContext, order_id: int) -> None:
        super().__init__(context)
        self._order_id = order_id
        self._order: OrderSummary | None = None
        self._lines: list[OrderLine] = []
        can_edit = context.user.can_edit_orders

        self._number_label = QLabel(objectName="productTitle")
        self._client_label = QLabel()
        self._date_label = QLabel()
        self._date_edit = QDateEdit(calendarPopup=True, displayFormat="dd.MM.yyyy")
        self._date_edit.setMinimumDate(QDate(2000, 1, 1))
        self._date_edit.setMaximumDate(QDate.currentDate())
        self._date_edit.setToolTip("Дата заказа не может быть позже сегодняшней")
        self._date_edit.dateChanged.connect(self._update_save_button)
        self._save_date_button = accent_button("Сохранить дату")
        self._save_date_button.clicked.connect(self._save_date)
        hide_unless(not can_edit, self._date_label)
        hide_unless(can_edit, self._date_edit, self._save_date_button)
        date_row = QHBoxLayout()
        date_row.addWidget(self._date_label)
        date_row.addWidget(self._date_edit)
        date_row.addWidget(self._save_date_button)
        date_row.addStretch()

        info = QFormLayout()
        info.addRow(self._number_label)
        info.addRow("Клиент:", self._client_label)
        info.addRow("Дата заказа:", date_row)

        self._table = make_table(COLUMNS)
        self._total_label = QLabel(objectName="total")
        delete_line_button = QPushButton("Удалить позицию")
        delete_line_button.setToolTip("Удалить выбранную позицию; пары вернутся в остатки")
        delete_line_button.clicked.connect(self._delete_line)
        hide_unless(can_edit, delete_line_button)
        bottom_row = QHBoxLayout()
        bottom_row.addWidget(self._total_label)
        bottom_row.addStretch()
        bottom_row.addWidget(delete_line_button)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addLayout(info)
        layout.addWidget(self._table, stretch=1)
        layout.addLayout(bottom_row)

    def on_activated(self) -> None:
        """Загружает заказ и его состав из базы."""
        try:
            order = self.context.orders.get_order(self._order_id)
            lines = self.context.orders.list_lines(self._order_id) if order else []
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        if order is None:
            messages.show_warning(self, f"Заказ №{self._order_id} не найден — "
                                        "возможно, его удалили.")
            self.navigator.go_back()
            return
        self._order = order
        self._lines = lines
        self._number_label.setText(f"Заказ №{order.order_id}")
        self._client_label.setText(order.client_name)
        self._date_label.setText(format_date(order.order_date))
        self._date_edit.blockSignals(True)
        self._date_edit.setDate(QDate(order.order_date.year, order.order_date.month,
                                      order.order_date.day))
        self._date_edit.blockSignals(False)
        self._update_save_button()
        self._table.setRowCount(len(lines))
        for row, line in enumerate(lines):
            self._table.setItem(row, 0, table_item(f"{line.product_name} — {line.manufacturer}"))
            self._table.setItem(row, 1, table_item(format_size(line.size), ALIGN_CENTER))
            self._table.setItem(row, 2, table_item(pairs(line.quantity), ALIGN_CENTER))
            self._table.setItem(row, 3, table_item(format_money(line.unit_price), ALIGN_RIGHT))
            self._table.setItem(row, 4, table_item(format_money(line.total), ALIGN_RIGHT))
        self._total_label.setText(f"Итоговая сумма по заказу: {format_money(order.total)}")

    def _update_save_button(self) -> None:
        """«Сохранить дату» доступна, только если дата действительно изменена."""
        changed = (self._order is not None
                   and self._date_edit.date().toPython() != self._order.order_date)
        self._save_date_button.setEnabled(changed)

    def _save_date(self) -> None:
        new_date = self._date_edit.date().toPython()
        try:
            self.context.orders.change_order_date(self._order_id, new_date)
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        messages.show_info(self, f"Дата заказа №{self._order_id} изменена "
                                 f"на {format_date(new_date)}.")
        self.on_activated()

    def _delete_line(self) -> None:
        rows = self._table.selectionModel().selectedRows()
        if not rows:
            messages.show_warning(self, "Выберите в составе заказа позицию, которую нужно удалить.")
            return
        if len(self._lines) == 1:
            messages.show_warning(self, LAST_LINE_MESSAGE)
            return
        line = self._lines[rows[0].row()]
        if not messages.ask_confirmation(
            self,
            f"Удалить из заказа №{self._order_id} позицию «{line.product_name}», "
            f"размер {format_size(line.size)}, {pairs(line.quantity)}?\n"
            "Пары вернутся в остатки.",
        ):
            return
        try:
            self.context.orders.delete_line(line.order_item_id)
        except OrderError as error:
            messages.show_warning(self, str(error))
            return
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self.on_activated()
