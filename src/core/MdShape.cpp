#include "MdShape.h"

#include "MdTokens.h"

#include <QtCore/QPointF>
#include <algorithm>

namespace md {

qreal MdShape::radius(ShapeCorner corner)
{
    return MdSystemTokens::cornerRadius(corner);
}

QList<qreal> MdShape::radii(ShapeCorner corner)
{
    return MdSystemTokens::cornerRadii(corner);
}

QList<qreal> MdShape::resolvedRadii(ShapeCorner corner, const QSizeF &size)
{
    const qreal value = resolvedRadius(corner, size);
    return {value, value, value, value};
}

qreal MdShape::resolvedRadius(ShapeCorner corner, const QSizeF &size)
{
    const qreal value = MdSystemTokens::cornerRadius(corner);
    if (value < 0.0) {
        return std::min(size.width(), size.height()) / 2.0;
    }
    return value;
}

qreal MdShape::lerpRadius(qreal from, qreal to, qreal t)
{
    return from + (to - from) * t;
}

QList<qreal> MdShape::lerpRadii(const QList<qreal> &from, const QList<qreal> &to, qreal t)
{
    QList<qreal> result;
    result.reserve(4);
    for (int i = 0; i < 4; ++i) {
        const qreal a = i < from.size() ? from.at(i) : 0.0;
        const qreal b = i < to.size() ? to.at(i) : 0.0;
        result << lerpRadius(a, b, t);
    }
    return result;
}

QPainterPath MdShape::roundedRect(const QRectF &rect, const QList<qreal> &radii)
{
    QPainterPath path;
    if (rect.isEmpty()) {
        return path;
    }

    const qreal limit = std::min(rect.width(), rect.height()) / 2.0;
    qreal tl = radii.size() > 0 ? radii.at(0) : 0.0;
    qreal tr = radii.size() > 1 ? radii.at(1) : 0.0;
    qreal br = radii.size() > 2 ? radii.at(2) : 0.0;
    qreal bl = radii.size() > 3 ? radii.at(3) : 0.0;

    // Clamp each edge so adjacent corners cannot overlap, matching CSS.
    const qreal topScale = (tl + tr) > rect.width() && (tl + tr) > 0.0
                               ? rect.width() / (tl + tr)
                               : 1.0;
    const qreal bottomScale = (bl + br) > rect.width() && (bl + br) > 0.0
                                  ? rect.width() / (bl + br)
                                  : 1.0;
    const qreal leftScale = (tl + bl) > rect.height() && (tl + bl) > 0.0
                                ? rect.height() / (tl + bl)
                                : 1.0;
    const qreal rightScale = (tr + br) > rect.height() && (tr + br) > 0.0
                                 ? rect.height() / (tr + br)
                                 : 1.0;

    tl = std::min({tl * std::min(topScale, leftScale), limit});
    tr = std::min({tr * std::min(topScale, rightScale), limit});
    br = std::min({br * std::min(bottomScale, rightScale), limit});
    bl = std::min({bl * std::min(bottomScale, leftScale), limit});

    const qreal x = rect.x();
    const qreal y = rect.y();
    const qreal w = rect.width();
    const qreal h = rect.height();

    path.moveTo(x + tl, y);
    path.lineTo(x + w - tr, y);
    if (tr > 0.0) {
        path.arcTo(QRectF(x + w - 2 * tr, y, 2 * tr, 2 * tr), 90.0, -90.0);
    }
    path.lineTo(x + w, y + h - br);
    if (br > 0.0) {
        path.arcTo(QRectF(x + w - 2 * br, y + h - 2 * br, 2 * br, 2 * br), 0.0, -90.0);
    }
    path.lineTo(x + bl, y + h);
    if (bl > 0.0) {
        path.arcTo(QRectF(x, y + h - 2 * bl, 2 * bl, 2 * bl), 270.0, -90.0);
    }
    path.lineTo(x, y + tl);
    if (tl > 0.0) {
        path.arcTo(QRectF(x, y, 2 * tl, 2 * tl), 180.0, -90.0);
    }
    path.closeSubpath();
    return path;
}

QPainterPath MdShape::roundedRect(const QRectF &rect, ShapeCorner corner)
{
    return roundedRect(rect, resolvedRadii(corner, rect.size()));
}

QPainterPath MdShape::morph(const QPainterPath &from, const QPainterPath &to, qreal t)
{
    if (t <= 0.0) {
        return from;
    }
    if (t >= 1.0) {
        return to;
    }
    if (from.elementCount() != to.elementCount() || from.elementCount() == 0) {
        // Structurally different paths cannot be interpolated pointwise.
        return t < 0.5 ? from : to;
    }

    QPainterPath result;
    for (int i = 0; i < from.elementCount(); ++i) {
        const QPainterPath::Element a = from.elementAt(i);
        const QPainterPath::Element b = to.elementAt(i);
        const QPointF point(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
        switch (a.type) {
        case QPainterPath::MoveToElement:
            result.moveTo(point);
            break;
        case QPainterPath::LineToElement:
            result.lineTo(point);
            break;
        case QPainterPath::CurveToElement:
        case QPainterPath::CurveToDataElement:
            // Cubic segments are emitted as three consecutive elements; the
            // CurveToElement starts one.
            if (a.type == QPainterPath::CurveToElement) {
                const QPainterPath::Element a2 = from.elementAt(i + 1);
                const QPainterPath::Element a3 = from.elementAt(i + 2);
                const QPainterPath::Element b2 = to.elementAt(i + 1);
                const QPainterPath::Element b3 = to.elementAt(i + 2);
                result.cubicTo(point,
                               QPointF(a2.x + (b2.x - a2.x) * t, a2.y + (b2.y - a2.y) * t),
                               QPointF(a3.x + (b3.x - a3.x) * t, a3.y + (b3.y - a3.y) * t));
            }
            break;
        }
    }
    return result;
}

QPainterPath MdShape::morphRoundedRect(const QRectF &rect,
                                       const QList<qreal> &fromRadii,
                                       const QList<qreal> &toRadii,
                                       qreal t)
{
    return roundedRect(rect, lerpRadii(fromRadii, toRadii, t));
}

int MdShape::decorativeShapeCount()
{
    // The 35 M3 Expressive decorative shape paths are not ported yet; see
    // docs/porting-todo.md. Returning 0 here keeps the gap explicit rather
    // than shipping invented geometry.
    return 0;
}

} // namespace md
