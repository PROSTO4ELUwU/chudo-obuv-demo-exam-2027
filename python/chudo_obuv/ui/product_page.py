"""Форма просмотра товара с выбором размера и количества."""

from datetime import date

import psycopg
from PySide6.QtCore import QSize, Qt
from PySide6.QtWidgets import (
    QComboBox,
    QFormLayout,
    QFrame,
    QHBoxLayout,
    QLabel,
    QSpinBox,
    QVBoxLayout,
)

from chudo_obuv.formatting import format_money, format_size, pairs
from chudo_obuv.models import Product, StockPosition
from chudo_obuv.order_draft import OrderDraftError
from chudo_obuv.ui import messages
from chudo_obuv.ui.images import product_pixmap
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.widgets import accent_button

IMAGE_SIZE = QSize(320, 240)


class ProductPage(Page):
    """Данные выбранной модели и добавление товарной позиции в заказ.

    При каждом показе модель и её размеры читаются из базы заново,
    а доступное количество уменьшается на пары, уже добавленные в заказ.
    """

    title = "Просмотр товара"

    def __init__(self, context: AppContext, product_id: int) -> None:
        super().__init__(context)
        self._product_id = product_id
        self._product: Product | None = None

        self._image = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        self._image.setFixedSize(IMAGE_SIZE)
        self._name = QLabel(objectName="productTitle", wordWrap=True)
        self._manufacturer = QLabel()
        self._category = QLabel()
        self._price_caption = QLabel()
        self._old_price = QLabel(objectName="oldPrice")
        self._price = QLabel(objectName="price")
        self._composition = QLabel(wordWrap=True)
        self._description = QLabel(wordWrap=True)
        self._size_range = QLabel(wordWrap=True)
        self._available_sizes = QLabel(wordWrap=True)

        prices = QHBoxLayout()
        prices.setSpacing(12)
        prices.addWidget(self._price)
        prices.addWidget(self._old_price)
        prices.addStretch()

        details = QFormLayout()
        details.setVerticalSpacing(8)
        details.addRow(self._name)
        details.addRow("Производство:", self._manufacturer)
        details.addRow("Категория:", self._category)
        details.addRow(self._price_caption, prices)
        details.addRow("Состав:", self._composition)
        details.addRow("Описание:", self._description)
        details.addRow("Размерный ряд:", self._size_range)
        details.addRow("Доступные размеры:", self._available_sizes)

        top = QHBoxLayout()
        top.setSpacing(24)
        top.addWidget(self._image, alignment=Qt.AlignmentFlag.AlignTop)
        top.addLayout(details, stretch=1)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 16, 16, 16)
        layout.addLayout(top)
        layout.addStretch()
        layout.addWidget(self._build_order_panel())

    def _build_order_panel(self) -> QFrame:
        """Выбор размера и количества пар для добавления в заказ."""
        self._size_combo = QComboBox(sizeAdjustPolicy=QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._size_combo.currentIndexChanged.connect(self._update_quantity_limit)
        self._quantity_spin = QSpinBox()
        self._quantity_spin.setToolTip("Количество ограничено числом пар в наличии")
        self._stock_hint = QLabel(objectName="hint")
        self._add_button = accent_button("Добавить в заказ")
        self._add_button.clicked.connect(self._add_to_order)

        panel = QFrame(objectName="panel")
        layout = QHBoxLayout(panel)
        layout.addWidget(QLabel("Размер:"))
        layout.addWidget(self._size_combo)
        layout.addWidget(QLabel("Количество пар:"))
        layout.addWidget(self._quantity_spin)
        layout.addWidget(self._stock_hint)
        layout.addStretch()
        layout.addWidget(self._add_button)
        return panel

    def on_activated(self) -> None:
        """Загружает модель и её размеры из базы."""
        try:
            product = self.context.products.get_product(self._product_id, date.today())
            positions = self.context.products.list_positions(self._product_id) if product else []
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        if product is None:
            messages.show_warning(self, "Этого товара больше нет в каталоге.")
            self.navigator.go_back()
            return
        self._product = product
        self._show_product(product)
        self._show_positions(positions)

    def _show_product(self, product: Product) -> None:
        self._image.setPixmap(product_pixmap(product.image_file, IMAGE_SIZE))
        self._name.setText(product.name)
        self._manufacturer.setText(product.manufacturer)
        self._category.setText(product.category)
        # Цена всегда с учётом скидки; если скидки нет, подпись не обещает её
        self._price_caption.setText("Цена со скидкой:" if product.has_discount else "Цена:")
        self._price.setText(format_money(product.price))
        self._old_price.setText(format_money(product.base_price) if product.has_discount else "")
        self._composition.setText(product.composition)
        self._description.setText(product.description)

    def _free_quantity(self, position: StockPosition) -> int:
        """Пары, которые ещё можно добавить: остаток минус уже выбранные в заказ."""
        return position.quantity - self.context.draft.reserved(position.stock_item_id)

    def _show_positions(self, positions: list[StockPosition]) -> None:
        """Размерный ряд модели и размеры, доступные для заказа."""
        available = [position for position in positions if self._free_quantity(position) > 0]
        self._size_range.setText(", ".join(format_size(position.size) for position in positions))
        self._available_sizes.setText(
            "; ".join(f"{format_size(position.size)} — {pairs(self._free_quantity(position))}"
                      for position in available)
            or "нет: все пары этой модели закончились или уже добавлены в заказ"
        )
        self._size_combo.clear()
        for position in available:
            self._size_combo.addItem(format_size(position.size), position)
        for widget in (self._size_combo, self._quantity_spin, self._add_button):
            widget.setEnabled(bool(available))
        self._update_quantity_limit()

    def _update_quantity_limit(self) -> None:
        """Поле количества не позволяет выбрать больше пар, чем доступно."""
        position = self._size_combo.currentData()
        if position is None:
            self._quantity_spin.setRange(0, 0)
            self._stock_hint.clear()
            return
        free = self._free_quantity(position)
        self._quantity_spin.setRange(1, free)
        self._stock_hint.setText(f"в наличии {pairs(free)}")

    def _add_to_order(self) -> None:
        position = self._size_combo.currentData()
        if position is None or self._product is None:
            messages.show_warning(self, "Выберите размер из списка доступных.")
            return
        quantity = self._quantity_spin.value()
        try:
            self.context.draft.add(self._product, position, quantity)
        except OrderDraftError as error:
            messages.show_warning(self, str(error))
            return
        added = f"{self._product.name}, размер {format_size(position.size)}, {pairs(quantity)}"
        if messages.ask_choice(self, f"В заказ добавлено: {added}.\n\n"
                                     "Оформить заказ сейчас или продолжить выбор товаров?",
                               "Оформить заказ", "Продолжить выбор"):
            self.navigator.show_order_draft()
        else:
            self.navigator.go_back()
