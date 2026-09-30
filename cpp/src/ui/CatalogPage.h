#pragma once

#include "ui/Page.h"

#include <QHash>

#include <vector>

class ProductCard;
class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class QVBoxLayout;

// Главная форма: каталог моделей обуви в виде карточек. Авторизованным
// пользователям доступны поиск, фильтр по категории и сортировка по цене;
// они применяются сразу при изменении условий. Гость видит каталог
// без этих инструментов.
class CatalogPage : public Page
{
    Q_OBJECT

public:
    explicit CatalogPage(AppContext &context);

    QString title() const override { return QStringLiteral("Каталог товаров"); }

    // Каталог перечитывается при каждом показе: остатки могли измениться
    void onActivated() override;

private:
    QFrame *buildFilterPanel();
    void fillCategories(const QStringList &categories);
    void createCards();
    void applyFilters();

    std::vector<Product> m_products;
    QHash<int, ProductCard *> m_cards;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QLabel *m_foundLabel = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_cardsContainer = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
};
