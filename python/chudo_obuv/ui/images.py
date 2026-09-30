"""Изображения товаров с картинкой-заглушкой."""

from functools import cache

from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QPixmap

from chudo_obuv.config import IMAGES_DIR, PLACEHOLDER_FILE


@cache
def _load_pixmap(image_file: str | None) -> QPixmap:
    """Файл читается с диска один раз, затем берётся из кэша."""
    if image_file:
        pixmap = QPixmap(str(IMAGES_DIR / image_file))
        if not pixmap.isNull():
            return pixmap
    return QPixmap(str(PLACEHOLDER_FILE))


def product_pixmap(image_file: str | None, size: QSize) -> QPixmap:
    """Изображение товара, вписанное в размер без искажения пропорций.

    Если изображения нет в базе или файл не найден, возвращается
    заглушка picture.png из ресурсов.
    """
    return _load_pixmap(image_file).scaled(
        size, Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation
    )
