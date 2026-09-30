#pragma once

#include "Models.h"
#include "OrderDraft.h"
#include "Repositories.h"

#include <QWidget>

// Переходы между страницами; реализуются главным окном
class Navigator
{
public:
    virtual ~Navigator() = default;

    virtual void login(const User &user) = 0;
    virtual void logout() = 0;
    virtual void goBack() = 0;
};

// Всё, что нужно страницам: доступ к данным, пользователь и его заказ
struct AppContext
{
    AppContext(UserRepository &users, ProductRepository &products, OrderRepository &orders)
        : users(users)
        , products(products)
        , orders(orders)
    {
    }

    UserRepository &users;
    ProductRepository &products;
    OrderRepository &orders;
    User user = User::guest();
    OrderDraft draft;
    Navigator *navigator = nullptr;
};

// Страница, которую главное окно показывает в стеке навигации
class Page : public QWidget
{
    Q_OBJECT

public:
    explicit Page(AppContext &context);

    // Заголовок окна и шапки, соответствующий назначению страницы
    virtual QString title() const = 0;

    // Вызывается при каждом показе страницы, в том числе при возврате «Назад»
    virtual void onActivated() {}

protected:
    Navigator &navigator() const { return *m_context.navigator; }

    AppContext &m_context;
};
