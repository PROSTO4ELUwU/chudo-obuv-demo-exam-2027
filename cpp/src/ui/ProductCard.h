#pragma once

#include "Models.h"

#include <QFrame>

// Карточка товара в каталоге по макету: изображение, «Производство |
// Наименование», категория, количество, состав и цена. Товары, которых
// осталось три пары и меньше, подсвечиваются цветом #FF8080.
class ProductCard : public QFrame
{
    Q_OBJECT

public:
    ProductCard(const Product &product, bool clickable, QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    // Нажатие на карточку открывает товар, если пользователю доступен заказ
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool m_clickable = false;
};
