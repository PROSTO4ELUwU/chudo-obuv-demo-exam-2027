#include "Money.h"

#include <QStringList>

#include <stdexcept>

Money Money::fromDecimalString(const QString &text)
{
    QString digits = text.trimmed();
    const bool negative = digits.startsWith(u'-');
    if (negative)
        digits.remove(0, 1);

    const QStringList parts = digits.split(u'.');
    const QString fraction = parts.value(1).leftJustified(2, u'0');
    bool rublesValid = false;
    bool fractionValid = false;
    const qint64 rubles = parts.value(0).toLongLong(&rublesValid);
    const qint64 kopecks = fraction.toLongLong(&fractionValid);
    if (!rublesValid || !fractionValid || parts.size() > 2 || fraction.size() > 2)
        throw std::invalid_argument("Неверная запись денежной суммы: " + text.toStdString());

    const qint64 total = rubles * 100 + kopecks;
    return fromKopecks(negative ? -total : total);
}

QString Money::toDecimalString() const
{
    const qint64 absolute = m_kopecks < 0 ? -m_kopecks : m_kopecks;
    return QStringLiteral("%1%2.%3")
        .arg(m_kopecks < 0 ? QStringLiteral("-") : QString())
        .arg(absolute / 100)
        .arg(absolute % 100, 2, 10, QChar(u'0'));
}
