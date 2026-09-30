"""Главное окно: шапка с логотипом и стек страниц."""

from PySide6.QtCore import Qt
from PySide6.QtGui import QPixmap
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from chudo_obuv import APP_NAME
from chudo_obuv.config import LOGO_FILE
from chudo_obuv.ui.catalog_page import CatalogPage
from chudo_obuv.ui.pages import AppContext, Page

LOGO_SIZE = 56


class MainWindow(QMainWindow):
    """Главное окно приложения.

    Страницы открываются поверх друг друга в QStackedWidget, поэтому
    возврат на предыдущую страницу всегда возможен.
    """

    def __init__(self, context: AppContext) -> None:
        super().__init__()
        self._context = context
        context.navigator = self
        self._stack = QStackedWidget()

        central = QWidget()
        layout = QVBoxLayout(central)
        layout.setContentsMargins(0, 0, 0, 0)
        layout.setSpacing(0)
        layout.addWidget(self._build_header())
        layout.addWidget(self._stack, stretch=1)
        self.setCentralWidget(central)
        self.resize(1180, 780)
        self.setMinimumSize(960, 640)
        self.show_catalog()

    def _build_header(self) -> QFrame:
        """Шапка: логотип и название компании, заголовок текущей страницы."""
        logo = QLabel()
        logo.setPixmap(QPixmap(str(LOGO_FILE)).scaled(
            LOGO_SIZE, LOGO_SIZE,
            Qt.AspectRatioMode.KeepAspectRatio, Qt.TransformationMode.SmoothTransformation,
        ))
        self._page_title = QLabel(objectName="pageTitle")

        titles = QVBoxLayout()
        titles.setSpacing(0)
        titles.addWidget(QLabel(APP_NAME, objectName="appTitle"))
        titles.addWidget(self._page_title)

        header = QFrame(objectName="header")
        layout = QHBoxLayout(header)
        layout.setContentsMargins(16, 8, 16, 8)
        layout.addWidget(logo)
        layout.addLayout(titles)
        layout.addStretch()
        return header

    def show_catalog(self) -> None:
        """Открывает каталог товаров."""
        self._open_page(CatalogPage(self._context))

    def go_back(self) -> None:
        """Возвращает на предыдущую страницу."""
        if self._stack.count() > 1:
            page = self._stack.currentWidget()
            self._stack.removeWidget(page)
            page.deleteLater()
            self._activate(self._stack.currentWidget())

    def _open_page(self, page: Page) -> None:
        self._stack.addWidget(page)
        self._stack.setCurrentWidget(page)
        self._activate(page)

    def _activate(self, page: Page) -> None:
        """Заголовок окна соответствует назначению показанной страницы."""
        self.setWindowTitle(f"{APP_NAME} — {page.title}")
        self._page_title.setText(page.title)
        page.on_activated()
