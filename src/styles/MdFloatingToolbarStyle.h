#ifndef MD_FLOATING_TOOLBAR_STYLE_H
#define MD_FLOATING_TOOLBAR_STYLE_H

// MdFloatingToolbarStyle — the layout and paint of `MdFloatingToolbar`.
//
// Unlike the docked toolbar, the floating toolbar **is** a Compose component:
// `HorizontalFloatingToolbar` and `VerticalFloatingToolbar` in
// `FloatingToolbar.kt`, which between them cover five of the file's six
// private layouts. Those are the behaviour source; the export supplies the
// numbers; the spec page supplies the taxonomy. material-web ships no
// toolbars implementation at all.
//
// **The widget's rectangle is not always the pill.** A floating toolbar is a
// pill — `corner-full`, `surface-container` (or `primary-container` when
// vibrant), 64 px across, its slots inset 8 px all round. When the caller
// docks an action button, Compose's `Layout` measures *both* children and
// reports `width = toolbarMaxWidth + toolbarToFabGap + FabSizeRange.start`,
// so the composable's bounds include a strip for the FAB and the pill occupies
// only the rest:
//
//   * a **horizontal** toolbar keeps the pill on the left when the FAB is at
//     `End` (the default), and on the right when it is at `Start`;
//   * a **vertical** toolbar is the same transposed, and the strip is at the
//     bottom for the default (`Bottom`, i.e. `End`) position.
//
// The strip is reserved at the **expanded** FAB size (56) even though the FAB
// is 80 when the toolbar is collapsed, which is why a collapsed toolbar's
// action button overlaps the pill's ruins — that is Compose's arithmetic, not
// an error: `width - 80` really does land 16 px inside `width - 64`.
//
// **The pill lengths along the main axis with `expandedProgress`.** Compose
// measures the toolbar at `maxIntrinsicWidth * expandedProgress`, so at
// progress 0 the pill has no extent at all and at progress 1 it is its natural
// size. The content inside does *not* reflow while that happens — the
// toolbar's Row carries `horizontalScroll(rememberScrollState())`, so the
// slots stay put and the pill's own rectangle clips them. `layoutFor`
// therefore lays the bands out against the **expanded** pill and reports a
// `container` that is `progress` times as long; this is the same split the
// docked toolbar makes, where the whole bar collapses instead.
//
// The clipping itself is the widget's job rather than this style's: Compose
// gets it from `graphicsLayer { clip = true; shape = shape }`, while Qt clips a
// child to its *widget* and not to a shape drawn inside it. `MdFloatingToolbar`
// therefore hides a child whose container has left the pill rather than letting
// it float over the page behind — a discretised version of the same reveal.
// Recorded in docs/porting-todo.md.
//
// **Leading and trailing slots exist only while the toolbar is expanded.**
// Compose wraps each in `AnimatedVisibility(visible = expandedState)`, so they
// are absent from the layout at progress 0 and present at progress > 0. They
// are the "extra" actions a collapsed toolbar hides to leave room for the
// action button that replaces them.
//
// **The slots are separated by `container.between-space` (4 px).** This is one
// of the very few places where this port is *more* faithful than its behaviour
// source: Compose declares `FloatingToolbarTokens.ContainerBetweenSpace` and
// never reads it — `grep` finds the row only in its own token file, because a
// `HorizontalFloatingToolbar`'s items are arranged by the *caller's* `Row` and
// upstream ships no spacing at all. This widget owns the arrangement of its
// children, so it applies the published value: the run is
// `leading + content + trailing` with one gap between each neighbouring pair,
// which is why the pill is `8 + n * item + (n - 1) * 4 + 8` long. Recorded in
// docs/porting-todo.md, and pinned by
// `TestMd3Toolbar::betweenSpaceSeparatesTheSlots` so it cannot quietly become
// another carried-but-unread row.
//
// Because the pill's own accent is the FAB and not the toolbar, the container
// shape never morphs: there is no pressed shape here, which is the one thing
// every other shape-bearing component in this library does differently.
//
// The layout places **containers**, never widgets — see `MdChildBox` for the
// 7.5 px focus-ring margin and the deliberate widget-rect overlap it implies.
// The one place that differs is the action button: its size is *not* a token
// the component publishes but the FAB's own two size sets, so it is placed
// with `MdChildBox::resizedGeometryOn` and asked to take 56 or 80 px.

#include "MdStyleBase.h"
#include "core/MdFloatingToolbarTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdFloatingToolbar;

/// Pattern A style for `MdFloatingToolbar`.
class QT_MD3_EXPORT MdFloatingToolbarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdFloatingToolbarStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdFloatingToolbarStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted pill, at the *current* expansion. Its main-axis extent
        /// is the expanded extent times `expandedProgress`, so it is empty
        /// when the toolbar is fully collapsed.
        QRectF container;
        QList<qreal> radii;

        /// The band the slots are distributed in, at **full** expansion — the
        /// pill inset by the content padding. Not the same rectangle as the
        /// container once the toolbar is collapsing.
        QRectF content;

        /// The container box each slot child belongs on. `leadingChildBoxes`
        /// and `trailingChildBoxes` are empty while the toolbar is collapsed.
        QList<QRectF> leadingChildBoxes;
        QList<QRectF> contentChildBoxes;
        QList<QRectF> trailingChildBoxes;

        /// The action button's container box — 56 px when expanded, 80 px when
        /// collapsed — and the pill radii that match it. Empty when the
        /// toolbar carries no action button. The box is sized by the FAB's own
        /// two size sets rather than by the child's `sizeHint()`.
        QRectF fabBox;
        QList<qreal> fabRadii;
    };

    static Layout layoutFor(const MdFloatingToolbar &toolbar,
                            const MdFloatingToolbarTokens &tokens);

    static void paintFloatingToolbar(QPainter &painter, const MdFloatingToolbar &toolbar,
                                     const MdFloatingToolbarTokens &tokens, const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_FLOATING_TOOLBAR_STYLE_H
