#ifndef MD_FLOATING_TOOLBAR_H
#define MD_FLOATING_TOOLBAR_H

// MdFloatingToolbar — the MD3 floating toolbar.
//
// m3.material.io: the floating toolbar is one of the two expressive variants
// of Toolbars ("Toolbars display frequently used actions relevant to the
// current page"), and it is the one with a Compose component of its own —
// `HorizontalFloatingToolbar` / `VerticalFloatingToolbar`. Both are modelled
// here by the two `MdToolbarOrientation` values rather than by two classes,
// because the two differ only in their axis: every row in the export has a
// horizontal and a vertical twin (`horizontal.container.height` /
// `vertical.container.width`, `horizontal.container.external-space` /
// `vertical.container.external-space`), and the layouts are exact transposes.
//
// The component is a pill of slots:
//
//   * **leading** and **trailing** exist only while the toolbar is expanded —
//     they are the extra actions a collapsed toolbar hides;
//   * **content** is the toolbar proper and stays through the collapse;
//   * an optional **action button** floats beside the pill, 56 px while the
//     toolbar is expanded and 80 px while it is collapsed, so that *it* grows
//     as the toolbar shrinks and the pair keeps a roughly constant footprint.
//
// Two consequences of that last point shape this class:
//
//   * **The widget is not always the pill.** With an action button the
//     widget's rectangle is the pill *plus* a strip for the button, exactly as
//     Compose's `Layout` reports `width = toolbarMaxWidth + gap + 56`. The
//     widget therefore never paints into that strip and the button is just a
//     child; `MdFloatingToolbarStyle::Layout::container` is where the pill
//     actually is. Without a button the two coincide.
//   * **Collapsing lengths the pill but not the widget.** Compose measures the
//     toolbar at `maxIntrinsicWidth * expandedProgress` inside fixed outer
//     bounds; the slots do not reflow, they are clipped. `setExpanded()` runs
//     the same effects spring the app bars use and drives `expansionProgress`.
//
// The colour schemes are the two published ones — Standard and Vibrant — and
// both tables live in `MdFloatingToolbarTokens`; the item colours are for the
// caller to apply to whatever widgets fill the slots, since a slot here is any
// `QWidget` (`MdIconButton` and `MdButton` are the conventional fillers).
//
// One thing this port cannot do yet: the action button's two size sets (56 /
// 80, with their 24 / 28 px icons and `corner-large` / `corner-large-increased`
// corners) are a *FAB family* concept, and `MdFab` has no size or shape
// property. The toolbar places the child's container at the right size, so a
// plain widget fills it correctly and a future sized `MdFab` will too, but an
// `MdFab` handed in today keeps its own 56 px metrics inside an 80 px box.
// Recorded in docs/porting-todo.md rather than faked.

#include "core/MdFloatingToolbarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdFloatingToolbarStyle;

class QT_MD3_EXPORT MdFloatingToolbar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal expansionProgress READ expansionProgress WRITE setExpansionProgress NOTIFY
                   expansionProgressChanged)
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    Q_PROPERTY(md::MdToolbarOrientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged)
    Q_PROPERTY(md::MdToolbarColorScheme colorScheme READ colorScheme WRITE setColorScheme NOTIFY
                   colorSchemeChanged)
    Q_PROPERTY(md::MdToolbarFabPosition fabPosition READ fabPosition WRITE setFabPosition NOTIFY
                   fabPositionChanged)

public:
    explicit MdFloatingToolbar(QWidget *parent = nullptr);
    ~MdFloatingToolbar() override;

    // --- shape --------------------------------------------------------------
    /// Horizontal (the default) is a Row; Vertical is the same toolbar
    /// transposed. The published layout row is a configuration, not a variant,
    /// which is why it is an enum here and not a second class.
    MdToolbarOrientation orientation() const { return m_orientation; }
    void setOrientation(MdToolbarOrientation orientation);

    /// Standard (the default) is `surface-container`; Vibrant is
    /// `primary-container`. Expressive only.
    MdToolbarColorScheme colorScheme() const { return m_colorScheme; }
    void setColorScheme(MdToolbarColorScheme scheme);

    // --- slots --------------------------------------------------------------
    /// The leading slot, shown only while expanded. A composite leading slot
    /// should be one container widget — the slot is a single child, as
    /// `MdTopAppBar`'s navigation slot is.
    QWidget *leadingWidget() const { return m_leading; }
    void setLeadingWidget(QWidget *widget);

    /// The trailing slot, shown only while expanded.
    QWidget *trailingWidget() const { return m_trailing; }
    void setTrailingWidget(QWidget *widget);

    /// The toolbar's own items, in order from the leading edge. They stay
    /// through the collapse.
    void addContentWidget(QWidget *widget);
    void insertContentWidget(int index, QWidget *widget);
    void removeContentWidget(QWidget *widget);
    void clearContentWidgets();
    QList<QWidget *> contentWidgets() const { return m_content; }

    // --- the action button --------------------------------------------------
    /// The optional action button that floats beside the pill. `nullptr`
    /// removes it and gives the pill back the strip it was reserving.
    QWidget *fab() const { return m_fab; }
    void setFab(QWidget *widget);

    /// Where the button sits on the toolbar's main axis. The default is `End`
    /// — the trailing edge for a horizontal toolbar, the bottom for a vertical
    /// one — which is Compose's own default for both orientations.
    MdToolbarFabPosition fabPosition() const { return m_fabPosition; }
    void setFabPosition(MdToolbarFabPosition position);

    /// The action button's container size right now: 56 px expanded, 80 px
    /// collapsed, interpolated in between (`MdToolbarFabTokens::sizeFor`).
    qreal fabSize() const;

    // --- expansion ----------------------------------------------------------
    bool isExpanded() const { return m_expanded; }
    /// Animated: runs the effects spring from the current progress to 1 or 0.
    void setExpanded(bool expanded);

    /// 0 fully collapsed .. 1 fully expanded. What the style lengths the pill
    /// by and what sizes the action button.
    qreal expansionProgress() const { return m_progress; }
    /// Jump without animating — for a scroll behaviour that is already
    /// tracking a gesture.
    void setExpansionProgress(qreal progress);

    // --- tokens -------------------------------------------------------------
    const MdFloatingToolbarTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides. Mutating through this accessor
    /// drops the cache, so an override takes effect on the next repaint.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void widgetsChanged();
    void expandedChanged(bool expanded);
    void expansionProgressChanged(qreal progress);
    void orientationChanged(md::MdToolbarOrientation orientation);
    void colorSchemeChanged(md::MdToolbarColorScheme scheme);
    void fabPositionChanged(md::MdToolbarFabPosition position);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void repolish();
    void placeChildren();
    bool adopt(QWidget **slot, QWidget *widget);

    QWidget *m_leading = nullptr;
    QWidget *m_trailing = nullptr;
    QWidget *m_fab = nullptr;
    QList<QWidget *> m_content;

    MdToolbarOrientation m_orientation = MdToolbarOrientation::Horizontal;
    MdToolbarColorScheme m_colorScheme = MdToolbarColorScheme::Standard;
    MdToolbarFabPosition m_fabPosition = MdToolbarFabPosition::End;

    bool m_expanded = true;
    qreal m_progress = 1.0;

    QTimer *m_expandTimer = nullptr;
    int m_expandElapsedMs = 0;
    qreal m_expandFrom = 1.0;
    qreal m_expandTo = 1.0;

    mutable MdFloatingToolbarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_FLOATING_TOOLBAR_H
