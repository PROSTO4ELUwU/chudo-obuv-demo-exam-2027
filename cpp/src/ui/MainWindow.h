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

    void login(const User &user) override;
    void logout() override;
    void goBack() override;

private:
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
