#pragma once

#include "ui/Page.h"

#include <optional>
#include <vector>

class QDateEdit;
class QLabel;
class QPushButton;
class QTableWidget;

// Товарные позиции заказа с количеством, ценой за единицу и итоговой суммой
// (задание 4). Администратор может изменить дату заказа и удалить позиции,
// менеджер видит состав только для чтения.
class OrderDetailsPage : public Page
{
    Q_OBJECT

public:
    OrderDetailsPage(AppContext &context, int orderId);

    QString title() const override { return QStringLiteral("Состав заказа"); }

    void onActivated() override;

private:
    void updateSaveButton();
    void saveDate();
    void deleteLine();

    int m_orderId = 0;
    std::optional<OrderSummary> m_order;
    std::vector<OrderLine> m_lines;
    QLabel *m_numberLabel = nullptr;
    QLabel *m_clientLabel = nullptr;
    QLabel *m_dateLabel = nullptr;
    QDateEdit *m_dateEdit = nullptr;
    QPushButton *m_saveDateButton = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_totalLabel = nullptr;
};
