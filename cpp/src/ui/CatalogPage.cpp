#include "ui/CatalogPage.h"

#include "ui/Messages.h"
#include "ui/ProductCard.h"

#include <QScrollArea>
#include <QVBoxLayout>

CatalogPage::CatalogPage(AppContext &context)
    : Page(context)
{
    m_cardsContainer = new QWidget;
    m_cardsContainer->setObjectName(QStringLiteral("cardsContainer"));
    m_cardsLayout = new QVBoxLayout(m_cardsContainer);
    m_cardsLayout->setSpacing(10);
    m_cardsLayout->addStretch();

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_cardsContainer);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addWidget(scroll, 1);
}

void CatalogPage::onActivated()
{
    try {
        m_products = m_context.products.listProducts(QDate::currentDate());
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    createCards();
    showCards();
}

void CatalogPage::createCards()
{
    for (ProductCard *card : std::as_const(m_cards)) {
        // deleteLater удаляет карточку только при возврате в цикл событий,
        // поэтому до этого она скрывается, чтобы не мелькать за диалогами
        card->hide();
        card->deleteLater();
    }
    m_cards.clear();
    // Карточки сразу получают родителя, поэтому удаляются вместе со страницей
    for (const Product &product : m_products)
        m_cards.insert(product.id, new ProductCard(product, false, m_cardsContainer));
}

void CatalogPage::showCards()
{
    while (m_cardsLayout->count() > 1)
        delete m_cardsLayout->takeAt(0);
    for (int index = 0; index < static_cast<int>(m_products.size()); ++index)
        m_cardsLayout->insertWidget(index, m_cards.value(m_products[index].id));
}
