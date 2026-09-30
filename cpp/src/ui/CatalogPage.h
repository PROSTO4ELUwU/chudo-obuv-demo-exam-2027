#pragma once

#include "ui/Page.h"

#include <QHash>

#include <vector>

class ProductCard;
class QLabel;
class QVBoxLayout;

// Главная форма: каталог моделей обуви в виде карточек
class CatalogPage : public Page
{
    Q_OBJECT

public:
    explicit CatalogPage(AppContext &context);

    QString title() const override { return QStringLiteral("Каталог товаров"); }

    // Каталог перечитывается при каждом показе: остатки могли измениться
    void onActivated() override;

private:
    void createCards();
    void showCards();

    std::vector<Product> m_products;
    QHash<int, ProductCard *> m_cards;
    QWidget *m_cardsContainer = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
};
