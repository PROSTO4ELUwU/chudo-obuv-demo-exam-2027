"""Окна сообщений: заголовок и значок соответствуют типу сообщения."""

from PySide6.QtWidgets import QMessageBox, QWidget


def show_error(parent: QWidget | None, text: str) -> None:
    """Ошибка: действие не выполнено."""
    QMessageBox.critical(parent, "Ошибка", text)


def show_warning(parent: QWidget | None, text: str) -> None:
    """Предупреждение: пользователь ввёл неверные данные или действие запрещено."""
    QMessageBox.warning(parent, "Предупреждение", text)


def show_info(parent: QWidget | None, text: str) -> None:
    """Информация о результате действия."""
    QMessageBox.information(parent, "Информация", text)


def show_database_error(parent: QWidget | None, error: Exception) -> None:
    """Ошибка обращения к базе данных с порядком действий для пользователя."""
    show_error(
        parent,
        "Не удалось выполнить операцию с базой данных.\n"
        "Проверьте, что сервер PostgreSQL запущен, и повторите действие.\n\n"
        f"Подробности: {error}",
    )


def ask_confirmation(parent: QWidget | None, text: str) -> bool:
    """Подтверждение необратимого действия; по умолчанию выбрано «Нет»."""
    answer = QMessageBox.question(
        parent,
        "Подтверждение",
        text,
        QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
        QMessageBox.StandardButton.No,
    )
    return answer == QMessageBox.StandardButton.Yes
