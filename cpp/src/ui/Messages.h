#pragma once

#include <QString>

class QWidget;

// Окна сообщений: заголовок и значок соответствуют типу сообщения
namespace Messages {

// Ошибка: действие не выполнено
void showError(QWidget *parent, const QString &text);

// Предупреждение: пользователь ввёл неверные данные или действие запрещено
void showWarning(QWidget *parent, const QString &text);

// Информация о результате действия
void showInfo(QWidget *parent, const QString &text);

// Ошибка обращения к базе данных с порядком действий для пользователя
void showDatabaseError(QWidget *parent, const QString &details);

// Вопрос с двумя вариантами ответа; true — выбран первый вариант
bool askChoice(QWidget *parent, const QString &text, const QString &acceptText,
               const QString &rejectText);

// Подтверждение необратимого действия; по умолчанию выбрано «Нет»
bool askConfirmation(QWidget *parent, const QString &text);

} // namespace Messages
