"""Общие элементы интерфейса."""

from PySide6.QtWidgets import QPushButton


def accent_button(text: str) -> QPushButton:
    """Кнопка целевого действия, выделенная акцентным цветом #70B2AF."""
    button = QPushButton(text)
    button.setProperty("accent", True)
    return button
