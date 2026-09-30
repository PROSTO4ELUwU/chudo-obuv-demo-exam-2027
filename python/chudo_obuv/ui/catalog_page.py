"""Главная форма: каталог товаров с поиском, фильтрацией и сортировкой."""

from datetime import date

import psycopg
from PySide6.QtCore import Qt
from PySide6.QtWidgets import (
    QComboBox,
    QFrame,
    QHBoxLayout,
    QLabel,
    QLineEdit,
    QScrollArea,
    QVBoxLayout,
    QWidget,
)

from chudo_obuv.catalog_filter import ALL_CATEGORIES, SortOrder, filter_products
from chudo_obuv.models import Product
from chudo_obuv.ui import messages
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.product_card import ProductCard


class CatalogPage(Page):
    """Каталог моделей обуви в виде карточек.

    Авторизованным пользователям доступны поиск, фильтр по категории
    и сортировка по цене; они применяются сразу при изменении условий.
    Гость видит каталог без этих инструментов.
    """

    title = "Каталог товаров"

    def __init__(self, context: AppContext) -> None:
        super().__init__(context)
        self._products: list[Product] = []
        self._cards: dict[int, ProductCard] = {}

        self._search_edit = QLineEdit(placeholderText="Наименование или описание товара",
                                      clearButtonEnabled=True)
        self._search_edit.textChanged.connect(self._apply_filters)
        self._category_combo = QComboBox(sizeAdjustPolicy=QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._category_combo.currentTextChanged.connect(self._apply_filters)
        self._sort_combo = QComboBox(sizeAdjustPolicy=QComboBox.SizeAdjustPolicy.AdjustToContents)
        self._sort_combo.addItems([order.value for order in SortOrder])
        self._sort_combo.currentTextChanged.connect(self._apply_filters)
        self._found_label = QLabel(objectName="hint")

        filter_panel = QFrame(objectName="panel")
        filters = QHBoxLayout(filter_panel)
        filters.addWidget(QLabel("Поиск:"))
        filters.addWidget(self._search_edit, stretch=1)
        filters.addWidget(QLabel("Категория:"))
        filters.addWidget(self._category_combo)
        filters.addWidget(QLabel("Сортировка:"))
        filters.addWidget(self._sort_combo)
        filters.addWidget(self._found_label)
        filter_panel.setVisible(context.user.can_order)

        container = QWidget(objectName="cardsContainer")
        self._cards_layout = QVBoxLayout(container)
        self._cards_layout.setSpacing(10)
        self._cards_layout.addStretch()
        scroll = QScrollArea(widgetResizable=True, frameShape=QFrame.Shape.NoFrame)
        scroll.setWidget(container)

        self._empty_label = QLabel("Товары не найдены. Измените строку поиска "
                                   "или выберите другую категорию.",
                                   objectName="hint", alignment=Qt.AlignmentFlag.AlignCenter)
        self._empty_label.hide()

        layout = QVBoxLayout(self)
        layout.setContentsMargins(16, 12, 16, 12)
        layout.addWidget(filter_panel)
        layout.addWidget(self._empty_label)
        layout.addWidget(scroll, stretch=1)

    def on_activated(self) -> None:
        """Каталог перечитывается при каждом показе: остатки могли измениться."""
        try:
            self._products = self.context.products.list_products(date.today())
            categories = self.context.products.list_categories()
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        self._fill_categories(categories)
        self._create_cards()
        self._apply_filters()

    def _fill_categories(self, categories: list[str]) -> None:
        """Первый пункт — «Все категории»; выбранная категория сохраняется при обновлении."""
        selected = self._category_combo.currentText()
        self._category_combo.blockSignals(True)
        self._category_combo.clear()
        self._category_combo.addItems([ALL_CATEGORIES, *categories])
        self._category_combo.setCurrentText(selected)
        self._category_combo.blockSignals(False)

    def _create_cards(self) -> None:
        """Карточки открывают форму товара, если пользователю доступен заказ."""
        for card in self._cards.values():
            card.deleteLater()
        clickable = self.context.user.can_order
        self._cards = {}
        for product in self._products:
            card = ProductCard(product, clickable)
            card.clicked.connect(lambda product_id=product.product_id:
                                 self.navigator.show_product(product_id))
            self._cards[product.product_id] = card

    def _apply_filters(self) -> None:
        """Показывает карточки, прошедшие поиск и фильтр, в выбранном порядке.

        Карточки не пересоздаются, а только переставляются в макете,
        поэтому поиск работает без задержек на каждое нажатие клавиши.
        """
        visible = filter_products(self._products, self._search_edit.text(),
                                  self._category_combo.currentText(),
                                  SortOrder(self._sort_combo.currentText()))
        while self._cards_layout.count() > 1:
            self._cards_layout.takeAt(0)
        for card in self._cards.values():
            card.hide()
        for index, product in enumerate(visible):
            card = self._cards[product.product_id]
            self._cards_layout.insertWidget(index, card)
            card.show()
        self._empty_label.setVisible(not visible)
        self._found_label.setText(f"Найдено: {len(visible)} из {len(self._products)}")
