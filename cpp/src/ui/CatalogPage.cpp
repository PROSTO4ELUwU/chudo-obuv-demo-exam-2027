#include "ui/CatalogPage.h"

#include "CatalogFilter.h"
#include "Formatting.h"
#include "ui/Messages.h"
#include "ui/ProductCard.h"
#include "ui/Widgets.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

    m_emptyLabel = new QLabel(QStringLiteral("Товары не найдены. Измените строку поиска "
                                             "или выберите другую категорию."));
    m_emptyLabel->setObjectName(QStringLiteral("hint"));
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_emptyLabel->hide();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 12, 16, 12);
    layout->addWidget(buildFilterPanel());
    layout->addLayout(buildActions());
    layout->addWidget(m_emptyLabel);
    layout->addWidget(scroll, 1);
}

QHBoxLayout *CatalogPage::buildActions()
{
    // Переход к формируемому заказу
    m_draftButton = new QPushButton;
    m_draftButton->setToolTip(QStringLiteral("Просмотреть выбранные товары и подтвердить заказ"));
    connect(m_draftButton, &QPushButton::clicked, this, [this] { navigator().showOrderDraft(); });
    Widgets::hideUnless(m_context.user.canOrder(), {m_draftButton});

    auto *layout = new QHBoxLayout;
    layout->addStretch();
    layout->addWidget(m_draftButton);
    return layout;
}

void CatalogPage::updateDraftButton()
{
    // Кнопка формируемого заказа показывает количество выбранных пар
    const OrderDraft &draft = m_context.draft;
    m_draftButton->setEnabled(!draft.isEmpty());
    m_draftButton->setText(draft.isEmpty()
                               ? QStringLiteral("Формируемый заказ")
                               : QStringLiteral("Формируемый заказ: ")
                                     + Formatting::pairs(draft.pairsCount()));
}

QFrame *CatalogPage::buildFilterPanel()
{
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("Наименование или описание товара"));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CatalogPage::applyFilters);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &CatalogPage::applyFilters);

    m_sortCombo = new QComboBox;
    m_sortCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    for (const SortOrder order : allSortOrders)
        m_sortCombo->addItem(sortOrderName(order), static_cast<int>(order));
    connect(m_sortCombo, &QComboBox::currentIndexChanged, this, &CatalogPage::applyFilters);

    m_foundLabel = new QLabel;
    m_foundLabel->setObjectName(QStringLiteral("hint"));

    auto *panel = new QFrame;
    panel->setObjectName(QStringLiteral("panel"));
    auto *layout = new QHBoxLayout(panel);
    layout->addWidget(new QLabel(QStringLiteral("Поиск:")));
    layout->addWidget(m_searchEdit, 1);
    layout->addWidget(new QLabel(QStringLiteral("Категория:")));
    layout->addWidget(m_categoryCombo);
    layout->addWidget(new QLabel(QStringLiteral("Сортировка:")));
    layout->addWidget(m_sortCombo);
    layout->addWidget(m_foundLabel);
    Widgets::hideUnless(m_context.user.canOrder(), {panel});
    return panel;
}

void CatalogPage::onActivated()
{
    updateDraftButton();
    QStringList categories;
    try {
        m_products = m_context.products.listProducts(QDate::currentDate());
        categories = m_context.products.listCategories();
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    fillCategories(categories);
    createCards();
    applyFilters();
}

void CatalogPage::fillCategories(const QStringList &categories)
{
    // Первый пункт — «Все категории»; выбранная категория сохраняется при обновлении
    const QString selected = m_categoryCombo->currentText();
    const QSignalBlocker blocker(m_categoryCombo);
    m_categoryCombo->clear();
    m_categoryCombo->addItem(allCategoriesName());
    m_categoryCombo->addItems(categories);
    m_categoryCombo->setCurrentText(selected);
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
    // Карточки сразу получают родителя, поэтому удаляются вместе со страницей;
    // форму товара открывают только те, кому доступен заказ
    const bool clickable = m_context.user.canOrder();
    for (const Product &product : m_products) {
        auto *card = new ProductCard(product, clickable, m_cardsContainer);
        const int productId = product.id;
        connect(card, &ProductCard::clicked, this,
                [this, productId] { navigator().showProduct(productId); });
        m_cards.insert(productId, card);
    }
}

void CatalogPage::applyFilters()
{
    // Карточки не пересоздаются, а только переставляются в макете,
    // поэтому поиск работает без задержек на каждое нажатие клавиши
    const auto sortOrder = static_cast<SortOrder>(m_sortCombo->currentData().toInt());
    const std::vector<Product> visible = filterProducts(
        m_products, m_searchEdit->text(), m_categoryCombo->currentText(), sortOrder);
    while (m_cardsLayout->count() > 1)
        delete m_cardsLayout->takeAt(0);
    for (ProductCard *card : std::as_const(m_cards))
        card->hide();
    for (int index = 0; index < static_cast<int>(visible.size()); ++index) {
        ProductCard *card = m_cards.value(visible[index].id);
        m_cardsLayout->insertWidget(index, card);
        card->show();
    }
    m_emptyLabel->setVisible(visible.empty());
    m_foundLabel->setText(QStringLiteral("Найдено: %1 из %2").arg(visible.size()).arg(m_products.size()));
}
