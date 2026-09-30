#pragma once

#include <QString>

#include <stdexcept>

// Ошибка приложения: её текст понятен пользователю и показывается в окне сообщения
class AppError : public std::runtime_error
{
public:
    explicit AppError(const QString &message)
        : std::runtime_error(message.toStdString())
    {
    }

    QString message() const { return QString::fromStdString(what()); }
};

// Нет файла настроек или в нём не хватает параметров
class ConfigError : public AppError
{
    using AppError::AppError;
};

// Сбой обращения к базе данных; текст — подробности от драйвера
class DatabaseError : public AppError
{
    using AppError::AppError;
};

// Операция с заказом недопустима по правилам предметной области
class OrderError : public AppError
{
    using AppError::AppError;
};
