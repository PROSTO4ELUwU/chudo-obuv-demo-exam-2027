#pragma once

#include "ui/Page.h"

#include <vector>

class QTableWidget;

// Список заказов для менеджера и администратора (задание 4): дата, ФИО
// клиента и сумма; добавление, удаление и просмотр состава заказа
class OrdersPage : public Page
{
    Q_OBJECT

public:
    explicit OrdersPage(AppContext &context);

    QString title() const override { return QStringLiteral("Заказы"); }

    // Список обновляется после добавления, изменения и удаления заказов
    void onActivated() override;

private:
    const OrderSummary *selectedOrder() const;
    void openDetails();
    void deleteOrder();

    std::vector<OrderSummary> m_orders;
    QTableWidget *m_table = nullptr;
};
