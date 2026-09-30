"""Точка входа приложения «Чудо Обувь»."""

import sys
import traceback
from types import TracebackType

import psycopg
from PySide6.QtCore import QLibraryInfo, QLocale, QTranslator
from PySide6.QtGui import QFont, QIcon
from PySide6.QtWidgets import QApplication

from chudo_obuv import APP_NAME
from chudo_obuv.config import ICON_FILE, load_database_settings
from chudo_obuv.database import Database
from chudo_obuv.repositories import OrderRepository, ProductRepository, UserRepository
from chudo_obuv.ui import messages
from chudo_obuv.ui.main_window import MainWindow
from chudo_obuv.ui.pages import AppContext
from chudo_obuv.ui.style import FONT_FAMILY, STYLE_SHEET


def install_russian_translation(app: QApplication) -> None:
    """Русские подписи стандартных кнопок и диалогов Qt: «Да», «Нет», «Отмена»."""
    translator = QTranslator(app)
    translations_dir = QLibraryInfo.path(QLibraryInfo.LibraryPath.TranslationsPath)
    if translator.load(QLocale(QLocale.Language.Russian), "qtbase", "_", translations_dir):
        app.installTranslator(translator)


def handle_unexpected_error(error_type: type[BaseException], error: BaseException,
                            error_traceback: TracebackType | None) -> None:
    """Последний рубеж: непредвиденная ошибка показывается, а не закрывает программу."""
    traceback.print_exception(error_type, error, error_traceback)
    if isinstance(error, psycopg.Error):
        messages.show_database_error(None, error)
    else:
        messages.show_error(
            None,
            f"Произошла непредвиденная ошибка: {error}\n\n"
            "Повторите действие. Если ошибка повторяется, перезапустите приложение.",
        )


def main() -> int:
    """Запускает приложение и возвращает код завершения."""
    app = QApplication(sys.argv)
    app.setApplicationName(APP_NAME)
    app.setWindowIcon(QIcon(str(ICON_FILE)))
    app.setFont(QFont(FONT_FAMILY, 11))
    app.setStyleSheet(STYLE_SHEET)
    install_russian_translation(app)
    sys.excepthook = handle_unexpected_error

    try:
        database = Database(load_database_settings())
        database.connection
    except (OSError, KeyError, psycopg.Error) as error:
        messages.show_error(
            None,
            "Не удалось подключиться к базе данных «Чудо Обувь».\n\n"
            "Проверьте, что сервер PostgreSQL запущен, база развёрнута скриптом "
            "database/deploy.bat, а параметры в config.ini указаны верно.\n\n"
            f"Подробности: {error}",
        )
        return 1

    context = AppContext(
        users=UserRepository(database),
        products=ProductRepository(database),
        orders=OrderRepository(database),
    )
    window = MainWindow(context)
    window.show()
    exit_code = app.exec()
    database.close()
    return exit_code


if __name__ == "__main__":
    sys.exit(main())
