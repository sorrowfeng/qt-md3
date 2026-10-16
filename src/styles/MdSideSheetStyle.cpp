#include "MdSideSheetStyle.h"

#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdSideSheet.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <cmath>
#include <limits>

namespace md {

MdSideSheetStyle::MdSideSheetStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSideSheetStyle *MdSideSheetStyle::shared()
{
    static QMutex mutex;
    static MdSideSheetStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdSideSheetStyle;
        installPaintFilter<MdSideSheet>(instance);
    }
    return instance;
}

bool MdSideSheetStyle::isInstalled()
{
    return hasPaintFilter(&MdSideSheet::staticMetaObject);
}

qreal MdSideSheetStyle::hiddenOffsetX(MdSideSheetEdge edge, qreal parentWidth, qreal sheetWidth)
{
    // MDC's `getHiddenOffset`: the parent's width for the right edge — the
    // sheet's left x clears the parent entirely. Mirrored for the left edge:
    // the sheet's left x sits one sheet-width past the parent's left.
    return edge == MdSideSheetEdge::Right ? parentWidth : -sheetWidth;
}

qreal MdSideSheetStyle::expandedOffsetX(MdSideSheetEdge edge, qreal parentWidth, qreal sheetWidth,
                                        qreal innerMargin)
{
    // MDC's `getExpandedOffset`: max(0, hiddenOffset - childWidth -
    // innerMargin) for the right edge; mirrored for the left.
    if (edge == MdSideSheetEdge::Right) {
        return std::max(0.0, parentWidth - sheetWidth - innerMargin);
    }
    return std::min(innerMargin, std::max(0.0, parentWidth - sheetWidth));
}

bool MdSideSheetStyle::hasAnchor(MdSideSheetState state, qreal sheetWidth)
{
    switch (state) {
    case MdSideSheetState::Hidden:
        return true;
    case MdSideSheetState::Expanded:
        return sheetWidth > 0.0;
    default:
        return false;
    }
}

qreal MdSideSheetStyle::anchorOffsetX(MdSideSheetEdge edge, MdSideSheetState state,
                                      qreal parentWidth, qreal sheetWidth, qreal innerMargin)
{
    if (!hasAnchor(state, sheetWidth)) {
        return std::numeric_limits<qreal>::quiet_NaN();
    }
    switch (state) {
    case MdSideSheetState::Hidden:
        return hiddenOffsetX(edge, parentWidth, sheetWidth);
    case MdSideSheetState::Expanded:
        return expandedOffsetX(edge, parentWidth, sheetWidth, innerMargin);
    default:
        return std::numeric_limits<qreal>::quiet_NaN();
    }
}

qreal MdSideSheetStyle::sheetTravel(qreal hiddenX, qreal expandedX)
{
    return std::abs(hiddenX - expandedX);
}

MdSideSheetStyle::Layout MdSideSheetStyle::layoutFor(MdSideSheetEdge edge, MdSideSheetKind kind,
                                                     qreal width, qreal height, bool hasDivider,
                                                     const MdSideSheetTokens &tokens)
{
    Layout layout;

    // The widget rect carries the shadow margin on the top/bottom edges and
    // the INNER (content-facing) edge; the DOCKED edge is flush because a
    // docked sheet is edge-to-edge with the parent's edge, and the hidden
    // slide clips outside the parent anyway. The DETACHED presentation
    // floats with `kDetachedMargin` on every edge (MDC's
    // `Widget.Material3.SideSheet.Detached`).
    const qreal m = MdSideSheetTokens::kShadowMargin;
    const QRectF widget(0.0, 0.0, width, height);
    if (kind == MdSideSheetKind::Detached) {
        const qreal d = MdSideSheetTokens::kDetachedMargin;
        layout.container = widget.adjusted(d, d, -d, -d);
    } else {
        layout.container = edge == MdSideSheetEdge::Right ? widget.adjusted(m, m, 0.0, -m)
                                                          : widget.adjusted(0.0, m, -m, -m);
    }

    // Corner radii: standard = corner-none (all square, flush against the
    // edge); modal = corner-large-start — the START pair faces the app
    // content. For a right-docked sheet the start pair is TL / BL; for a
    // left-docked sheet the content faces right, so the pair is TR / BR.
    // Detached = `detached.container.shape` (corner-large) on every corner.
    if (kind == MdSideSheetKind::Detached) {
        const qreal r = MdShape::resolvedRadius(tokens.detachedShape, layout.container.size());
        layout.radii = {r, r, r, r};
    } else if (kind == MdSideSheetKind::Modal) {
        const qreal r = MdShape::resolvedRadius(tokens.modalContainerShape,
                                                layout.container.size());
        layout.radii = edge == MdSideSheetEdge::Right ? QList<qreal>{r, 0.0, 0.0, r}
                                                      : QList<qreal>{0.0, r, r, 0.0};
    } else {
        layout.radii = {0.0, 0.0, 0.0, 0.0};
    }

    // The optional divider runs the container's full height along the
    // content-facing edge.
    if (hasDivider) {
        layout.dividerRect = edge == MdSideSheetEdge::Right
            ? QRectF(layout.container.left(), layout.container.top(), 1.0,
                     layout.container.height())
            : QRectF(layout.container.right() - 1.0, layout.container.top(), 1.0,
                     layout.container.height());
    }

    // The content area: the spec's 24 px padding all around.
    layout.contentRect = layout.container.adjusted(MdSideSheetTokens::kContentPadding,
                                                   MdSideSheetTokens::kContentPadding,
                                                   -MdSideSheetTokens::kContentPadding,
                                                   -MdSideSheetTokens::kContentPadding);
    return layout;
}

void MdSideSheetStyle::paintSheet(QPainter &painter, const MdSideSheet &sheet,
                                  const MdSideSheetTokens &tokens, const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow — the standard sheet rides level 0 (no shadow), the modal
    //    sheet level 1, the detached sheet the Expressive floating level 1
    //    (`md.comp.sheet.floating.container.elevation`).
    ElevationLevel elevation = tokens.standardContainerElevation;
    ColorRole containerColor = tokens.standardContainerColor;
    switch (sheet.kind()) {
    case MdSideSheetKind::Modal:
        elevation = tokens.modalContainerElevation;
        containerColor = tokens.modalContainerColor;
        break;
    case MdSideSheetKind::Detached:
        elevation = tokens.floatingContainerElevation;
        containerColor = tokens.floatingContainerColor;
        break;
    case MdSideSheetKind::Standard:
    default:
        break;
    }
    MdElevation::drawShadow(&painter, layout.container, layout.radii.isEmpty()
                                                       ? 0.0
                                                       : layout.radii.first(),
                            elevation, theme.color(tokens.containerShadowColor));

    // 2. Container.
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.color(containerColor));
    painter.drawPath(path);

    // 3. The optional divider along the content-facing edge.
    if (layout.dividerRect.isValid()) {
        painter.setBrush(theme.color(tokens.dividerColor));
        painter.drawRect(layout.dividerRect);
    }

    painter.restore();
}

void MdSideSheetStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *sheet = qobject_cast<MdSideSheet *>(widget);
    if (painter == nullptr || sheet == nullptr) {
        return;
    }
    const MdSideSheetTokens &tokens = sheet->tokens();
    paintSheet(*painter, *sheet, tokens, sheet->currentLayout());
}

} // namespace md
