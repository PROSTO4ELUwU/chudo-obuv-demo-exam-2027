#include "ui/Images.h"

#include <QGuiApplication>
#include <QHash>

namespace {

// Изображение, вписанное в размер без искажения пропорций и без размытия.
// При масштабе экрана 150 % в логическом пикселе полтора физических, поэтому
// картинка готовится в физических пикселях: уменьшенную до логического
// размера Qt растянул бы, и она выглядела бы размытой
QPixmap fitted(const QPixmap &pixmap, QSize size)
{
    const qreal ratio = qGuiApp->devicePixelRatio();
    QPixmap result = pixmap.scaled(size * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    result.setDevicePixelRatio(ratio);
    return result;
}

} // namespace

namespace Images {

QPixmap logo(QSize size)
{
    return fitted(QPixmap(QStringLiteral(":/logo.png")), size);
}

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
    return fitted(*cached, size);
}

} // namespace Images
