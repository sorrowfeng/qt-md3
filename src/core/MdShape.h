#ifndef MD_SHAPE_H
#define MD_SHAPE_H

// Shape scale: rounded-corner tokens, runtime interpolation, and shape morph.
//
// Corner values come from material-web tokens/versions/v0_192/_md-sys-shape.scss
// plus the M3 Expressive steps published in Compose's ShapeTokens.kt.
//
// Everything here is interpolatable on purpose: MD3 components must be able to
// animate between two shape tokens at runtime ("shape morph"), so no component
// should ever hard-code a corner radius.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtGui/QPainterPath>

namespace md {

class QT_MD3_EXPORT MdShape
{
public:
    /// Corner radius in logical px. ShapeCorner::Full yields -1 (a pill); use
    /// resolvedRadius()/resolvedRadii() to get a concrete value for a box.
    static qreal radius(ShapeCorner corner);

    /// {top-left, top-right, bottom-right, bottom-left}.
    static QList<qreal> radii(ShapeCorner corner);

    /// Concrete radii for `size`: `Full` becomes half the shorter side.
    static QList<qreal> resolvedRadii(ShapeCorner corner, const QSizeF &size);
    static qreal resolvedRadius(ShapeCorner corner, const QSizeF &size);

    /// Linear interpolation between two radii, and between two radius sets.
    static qreal lerpRadius(qreal from, qreal to, qreal t);
    static QList<qreal> lerpRadii(const QList<qreal> &from, const QList<qreal> &to, qreal t);

    /// A rounded rectangle with independent corner radii, in TL, TR, BR, BL
    /// order. Radii are clamped so opposite corners never overlap.
    static QPainterPath roundedRect(const QRectF &rect, const QList<qreal> &radii);
    static QPainterPath roundedRect(const QRectF &rect, ShapeCorner corner);

    /// Morph one path into another. The two paths must have the same element
    /// count; when they do not, `to` wins from t >= 1 and `from` below.
    static QPainterPath morph(const QPainterPath &from, const QPainterPath &to, qreal t);

    /// Convenience: a rounded rectangle whose corners can be morphed by
    /// interpolating the radius sets first.
    static QPainterPath morphRoundedRect(const QRectF &rect,
                                         const QList<qreal> &fromRadii,
                                         const QList<qreal> &toRadii,
                                         qreal t);

    /// Number of M3 Expressive decorative shapes currently available.
    /// 0 until the 35-path catalogue is ported (see docs/porting-todo.md).
    static int decorativeShapeCount();
};

} // namespace md

#endif // MD_SHAPE_H
