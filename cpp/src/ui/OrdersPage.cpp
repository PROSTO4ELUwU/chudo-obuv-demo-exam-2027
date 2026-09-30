#include "ui/OrdersPage.h"

#include "Formatting.h"
#include "ui/Messages.h"
#include "ui/Widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

OrdersPage::OrdersPage(AppContext &context)
    : Page(context)
{
    QPushButton *addButton = Widgets::accentButton(QStringLiteral("Добавить заказ"));
    addButton->setToolTip(QStringLiteral("Выбрать товары из каталога для нового заказа"));
    connect(addButton, &QPushButton::clicked, this, [this] { navigator().showCatalogForOrder(); });
    auto *detailsButton = new QPushButton(QStringLiteral("Состав заказа"));
    connect(detailsButton, &QPushButton::clicked, this, &OrdersPage::openDetails);
    auto *deleteButton = new QPushButton(QStringLiteral("Удалить заказ"));
    connect(deleteButton, &QPushButton::clicked, this, &OrdersPage::deleteOrder);
    auto *hint = new QLabel(QStringLiteral("Двойной щелчок по заказу открывает его состав"));
    hint->setObjectName(QStringLiteral("hint"));

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(addButton);
    buttons->addWidget(detailsButton);
    buttons->addWidget(deleteButton);
    buttons->addStretch();
    buttons->addWidget(hint);

    m_table = Widgets::makeTable({QStringLiteral("№ заказа"), QStringLiteral("Дата заказа"),
                                  QStringLiteral("ФИО клиента"), QStringLiteral("Сумма")},
                                 2);
    connect(m_table, &QTableWidget::doubleClicked, this, &OrdersPage::openDetails);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addLayout(buttons);
    layout->addWidget(m_table, 1);
}

void OrdersPage::onActivated()
{
    const OrderSummary *selected = selectedOrder();
    const int selectedId = selected ? selected->id : 0;
    try {
        m_orders = m_context.orders.listOrders();
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    m_table->clearSelection();
    m_table->setRowCount(static_cast<int>(m_orders.size()));
    for (int row = 0; row < static_cast<int>(m_orders.size()); ++row) {
        const OrderSummary &order = m_orders[static_cast<std::size_t>(row)];
        m_table->setItem(row, 0, Widgets::tableItem(QString::number(order.id), Widgets::alignCenter));
        m_table->setItem(row, 1, Widgets::tableItem(Formatting::date(order.date), Widgets::alignCenter));
        m_table->setItem(row, 2, Widgets::tableItem(order.clientName));
        m_table->setItem(row, 3, Widgets::tableItem(Formatting::money(order.total), Widgets::alignRight));
        if (order.id == selectedId)
            m_table->selectRow(row);
    }
}

const OrderSummary *OrdersPage::selectedOrder() const
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty() || rows.first().row() >= static_cast<int>(m_orders.size()))
        return nullptr;
    return &m_orders[static_cast<std::size_t>(rows.first().row())];
}

void OrdersPage::openDetails()
{
    const OrderSummary *order = selectedOrder();
    if (!order) {
        Messages::showWarning(this, QStringLiteral("Выберите заказ в списке."));
        return;
    }
    navigator().showOrderDetails(order->id);
}

void OrdersPage::deleteOrder()
{
    const OrderSummary *order = selectedOrder();
    if (!order) {
        Messages::showWarning(this, QStringLiteral("Выберите заказ, который нужно удалить."));
        return;
    }
    if (!Messages::askConfirmation(this, QStringLiteral("Удалить заказ №%1 от %2, клиент %3?\n"
                                                        "Товары заказа вернутся в остатки. "
                                                        "Действие нельзя отменить.")
                                             .arg(order->id)
                                             .arg(Formatting::date(order->date), order->clientName))) {
        return;
    }
    try {
        m_context.orders.deleteOrder(order->id);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    onActivated();
}
