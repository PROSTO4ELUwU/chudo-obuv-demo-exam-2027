#pragma once

#include "ui/Page.h"

class QLineEdit;

// Вход только по логину из списка пользователей, без пароля, или вход гостем
class LoginPage : public Page
{
    Q_OBJECT

public:
    explicit LoginPage(AppContext &context);

    QString title() const override { return QStringLiteral("Вход в систему"); }

    // После выхода из системы поле логина пустое и готово к вводу
    void onActivated() override;

private:
    void login();
    void showLoginHint();

    QLineEdit *m_loginEdit = nullptr;
};
