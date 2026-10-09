#ifndef MD_NAVIGATION_RAIL_STYLE_H
#define MD_NAVIGATION_RAIL_STYLE_H

// MdNavigationRailStyle — the layout and paint of `MdNavigationRail`.
//
// A vertical column: an optional header (a FAB, a menu icon) at the top, then
// the destinations. Everything the layout decides comes from two states the
// rail carries:
//
//   * **collapsed / expanded** — the flexible family's two container widths.
//     Collapsed, the items are `Top`-arranged pills centred in the full width,
//     separated by the collapsed family's `item-vertical-space` (4). Expanded,
//     they are `Start`-arranged pills hugging the leading edge with the
//     hard-coded 20 px of trailing room, separated by the expanded family's
//     `between-item-space` (0) — and the rail's width follows its content
//     between the published 220 and 360 bounds.
//   * **standard / modal** — the modal rows are the drawer's replacement: a
//     `surface-container` container at level 2 with a `corner-large` shape,
//     arriving over a scrim the host provides (this paint draws the container
//     only; the scrim is a `MdNavigationRailHost` matter, see
//     docs/porting-todo.md).
//
// The baseline rail is always the collapsed shape: it publishes no expanded
// rows, so `supportsExpanded()` is false and `setExpanded(true)` is refused.
//
// Two rows are carried and not painted, on the same grounds as the bar's
// elevation: the baseline's `container-elevation: level0` is nothing to draw
// anyway, and the *modal* container's level2 falls outside the widget rect Qt
// gives a child rail — recorded in docs/porting-todo.md rather than drawn
// clipped.

#include "MdStyleBase.h"
#include "core/MdNavigationRailTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdNavigationRail;

/// Pattern A style for `MdNavigationRail`.
class QT_MD3_EXPORT MdNavigationRailStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdNavigationRailStyle(QObject *parent = nullptr);

    static MdNavigationRailStyle *shared();
    static bool isInstalled();

    struct Layout
    {
        QRectF container;
        QList<qreal> radii;
        /// The header's box, or empty when the rail has none.
        QRectF headerBox;
        /// Where each item goes, in `items()` order. Collapsed items get the
        /// full width and centre their own content; expanded items are held at
        /// their natural width on the leading edge.
        QList<QRectF> itemBoxes;
    };

    /// `expanded` selects which side of the flexible family's spacing rows
    /// applies; `modal` selects which container colour / shape / elevation set
    /// the paint uses.
    static Layout layoutFor(const MdNavigationRail &rail, const MdNavigationRailTokens &tokens,
                            bool expanded, bool modal);

    static void paintNavigationRail(QPainter &painter, const MdNavigationRail &rail,
                                    const MdNavigationRailTokens &tokens, const Layout &layout,
                                    bool modal);

    /// The width an expanded flexible rail wants: the widest element (header or
    /// item plus the expanded padding) between the published bounds. The
    /// baseline family has no expanded rows, so the answer is its collapsed
    /// width.
    static qreal expandedWidthFor(const MdNavigationRail &rail,
                                  const MdNavigationRailTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_NAVIGATION_RAIL_STYLE_H
