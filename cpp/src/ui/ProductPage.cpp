#include "ui/ProductPage.h"

#include "Formatting.h"
#include "ui/Images.h"
#include "ui/Messages.h"
#include "ui/Widgets.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

namespace {

constexpr QSize imageSize(320, 240);

QLabel *wrappedLabel()
{
    auto *label = new QLabel;
    label->setWordWrap(true);
    return label;
}

} // namespace

ProductPage::ProductPage(AppContext &context, int productId)
    : Page(context)
    , m_productId(productId)
{
    m_image = new QLabel;
    m_image->setFixedSize(imageSize);
    m_image->setAlignment(Qt::AlignCenter);
    m_name = wrappedLabel();
    m_name->setObjectName(QStringLiteral("productTitle"));
    m_manufacturer = new QLabel;
    m_category = new QLabel;
    m_priceCaption = new QLabel;
    m_price = new QLabel;
    m_price->setObjectName(QStringLiteral("price"));
    m_oldPrice = new QLabel;
    m_oldPrice->setObjectName(QStringLiteral("oldPrice"));
    m_composition = wrappedLabel();
    m_description = wrappedLabel();
    m_sizeRange = wrappedLabel();
    m_availableSizes = wrappedLabel();

    auto *prices = new QHBoxLayout;
    prices->setSpacing(12);
    prices->addWidget(m_price);
    prices->addWidget(m_oldPrice);
    prices->addStretch();

    auto *details = new QFormLayout;
    details->setVerticalSpacing(8);
    details->addRow(m_name);
    details->addRow(QStringLiteral("Производство:"), m_manufacturer);
    details->addRow(QStringLiteral("Категория:"), m_category);
    details->addRow(m_priceCaption, prices);
    details->addRow(QStringLiteral("Состав:"), m_composition);
    details->addRow(QStringLiteral("Описание:"), m_description);
    details->addRow(QStringLiteral("Размерный ряд:"), m_sizeRange);
    details->addRow(QStringLiteral("Доступные размеры:"), m_availableSizes);

    auto *top = new QHBoxLayout;
    top->setSpacing(24);
    top->addWidget(m_image, 0, Qt::AlignTop);
    top->addLayout(details, 1);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->addLayout(top);
    layout->addStretch();
    layout->addWidget(buildOrderPanel());
}

QFrame *ProductPage::buildOrderPanel()
{
    m_sizeCombo = new QComboBox;
    m_sizeCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_sizeCombo, &QComboBox::currentIndexChanged, this, &ProductPage::updateQuantityLimit);
    m_quantitySpin = new QSpinBox;
    m_quantitySpin->setToolTip(QStringLiteral("Количество ограничено числом пар в наличии"));
    m_stockHint = new QLabel;
    m_stockHint->setObjectName(QStringLiteral("hint"));
    m_addButton = Widgets::accentButton(QStringLiteral("Добавить в заказ"));
    connect(m_addButton, &QPushButton::clicked, this, &ProductPage::addToOrder);

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *layout = new QHBoxLayout(panel);
    layout->addWidget(new QLabel(QStringLiteral("Размер:")));
    layout->addWidget(m_sizeCombo);
    layout->addWidget(new QLabel(QStringLiteral("Количество пар:")));
    layout->addWidget(m_quantitySpin);
    layout->addWidget(m_stockHint);
    layout->addStretch();
    layout->addWidget(m_addButton);
    return panel;
}

