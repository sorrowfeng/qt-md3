#ifndef MD_ELEVATION_H
#define MD_ELEVATION_H

// Elevation.
//
// MD3 expresses height with tonal surfaces, not drop shadows: a raised surface
// is a slightly different surface tone. That is exactly why the colour scheme
// ships the `surface-container-*` family. Shadows only appear where a
// component spec explicitly asks for one.
//
// The dp values below are the authoritative md.sys.elevation tokens; the
// level -> surface-role mapping is a project decision (recorded in
// docs/porting-todo.md) because MD3 does not publish a literal 1:1 table.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtGui/QColor>
#include <QtGui/QPainter>

namespace md {

class MdColorScheme;

class QT_MD3_EXPORT MdElevation
{
public:
    /// md.sys.elevation.level0..5 in dp: 0, 1, 3, 6, 8, 12.
    static qreal shadowDp(ElevationLevel level);

    /// Surface role that expresses this elevation level.
    static ColorRole surfaceRole(ElevationLevel level);

    /// Resolved surface colour for a level in the given scheme.
    static QColor surfaceColor(ElevationLevel level, const MdColorScheme &scheme);

    /// Paint a soft shadow for `rect` at `level`.
    ///
    /// Deliberately conservative: MD3 rarely wants a shadow, so callers must
    /// opt in per component spec. Uses layered rounded rectangles rather than a
    /// QGraphicsEffect, because an effect would reparent the widget.
    static void drawShadow(QPainter *painter,
                           const QRectF &rect,
                           qreal cornerRadius,
                           ElevationLevel level,
                           const QColor &shadowColor);

    /// The same shadow, at an explicit dp height. Components whose
    /// elevation *animates* between a ladder's levels (the card family) call
    /// this with the current interpolated value; the level overload above
    /// simply converts and forwards.
    static void drawShadowDp(QPainter *painter,
                             const QRectF &rect,
                             qreal cornerRadius,
                             qreal dp,
                             const QColor &shadowColor);
};

} // namespace md

#endif // MD_ELEVATION_H
