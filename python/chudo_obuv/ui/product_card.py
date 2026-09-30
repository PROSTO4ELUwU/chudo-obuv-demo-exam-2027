"""Карточка товара в каталоге по макету из задания."""

from PySide6.QtCore import QSize, Qt, Signal
from PySide6.QtGui import QMouseEvent
from PySide6.QtWidgets import QFrame, QHBoxLayout, QLabel, QVBoxLayout, QWidget

from chudo_obuv.formatting import format_money
from chudo_obuv.models import Product
from chudo_obuv.ui.images import product_pixmap

IMAGE_SIZE = QSize(160, 120)


class ProductCard(QFrame):
    """Изображение, «Производство | Наименование», категория, количество, состав и цена.

    Товары, которых осталось три пары и меньше, подсвечиваются
    цветом #FF8080 (свойство lowStock в таблице стилей).
    """

    clicked = Signal()

    def __init__(self, product: Product, clickable: bool, parent: QWidget | None = None) -> None:
        super().__init__(parent)
        self.setObjectName("productCard")
        self.setProperty("lowStock", product.is_running_out)
        self._clickable = clickable
        if clickable:
            self.setCursor(Qt.CursorShape.PointingHandCursor)
            self.setToolTip("Нажмите, чтобы открыть товар и выбрать размер")

        image = QLabel()
        image.setFixedSize(IMAGE_SIZE)
        image.setAlignment(Qt.AlignmentFlag.AlignCenter)
        image.setPixmap(product_pixmap(product.image_file, IMAGE_SIZE))

        info = QVBoxLayout()
        info.addWidget(QLabel(f"{product.manufacturer} | {product.name}",
                              objectName="productTitle", wordWrap=True))
        info.addWidget(QLabel(f"Категория: {product.category}"))
        info.addWidget(QLabel(f"Количество: {product.quantity_text}"))
        info.addWidget(QLabel(f"Состав: {product.composition}",
                              objectName="composition", wordWrap=True))
        info.addStretch()

        prices = QVBoxLayout()
        if product.has_discount:
            prices.addWidget(QLabel(format_money(product.base_price), objectName="oldPrice"),
                             alignment=Qt.AlignmentFlag.AlignRight)
        prices.addWidget(QLabel(format_money(product.price), objectName="price"),
                         alignment=Qt.AlignmentFlag.AlignRight)
        if product.has_discount:
            prices.addWidget(QLabel(f"скидка {product.discount} %", objectName="hint"),
                             alignment=Qt.AlignmentFlag.AlignRight)
        prices.addStretch()

        layout = QHBoxLayout(self)
        layout.setContentsMargins(12, 10, 16, 10)
        layout.setSpacing(16)
        layout.addWidget(image)
        layout.addLayout(info, stretch=1)
        layout.addLayout(prices)

    def mousePressEvent(self, event: QMouseEvent) -> None:
        """Нажатие на карточку открывает товар, если пользователю доступен заказ."""
        if self._clickable and event.button() == Qt.MouseButton.LeftButton:
            self.clicked.emit()
        super().mousePressEvent(event)
