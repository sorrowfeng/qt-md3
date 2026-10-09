#ifndef MD_BOTTOM_APP_BAR_STYLE_H
#define MD_BOTTOM_APP_BAR_STYLE_H

// MdBottomAppBarStyle — the layout and paint of `MdBottomAppBar`.
//
// The bottom app bar is a single centred row, so its layout is Compose's
// `BottomAppBarLayout` and nothing else: one container, a content band inset by
// `contentLeadingSpace` / `contentTopSpace` / `contentTrailingSpace`, and — when
// the caller docks a floating action button — a box at the trailing edge whose
// own padding puts the FAB's *container* 16 px from the edge and 12 px from the
// top.
//
// The layout places **containers**, never widgets. Every child that can show a
// focus ring reserves the ring's room inside itself (`MdIconButton` is a
// 55 x 55 widget around a 40 x 40 container, `MdFab` a 71 x 71 one around a
// 56 x 56 container), so laying a child out by its `sizeHint` parks the visible
// box 7.5 px inside the token position and spaces siblings 15 px too far apart.
// `MdChildBox` is the shared answer and carries the full reasoning; this file
// just applies it — `layoutFor` reports the *container* box each child belongs
// on, and `placeChildren()` offsets the widget onto it.
//
// One consequence is inherited from `MdButtonGroup` and is deliberate rather
// than overlooked: two children whose containers are meant to touch have widget
// rects that overlap by `2 * margin` (15 px for two icon buttons). The overlap
// is transparent — the widget draws nothing outside its container — and it is
// the price of exact container geometry in Qt, where a child is clipped to its
// own rectangle and so cannot paint a ring outside it. See docs/porting-todo.md.
//
// One asymmetry is Compose's and is reproduced rather than corrected:
// `BottomAppBarDefaults.ContentPadding` is `PaddingValues(start = 4, top = 4,
// end = 4)` with **no bottom**, so the band the children are centred in runs
// from `top + 4` to the bottom edge. A 1-line of content therefore sits 2 px
// below the container's true centre. Changing it would look "nicer" and be
// wrong.

#include "core/MdAppBarTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdBottomAppBar;

/// Pattern A style for `MdBottomAppBar`.
class QT_MD3_EXPORT MdBottomAppBarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdBottomAppBarStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdBottomAppBarStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted container — the whole widget.
        QRectF container;
        QList<qreal> radii;
        /// The band the children are distributed in.
        QRectF content;
        /// The docked FAB's *container* target rectangle: 56 x 56 (for the
        /// default medium FAB) placed 16 px from the container's trailing edge
        /// and 12 px from its top. Empty when the bar has no FAB.
        QRectF fabBox;
        /// The band left for the row's own children: the content band, minus
        /// the FAB's strip when there is one.
        QRectF actions;
        /// The container box each of the bar's own children belongs on, in
        /// `widgets()` order — what the arrangement actually distributes.
        QList<QRectF> childBoxes;
    };

    static Layout layoutFor(const MdBottomAppBar &bar, const MdBottomAppBarTokens &tokens);

    static void paintBottomAppBar(QPainter &painter, const MdBottomAppBar &bar,
                                  const MdBottomAppBarTokens &tokens, const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_BOTTOM_APP_BAR_STYLE_H
