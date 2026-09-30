#include "ui/Messages.h"

#include <QMessageBox>
#include <QPushButton>

namespace Messages {

void showError(QWidget *parent, const QString &text)
{
    QMessageBox::critical(parent, QStringLiteral("Ошибка"), text);
}

void showWarning(QWidget *parent, const QString &text)
{
    QMessageBox::warning(parent, QStringLiteral("Предупреждение"), text);
}

void showInfo(QWidget *parent, const QString &text)
{
    QMessageBox::information(parent, QStringLiteral("Информация"), text);
}

void showDatabaseError(QWidget *parent, const QString &details)
{
    showError(parent, QStringLiteral("Не удалось выполнить операцию с базой данных.\n"
                                     "Проверьте, что сервер PostgreSQL запущен, "
                                     "и повторите действие.\n\nПодробности: %1")
                          .arg(details));
}

bool askChoice(QWidget *parent, const QString &text, const QString &acceptText,
               const QString &rejectText)
{
    QMessageBox box(QMessageBox::Question, QStringLiteral("Вопрос"), text,
                    QMessageBox::NoButton, parent);
    QPushButton *acceptButton = box.addButton(acceptText, QMessageBox::AcceptRole);
    box.addButton(rejectText, QMessageBox::RejectRole);
    box.setDefaultButton(acceptButton);
    box.exec();
    return box.clickedButton() == acceptButton;
}

bool askConfirmation(QWidget *parent, const QString &text)
{
    const auto answer = QMessageBox::question(parent, QStringLiteral("Подтверждение"), text,
                                              QMessageBox::Yes | QMessageBox::No,
                                              QMessageBox::No);
    return answer == QMessageBox::Yes;
}

} // namespace Messages
