#ifndef MD_DOCKED_TOOLBAR_STYLE_H
#define MD_DOCKED_TOOLBAR_STYLE_H

// MdDockedToolbarStyle — the layout and paint of `MdDockedToolbar`.
//
// A docked toolbar is one full-width row. Its layout is Compose's
// `BottomAppBarLayout` with `DockedToolbarTokens` substituted for the bottom
// app bar's rows — which is exactly how Compose realises the docked toolbar,
// since `FlexibleBottomAppBar` is the only place the variant exists as
// behaviour:
//
//   BottomAppBarLayout(
//       containerHeight = BottomAppBarDefaults.FlexibleBottomAppBarHeight, // 64
//       contentPadding  = BottomAppBarDefaults.FlexibleContentPadding,     // 16 / 16
//       horizontalArrangement = BottomAppBarDefaults.FlexibleHorizontalArrangement,
//   )
//
//   FlexibleHorizontalArrangement = Arrangement.spacedBy(
//       DockedToolbarTokens.ContainerMaxSpacing /* 32 */, Alignment.CenterHorizontally)
//
// So the row is *centred* with at least 32 px between items. `spacedBy` with a
// horizontal alignment does not clamp to the start when the content overflows:
// it overflows on both sides, still centred. That is reproduced here rather
// than "fixed" — a docked toolbar with more items than fit has always spilled
// evenly.
//
// The layout places **containers**, never widgets, for the reason
// `MdChildBox` documents: a child that can show a focus ring reserves the
// ring's room inside its own rect (`MdIconButton` is 55 x 55 around a
// 40 x 40 container), so laying a child out by `sizeHint()` parks the visible
// box 7.5 px inside the token position and spaces siblings 15 px too far
// apart. Two children meant to touch therefore have widget rects that overlap
// by 15 px; the overhang is transparent and the overlap is deliberate.
//
// The collapse is the *whole* bar, not a row. Compose sets
// `heightOffsetLimit = -placeable.height`, so scrolling a docked toolbar away
// takes its entire height, unlike a two-row top app bar which keeps its icon
// row. `layoutFor` therefore lays the children out against the **expanded**
// geometry and only `container` follows the collapsed height: the content does
// not reflow as the bar shrinks, it is clipped by the widget's own rect —
// which is what Compose does by laying the `Surface` out shorter than the
// `Row` it contains.

#include "MdStyleBase.h"
#include "core/MdDockedToolbarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdDockedToolbar;

/// Pattern A style for `MdDockedToolbar`.
class QT_MD3_EXPORT MdDockedToolbarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdDockedToolbarStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdDockedToolbarStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted container — as tall as the bar currently is, which is
        /// `expanded` when the bar is not scrolled away.
        QRectF container;
        QList<qreal> radii;
        /// The band the children are distributed in, at **expanded** height.
        QRectF content;
        /// The container box each of the bar's children belongs on, in
        /// `widgets()` order — what the centred arrangement produces.
        QList<QRectF> childBoxes;
    };

    static Layout layoutFor(const MdDockedToolbar &toolbar, const MdDockedToolbarTokens &tokens);

    static void paintDockedToolbar(QPainter &painter, const MdDockedToolbar &toolbar,
                                   const MdDockedToolbarTokens &tokens, const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_DOCKED_TOOLBAR_STYLE_H
