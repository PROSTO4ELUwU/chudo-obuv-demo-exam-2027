#pragma once

#include "Models.h"

#include <array>
#include <vector>

// Поиск, фильтрация и сортировка каталога (задание 3)

// Варианты сортировки в выпадающем списке
enum class SortOrder { None, PriceAscending, PriceDescending };

inline constexpr std::array allSortOrders{SortOrder::None, SortOrder::PriceAscending,
                                          SortOrder::PriceDescending};

QString sortOrderName(SortOrder order);

// Первый пункт фильтра категорий, сбрасывающий фильтр
QString allCategoriesName();

// Товары выбранной категории, в наименовании или описании которых есть строка
// поиска. Фильтр и поиск применяются совместно, а сортировка — к их результату,
// поэтому выбранный порядок сохраняется при любом изменении условий.
// Сортировка устойчивая: товары с одинаковой ценой остаются в порядке каталога.
std::vector<Product> filterProducts(const std::vector<Product> &products,
                                    const QString &searchText, const QString &category,
                                    SortOrder sortOrder);
