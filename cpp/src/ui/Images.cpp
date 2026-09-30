#include "ui/Images.h"

#include <QHash>

namespace Images {

QPixmap productPixmap(const QString &imageFile, QSize size)
{
    static QHash<QString, QPixmap> cache;
    auto cached = cache.constFind(imageFile);
    if (cached == cache.constEnd()) {
        QPixmap pixmap(QStringLiteral(":/images/") + imageFile);
        if (imageFile.isEmpty() || pixmap.isNull())
            pixmap.load(QStringLiteral(":/picture.png"));
        cached = cache.insert(imageFile, pixmap);
    }
    return cached->scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

} // namespace Images
