#include "ui/MainWindow.h"

#include "ui/CatalogPage.h"
#include "ui/LoginPage.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {

constexpr int logoSize = 56;
const QString appName = QStringLiteral("Чудо Обувь");

} // namespace

MainWindow::MainWindow(AppContext &context)
    : m_context(context)
    , m_stack(new QStackedWidget)
{
    context.navigator = this;

    auto *central = new QWidget;
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildHeader());
    layout->addWidget(m_stack, 1);
    setCentralWidget(central);
    resize(1180, 780);
    setMinimumSize(960, 640);
    resetStack(new LoginPage(context));
}

QWidget *MainWindow::buildHeader()
{
    m_backButton = new QPushButton(QStringLiteral("← Назад"));
    m_backButton->setToolTip(QStringLiteral("Вернуться на предыдущую страницу"));
    connect(m_backButton, &QPushButton::clicked, this, &MainWindow::goBack);

    auto *logo = new QLabel;
    logo->setPixmap(QPixmap(QStringLiteral(":/logo.png"))
                        .scaled(logoSize, logoSize, Qt::KeepAspectRatio,
                                Qt::SmoothTransformation));
    auto *appTitle = new QLabel(appName);
    appTitle->setObjectName(QStringLiteral("appTitle"));
    m_pageTitle = new QLabel;
    m_pageTitle->setObjectName(QStringLiteral("pageTitle"));
    auto *titles = new QVBoxLayout;
    titles->setSpacing(0);
    titles->addWidget(appTitle);
    titles->addWidget(m_pageTitle);

    m_userName = new QLabel;
    m_userName->setObjectName(QStringLiteral("userName"));
    m_userName->setAlignment(Qt::AlignRight);
    m_userRole = new QLabel;
    m_userRole->setObjectName(QStringLiteral("userRole"));
    m_userRole->setAlignment(Qt::AlignRight);
    auto *userBox = new QVBoxLayout;
    userBox->setSpacing(0);
    userBox->addWidget(m_userName);
    userBox->addWidget(m_userRole);
    m_logoutButton = new QPushButton;
    connect(m_logoutButton, &QPushButton::clicked, this, &MainWindow::logout);

    auto *header = new QFrame;
    header->setObjectName(QStringLiteral("header"));
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(16, 8, 16, 8);
    layout->addWidget(m_backButton);
    layout->addWidget(logo);
    layout->addLayout(titles);
    layout->addStretch();
    layout->addLayout(userBox);
    layout->addWidget(m_logoutButton);
    return header;
}

void MainWindow::login(const User &user)
{
    m_context.user = user;
    resetStack(new CatalogPage(m_context));
}

void MainWindow::logout()
{
    m_context.user = User::guest();
    resetStack(new LoginPage(m_context));
}

void MainWindow::goBack()
{
    if (m_stack->count() > 1) {
        removeCurrentPage();
        activate(static_cast<Page *>(m_stack->currentWidget()));
    }
}

void MainWindow::openPage(Page *page)
{
    m_stack->addWidget(page);
    m_stack->setCurrentWidget(page);
    activate(page);
}

void MainWindow::removeCurrentPage()
{
    QWidget *page = m_stack->currentWidget();
    m_stack->removeWidget(page);
    page->deleteLater();
}

void MainWindow::resetStack(Page *page)
{
    while (m_stack->count() > 0)
        removeCurrentPage();
    openPage(page);
}

void MainWindow::activate(Page *page)
{
    setWindowTitle(appName + QStringLiteral(" — ") + page->title());
    m_pageTitle->setText(page->title());
    m_backButton->setVisible(m_stack->count() > 1);

    const User &user = m_context.user;
    const bool signedIn = qobject_cast<LoginPage *>(page) == nullptr;
    const bool showRole = signedIn && user.role != Role::Guest;
    m_userName->setText(signedIn ? user.fullName() : QString());
    m_userRole->setText(showRole ? roleName(user.role) : QString());
    m_logoutButton->setText(user.role == Role::Guest ? QStringLiteral("Войти")
                                                     : QStringLiteral("Выйти"));
    m_logoutButton->setVisible(signedIn);
    page->onActivated();
}
