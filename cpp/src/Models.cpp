#include "Models.h"

#include <QStringList>

QString roleName(Role role)
{
    switch (role) {
    case Role::Admin:
        return QStringLiteral("Администратор");
    case Role::Manager:
        return QStringLiteral("Менеджер");
    case Role::Client:
        return QStringLiteral("Авторизованный пользователь");
    case Role::Guest:
        break;
    }
    return QStringLiteral("Гость");
}

std::optional<Role> roleFromName(const QString &name)
{
    for (const Role role : {Role::Admin, Role::Manager, Role::Client}) {
        if (roleName(role) == name)
            return role;
    }
    return std::nullopt;
}

User User::guest()
{
    User user;
    user.lastName = QStringLiteral("Гость");
    return user;
}

QString User::fullName() const
{
    QStringList parts;
    for (const QString &part : {lastName, firstName, patronymic}) {
        if (!part.isEmpty())
            parts << part;
    }
    return parts.join(u' ');
}

QString Product::quantityText() const
{
    return totalQuantity > manyThreshold ? QStringLiteral("много") : QStringLiteral("мало");
}
