"""Оформление по руководству по стилю заказчика (Приложение 3)."""

FONT_FAMILY = "Calibri"
MAIN_BACKGROUND = "#FFFFFF"
EXTRA_BACKGROUND = "#D2F6E7"
ACCENT_COLOR = "#70B2AF"
LOW_STOCK_COLOR = "#FF8080"
TEXT_COLOR = "#1F2D2B"
SECONDARY_TEXT = "#5F6B69"
BORDER_COLOR = "#9DBFBC"

STYLE_SHEET = f"""
QWidget {{
    font-family: "{FONT_FAMILY}";
    font-size: 11pt;
    color: {TEXT_COLOR};
}}
QMainWindow, QStackedWidget, QScrollArea, #cardsContainer {{
    background: {MAIN_BACKGROUND};
}}
#header, #panel {{
    background: {EXTRA_BACKGROUND};
}}
#panel {{
    border-radius: 6px;
}}
#appTitle {{
    font-size: 18pt;
    font-weight: bold;
}}
#pageTitle {{
    font-size: 13pt;
}}
#userName {{
    font-weight: bold;
}}
#hint, #userRole {{
    color: {SECONDARY_TEXT};
}}
QPushButton {{
    background: {MAIN_BACKGROUND};
    border: 1px solid {ACCENT_COLOR};
    border-radius: 4px;
    padding: 6px 16px;
}}
QPushButton:hover {{
    background: {EXTRA_BACKGROUND};
}}
QPushButton:disabled {{
    color: #9AA5A3;
    border-color: #C9D3D1;
}}
QPushButton[accent="true"] {{
    background: {ACCENT_COLOR};
    color: {MAIN_BACKGROUND};
    font-weight: bold;
}}
QPushButton[accent="true"]:hover {{
    border-color: #1F2D2B;
}}
QLineEdit, QComboBox, QSpinBox, QDateEdit {{
    background: {MAIN_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 4px;
    padding: 4px 6px;
}}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus {{
    border-color: {ACCENT_COLOR};
}}
#productCard {{
    background: {MAIN_BACKGROUND};
    border: 1px solid {BORDER_COLOR};
    border-radius: 6px;
}}
#productCard[lowStock="true"] {{
    background: {LOW_STOCK_COLOR};
}}
#productTitle {{
    font-size: 13pt;
    font-weight: bold;
}}
#price {{
    font-size: 16pt;
    font-weight: bold;
}}
#oldPrice {{
    color: {SECONDARY_TEXT};
    text-decoration: line-through;
}}
#composition {{
    font-size: 10pt;
}}
#total {{
    font-size: 14pt;
    font-weight: bold;
}}
QTableWidget {{
    gridline-color: #D8E6E4;
    selection-background-color: {EXTRA_BACKGROUND};
    selection-color: #000000;
}}
QHeaderView::section {{
    background: {EXTRA_BACKGROUND};
    border: none;
    border-right: 1px solid {MAIN_BACKGROUND};
    padding: 6px;
    font-weight: bold;
}}
"""
