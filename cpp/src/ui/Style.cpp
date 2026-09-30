#include "ui/Style.h"

#include <QList>
#include <QPair>

namespace Style {

QString styleSheet()
{
    QString sheet = QStringLiteral(R"(
QWidget {
    font-family: "{font}";
    font-size: 11pt;
    color: {text};
}
QMainWindow, QStackedWidget, QScrollArea, #cardsContainer {
    background: {main};
}
#header, #panel {
    background: {extra};
}
#panel {
    border-radius: 6px;
}
#appTitle {
    font-size: 18pt;
    font-weight: bold;
}
#pageTitle {
    font-size: 13pt;
}
#userName {
    font-weight: bold;
}
#hint, #userRole {
    color: {secondary};
}
QPushButton {
    background: {main};
    border: 1px solid {accent};
    border-radius: 4px;
    padding: 6px 16px;
}
QPushButton:hover {
    background: {extra};
}
QPushButton:disabled {
    color: #9AA5A3;
    border-color: #C9D3D1;
}
QPushButton[accent="true"] {
    background: {accent};
    color: {main};
    font-weight: bold;
}
QPushButton[accent="true"]:hover {
    border-color: #1F2D2B;
}
QPushButton[accent="true"]:disabled {
    background: #E9EFEE;
    color: #9AA5A3;
    border-color: #C9D3D1;
}
QLineEdit, QComboBox, QSpinBox, QDateEdit {
    background: {main};
    border: 1px solid {border};
    border-radius: 4px;
    padding: 4px 6px;
}
QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDateEdit:focus {
    border-color: {accent};
}
QDateEdit {
    min-width: 7em;
}
QSpinBox {
    min-width: 4em;
}
QSpinBox::up-button, QSpinBox::down-button {
    width: 20px;
}
#productCard {
    background: {main};
    border: 1px solid {border};
    border-radius: 6px;
}
#productCard[lowStock="true"] {
    background: {lowStock};
}
#productTitle {
    font-size: 13pt;
    font-weight: bold;
}
#price {
    font-size: 16pt;
    font-weight: bold;
}
#oldPrice {
    color: {secondary};
    text-decoration: line-through;
}
#composition {
    font-size: 10pt;
}
#total {
    font-size: 14pt;
    font-weight: bold;
}
QTableWidget {
    gridline-color: #D8E6E4;
    selection-background-color: {extra};
    selection-color: #000000;
}
QHeaderView::section {
    background: {extra};
    border: none;
    border-right: 1px solid {main};
    padding: 6px;
    font-weight: bold;
}
)");
    const QList<QPair<QString, QString>> colors = {
        {QStringLiteral("{font}"), QString::fromLatin1(fontFamily)},
        {QStringLiteral("{main}"), QString::fromLatin1(mainBackground)},
        {QStringLiteral("{extra}"), QString::fromLatin1(extraBackground)},
        {QStringLiteral("{accent}"), QString::fromLatin1(accentColor)},
        {QStringLiteral("{lowStock}"), QString::fromLatin1(lowStockColor)},
        {QStringLiteral("{text}"), QString::fromLatin1(textColor)},
        {QStringLiteral("{secondary}"), QString::fromLatin1(secondaryText)},
        {QStringLiteral("{border}"), QString::fromLatin1(borderColor)},
    };
    for (const auto &[token, value] : colors)
        sheet.replace(token, value);
    return sheet;
}

} // namespace Style
