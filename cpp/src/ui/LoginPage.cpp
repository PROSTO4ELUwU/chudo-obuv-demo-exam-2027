#include "ui/LoginPage.h"

#include "ui/Images.h"
#include "ui/Messages.h"
#include "ui/Widgets.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QToolTip>
#include <QVBoxLayout>
#include <QValidator>

#include <algorithm>

namespace {

constexpr QSize logoSize(140, 140);

// Логин: латинские буквы, цифры, точка, дефис и подчёркивание. Пробелы
// и переносы строк отбрасываются: при копировании логина из ячейки таблицы
// или из текста они попадают в буфер обмена, и вставка (Ctrl+V) иначе
// отклонялась бы целиком
class LoginValidator : public QValidator
{
public:
    using QValidator::QValidator;

    State validate(QString &input, int &pos) const override
    {
        const auto isSpace = [](QChar ch) { return ch.isSpace(); };
        pos -= static_cast<int>(std::count_if(input.cbegin(), input.cbegin() + pos, isSpace));
        input.removeIf(isSpace);
        static const QRegularExpression login(
            QRegularExpression::anchoredPattern(QStringLiteral("[A-Za-z0-9._-]*")));
        return login.match(input).hasMatch() ? Acceptable : Invalid;
    }
};

} // namespace

LoginPage::LoginPage(AppContext &context)
    : Page(context)
{
    auto *logo = new QLabel;
    logo->setAlignment(Qt::AlignCenter);
    logo->setPixmap(Images::logo(logoSize));

    auto *heading = new QLabel(QStringLiteral("Вход в систему"));
    heading->setObjectName(QStringLiteral("appTitle"));
    heading->setAlignment(Qt::AlignCenter);
    auto *hint = new QLabel(QStringLiteral("Введите логин из списка пользователей.\n"
                                           "Пароль не требуется."));
    hint->setObjectName(QStringLiteral("hint"));
    hint->setAlignment(Qt::AlignCenter);

    m_loginEdit = new QLineEdit;
    m_loginEdit->setPlaceholderText(QStringLiteral("Логин"));
    m_loginEdit->setMaxLength(50);
    m_loginEdit->setClearButtonEnabled(true);
    m_loginEdit->setValidator(new LoginValidator(m_loginEdit));
    connect(m_loginEdit, &QLineEdit::inputRejected, this, &LoginPage::showLoginHint);
    connect(m_loginEdit, &QLineEdit::returnPressed, this, &LoginPage::login);

    QPushButton *loginButton = Widgets::accentButton(QStringLiteral("Войти"));
    connect(loginButton, &QPushButton::clicked, this, &LoginPage::login);
    auto *guestButton = new QPushButton(QStringLiteral("Войти как гость"));
    guestButton->setToolTip(QStringLiteral("Без входа доступен только просмотр каталога"));
    connect(guestButton, &QPushButton::clicked, this,
            [this] { navigator().login(User::guest()); });

    auto *form = new QFrame;
    form->setFixedWidth(360);
    auto *formLayout = new QVBoxLayout(form);
    formLayout->setSpacing(10);
    formLayout->addWidget(logo);
    formLayout->addWidget(heading);
    formLayout->addWidget(hint);
    formLayout->addWidget(m_loginEdit);
    formLayout->addWidget(loginButton);
    formLayout->addWidget(guestButton);

    auto *layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(form, 0, Qt::AlignCenter);
    layout->addStretch(2);
}

void LoginPage::onActivated()
{
    m_loginEdit->clear();
    m_loginEdit->setFocus();
}

void LoginPage::showLoginHint()
{
    // Подсказка при попытке ввести недопустимый символ, например кириллицу
    const QPoint position = m_loginEdit->mapToGlobal(QPoint(0, m_loginEdit->height()));
    QToolTip::showText(position,
                       QStringLiteral("Логин вводится латинскими буквами и цифрами — "
                                      "проверьте раскладку клавиатуры"),
                       m_loginEdit);
}

void LoginPage::login()
{
    const QString login = m_loginEdit->text().trimmed();
    if (login.isEmpty()) {
        Messages::showWarning(this, QStringLiteral("Введите логин.\nБез логина можно войти "
                                                   "как гость и просматривать каталог."));
        m_loginEdit->setFocus();
        return;
    }
    std::optional<User> user;
    try {
        user = m_context.users.findByLogin(login);
    } catch (const DatabaseError &error) {
        Messages::showDatabaseError(this, error.message());
        return;
    }
    if (!user) {
        Messages::showError(this, QStringLiteral("Пользователь с логином «%1» не найден.\n"
                                                 "Проверьте правильность ввода "
                                                 "или войдите как гость.")
                                      .arg(login));
        m_loginEdit->selectAll();
        m_loginEdit->setFocus();
        return;
    }
    navigator().login(*user);
}
