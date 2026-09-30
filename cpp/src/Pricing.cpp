#include "Pricing.h"

namespace Pricing {

Period previousMonth(QDate calculationDate)
{
    const QDate end(calculationDate.year(), calculationDate.month(), 1);
    return {end.addMonths(-1), end};
}

int discountFor(bool hasOrdersInPreviousMonth)
{
    return hasOrdersInPreviousMonth ? 0 : discountPercent;
}

Money priceWithDiscount(Money basePrice, int discount)
{
    // +50 перед целочисленным делением на 100 даёт округление «половина вверх»
    const qint64 kopecks = (basePrice.kopecks() * (100 - discount) + 50) / 100;
    return Money::fromKopecks(kopecks);
}

} // namespace Pricing
