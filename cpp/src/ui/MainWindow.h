#pragma once

#include "ui/Page.h"

#include <QMainWindow>

class QLabel;
class QPushButton;
class QStackedWidget;

// Главное окно приложения — последовательный интерфейс. Страницы открываются
// поверх друг друга в QStackedWidget, кнопка «Назад» возвращает на предыдущую.
// После входа первой страницей становится каталог, а ФИО пользователя
// выводится в правом верхнем углу.
class MainWindow : public QMainWindow, public Navigator
{
    Q_OBJECT

public:
    explicit MainWindow(AppContext &context);

    // Показывает окно, а если оно не помещается на экране — развёрнутым.
    // Окно 1180×780 не помещается по высоте на мониторе 1366×768 и на ноутбуке
    // 1920×1080 с масштабом 150 %: нижний край с кнопками ушёл бы под панель задач.
    void showOnScreen();

    void login(const User &user) override;
    void logout() override;
    void showProduct(int productId) override;
    void showOrderDraft() override;
    void showOrders() override;
    void showOrderDetails(int orderId) override;
    void showCatalogForOrder() override;
    void returnAfterOrder() override;
    void goBack() override;

protected:
    // Закрытие окна с неподтверждённым заказом требует подтверждения
    void closeEvent(QCloseEvent *event) override;

private:
    bool confirmDraftLoss();
    QWidget *buildHeader();
    void openPage(Page *page);
    void removeCurrentPage();
    void resetStack(Page *page);
    void activate(Page *page);

    AppContext &m_context;
    QStackedWidget *m_stack = nullptr;
    QPushButton *m_backButton = nullptr;
    QLabel *m_pageTitle = nullptr;
    QLabel *m_userName = nullptr;
    QLabel *m_userRole = nullptr;
    QPushButton *m_logoutButton = nullptr;
};
