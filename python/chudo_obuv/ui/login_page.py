"""Вход в систему по логину."""

import psycopg
from PySide6.QtCore import QPoint, QRegularExpression, QSize, Qt
from PySide6.QtGui import QRegularExpressionValidator
from PySide6.QtWidgets import QFrame, QLabel, QLineEdit, QPushButton, QToolTip, QVBoxLayout

from chudo_obuv.models import GUEST
from chudo_obuv.ui import messages
from chudo_obuv.ui.images import logo_pixmap
from chudo_obuv.ui.pages import AppContext, Page
from chudo_obuv.ui.widgets import accent_button

LOGIN_PATTERN = r"[A-Za-z0-9._-]*"
LOGO_SIZE = QSize(140, 140)


class LoginPage(Page):
    """Вход только по логину из списка пользователей, без пароля, или вход гостем."""

    title = "Вход в систему"

    def __init__(self, context: AppContext) -> None:
        super().__init__(context)
        logo = QLabel(alignment=Qt.AlignmentFlag.AlignCenter)
        logo.setPixmap(logo_pixmap(LOGO_SIZE))

        self._login_edit = QLineEdit(placeholderText="Логин", maxLength=50, clearButtonEnabled=True)
        self._login_edit.setValidator(QRegularExpressionValidator(QRegularExpression(LOGIN_PATTERN)))
        self._login_edit.inputRejected.connect(self._show_login_hint)
        self._login_edit.returnPressed.connect(self._login)

        login_button = accent_button("Войти")
        login_button.clicked.connect(self._login)
        guest_button = QPushButton("Войти как гость")
        guest_button.setToolTip("Без входа доступен только просмотр каталога")
        guest_button.clicked.connect(lambda: self.navigator.login(GUEST))

        form = QFrame()
        form.setFixedWidth(360)
        form_layout = QVBoxLayout(form)
        form_layout.setSpacing(10)
        form_layout.addWidget(logo)
        form_layout.addWidget(QLabel("Вход в систему", objectName="appTitle",
                                     alignment=Qt.AlignmentFlag.AlignCenter))
        form_layout.addWidget(QLabel("Введите логин из списка пользователей.\nПароль не требуется.",
                                     objectName="hint", alignment=Qt.AlignmentFlag.AlignCenter))
        form_layout.addWidget(self._login_edit)
        form_layout.addWidget(login_button)
        form_layout.addWidget(guest_button)

        layout = QVBoxLayout(self)
        layout.addStretch()
        layout.addWidget(form, alignment=Qt.AlignmentFlag.AlignCenter)
        layout.addStretch(2)

    def on_activated(self) -> None:
        """После выхода из системы поле логина пустое и готово к вводу."""
        self._login_edit.clear()
        self._login_edit.setFocus()

    def _show_login_hint(self) -> None:
        """Подсказка при попытке ввести недопустимый символ, например кириллицу."""
        position = self._login_edit.mapToGlobal(QPoint(0, self._login_edit.height()))
        QToolTip.showText(position, "Логин вводится латинскими буквами и цифрами — "
                                    "проверьте раскладку клавиатуры", self._login_edit)

    def _login(self) -> None:
        login = self._login_edit.text().strip()
        if not login:
            messages.show_warning(self, "Введите логин.\n"
                                        "Без логина можно войти как гость и просматривать каталог.")
            self._login_edit.setFocus()
            return
        try:
            user = self.context.users.find_by_login(login)
        except psycopg.Error as error:
            messages.show_database_error(self, error)
            return
        if user is None:
            messages.show_error(self, f"Пользователь с логином «{login}» не найден.\n"
                                      "Проверьте правильность ввода или войдите как гость.")
            self._login_edit.selectAll()
            self._login_edit.setFocus()
            return
        self.navigator.login(user)
