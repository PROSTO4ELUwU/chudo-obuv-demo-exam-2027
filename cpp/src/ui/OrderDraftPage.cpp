#include "ui/OrderDraftPage.h"

#include "Formatting.h"
#include "ui/Messages.h"
#include "ui/Style.h"
#include "ui/Widgets.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {

constexpr int sumColumn = 4;

} // namespace

OrderDraftPage::OrderDraftPage(AppContext &context)
    : Page(context)
{
    m_clientCombo = new QComboBox;
    m_clientCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_clientCombo->setToolTip(QStringLiteral("Клиент, на которого оформляется заказ"));
    m_clientName = new QLabel;
    m_clientName->setObjectName(QStringLiteral("userName"));
    auto *clientRow = new QHBoxLayout;
    clientRow->addWidget(new QLabel(QStringLiteral("Клиент:")));
    clientRow->addWidget(m_clientCombo);
    clientRow->addWidget(m_clientName);
    clientRow->addStretch();

    m_table = Widgets::makeTable({QStringLiteral("Модель"), QStringLiteral("Размер"),
                                  QStringLiteral("Цена за пару"), QStringLiteral("Количество"),
                                  QStringLiteral("Сумма"), QString()});
    m_table->setSelectionMode(QAbstractItemView::NoSelection);
    m_emptyLabel = new QLabel(QStringLiteral("В заказе пока нет товаров. "
                                             "Выберите товары в каталоге."));
    m_emptyLabel->setObjectName(QStringLiteral("hint"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_totalLabel = new QLabel;
    m_totalLabel->setObjectName(QStringLiteral("total"));

    m_cancelButton = new QPushButton(QStringLiteral("Отказаться от заказа"));
    connect(m_cancelButton, &QPushButton::clicked, this, &OrderDraftPage::cancelOrder);
    m_confirmButton = Widgets::accentButton(QStringLiteral("Подтвердить заказ"));
    connect(m_confirmButton, &QPushButton::clicked, this, &OrderDraftPage::confirmOrder);
    auto *bottomRow = new QHBoxLayout;
    bottomRow->addWidget(m_totalLabel);
    bottomRow->addStretch();
    bottomRow->addWidget(m_cancelButton);
    bottomRow->addWidget(m_confirmButton);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addLayout(clientRow);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_emptyLabel);
    layout->addLayout(bottomRow);
}

void OrderDraftPage::onActivated()
{
    const User &user = m_context.user;
    m_clientCombo->setVisible(user.canManageOrders());
    m_clientName->setVisible(!user.canManageOrders());
    if (user.canManageOrders())
        loadClients();
    else
        m_clientName->setText(user.fullName());
    fillTable();
}

void OrderDraftPage::loadClients()
{
    // Список клиентов; выбранный ранее клиент сохраняется
    const QString selected = m_clientCombo->currentText();
    try {
        m_clients = m_context.users.listClients();
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    m_clientCombo->clear();
    m_clientCombo->addItem(QStringLiteral("— выберите клиента —"), -1);
    for (int index = 0; index < static_cast<int>(m_clients.size()); ++index)
        m_clientCombo->addItem(m_clients[static_cast<std::size_t>(index)].fullName(), index);
    m_clientCombo->setCurrentText(selected);
}

void OrderDraftPage::fillTable()
{
    const std::vector<DraftLine> &lines = m_context.draft.lines();
    m_table->setRowCount(static_cast<int>(lines.size()));
    for (int row = 0; row < static_cast<int>(lines.size()); ++row) {
        const DraftLine &line = lines[static_cast<std::size_t>(row)];
        m_table->setItem(row, 0, Widgets::tableItem(
                                     QStringLiteral("%1 (%2)").arg(line.productName, line.manufacturer)));
        m_table->setItem(row, 1, Widgets::tableItem(Formatting::size(line.sizeTenths),
                                                    Widgets::alignCenter));
        m_table->setItem(row, 2, Widgets::tableItem(Formatting::money(line.unitPrice),
                                                    Widgets::alignRight));
        m_table->setItem(row, sumColumn, Widgets::tableItem(Formatting::money(line.total()),
                                                            Widgets::alignRight));

        auto *quantitySpin = new QSpinBox;
        quantitySpin->setRange(1, std::max(line.available, line.quantity));
        quantitySpin->setValue(line.quantity);
        quantitySpin->setEnabled(line.available > 0);
        const int stockItemId = line.stockItemId;
        connect(quantitySpin, &QSpinBox::valueChanged, this,
                [this, stockItemId](int quantity) { changeQuantity(stockItemId, quantity); });
        m_table->setCellWidget(row, 3, quantitySpin);

        auto *removeButton = new QPushButton(QStringLiteral("Удалить"));
        connect(removeButton, &QPushButton::clicked, this,
                [this, stockItemId] { removeLine(stockItemId); });
        m_table->setCellWidget(row, 5, removeButton);

        if (line.exceedsStock()) {
            highlightRow(row, QStringLiteral("Этого размера больше нет в наличии "
                                             "(осталось %1) — удалите позицию")
                                  .arg(Formatting::pairs(line.available)));
        }
    }
    const bool empty = lines.empty();
    m_table->setVisible(!empty);
    m_emptyLabel->setVisible(empty);
    m_cancelButton->setEnabled(!empty);
    m_confirmButton->setEnabled(!empty);
    updateTotal();
}

void OrderDraftPage::highlightRow(int row, const QString &tooltip)
{
    for (int column = 0; column < m_table->columnCount(); ++column) {
        if (QTableWidgetItem *item = m_table->item(row, column)) {
            item->setBackground(QColor(QString::fromLatin1(Style::lowStockColor)));
            item->setToolTip(tooltip);
        }
    }
}

void OrderDraftPage::updateTotal()
{
    const OrderDraft &draft = m_context.draft;
    m_totalLabel->setText(QStringLiteral("Итого: %1 на сумму %2")
                              .arg(Formatting::pairs(draft.pairsCount()),
                                   Formatting::money(draft.total())));
}

void OrderDraftPage::changeQuantity(int stockItemId, int quantity)
{
    try {
        m_context.draft.setQuantity(stockItemId, quantity);
    } catch (const OrderDraftError &error) {
        Messages::showWarning(this, error.message());
        fillTable();
        return;
    }
    const std::vector<DraftLine> &lines = m_context.draft.lines();
    for (int row = 0; row < static_cast<int>(lines.size()); ++row)
        m_table->item(row, sumColumn)->setText(Formatting::money(lines[static_cast<std::size_t>(row)].total()));
    updateTotal();
}

void OrderDraftPage::removeLine(int stockItemId)
{
    const auto &lines = m_context.draft.lines();
    const auto line = std::ranges::find(lines, stockItemId, &DraftLine::stockItemId);
    if (line == lines.end())
        return;
    if (Messages::askConfirmation(this, QStringLiteral("Удалить из заказа «%1», размер %2?")
                                            .arg(line->productName,
                                                 Formatting::size(line->sizeTenths)))) {
        m_context.draft.remove(stockItemId);
        fillTable();
    }
}

const User *OrderDraftPage::selectedClient() const
{
    if (!m_context.user.canManageOrders())
        return &m_context.user;
    const int index = m_clientCombo->currentData().toInt();
    if (index < 0 || index >= static_cast<int>(m_clients.size()))
        return nullptr;
    return &m_clients[static_cast<std::size_t>(index)];
}

void OrderDraftPage::cancelOrder()
{
    if (Messages::askConfirmation(this, QStringLiteral("Отказаться от заказа?\n"
                                                       "Все выбранные позиции будут удалены."))) {
        m_context.draft.clear();
        navigator().returnAfterOrder();
    }
}

void OrderDraftPage::confirmOrder()
{
    // Сохраняет заказ с сегодняшней датой и списывает остатки
    OrderDraft &draft = m_context.draft;
    if (draft.isEmpty()) {
        Messages::showWarning(this, QStringLiteral("В заказе нет товаров. "
                                                   "Добавьте товары из каталога."));
        return;
    }
    const User *client = selectedClient();
    if (!client) {
        Messages::showWarning(this, QStringLiteral("Выберите клиента, на которого "
                                                   "оформляется заказ."));
        m_clientCombo->setFocus();
        return;
    }

    const QDate orderDate = QDate::currentDate();
    const Money total = draft.total();
    const QString clientName = client->fullName();
    int orderId = 0;
    try {
        orderId = m_context.orders.createOrder(client->id, orderDate, draft.lines());
    } catch (const InsufficientStockError &error) {
        for (const auto &shortage : error.shortages())
            draft.updateAvailable(shortage.line.stockItemId, shortage.available);
        fillTable();
        Messages::showWarning(this, error.message()
                                        + QStringLiteral("\n\nКоличество уменьшено до доступного, "
                                                         "а позиции, которых не осталось, "
                                                         "выделены — удалите их и подтвердите "
                                                         "заказ снова."));
        return;
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }

    draft.clear();
    Messages::showInfo(this, QStringLiteral("Заказ №%1 от %2 оформлен.\nКлиент: %3\n"
                                            "Сумма заказа: %4")
                                 .arg(orderId)
                                 .arg(Formatting::date(orderDate), clientName,
                                      Formatting::money(total)));
    navigator().returnAfterOrder();
}
