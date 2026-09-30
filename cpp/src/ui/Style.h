#pragma once

#include <QString>

// Оформление по руководству по стилю заказчика (Приложение 3)
namespace Style {

inline constexpr auto fontFamily = "Calibri";
inline constexpr auto mainBackground = "#FFFFFF";
inline constexpr auto extraBackground = "#D2F6E7";
inline constexpr auto accentColor = "#70B2AF";
inline constexpr auto lowStockColor = "#FF8080";
inline constexpr auto textColor = "#1F2D2B";
inline constexpr auto secondaryText = "#5F6B69";
inline constexpr auto borderColor = "#9DBFBC";

// Таблица стилей Qt (QSS) — та же, что в Python-версии
QString styleSheet();

} // namespace Style
