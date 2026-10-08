#ifndef MD_BOTTOM_SHEET_STYLE_H
#define MD_BOTTOM_SHEET_STYLE_H

#include "core/MdBottomSheetTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdBottomSheet;

/// Pattern A style for `MdBottomSheet`: painting, geometry and the anchor
/// math for the `md.comp.sheet.bottom.*` family, registered in the paint hub
/// so the first `MdBottomSheet` construction installs it application-wide.
///
/// The anchor math is the port of Compose's `draggableAnchors` block — the
/// sheet's top-y position for each `MdSheetState`, against the parent's
/// height ("fullHeight" in Compose's layout). It lives here as pure
/// functions so tests pin the offsets without constructing widgets:
///
///   * Hidden      at fullHeight (modal always; standard only when the
///                 hidden state is not skipped);
///   * PartiallyExpanded at fullHeight - peek (standard) or
///                 fullHeight - min(fullHeight/2, sheetHeight/2) (modal, the
///                 deterministic rule Compose ships flag-on);
///   * Expanded    at max(0, fullHeight - sheetHeight).
class QT_MD3_EXPORT MdBottomSheetStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdBottomSheetStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdBottomSheetStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- anchor math (pure) -------------------------------------------------
    /// The modal PartiallyExpanded visible height: half the content size, at
    /// most half the screen — Compose's deterministic rule
    /// (`calculatePartiallyExpandedOffset`, flag default on).
    static qreal partialVisibleHeight(qreal fullHeight, qreal sheetHeight);

    /// Whether the state's anchor exists for the sheet configuration —
    /// Compose's `isPartiallyExpandedAnchorAvailable` plus the surrounding
    /// `sheetHeight != 0f` conditions, restated per kind.
    static bool hasAnchor(MdSheetKind kind, MdSheetState state, qreal fullHeight,
                          qreal sheetHeight, qreal peekHeight, bool skipHidden,
                          bool skipPartial);

    /// The sheet's top-y for a settled state, or NaN when the state has no
    /// anchor for this configuration.
    static qreal anchorOffset(MdSheetKind kind, MdSheetState state, qreal fullHeight,
                              qreal sheetHeight, qreal peekHeight, bool skipHidden,
                              bool skipPartial);

    // --- geometry (pure) ----------------------------------------------------
    struct Layout
    {
        /// The painted container: the widget rect with the shadow margin on
        /// the top/left/right and a flush bottom edge (a sheet is
        /// edge-to-edge with the parent's bottom).
        QRectF container;
        /// Corner radii, TL / TR / BR / BL — the top pair carries the
        /// `corner-extra-large-top` radius, the bottom pair is 0.
        QList<qreal> radii;
        /// The drag handle pill, centred. Invalid when there is no handle.
        QRectF handleRect;
        /// The content area below the handle (its 22 px touch padding
        /// included). Invalid without a handle.
        QRectF contentRect;
    };

    /// The layout for a widget rect of `width` x `height`. Pure: everything
    /// derives from the tokens, so the tests pin geometry without widgets.
    static Layout layoutFor(qreal width, qreal height, bool hasHandle,
                            const MdBottomSheetTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite the same way the
    /// other families' are.
    static void paintSheet(QPainter &painter, const MdBottomSheet &sheet,
                           const MdBottomSheetTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_BOTTOM_SHEET_STYLE_H
