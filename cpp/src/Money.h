#pragma once

#include <QString>
#include <QtGlobal>

#include <compare>

// Денежная сумма в копейках. Целые числа, в отличие от double, не дают
// погрешностей округления — это аналог Decimal из Python-версии.
class Money
{
public:
    constexpr Money() = default;

    static constexpr Money fromKopecks(qint64 kopecks)
    {
        Money money;
        money.m_kopecks = kopecks;
        return money;
    }

    // Разбирает число вида «2190.00», как его передаёт PostgreSQL;
    // при неверной записи бросает std::invalid_argument
    static Money fromDecimalString(const QString &text);

    constexpr qint64 kopecks() const { return m_kopecks; }

    // Запись для передачи в PostgreSQL: «1642.50»
    QString toDecimalString() const;

    constexpr Money operator*(qint64 multiplier) const
    {
        return fromKopecks(m_kopecks * multiplier);
    }

    constexpr Money operator+(Money other) const
    {
        return fromKopecks(m_kopecks + other.m_kopecks);
    }

    constexpr Money &operator+=(Money other)
    {
        m_kopecks += other.m_kopecks;
        return *this;
    }

    constexpr auto operator<=>(const Money &) const = default;

private:
    qint64 m_kopecks = 0;
};
