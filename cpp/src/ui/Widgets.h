#pragma once

#include <QStringList>

#include <initializer_list>

class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QWidget;

// Общие элементы интерфейса
namespace Widgets {

inline constexpr Qt::Alignment alignLeft = Qt::AlignLeft | Qt::AlignVCenter;
inline constexpr Qt::Alignment alignCenter = Qt::AlignCenter;
inline constexpr Qt::Alignment alignRight = Qt::AlignRight | Qt::AlignVCenter;

// Скрывает элементы, недоступные пользователю. Показывать их явно
// (setVisible(true)) при создании нельзя: виджет без родителя открылся бы
// отдельным окном, а внутри страницы элементы и так видны вместе с ней.
void hideUnless(bool allowed, std::initializer_list<QWidget *> widgets);

// Кнопка целевого действия, выделенная акцентным цветом #70B2AF
QPushButton *accentButton(const QString &text);

// Таблица только для просмотра: выделяется строка целиком, один столбец растягивается
QTableWidget *makeTable(const QStringList &headers, int stretchColumn = 0);

// Ячейка таблицы с выравниванием текста
QTableWidgetItem *tableItem(const QString &text, Qt::Alignment alignment = alignLeft);

} // namespace Widgets
