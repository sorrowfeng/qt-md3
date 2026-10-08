#ifndef MD_SIDE_SHEET_STYLE_H
#define MD_SIDE_SHEET_STYLE_H

#include "core/MdSideSheetTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdSideSheet;

/// Pattern A style for `MdSideSheet`: painting, geometry and the anchor math
/// for the `md.comp.sheet.side.*` family, registered in the paint hub so the
/// first `MdSideSheet` construction installs it application-wide.
///
/// The anchor math ports MDC-Android's sheet delegates (`LeftSheetDelegate` /
/// `RightSheetDelegate`) — the sheet's x position for each
/// `MdSideSheetState`, against the parent's width. The offsets are the x of
/// the CONTAINER's left edge in both cases, so the two edges mirror:
///
///   * Hidden      right: at parentWidth (fully off the right edge)
///                 left:  at -sheetWidth (fully off the left edge)
///   * Expanded    right: max(0, parentWidth - sheetWidth - innerMargin)
///                 left:  min(innerMargin, max(0, parentWidth - sheetWidth))
///
/// The release settle ports `isReleasedCloseToInnerEdge`: a drag released
/// closer to the expanded anchor than the hidden anchor settles expanded,
/// otherwise hidden — the velocity-weighted projection (the 500 px/s
/// significant threshold, the 0.1 hide friction) is recorded, not ported.
class QT_MD3_EXPORT MdSideSheetStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSideSheetStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdSideSheetStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- anchor math (pure) -------------------------------------------------
    /// The container's x when hidden — fully off the docked edge.
    static qreal hiddenOffsetX(MdSideSheetEdge edge, qreal parentWidth, qreal sheetWidth);

    /// The container's x when expanded — docked against the edge with the
    /// inner margin between the sheet and the opposite edge (MDC's
    /// `getExpandedOffset`).
    static qreal expandedOffsetX(MdSideSheetEdge edge, qreal parentWidth, qreal sheetWidth,
                                 qreal innerMargin);

    /// Whether the state's anchor exists (Expanded needs a non-zero sheet).
    static bool hasAnchor(MdSideSheetState state, qreal sheetWidth);

    /// The container's x for a settled state, or NaN when the state has no
    /// anchor for this configuration.
    static qreal anchorOffsetX(MdSideSheetEdge edge, MdSideSheetState state, qreal parentWidth,
                               qreal sheetWidth, qreal innerMargin);

    /// The sheet's width available when expanded: `parentWidth - expandedX -
    /// innerMargin` for the right edge, `expandedX` recomputed for the left —
    /// the travel the drag clamps into, exposed for tests.
    static qreal sheetTravel(qreal hiddenX, qreal expandedX);

    // --- geometry (pure) ----------------------------------------------------
    struct Layout
    {
        /// The painted container: the widget rect with the shadow margin on
        /// the top/bottom/inner edges and a flush docked edge (a docked
        /// sheet is edge-to-edge with the parent's edge).
        QRectF container;
        /// Corner radii, TL / TR / BR / BL. Standard: all 0 (`corner-none`).
        /// Modal: the `corner-large-start` radius on the START pair — for a
        /// right sheet the left pair (TL / BL), for a left sheet the right
        /// pair (TR / BR).
        QList<qreal> radii;
        /// The optional divider strip along the content-facing edge.
        QRectF dividerRect;
        /// The content area: the container with the spec's 24 px padding.
        QRectF contentRect;
    };

    /// The layout for a widget rect of `width` x `height`. Pure: everything
    /// derives from the tokens, so the tests pin geometry without widgets.
    static Layout layoutFor(MdSideSheetEdge edge, MdSideSheetKind kind, qreal width, qreal height,
                            bool hasDivider, const MdSideSheetTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// other families' are.
    static void paintSheet(QPainter &painter, const MdSideSheet &sheet,
                           const MdSideSheetTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_SIDE_SHEET_STYLE_H
