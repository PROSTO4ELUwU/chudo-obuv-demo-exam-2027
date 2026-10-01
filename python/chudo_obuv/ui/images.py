"""Изображения: логотип компании и картинки товаров с заглушкой."""

from functools import cache

from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import QApplication

from chudo_obuv.config import IMAGES_DIR, LOGO_FILE, PLACEHOLDER_FILE


def _fitted(pixmap: QPixmap, size: QSize) -> QPixmap:
    """Изображение, вписанное в размер без искажения пропорций и без размытия.

    При масштабе экрана 150 % в логическом пикселе полтора физических,
    поэтому картинка готовится в физических пикселях: уменьшенную
    до логического размера Qt растянул бы, и она выглядела бы размытой.
    """
    ratio = QApplication.instance().devicePixelRatio()
    result = pixmap.scaled(size * ratio, Qt.AspectRatioMode.KeepAspectRatio,
                           Qt.TransformationMode.SmoothTransformation)
    result.setDevicePixelRatio(ratio)
    return result


def logo_pixmap(size: QSize) -> QPixmap:
    """Логотип компании, вписанный в размер."""
    return _fitted(QPixmap(str(LOGO_FILE)), size)


@cache
def _load_pixmap(image_file: str | None) -> QPixmap:
    """Файл читается с диска один раз, затем берётся из кэша."""
    if image_file:
        pixmap = QPixmap(str(IMAGES_DIR / image_file))
        if not pixmap.isNull():
            return pixmap
    return QPixmap(str(PLACEHOLDER_FILE))


def product_pixmap(image_file: str | None, size: QSize) -> QPixmap:
    """Изображение товара, вписанное в размер.

    Если изображения нет в базе или файл не найден, возвращается
    заглушка picture.png из ресурсов.
    """
    return _fitted(_load_pixmap(image_file), size)
