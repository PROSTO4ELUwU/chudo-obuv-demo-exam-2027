#pragma once

#include "Money.h"

#include <QDate>
#include <QString>

#include <optional>

// Правила отображения каталога (задание 2)
inline constexpr int manyThreshold = 5;
inline constexpr int lowStockThreshold = 3;

// Роли пользователей; названия совпадают с roles.role_name в базе
enum class Role { Admin, Manager, Client, Guest };

QString roleName(Role role);
std::optional<Role> roleFromName(const QString &name);

// Пользователь, вошедший в систему
struct User
{
    int id = 0;
    QString lastName;
    QString firstName;
    QString patronymic;
    Role role = Role::Guest;

    static User guest();

    QString fullName() const;

    // Фильтровать каталог и оформлять заказы может любой вошедший по логину
    bool canOrder() const { return role != Role::Guest; }

    // Список заказов, их состав, добавление и удаление
    bool canManageOrders() const { return role == Role::Admin || role == Role::Manager; }

    // Изменение даты заказа и удаление позиций
    bool canEditOrders() const { return role == Role::Admin; }
};

// Модель обуви в каталоге с ценой, рассчитанной на дату загрузки
struct Product
{
    int id = 0;
    QString name;
    QString category;
    QString manufacturer;
    QString description;
    QString composition;
    Money basePrice;
    int discount = 0;
    Money price;
    int totalQuantity = 0;
    QString imageFile;

    bool hasDiscount() const { return discount > 0; }

    // «Много» — больше пяти пар по всем размерам, иначе «мало»
    QString quantityText() const;

    // Товар подсвечивается в каталоге: осталось три пары или меньше
    bool isRunningOut() const { return totalQuantity <= lowStockThreshold; }
};

// Товарная позиция: размер модели (в десятых долях: 365 = 36,5) и остаток
struct StockPosition
{
    int stockItemId = 0;
    int sizeTenths = 0;
    int quantity = 0;
};

// Строка списка заказов
struct OrderSummary
{
    int id = 0;
    QDate date;
    QString clientName;
    Money total;
};

// Позиция сохранённого заказа
struct OrderLine
{
    int orderItemId = 0;
    QString productName;
    QString manufacturer;
    int sizeTenths = 0;
    int quantity = 0;
    Money unitPrice;

    Money total() const { return unitPrice * quantity; }
};
