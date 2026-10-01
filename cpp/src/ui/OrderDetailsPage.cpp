#include "ui/OrderDetailsPage.h"

#include "Formatting.h"
#include "ui/Messages.h"
#include "ui/Widgets.h"

#include <QDateEdit>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

OrderDetailsPage::OrderDetailsPage(AppContext &context, int orderId)
    : Page(context)
    , m_orderId(orderId)
{
    const bool canEdit = context.user.canEditOrders();

    m_numberLabel = new QLabel;
    m_numberLabel->setObjectName(QStringLiteral("productTitle"));
    m_clientLabel = new QLabel;
    m_dateLabel = new QLabel;
    m_dateEdit = new QDateEdit;
    m_dateEdit->setCalendarPopup(true);
    m_dateEdit->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_dateEdit->setMinimumDate(QDate(2000, 1, 1));
    m_dateEdit->setMaximumDate(QDate::currentDate());
    m_dateEdit->setToolTip(QStringLiteral("Дата заказа не может быть позже сегодняшней"));
    connect(m_dateEdit, &QDateEdit::dateChanged, this, &OrderDetailsPage::updateSaveButton);
    m_saveDateButton = Widgets::accentButton(QStringLiteral("Сохранить дату"));
    connect(m_saveDateButton, &QPushButton::clicked, this, &OrderDetailsPage::saveDate);
    Widgets::hideUnless(!canEdit, {m_dateLabel});
    Widgets::hideUnless(canEdit, {m_dateEdit, m_saveDateButton});
    auto *dateRow = new QHBoxLayout;
    dateRow->addWidget(m_dateLabel);
    dateRow->addWidget(m_dateEdit);
    dateRow->addWidget(m_saveDateButton);
    dateRow->addStretch();

    auto *info = new QFormLayout;
    info->addRow(m_numberLabel);
    info->addRow(QStringLiteral("Клиент:"), m_clientLabel);
    info->addRow(QStringLiteral("Дата заказа:"), dateRow);

    m_table = Widgets::makeTable({QStringLiteral("Товарная позиция (модель — производство)"),
                                  QStringLiteral("Размер"), QStringLiteral("Количество"),
                                  QStringLiteral("Цена за единицу"), QStringLiteral("Сумма")});
    m_totalLabel = new QLabel;
    m_totalLabel->setObjectName(QStringLiteral("total"));
    auto *deleteLineButton = new QPushButton(QStringLiteral("Удалить позицию"));
    deleteLineButton->setToolTip(QStringLiteral("Удалить выбранную позицию; пары вернутся в остатки"));
    connect(deleteLineButton, &QPushButton::clicked, this, &OrderDetailsPage::deleteLine);
    Widgets::hideUnless(canEdit, {deleteLineButton});
    auto *bottomRow = new QHBoxLayout;
    bottomRow->addWidget(m_totalLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(deleteLineButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addLayout(info);
    layout->addWidget(m_table, 1);
    layout->addLayout(bottomRow);
}

void OrderDetailsPage::onActivated()
{
    std::optional<OrderSummary> order;
    std::vector<OrderLine> lines;
    try {
        order = m_context.orders.getOrder(m_orderId);
        if (order)
            lines = m_context.orders.listLines(m_orderId);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    if (!order) {
        Messages::showWarning(this, QStringLiteral("Заказ №%1 не найден — возможно, его удалили.")
                                        .arg(m_orderId));
        navigator().goBack();
        return;
    }
    m_order = order;
    m_lines = lines;
    m_numberLabel->setText(QStringLiteral("Заказ №%1").arg(order->id));
    m_clientLabel->setText(order->clientName);
    m_dateLabel->setText(Formatting::date(order->date));
    {
        const QSignalBlocker blocker(m_dateEdit);
        m_dateEdit->setDate(order->date);
    }
    updateSaveButton();

    // После удаления позиции в той же строке оказалась бы соседняя
    m_table->clearSelection();
    m_table->setRowCount(static_cast<int>(lines.size()));
    for (int row = 0; row < static_cast<int>(lines.size()); ++row) {
        const OrderLine &line = lines[static_cast<std::size_t>(row)];
        m_table->setItem(row, 0, Widgets::tableItem(line.productName + QStringLiteral(" — ")
                                                    + line.manufacturer));
        m_table->setItem(row, 1, Widgets::tableItem(Formatting::size(line.sizeTenths),
                                                    Widgets::alignCenter));
        m_table->setItem(row, 2, Widgets::tableItem(Formatting::pairs(line.quantity),
                                                    Widgets::alignCenter));
        m_table->setItem(row, 3, Widgets::tableItem(Formatting::money(line.unitPrice),
                                                    Widgets::alignRight));
        m_table->setItem(row, 4, Widgets::tableItem(Formatting::money(line.total()),
                                                    Widgets::alignRight));
    }
    m_totalLabel->setText(QStringLiteral("Итоговая сумма по заказу: ")
                          + Formatting::money(order->total));
}

void OrderDetailsPage::updateSaveButton()
{
    // «Сохранить дату» доступна, только если дата действительно изменена
    m_saveDateButton->setEnabled(m_order && m_dateEdit->date() != m_order->date);
}

void OrderDetailsPage::saveDate()
{
    const QDate newDate = m_dateEdit->date();
    try {
        m_context.orders.changeOrderDate(m_orderId, newDate);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    Messages::showInfo(this, QStringLiteral("Дата заказа №%1 изменена на %2.")
                                 .arg(m_orderId)
                                 .arg(Formatting::date(newDate)));
    onActivated();
}

void OrderDetailsPage::deleteLine()
{
    const QModelIndexList rows = m_table->selectionModel()->selectedRows();
    if (rows.isEmpty()) {
        Messages::showWarning(this, QStringLiteral("Выберите в составе заказа позицию, "
                                                   "которую нужно удалить."));
        return;
    }
    if (m_lines.size() == 1) {
        Messages::showWarning(this, lastLineMessage());
        return;
    }
    const OrderLine &line = m_lines[static_cast<std::size_t>(rows.first().row())];
    if (!Messages::askConfirmation(this, QStringLiteral("Удалить из заказа №%1 позицию «%2», "
                                                        "размер %3, %4?\nПары вернутся в остатки.")
                                             .arg(m_orderId)
                                             .arg(line.productName, Formatting::size(line.sizeTenths),
                                                  Formatting::pairs(line.quantity)))) {
        return;
    }
    try {
        m_context.orders.deleteLine(line.orderItemId);
    } catch (const OrderError &error) {
        Messages::showWarning(this, error.message());
        return;
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    onActivated();
}
