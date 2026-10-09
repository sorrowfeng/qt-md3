#ifndef MD_NAVIGATION_BAR_STYLE_H
#define MD_NAVIGATION_BAR_STYLE_H

// MdNavigationBarStyle — the layout and paint of `MdNavigationBar`.
//
// One row of destinations across a container the variant decides the height of.
// Everything interesting about the bar is in how the row is divided, and there
// are exactly two published answers:
//
//   * **`EqualWeight`** (the default in both families) — Compose wraps each item
//     in `Box(Modifier.weight(1f))`, so the items share the bar's width, and the
//     baseline's `Row` adds `Arrangement.spacedBy(8.dp)` between them.
//   * **`Centered`** — Compose's `CenteredContentMeasurePolicy` insets the run
//     by a *fraction of the bar's width* that shrinks as items are added:
//     `((100 - 10 * (count + 3)) / 2) / 100`. Three items sit in 60 % of the
//     bar, four in 70 %, five in 80 %, and from seven items on there is no inset
//     left. The items then share what remains, so the two arrangements differ
//     only in the band they divide — not in how they divide it.
//
// The paint is a colour and a shape. `container.elevation` (level 2 in both
// families) is carried and **not** painted, for the reason the bottom app bar
// records: a shadow around a widget laid out in place falls outside its rect,
// and Qt clips it. The spec agrees with the omission here — "Differences from
// M2: Elevation: No shadow" — so nothing is lost, and the row is still read
// from the export rather than dropped.

#include "MdStyleBase.h"
#include "core/MdNavigationBarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdNavigationBar;

/// Pattern A style for `MdNavigationBar`.
class QT_MD3_EXPORT MdNavigationBarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdNavigationBarStyle(QObject *parent = nullptr);

    static MdNavigationBarStyle *shared();
    static bool isInstalled();

    struct Layout
    {
        QRectF container;
        QList<qreal> radii;
        /// Where each item goes, in `items()` order. Every box is the full
        /// container height: the bar decides the row, and an item centres its
        /// own content inside whatever it is given — which is what Compose's
        /// `contentAlignment = Center` does.
        QList<QRectF> itemBoxes;
    };

    static Layout layoutFor(const MdNavigationBar &bar, const MdNavigationBarTokens &tokens);

    static void paintNavigationBar(QPainter &painter, const MdNavigationBar &bar,
                                   const MdNavigationBarTokens &tokens, const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_NAVIGATION_BAR_STYLE_H
