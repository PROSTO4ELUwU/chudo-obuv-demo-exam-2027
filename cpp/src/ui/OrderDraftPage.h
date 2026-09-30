#pragma once

#include "ui/Page.h"

#include <vector>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;

// Позиции, выбранные для заказа: изменение количества, удаление, подтверждение.
// Авторизованный пользователь оформляет заказ на себя, а менеджер
// и администратор выбирают клиента из списка.
class OrderDraftPage : public Page
{
    Q_OBJECT

public:
    explicit OrderDraftPage(AppContext &context);

    QString title() const override { return QStringLiteral("Формируемый заказ"); }

    void onActivated() override;

private:
    void loadClients();
    void fillTable();
    void highlightRow(int row, const QString &tooltip);
    void updateTotal();
    void changeQuantity(int stockItemId, int quantity);
    void removeLine(int stockItemId);
    const User *selectedClient() const;
    void cancelOrder();
    void confirmOrder();

    std::vector<User> m_clients;
    QComboBox *m_clientCombo = nullptr;
    QLabel *m_clientName = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_totalLabel = nullptr;
    QPushButton *m_cancelButton = nullptr;
    QPushButton *m_confirmButton = nullptr;
};
