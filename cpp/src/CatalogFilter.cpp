#include "CatalogFilter.h"

#include <algorithm>
#include <functional>
#include <iterator>

namespace {

// Текст для поиска без учёта регистра и различия «е» и «ё»
QString normalize(const QString &text)
{
    return text.toCaseFolded().replace(u'ё', u'е');
}

} // namespace

QString sortOrderName(SortOrder order)
{
    switch (order) {
    case SortOrder::PriceAscending:
        return QStringLiteral("Цена по возрастанию");
    case SortOrder::PriceDescending:
        return QStringLiteral("Цена по убыванию");
    case SortOrder::None:
        break;
    }
    return QStringLiteral("Без сортировки");
}

QString allCategoriesName()
{
    return QStringLiteral("Все категории");
}

std::vector<Product> filterProducts(const std::vector<Product> &products,
                                    const QString &searchText, const QString &category,
                                    SortOrder sortOrder)
{
    const QString needle = normalize(searchText.trimmed());
    const bool anyCategory = category == allCategoriesName();
    std::vector<Product> result;
    std::ranges::copy_if(products, std::back_inserter(result), [&](const Product &product) {
        return (anyCategory || product.category == category)
               && (normalize(product.name).contains(needle)
                   || normalize(product.description).contains(needle));
    });
    if (sortOrder == SortOrder::PriceAscending)
        std::ranges::stable_sort(result, std::ranges::less{}, &Product::price);
    else if (sortOrder == SortOrder::PriceDescending)
        std::ranges::stable_sort(result, std::ranges::greater{}, &Product::price);
    return result;
}
