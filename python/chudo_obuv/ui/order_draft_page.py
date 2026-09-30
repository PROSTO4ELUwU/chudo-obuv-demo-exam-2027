"""Формируемый заказ: позиции, клиент и подтверждение."""

from datetime import date

import psycopg
from PySide6.QtCore import Qt
from PySide6.QtGui import QColor
from PySide6.QtWidgets import (
    QAbstractItemView,
    QComboBox,
    QHBoxLayout,
    QLabel,
    QPushButton,
    QSpinBox,
    QVBoxLayout,
)

from chudo_obuv.formatting import format_date, format_money, format_size, pairs
from chudo_obuv.models import User
from chudo_obuv.order_draft import OrderDraftError
from chudo_obuv.repositories import InsufficientStockError
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.style import LOW_STOCK_COLOR
from chudo_obuv.ui.widgets import ALIGN_CENTER, ALIGN_RIGHT, accent_button, make_table, table_item

COLUMNS = ["Модель", "Размер", "Цена за пару", "Количество", "Сумма", ""]
SUM_COLUMN = 4
CLIENT_PLACEHOLDER = "— выберите клиента —"


class OrderDraftPage(Page):
    """Позиции, выбранные для заказа: изменение количества, удаление, подтверждение.

    Авторизованный пользователь оформляет заказ на себя, а менеджер
    и администратор выбирают клиента из списка.
    """

    title = "Формируемый заказ"

    def __init__(self, context: AppContext) -> None:
        super().__init__(context)
        self._client_combo = QComboBox(sizeAdjustPolicy=QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._client_combo.setToolTip("Клиент, на которого оформляется заказ")
        self._client_name = QLabel(objectName="userName")
        client_row = QHBoxLayout()
        client_row.addWidget(QLabel("Клиент:"))
        client_row.addWidget(self._client_combo)
        client_row.addWidget(self._client_name)
        client_row.addStretch()

        self._table = make_table(COLUMNS)
        self._table.setSelectionMode(QAbstractItemView.SelectionMode.NoSelection)
        self._empty_label = QLabel("В заказе пока нет товаров. Выберите товары в каталоге.",
                                   objectName="hint", alignment=Qt.AlignmentFlag.AlignCenter)
        self._total_label = QLabel(objectName="total")

        self._cancel_button = QPushButton("Отказаться от заказа")
        self._cancel_button.clicked.connect(self._cancel_order)
        self._confirm_button = accent_button("Подтвердить заказ")
        self._confirm_button.clicked.connect(self._confirm_order)
        bottom_row = QHBoxLayout()
        bottom_row.addWidget(self._total_label)
        bottom_row.addStretch()
        bottom_row.addWidget(self._cancel_button)
        bottom_row.addWidget(self._confirm_button)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addLayout(client_row)
        layout.addWidget(self._table, stretch=1)
        layout.addWidget(self._empty_label)
        layout.addLayout(bottom_row)

    def on_activated(self) -> None:
        """Показывает клиента и позиции заказа."""
        user = self.context.user
        self._client_combo.setVisible(user.can_manage_orders)
        self._client_name.setVisible(not user.can_manage_orders)
        if user.can_manage_orders:
            self._load_clients()
        else:
            self._client_name.setText(user.full_name)
        self._fill_table()

    def _load_clients(self) -> None:
        """Список клиентов; выбранный ранее клиент сохраняется."""
        selected = self._client_combo.currentText()
        try:
            clients = self.context.users.list_clients()
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self._client_combo.clear()
        self._client_combo.addItem(CLIENT_PLACEHOLDER, None)
        for client in clients:
            self._client_combo.addItem(client.full_name, client)
        self._client_combo.setCurrentText(selected)

    def _fill_table(self) -> None:
        lines = self.context.draft.lines
        self._table.setRowCount(len(lines))
        for row, line in enumerate(lines):
            self._table.setItem(row, 0, table_item(f"{line.product_name} ({line.manufacturer})"))
            self._table.setItem(row, 1, table_item(format_size(line.size), ALIGN_CENTER))
            self._table.setItem(row, 2, table_item(format_money(line.unit_price), ALIGN_RIGHT))
            self._table.setItem(row, SUM_COLUMN, table_item(format_money(line.total), ALIGN_RIGHT))

            quantity_spin = QSpinBox()
            quantity_spin.setRange(1, max(line.available, line.quantity))
            quantity_spin.setValue(line.quantity)
            quantity_spin.setEnabled(line.available > 0)
            # Значение по умолчанию фиксирует позицию текущей строки цикла:
            # без него все обработчики получили бы позицию из последней строки
            quantity_spin.valueChanged.connect(
                lambda quantity, item_id=line.stock_item_id: self._change_quantity(item_id, quantity))
            self._table.setCellWidget(row, 3, quantity_spin)

            remove_button = QPushButton("Удалить")
            remove_button.clicked.connect(
                lambda _checked, item_id=line.stock_item_id: self._remove_line(item_id))
            self._table.setCellWidget(row, 5, remove_button)

            if line.exceeds_stock:
                self._highlight_row(row, f"Этого размера больше нет в наличии "
                                         f"(осталось {pairs(line.available)}) — удалите позицию")
        is_empty = not lines
        self._table.setVisible(not is_empty)
        self._empty_label.setVisible(is_empty)
        self._cancel_button.setEnabled(not is_empty)
        self._confirm_button.setEnabled(not is_empty)
        self._update_total()

    def _highlight_row(self, row: int, tooltip: str) -> None:
        for column in range(self._table.columnCount()):
            item = self._table.item(row, column)
            if item is not None:
                item.setBackground(QColor(LOW_STOCK_COLOR))
                item.setToolTip(tooltip)

    def _update_total(self) -> None:
        draft = self.context.draft
        self._total_label.setText(f"Итого: {pairs(draft.pairs_count)} на сумму {format_money(draft.total)}")

    def _change_quantity(self, stock_item_id: int, quantity: int) -> None:
        try:
            self.context.draft.set_quantity(stock_item_id, quantity)
        except OrderDraftError as error:
            messages.show_warning(self, str(error))
            self._fill_table()
            return
        for row, line in enumerate(self.context.draft.lines):
            self._table.item(row, SUM_COLUMN).setText(format_money(line.total))
        self._update_total()

    def _remove_line(self, stock_item_id: int) -> None:
        line = next(line for line in self.context.draft.lines if line.stock_item_id == stock_item_id)
        if messages.ask_confirmation(self, f"Удалить из заказа «{line.product_name}», "
                                           f"размер {format_size(line.size)}?"):
            self.context.draft.remove(stock_item_id)
            self._fill_table()

    def _selected_client(self) -> User | None:
        if self.context.user.can_manage_orders:
            return self._client_combo.currentData()
        return self.context.user

    def _cancel_order(self) -> None:
        if messages.ask_confirmation(self, "Отказаться от заказа?\n"
                                           "Все выбранные позиции будут удалены."):
            self.context.draft.clear()
            self.navigator.return_after_order()

    def _confirm_order(self) -> None:
        """Сохраняет заказ с сегодняшней датой и списывает остатки."""
        draft = self.context.draft
        if draft.is_empty:
            messages.show_warning(self, "В заказе нет товаров. Добавьте товары из каталога.")
            return
        client = self._selected_client()
        if client is None:
            messages.show_warning(self, "Выберите клиента, на которого оформляется заказ.")
            self._client_combo.setFocus()
            return

        order_date = date.today()
        total = draft.total
        try:
            order_id = self.context.orders.create_order(client.user_id, order_date, draft.lines)
        except InsufficientStockError as error:
            for line, available in error.shortages:
                draft.update_available(line.stock_item_id, available)
            self._fill_table()
            messages.show_warning(self, f"{error}\n\nКоличество уменьшено до доступного, "
                                        "а позиции, которых не осталось, выделены — удалите их "
                                        "и подтвердите заказ снова.")
            return
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return

        draft.clear()
        messages.show_info(self, f"Заказ №{order_id} от {format_date(order_date)} оформлен.\n"
                                 f"Клиент: {client.full_name}\n"
                                 f"Сумма заказа: {format_money(total)}")
        self.navigator.return_after_order()
