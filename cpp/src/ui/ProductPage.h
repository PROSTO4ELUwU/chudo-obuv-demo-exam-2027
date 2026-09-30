#pragma once

#include "ui/Page.h"

#include <optional>
#include <vector>

class QComboBox;
class QFrame;
class QLabel;
class QPushButton;
class QSpinBox;

// Данные выбранной модели и добавление товарной позиции в заказ. При каждом
// показе модель и её размеры читаются из базы заново, а доступное количество
// уменьшается на пары, уже добавленные в заказ.
class ProductPage : public Page
{
    Q_OBJECT

public:
    ProductPage(AppContext &context, int productId);

    QString title() const override { return QStringLiteral("Просмотр товара"); }

    void onActivated() override;

private:
    QFrame *buildOrderPanel();
    void showProduct(const Product &product);
    void showPositions(const std::vector<StockPosition> &positions);
    int freeQuantity(const StockPosition &position) const;
    const StockPosition *selectedPosition() const;
    void updateQuantityLimit();
    void addToOrder();

    int m_productId = 0;
    std::optional<Product> m_product;
    std::vector<StockPosition> m_availablePositions;

    QLabel *m_image = nullptr;
    QLabel *m_name = nullptr;
    QLabel *m_manufacturer = nullptr;
    QLabel *m_category = nullptr;
    QLabel *m_price = nullptr;
    QLabel *m_oldPrice = nullptr;
    QLabel *m_composition = nullptr;
    QLabel *m_description = nullptr;
    QLabel *m_sizeRange = nullptr;
    QLabel *m_availableSizes = nullptr;
    QComboBox *m_sizeCombo = nullptr;
    QSpinBox *m_quantitySpin = nullptr;
    QLabel *m_stockHint = nullptr;
    QPushButton *m_addButton = nullptr;
};
