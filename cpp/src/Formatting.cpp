#include "Formatting.h"

namespace Formatting {

QString money(Money value)
{
    const qint64 kopecks = value.kopecks() < 0 ? -value.kopecks() : value.kopecks();
    QString rubles = QString::number(kopecks / 100);
    for (qsizetype position = rubles.size() - 3; position > 0; position -= 3)
        rubles.insert(position, nbsp);
    return QStringLiteral("%1%2,%3%4₽")
        .arg(value.kopecks() < 0 ? QStringLiteral("-") : QString(), rubles)
        .arg(kopecks % 100, 2, 10, QChar(u'0'))
        .arg(nbsp);
}

QString size(int sizeTenths)
{
    if (sizeTenths % 10 == 0)
        return QString::number(sizeTenths / 10);
    return QStringLiteral("%1,%2").arg(sizeTenths / 10).arg(sizeTenths % 10);
}

QString date(QDate value)
{
    return value.toString(QStringLiteral("dd.MM.yyyy"));
}

QString pairs(int count)
{
    const int lastDigit = count % 10;
    const int lastTwoDigits = count % 100;
    QString word = QStringLiteral("пар");
    if (lastDigit == 1 && lastTwoDigits != 11)
        word = QStringLiteral("пара");
    else if (lastDigit >= 2 && lastDigit <= 4 && (lastTwoDigits < 12 || lastTwoDigits > 14))
        word = QStringLiteral("пары");
    return QStringLiteral("%1%2%3").arg(count).arg(nbsp).arg(word);
}

} // namespace Formatting
