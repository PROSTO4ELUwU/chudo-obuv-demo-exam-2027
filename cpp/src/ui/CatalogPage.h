#pragma once

#include "ui/Page.h"

#include <QHash>

#include <vector>

class ProductCard;
class QComboBox;
class QFrame;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

// Главная форма: каталог моделей обуви в виде карточек. Авторизованным
// пользователям доступны поиск, фильтр по категории и сортировка по цене;
// они применяются сразу при изменении условий. Гость видит каталог
// без этих инструментов. В режиме выбора товаров (selectForOrder) каталог
// открыт из списка заказов для нового заказа.
class CatalogPage : public Page
{
    Q_OBJECT

public:
    explicit CatalogPage(AppContext &context, bool selectForOrder = false);

    QString title() const override;

    bool isSelectingForOrder() const { return m_selectForOrder; }

    // Каталог перечитывается при каждом показе: остатки могли измениться
    void onActivated() override;

private:
    QFrame *buildFilterPanel();
    QHBoxLayout *buildActions();
    void updateDraftButton();
    void fillCategories(const QStringList &categories);
    void createCards();
    void applyFilters();

    bool m_selectForOrder = false;
    std::vector<Product> m_products;
    QHash<int, ProductCard *> m_cards;
    QLineEdit *m_searchEdit = nullptr;
    QComboBox *m_categoryCombo = nullptr;
    QComboBox *m_sortCombo = nullptr;
    QLabel *m_foundLabel = nullptr;
    QPushButton *m_draftButton = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QWidget *m_cardsContainer = nullptr;
    QVBoxLayout *m_cardsLayout = nullptr;
};
