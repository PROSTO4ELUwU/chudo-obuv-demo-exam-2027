#include "ui/Widgets.h"

#include <QHeaderView>
#include <QPushButton>
#include <QTableWidget>

namespace Widgets {

void hideUnless(bool allowed, std::initializer_list<QWidget *> widgets)
{
    if (allowed)
        return;
    for (QWidget *widget : widgets)
        widget->hide();
}

QPushButton *accentButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setProperty("accent", true);
    return button;
}

QTableWidget *makeTable(const QStringList &headers, int stretchColumn)
{
    auto *table = new QTableWidget(0, static_cast<int>(headers.size()));
    table->setHorizontalHeaderLabels(headers);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(36);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(stretchColumn, QHeaderView::Stretch);
    return table;
}

QTableWidgetItem *tableItem(const QString &text, Qt::Alignment alignment)
{
    auto *item = new QTableWidgetItem(text);
    item->setTextAlignment(alignment);
    return item;
}

} // namespace Widgets
