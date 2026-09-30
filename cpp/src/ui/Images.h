#pragma once

#include <QPixmap>
#include <QSize>
#include <QString>

namespace Images {

// Изображение товара из ресурсов, вписанное в размер без искажения пропорций.
// Если изображения нет в базе или файл не найден, возвращается заглушка
// picture.png. Каждый файл читается один раз, затем берётся из кэша.
QPixmap productPixmap(const QString &imageFile, QSize size);

} // namespace Images
