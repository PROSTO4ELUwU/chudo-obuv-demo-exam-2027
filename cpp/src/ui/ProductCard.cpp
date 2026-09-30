#include "ui/ProductCard.h"

#include "Formatting.h"
#include "ui/Images.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QVBoxLayout>

namespace {

constexpr QSize imageSize(160, 120);

QLabel *label(const QString &text, const QString &objectName = {})
{
    auto *label = new QLabel(text);
    label->setObjectName(objectName);
    return label;
}

} // namespace

ProductCard::ProductCard(const Product &product, bool clickable, QWidget *parent)
    : QFrame(parent)
    , m_clickable(clickable)
{
    setObjectName(QStringLiteral("productCard"));
    setProperty("lowStock", product.isRunningOut());
    // Растяжки внутри карточки иначе делают её растущей по высоте,
    // и единственная найденная карточка занимала бы весь экран
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    if (clickable) {
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral("Нажмите, чтобы открыть товар и выбрать размер"));
    }

    auto *image = new QLabel;
    image->setFixedSize(imageSize);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(Images::productPixmap(product.imageFile, imageSize));

    QLabel *title = label(product.manufacturer + QStringLiteral(" | ") + product.name,
                          QStringLiteral("productTitle"));
    title->setWordWrap(true);
    QLabel *composition = label(QStringLiteral("Состав: ") + product.composition,
                                QStringLiteral("composition"));
    composition->setWordWrap(true);

    auto *info = new QVBoxLayout;
    info->addWidget(title);
    info->addWidget(label(QStringLiteral("Категория: ") + product.category));
    info->addWidget(label(QStringLiteral("Количество: ") + product.quantityText()));
    info->addWidget(composition);
    info->addStretch();

    auto *prices = new QVBoxLayout;
    if (product.hasDiscount()) {
        prices->addWidget(label(Formatting::money(product.basePrice), QStringLiteral("oldPrice")),
                          0, Qt::AlignRight);
    }
    prices->addWidget(label(Formatting::money(product.price), QStringLiteral("price")), 0,
                      Qt::AlignRight);
    if (product.hasDiscount()) {
        prices->addWidget(label(QStringLiteral("скидка %1 %").arg(product.discount),
                                QStringLiteral("hint")),
                          0, Qt::AlignRight);
    }
    prices->addStretch();

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 16, 10);
    layout->setSpacing(16);
    layout->addWidget(image);
    layout->addLayout(info, 1);
    layout->addLayout(prices);
}

void ProductCard::mousePressEvent(QMouseEvent *event)
{
    if (m_clickable && event->button() == Qt::LeftButton)
        emit clicked();
    QFrame::mousePressEvent(event);
}
