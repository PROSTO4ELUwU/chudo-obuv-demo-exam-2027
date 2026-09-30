"""Общие элементы интерфейса."""

from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QAbstractItemView,
    QHeaderView,
    QPushButton,
    QTableWidget,
    QTableWidgetItem,
    QWidget,
)

ALIGN_LEFT = Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignVCenter
ALIGN_CENTER = Qt.AlignmentFlag.AlignCenter
ALIGN_RIGHT = Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter


def hide_unless(allowed: bool, *widgets: QWidget) -> None:
    """Скрывает элементы, недоступные пользователю.

    Показывать их явно (setVisible(True)) при создании нельзя: виджет
    без родителя открылся бы отдельным окном, а внутри страницы
    элементы и так видны вместе с ней.
    """
    if not allowed:
        for widget in widgets:
            widget.hide()


def accent_button(text: str) -> QPushButton:
    """Кнопка целевого действия, выделенная акцентным цветом #70B2AF."""
    button = QPushButton(text)
    button.setProperty("accent", True)
    return button


def make_table(headers: list[str], stretch_column: int = 0) -> QTableWidget:
    """Таблица только для просмотра: выделяется строка целиком, один столбец растягивается."""
    table = QTableWidget(0, len(headers))
    table.setHorizontalHeaderLabels(headers)
    table.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
    table.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
    table.setSelectionMode(QAbstractItemView.SelectionMode.SingleSelection)
    table.verticalHeader().hide()
    table.verticalHeader().setDefaultSectionSize(36)
    header = table.horizontalHeader()
    header.setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
    header.setSectionResizeMode(stretch_column, QHeaderView.ResizeMode.Stretch)
    return table


def table_item(text: str, alignment: Qt.AlignmentFlag = ALIGN_LEFT) -> QTableWidgetItem:
    """Ячейка таблицы с выравниванием текста."""
    item = QTableWidgetItem(text)
    item.setTextAlignment(alignment)
    return item
