"""Главное окно: шапка с логотипом и ФИО пользователя, стек страниц."""

from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QCloseEvent
from PySide6.QtWidgets import (
    QFrame,
    QHBoxLayout,
    QLabel,
    QMainWindow,
    QPushButton,
    QStackedWidget,
    QVBoxLayout,
    QWidget,
)

from chudo_obuv import APP_NAME
from chudo_obuv.models import GUEST, Role, User
from chudo_obuv.ui import messages
from chudo_obuv.ui.catalog_page import CatalogPage
from chudo_obuv.ui.images import logo_pixmap
from chudo_obuv.ui.login_page import LoginPage
from chudo_obuv.ui.order_details_page import OrderDetailsPage
from chudo_obuv.ui.order_draft_page import OrderDraftPage
from chudo_obuv.ui.orders_page import OrdersPage
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.product_page import ProductPage

LOGO_SIZE = QSize(56, 56)


class MainWindow(QMainWindow):
    """Главное окно приложения — последовательный интерфейс.

    Страницы открываются поверх друг друга в QStackedWidget, кнопка
    «Назад» возвращает на предыдущую. После входа первой страницей
    становится каталог, а ФИО пользователя выводится в правом верхнем углу.
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
        self._reset_stack(LoginPage(context))

    def _build_header(self) -> QFrame:
        """Шапка: «Назад», логотип, название и заголовок страницы, пользователь."""
        self._back_button = QPushButton("← Назад")
        self._back_button.setToolTip("Вернуться на предыдущую страницу")
        self._back_button.clicked.connect(self.go_back)

        logo = QLabel()
        logo.setPixmap(logo_pixmap(LOGO_SIZE))
        self._page_title = QLabel(objectName="pageTitle")
        titles = QVBoxLayout()
        titles.setSpacing(0)
        titles.addWidget(QLabel(APP_NAME, objectName="appTitle"))
        titles.addWidget(self._page_title)

        self._user_name = QLabel(objectName="userName", alignment=Qt.AlignmentFlag.AlignRight)
        self._user_role = QLabel(objectName="userRole", alignment=Qt.AlignmentFlag.AlignRight)
        user_box = QVBoxLayout()
        user_box.setSpacing(0)
        user_box.addWidget(self._user_name)
        user_box.addWidget(self._user_role)
        self._logout_button = QPushButton()
        self._logout_button.clicked.connect(self.logout)

        header = QFrame(objectName="header")
        layout = QHBoxLayout(header)
        layout.setContentsMargins(16, 8, 16, 8)
        layout.addWidget(self._back_button)
        layout.addWidget(logo)
        layout.addLayout(titles)
        layout.addStretch()
        layout.addLayout(user_box)
        layout.addWidget(self._logout_button)
        return header

    def show_on_screen(self) -> None:
        """Показывает окно, а если оно не помещается на экране — развёрнутым.

        Окно 1180×780 не помещается по высоте на мониторе 1366×768
        и на ноутбуке 1920×1080 с масштабом 150 %: нижний край с кнопками
        ушёл бы под панель задач.
        """
        self.show()
        if not self.screen().availableGeometry().contains(self.frameGeometry()):
            self.showMaximized()

    def login(self, user: User) -> None:
        """Вход выполнен: каталог становится первой страницей."""
        self._context.user = user
        self._reset_stack(CatalogPage(self._context))

    def logout(self) -> None:
        """Выход из системы на страницу входа."""
        if not self._confirm_draft_loss():
            return
        self._context.draft.clear()
        self._context.user = GUEST
        self._reset_stack(LoginPage(self._context))

    def show_product(self, product_id: int) -> None:
        """Открывает форму просмотра выбранного товара."""
        self._open_page(ProductPage(self._context, product_id))

    def show_order_draft(self) -> None:
        """Открывает формируемый заказ."""
        self._open_page(OrderDraftPage(self._context))

    def show_orders(self) -> None:
        """Открывает список заказов (менеджер и администратор)."""
        self._open_page(OrdersPage(self._context))

    def show_order_details(self, order_id: int) -> None:
        """Открывает состав заказа."""
        self._open_page(OrderDetailsPage(self._context, order_id))

    def show_catalog_for_order(self) -> None:
        """Открывает каталог для выбора товаров нового заказа из списка заказов."""
        self._open_page(CatalogPage(self._context, select_for_order=True))

    def return_after_order(self) -> None:
        """После подтверждения заказа или отказа от него — туда, где начат выбор товаров.

        Если заказ добавлялся из списка заказов, возвращаемся к списку,
        иначе — в основной каталог.
        """
        while not isinstance(self._stack.currentWidget(), CatalogPage):
            self._remove_current_page()
        if self._stack.currentWidget().select_for_order:
            self._remove_current_page()
        self._activate(self._stack.currentWidget())

    def closeEvent(self, event: QCloseEvent) -> None:
        """Закрытие окна с неподтверждённым заказом требует подтверждения."""
        if self._confirm_draft_loss():
            event.accept()
        else:
            event.ignore()

    def _confirm_draft_loss(self) -> bool:
        if self._context.draft.is_empty:
            return True
        return messages.ask_confirmation(self, "Формируемый заказ не подтверждён, выбранные "
                                               "товары будут потеряны.\nПродолжить?")

    def go_back(self) -> None:
        """Возвращает на предыдущую страницу."""
        if self._stack.count() > 1:
            self._remove_current_page()
            self._activate(self._stack.currentWidget())

    def _open_page(self, page: Page) -> None:
        self._stack.addWidget(page)
        self._stack.setCurrentWidget(page)
        self._activate(page)

    def _remove_current_page(self) -> None:
        page = self._stack.currentWidget()
        self._stack.removeWidget(page)
        page.deleteLater()

    def _reset_stack(self, page: Page) -> None:
        while self._stack.count():
            self._remove_current_page()
        self._open_page(page)

    def _activate(self, page: Page) -> None:
        """Обновляет заголовки и шапку под показанную страницу."""
        self.setWindowTitle(f"{APP_NAME} — {page.title}")
        self._page_title.setText(page.title)
        self._back_button.setVisible(self._stack.count() > 1)

        user = self._context.user
        signed_in = not isinstance(page, LoginPage)
        show_role = signed_in and user.role is not Role.GUEST
        self._user_name.setText(user.full_name if signed_in else "")
        self._user_role.setText(user.role.value if show_role else "")
        self._logout_button.setText("Войти" if user.role is Role.GUEST else "Выйти")
        self._logout_button.setVisible(signed_in)
        page.on_activated()
