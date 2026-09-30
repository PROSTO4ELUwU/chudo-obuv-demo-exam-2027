#pragma once

#include <QApplication>

// Приложение с последним рубежом обработки ошибок: исключение, не пойманное
// в обработчике события, показывается пользователю, а не закрывает программу.
// Ожидаемые ошибки (база данных, правила заказа) обрабатываются там, где
// возникли: Qt не рассчитан на исключения, проходящие через его код.
class Application : public QApplication
{
public:
    using QApplication::QApplication;

    bool notify(QObject *receiver, QEvent *event) override;
};

// Оформление по руководству по стилю и русский перевод стандартных диалогов
void configureApplication(QApplication &app);
