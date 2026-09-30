#include "OrderDraft.h"

#include "Formatting.h"

#include <algorithm>
#include <stdexcept>

Money OrderDraft::total() const
{
    Money sum;
    for (const DraftLine &line : m_lines)
        sum += line.total();
    return sum;
}

int OrderDraft::pairsCount() const
{
    int count = 0;
    for (const DraftLine &line : m_lines)
        count += line.quantity;
    return count;
}

int OrderDraft::reserved(int stockItemId) const
{
    const DraftLine *line = find(stockItemId);
    return line ? line->quantity : 0;
}

void OrderDraft::add(const Product &product, const StockPosition &position, int quantity)
{
    if (quantity < 1)
        throw OrderDraftError(QStringLiteral("Количество пар должно быть не меньше одной."));
    const int alreadyReserved = reserved(position.stockItemId);
    if (alreadyReserved + quantity > position.quantity) {
        throw OrderDraftError(
            QStringLiteral("Размер %1: в наличии %2, в заказе уже %3, можно добавить: %4.")
                .arg(Formatting::size(position.sizeTenths), Formatting::pairs(position.quantity),
                     Formatting::pairs(alreadyReserved),
                     Formatting::pairs(position.quantity - alreadyReserved)));
    }
    if (DraftLine *line = find(position.stockItemId)) {
        line->quantity += quantity;
        line->available = position.quantity;
        return;
    }
    m_lines.push_back({position.stockItemId, product.name, product.manufacturer,
                       position.sizeTenths, product.price, quantity, position.quantity});
}

void OrderDraft::setQuantity(int stockItemId, int quantity)
{
    DraftLine *line = find(stockItemId);
    if (!line)
        throw std::out_of_range("Позиции нет в формируемом заказе");
    if (quantity < 1 || quantity > line->available) {
        throw OrderDraftError(
            QStringLiteral("Размер %1: можно заказать от 1 до %2. "
                           "Чтобы убрать позицию, удалите её из заказа.")
                .arg(Formatting::size(line->sizeTenths), Formatting::pairs(line->available)));
    }
    line->quantity = quantity;
}

void OrderDraft::updateAvailable(int stockItemId, int available)
{
    DraftLine *line = find(stockItemId);
    if (!line)
        throw std::out_of_range("Позиции нет в формируемом заказе");
    line->available = available;
    if (available > 0)
        line->quantity = std::min(line->quantity, available);
}

void OrderDraft::remove(int stockItemId)
{
    std::erase_if(m_lines, [stockItemId](const DraftLine &line) {
        return line.stockItemId == stockItemId;
    });
}

DraftLine *OrderDraft::find(int stockItemId)
{
    const auto line = std::ranges::find(m_lines, stockItemId, &DraftLine::stockItemId);
    return line == m_lines.end() ? nullptr : &*line;
}

const DraftLine *OrderDraft::find(int stockItemId) const
{
    const auto line = std::ranges::find(m_lines, stockItemId, &DraftLine::stockItemId);
    return line == m_lines.end() ? nullptr : &*line;
}
