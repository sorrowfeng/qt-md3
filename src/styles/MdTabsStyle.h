#ifndef MD_TABS_STYLE_H
#define MD_TABS_STYLE_H

// MdTabsStyle — the paint and the layout of the tabs row.
//
// The row paints three layers in one pass: the container (flat `surface`,
// level0, `corner-none` — the family publishes no hover or press rows for the
// container because its interaction *is* selecting a tab), the deprecated 1 px
// divider at the bottom, and the indicator above the divider. The tabs
// themselves are child widgets painted by `MdTabStyle`.
//
// The layout is the family's own arithmetic, and it is a **pure function**:
// every value is read from the tabs' `sizeHint()`s and the tokens, and no
// child is resized or measured on the paint path — the rule the navigation
// rail's measure-in-paint defect wrote into the house style. The indicator's
// *target* rectangle per tab is exposed (`indicatorRectFor`) so `MdTabs` can
// animate its offset and width between the targets.

#include "MdStyleBase.h"
#include "core/MdTabsTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdTabs;

/// Pattern A style for `MdTabs`.
class QT_MD3_EXPORT MdTabsStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTabsStyle(QObject *parent = nullptr);

    static MdTabsStyle *shared();
    static bool isInstalled();

    /// The row's geometry, in **content** coordinates (a scrollable row draws
    /// its children and its indicator at `x - scrollOffset`). Pure reads.
    struct Layout
    {
        /// Every tab's rect; widths are the equal share (fixed) or the
        /// content-derived width (scrollable).
        QList<QRectF> tabRects;
        /// The row height: the tallest tab's own hint — 48 text-only, 64 with
        /// icon and label.
        qreal rowHeight = 48.0;
        /// The content's total width. A fixed row's equals its widget width.
        qreal contentWidth = 0.0;
    };

    static Layout layoutFor(const MdTabs &tabs, const MdTabsTokens &tokens);

    /// The indicator's target rectangle for one tab, centred in the tab: the
    /// primary family's is the tab's *content* width (pass
    /// `MdTab::indicatorContentWidth()` as `contentWidth`), the secondary's
    /// the whole tab (`contentWidth` is ignored). See the class note and
    /// porting-todo.md for the centring divergence.
    static QRectF indicatorRectFor(const Layout &layout, int index,
                                   const MdTabsVariantTokens &tokens, qreal contentWidth);

    static void paintTabs(QPainter &painter, const MdTabs &tabs, const MdTabsTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TABS_STYLE_H