void ProductPage::onActivated()
{
    std::optional<Product> product;
    std::vector<StockPosition> positions;
    try {
        product = m_context.products.getProduct(m_productId, QDate::currentDate());
        if (product)
            positions = m_context.products.listPositions(m_productId);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    if (!product) {
        Messages::showWarning(this, QStringLiteral("Этого товара больше нет в каталоге."));
        navigator().goBack();
        return;
    }
    m_product = product;
    showProduct(*product);
    showPositions(positions);
}

void ProductPage::showProduct(const Product &product)
{
    m_image->setPixmap(Images::productPixmap(product.imageFile, imageSize));
    m_name->setText(product.name);
    m_manufacturer->setText(product.manufacturer);
    m_category->setText(product.category);
    // Цена всегда с учётом скидки; если скидки нет, подпись не обещает её
    m_priceCaption->setText(product.hasDiscount() ? QStringLiteral("Цена со скидкой:")
                                                  : QStringLiteral("Цена:"));
    m_price->setText(Formatting::money(product.price));
    m_oldPrice->setText(product.hasDiscount() ? Formatting::money(product.basePrice) : QString());
    m_composition->setText(product.composition);
    m_description->setText(product.description);
}

int ProductPage::freeQuantity(const StockPosition &position) const
{
    // Пары, которые ещё можно добавить: остаток минус уже выбранные в заказ
    return position.quantity - m_context.draft.reserved(position.stockItemId);
}

void ProductPage::showPositions(const std::vector<StockPosition> &positions)
{
    QStringList sizeRange;
    QStringList availableSizes;
    m_availablePositions.clear();
    for (const StockPosition &position : positions) {
        sizeRange << Formatting::size(position.sizeTenths);
        if (freeQuantity(position) > 0) {
            m_availablePositions.push_back(position);
            availableSizes << QStringLiteral("%1 — %2").arg(Formatting::size(position.sizeTenths),
                                                          Formatting::pairs(freeQuantity(position)));
        }
    }
    m_sizeRange->setText(sizeRange.join(QStringLiteral(", ")));
    m_availableSizes->setText(
        availableSizes.isEmpty()
            ? QStringLiteral("нет: все пары этой модели закончились или уже добавлены в заказ")
            : availableSizes.join(QStringLiteral("; ")));

    {
        const QSignalBlocker blocker(m_sizeCombo);
        m_sizeCombo->clear();
        for (const StockPosition &position : m_availablePositions)
            m_sizeCombo->addItem(Formatting::size(position.sizeTenths));
    }
    const bool hasSizes = !m_availablePositions.empty();
    m_sizeCombo->setEnabled(hasSizes);
    m_quantitySpin->setEnabled(hasSizes);
    m_addButton->setEnabled(hasSizes);
    updateQuantityLimit();
}

const StockPosition *ProductPage::selectedPosition() const
{
    const int index = m_sizeCombo->currentIndex();
    if (index < 0 || index >= static_cast<int>(m_availablePositions.size()))
        return nullptr;
    return &m_availablePositions[static_cast<std::size_t>(index)];
}

void ProductPage::updateQuantityLimit()
{
    // Поле количества не позволяет выбрать больше пар, чем доступно
    const StockPosition *position = selectedPosition();
    if (!position) {
        m_quantitySpin->setRange(0, 0);
        m_stockHint->clear();
        return;
    }
    const int free = freeQuantity(*position);
    m_quantitySpin->setRange(1, free);
    m_stockHint->setText(QStringLiteral("в наличии ") + Formatting::pairs(free));
}

void ProductPage::addToOrder()
{
    const StockPosition *position = selectedPosition();
    if (!position || !m_product) {
        Messages::showWarning(this, QStringLiteral("Выберите размер из списка доступных."));
        return;
    }
    const int quantity = m_quantitySpin->value();
    try {
        m_context.draft.add(*m_product, *position, quantity);
    } catch (const OrderDraftError &error) {
        Messages::showWarning(this, error.message());
        return;
    }
    const QString added = QStringLiteral("%1, размер %2, %3")
                              .arg(m_product->name, Formatting::size(position->sizeTenths),
                                   Formatting::pairs(quantity));
    if (Messages::askChoice(this,
                            QStringLiteral("В заказ добавлено: %1.\n\n"
                                           "Оформить заказ сейчас или продолжить выбор товаров?")
                                .arg(added),
                            QStringLiteral("Оформить заказ"), QStringLiteral("Продолжить выбор"))) {
        navigator().showOrderDraft();
    } else {
        navigator().goBack();
    }
}
