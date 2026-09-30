#pragma once

#include "Errors.h"
#include "Models.h"

#include <vector>

// Недопустимое изменение формируемого заказа; текст показывается пользователю
class OrderDraftError : public AppError
{
    using AppError::AppError;
};

// Позиция формируемого заказа
struct DraftLine
{
    int stockItemId = 0;
    QString productName;
    QString manufacturer;
    int sizeTenths = 0;
    Money unitPrice;
    int quantity = 0;
    int available = 0;

    Money total() const { return unitPrice * quantity; }

    // В позиции больше пар, чем осталось (после проверки при подтверждении)
    bool exceedsStock() const { return quantity > available; }
};

// Позиции, которые пользователь добавил в заказ, но ещё не подтвердил.
// Остатки в базе не меняются, пока заказ не подтверждён: проверка
// и списание выполняются одной транзакцией при сохранении заказа.
class OrderDraft
{
public:
    // Позиции в порядке добавления
    const std::vector<DraftLine> &lines() const { return m_lines; }

    bool isEmpty() const { return m_lines.empty(); }
    Money total() const;
    int pairsCount() const;

    // Сколько пар этой товарной позиции уже в заказе
    int reserved(int stockItemId) const;

    // Добавляет пары; повторный выбор того же размера увеличивает количество
    void add(const Product &product, const StockPosition &position, int quantity);

    // Изменяет количество пар в позиции в пределах остатка
    void setQuantity(int stockItemId, int quantity);

    // Запоминает остаток из базы и уменьшает количество в позиции до него.
    // Позиция, которой не осталось совсем, не удаляется молча:
    // пользователь увидит её выделенной и удалит сам.
    void updateAvailable(int stockItemId, int available);

    void remove(int stockItemId);
    void clear() { m_lines.clear(); }

private:
    DraftLine *find(int stockItemId);
    const DraftLine *find(int stockItemId) const;

    std::vector<DraftLine> m_lines;
};
