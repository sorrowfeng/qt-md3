#ifndef MD_TOP_APP_BAR_STYLE_H
#define MD_TOP_APP_BAR_STYLE_H

// MdTopAppBarStyle — the layout and paint of `MdTopAppBar`.
//
// The layout is Compose's `TopAppBarLayout` + `TopAppBarMeasurePolicy`, which
// is the only place these numbers exist: material-web has no production top
// app bar, so nothing in the export describes how a title, a leading button
// and an action row divide a bar. Three consequences worth stating:
//
//   * **The 4 px is not a mystery.** Compose wraps each slot in
//     `padding(horizontal = TopAppBarHorizontalPadding)` where that constant
//     *is* the published `leading-space` / `trailing-space` (4 px). A
//     leading icon button therefore starts 4 px from the edge and its own
//     12 px of internal padding brings the glyph to 16 px — which is what the
//     spec's "16dp from the edge" measurement means. The title carries the
//     same 4 px on *both* sides of its own box, so a leading-aligned title
//     lands at `max(titleInset, nav+4) + 4` px.
//   * **A title is centred on the whole bar, then pushed.** Compose aligns the
//     title within `constraints.maxWidth` first and only then slides it right
//     past the navigation element or left past the actions. The result is that
//     a centred title stays centred on the *bar*, not in the gap between its
//     neighbours — visibly different when the two sides differ in width.
//   * **A two-row bar is two `TopAppBarLayout`s in a column.** The first is
//     `collapsedHeight` tall and holds the navigation button, the actions and
//     the *small* title; the second is `expandedHeight - collapsedHeight` tall
//     and holds only the expanded title and subtitle, bottom-aligned from
//     their last baseline. The two titles cross-fade: the expanded one on
//     `1 - collapsedFraction`, the small one on `TopTitleAlphaEasing`.

#include "core/MdAppBarTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdTopAppBar;

/// Pattern A style for `MdTopAppBar`: the container, the text, and the
/// geometry every slot widget is placed into.
class QT_MD3_EXPORT MdTopAppBarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTopAppBarStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdTopAppBarStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// Where everything goes for the bar's current size and state.
    struct Layout
    {
        /// The painted container — the whole widget.
        QRectF container;
        QList<qreal> radii;

        /// The row holding the navigation button and the actions. Equal to
        /// `container` for a single-row bar.
        QRectF leadingRow;
        /// The text row of a two-row bar; empty for a single-row bar.
        QRectF textRow;

        /// The leading slot: the navigation widget's **container** box,
        /// vertically centred in the row. A container box, not a widget box —
        /// see `MdChildBox`.
        QRectF navigation;
        /// The trailing band the action widgets are distributed in, 4 px in
        /// from the bar's trailing edge. A band rather than a box: it spans all
        /// of them.
        QRectF actions;
        /// The container box of each action widget, in order, already separated
        /// by `iconButtonSpace` and centred vertically. Parallel to
        /// `MdTopAppBar::actionWidgets()`.
        QList<QRectF> actionBoxes;
        /// The centre slot (a search field), spanning the gap between the two.
        /// This one is a *widget* box, not a container box: the field fills the
        /// gap the way Compose's `fillMaxWidth()` does. Empty when the bar has
        /// no centre widget.
        QRectF center;

        /// The expanded title's text area — already inside the title box's own
        /// 4 px pads — and the subtitle's.
        QRectF title;
        QRectF subtitle;
        /// The first row's small title / subtitle, for a two-row bar.
        QRectF leadingTitle;
        QRectF leadingSubtitle;

        /// True when the bar lays its title out in a second row.
        bool twoRows = false;
        /// The first row's small-title opacity, `TopTitleAlphaEasing` applied
        /// to the collapsed fraction.
        qreal leadingTitleAlpha = 0.0;
        /// The expanded title's opacity, `1 - collapsedFraction`.
        qreal titleAlpha = 1.0;
        /// True when there is room to draw the expanded title at all.
        bool hasTitle = true;
    };

    static Layout layoutFor(const MdTopAppBar &bar, const MdAppBarTokens &tokens,
                            qreal collapsedFraction);

    /// `TopAppBarColors.containerColor(fraction)` — the two roles interpolated
    /// on `FastOutLinearInEasing` and, following Compose's
    /// `animateColorAsState`, on the Oklab colour vector converter.
    static QColor containerColorFor(const MdAppBarTokens &tokens, qreal fraction);

    /// The fraction `containerColorFor` is asked for.
    ///
    /// A single-row bar has no height to collapse, so its colour reads the
    /// *overlapped* fraction and steps at Compose's `> 0.01` threshold; a
    /// two-row bar reads its collapsed fraction directly, which is why a large
    /// bar's colour tracks its shrink continuously.
    static qreal colorTransitionFraction(const MdAppBarTokens &tokens, qreal collapsedFraction,
                                         qreal overlappedFraction);

    /// `FastOutLinearInEasing` = `cubic-bezier(0.4, 0, 1, 1)`.
    static qreal fastOutLinearIn(qreal t);

    static void paintTopAppBar(QPainter &painter, const MdTopAppBar &bar,
                               const MdAppBarTokens &tokens, const Layout &layout,
                               const QColor &containerColor);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TOP_APP_BAR_STYLE_H
