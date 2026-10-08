#include "MdBottomSheetStyle.h"

#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdBottomSheet.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <cmath>
#include <limits>

namespace md {

namespace {

/// The top-pair radii for the container: `corner-extra-large-top` — the
/// shape token's radius on TL / TR, square on BR / BL (a sheet meets the
/// parent's bottom edge).
QList<qreal> topRadii(ShapeCorner corner, const QSizeF &size)
{
    qreal value = MdShape::resolvedRadius(corner, size);
    // `Full` resolves against the shorter side; a bottom sheet never wants a
    // pill on its top corners — clamp against the width like the shape scale
    // does, but keep the resolved semantics.
    return {value, value, 0.0, 0.0};
}

} // namespace

MdBottomSheetStyle::MdBottomSheetStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdBottomSheetStyle *MdBottomSheetStyle::shared()
{
    static QMutex mutex;
    static MdBottomSheetStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdBottomSheetStyle;
        installPaintFilter<MdBottomSheet>(instance);
    }
    return instance;
}

bool MdBottomSheetStyle::isInstalled()
{
    return hasPaintFilter(&MdBottomSheet::staticMetaObject);
}

qreal MdBottomSheetStyle::partialVisibleHeight(qreal fullHeight, qreal sheetHeight)
{
    // Compose's deterministic rule (flag default on): half the content size,
    // at most half the screen.
    return std::min(fullHeight / 2.0, sheetHeight / 2.0);
}

bool MdBottomSheetStyle::hasAnchor(MdSheetKind kind, MdSheetState state, qreal fullHeight,
                                   qreal sheetHeight, qreal peekHeight, bool skipHidden,
                                   bool skipPartial)
{
    Q_UNUSED(fullHeight);
    switch (state) {
    case MdSheetState::Hidden:
        // Modal: always. Standard: only when the hidden state is not skipped
        // (Compose's skipHiddenState defaults true for the scaffold) — the
        // zero-size degenerate cases from the scaffold's anchor block fold
        // into the same availability.
        return kind == MdSheetKind::Modal || !skipHidden || sheetHeight <= 0.0
            || peekHeight <= 0.0;
    case MdSheetState::PartiallyExpanded:
        if (skipPartial || sheetHeight <= 0.0) {
            return false;
        }
        // Standard: the peek height drives the partial anchor, and a peek
        // equal to the sheet height collapses partial into expanded.
        if (kind == MdSheetKind::Standard) {
            return peekHeight > 0.0 && !qFuzzyCompare(peekHeight, sheetHeight);
        }
        // Modal: with the deterministic flag on, any non-zero sheet height
        // gets a partial anchor.
        return true;
    case MdSheetState::Expanded:
        return sheetHeight > 0.0;
    default:
        return false;
    }
}

qreal MdBottomSheetStyle::anchorOffset(MdSheetKind kind, MdSheetState state, qreal fullHeight,
                                       qreal sheetHeight, qreal peekHeight, bool skipHidden,
                                       bool skipPartial)
{
    if (!hasAnchor(kind, state, fullHeight, sheetHeight, peekHeight, skipHidden, skipPartial)) {
        return std::numeric_limits<qreal>::quiet_NaN();
    }
    switch (state) {
    case MdSheetState::Hidden:
        return fullHeight;
    case MdSheetState::PartiallyExpanded:
        if (kind == MdSheetKind::Standard) {
            return fullHeight - peekHeight;
        }
        return fullHeight - partialVisibleHeight(fullHeight, sheetHeight);
    case MdSheetState::Expanded:
        return std::max(0.0, fullHeight - sheetHeight);
    default:
        return std::numeric_limits<qreal>::quiet_NaN();
    }
}

MdBottomSheetStyle::Layout MdBottomSheetStyle::layoutFor(qreal width, qreal height, bool hasHandle,
                                                         const MdBottomSheetTokens &tokens)
{
    Layout layout;

    // The widget rect carries the shadow margin on the top/left/right only
    // (the snackbar idiom, trimmed): the bottom edge is flush because a
    // sheet is edge-to-edge with the parent's bottom, and the hidden slide
    // clips below the parent anyway.
    layout.container = QRectF(0.0, 0.0, width, height)
                           .adjusted(MdBottomSheetTokens::kShadowMargin,
                                     MdBottomSheetTokens::kShadowMargin,
                                     -MdBottomSheetTokens::kShadowMargin, 0.0);
    layout.radii = topRadii(tokens.containerShape, layout.container.size());

    if (hasHandle) {
        // BottomSheetDefaults.DragHandle: the 32 x 4 pill centred, with its
        // 22 px vertical touch padding.
        const qreal handleWidth = tokens.dragHandleWidth;
        const qreal handleHeight = tokens.dragHandleHeight;
        const qreal handleTop = layout.container.top()
            + MdBottomSheetTokens::kDragHandleVerticalPadding;
        layout.handleRect =
            QRectF(layout.container.left() + (layout.container.width() - handleWidth) / 2.0,
                   handleTop, handleWidth, handleHeight);
        layout.contentRect = QRectF(
            layout.container.left(),
            handleTop + handleHeight + MdBottomSheetTokens::kDragHandleVerticalPadding,
            layout.container.width(), layout.container.bottom() - handleTop - handleHeight
                - MdBottomSheetTokens::kDragHandleVerticalPadding);
    }
    return layout;
}

void MdBottomSheetStyle::paintSheet(QPainter &painter, const MdBottomSheet &sheet,
                                    const MdBottomSheetTokens &tokens, const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow — the container's level-1 elevation is static; a bottom
    //    sheet has no interactive state rows of its own.
    MdElevation::drawShadow(&painter, layout.container, layout.radii.isEmpty()
                                                       ? 0.0
                                                       : layout.radii.first(),
                            tokens.containerElevation, theme.color(tokens.containerShadowColor));

    // 2. Container.
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.color(tokens.containerColor));
    painter.drawPath(path);

    // 3. The drag handle pill. The export's deprecated 0.4 opacity row is
    //    recorded, not applied — Compose's DragHandle paints the full colour.
    if (layout.handleRect.isValid()) {
        painter.setBrush(theme.color(tokens.dragHandleColor));
        painter.drawRoundedRect(layout.handleRect, layout.handleRect.height() / 2.0,
                                layout.handleRect.height() / 2.0);
    }

    painter.restore();
}

void MdBottomSheetStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *sheet = qobject_cast<MdBottomSheet *>(widget);
    if (painter == nullptr || sheet == nullptr) {
        return;
    }
    const MdBottomSheetTokens &tokens = sheet->tokens();
    paintSheet(*painter, *sheet, tokens, sheet->currentLayout());
}

} // namespace md
