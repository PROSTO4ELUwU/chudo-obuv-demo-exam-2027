"""Главная форма: каталог товаров."""

from datetime import date

import psycopg
from PySide6.QtWidgets import QFrame, QScrollArea, QVBoxLayout, QWidget

from chudo_obuv.models import Product
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.product_card import ProductCard


class CatalogPage(Page):
    """Каталог моделей обуви в виде карточек."""

    title = "Каталог товаров"

    def __init__(self, context: AppContext) -> None:
        super().__init__(context)
        self._products: list[Product] = []

        container = QWidget(objectName="cardsContainer")
        self._cards_layout = QVBoxLayout(container)
        self._cards_layout.setSpacing(10)
        self._cards_layout.addStretch()

        scroll = QScrollArea(widgetResizable=True, frameShape=QFrame.Shape.NoFrame)
        scroll.setWidget(container)

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addWidget(scroll)

    def on_activated(self) -> None:
        """Каталог перечитывается при каждом показе: остатки могли измениться."""
        try:
            self._products = self.context.products.list_products(date.today())
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self._show_cards()

    def _show_cards(self) -> None:
        """Пересоздаёт карточки по загруженным товарам."""
        while self._cards_layout.count() > 1:
            self._cards_layout.takeAt(0).widget().deleteLater()
        for product in self._products:
            card = ProductCard(product, clickable=False)
            self._cards_layout.insertWidget(self._cards_layout.count() - 1, card)
