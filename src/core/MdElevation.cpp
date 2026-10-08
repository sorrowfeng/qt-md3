#include "MdElevation.h"

#include "MdColorScheme.h"
#include "MdShape.h"
#include "MdTokens.h"

#include <QtGui/QColor>
#include <algorithm>

namespace md {

qreal MdElevation::shadowDp(ElevationLevel level)
{
    return MdSystemTokens::elevationDp(level);
}

ColorRole MdElevation::surfaceRole(ElevationLevel level)
{
    // Project mapping: walk the surface-container family in order so that
    // "higher" always means a more prominent tonal step. MD3 publishes the
    // palette but not a mandatory level -> role table, so this is recorded as
    // a project decision rather than an official token.
    switch (level) {
    case ElevationLevel::Level0: return ColorRole::Surface;
    case ElevationLevel::Level1: return ColorRole::SurfaceContainerLow;
    case ElevationLevel::Level2: return ColorRole::SurfaceContainer;
    case ElevationLevel::Level3: return ColorRole::SurfaceContainerHigh;
    case ElevationLevel::Level4: return ColorRole::SurfaceContainerHigh;
    case ElevationLevel::Level5: return ColorRole::SurfaceContainerHighest;
    case ElevationLevel::Count: break;
    }
    return ColorRole::Surface;
}

QColor MdElevation::surfaceColor(ElevationLevel level, const MdColorScheme &scheme)
{
    return scheme.color(surfaceRole(level));
}

void MdElevation::drawShadow(QPainter *painter,
                             const QRectF &rect,
                             qreal cornerRadius,
                             ElevationLevel level,
                             const QColor &shadowColor)
{
    drawShadowDp(painter, rect, cornerRadius, shadowDp(level), shadowColor);
}

void MdElevation::drawShadowDp(QPainter *painter,
                               const QRectF &rect,
                               qreal cornerRadius,
                               qreal dp,
                               const QColor &shadowColor)
{
    if (!painter || dp <= 0.0) {
        return;
    }
    painter->save();
    painter->setPen(Qt::NoPen);

    // Approximate a soft shadow with a handful of growing, fading rings.
    constexpr int kLayers = 6;
    for (int i = kLayers; i >= 1; --i) {
        const qreal progress = qreal(i) / qreal(kLayers);
        const qreal spread = dp * progress;
        const qreal alpha = (1.0 - progress) * 0.16 + 0.04;
        QColor layer = shadowColor;
        layer.setAlphaF(std::clamp(alpha, 0.0, 1.0));

        const QRectF ring = rect.adjusted(-spread / 2.0, spread / 2.0, spread / 2.0, spread);
        const qreal ringRadius = cornerRadius < 0.0 ? ring.height() / 2.0 : cornerRadius + spread / 2.0;
        painter->setBrush(layer);
        painter->drawPath(MdShape::roundedRect(ring, QList<qreal>{ringRadius, ringRadius, ringRadius, ringRadius}));
    }

    painter->restore();
}

} // namespace md
