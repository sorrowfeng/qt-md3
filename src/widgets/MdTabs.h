#ifndef MD_TABS_H
#define MD_TABS_H

// MdTabs — the MD3 tabs row, both published families.
//
// Compose ships the family as four composables — `PrimaryTabRow` /
// `SecondaryTabRow` and their scrollable counterparts — over one
// `TabRowImpl`. The variants this port exposes are the same two axes:
//
//   * `variant` — primary (a 3 px indicator rounded on top, animated to the
//     selected tab's *content* width, content colours in `primary`) or
//     secondary (a 2 px square indicator spanning the whole tab, content in
//     `on-surface`). Which is Compose's `matchContentSize` split: the primary
//     row builds its indicator with `matchContentSize = true`, the secondary
//     with `false`.
//   * `layout` — `Fixed` (tabs divide the row evenly; the only shapes
//     m3.material.io describes) or `Scrollable` (tabs take their content width
//     from 90 px up, the row keeps a 52 px edge padding, and the selection
//     scrolls itself towards the centre).
//
// ## Where the indicator's geometry comes from
//
// Compose's `TabRowImpl` measures every tab to the equal share, reports each
// tab's `TabPosition(left, width, contentWidth)` — the content width being
// `max(min(intrinsic, tabWidth) - 2 * HorizontalTextPadding, 24)` — and
// animates the indicator's offset and width between those on the *default
// spatial* spring. Its scrollable implementation centres the indicator in the
// tab explicitly; its fixed implementation does not. Flutter's M3 defaults —
// generated from the same token database, `TabBarIndicatorSize.label` for the
// primary family and `.tab` for the secondary — *do* centre it. The two
// implementations disagree and this port follows the centring (see
// porting-todo.md for the recorded divergence).
//
// ## What the row paints and what it does not
//
// The row paints three layers in `paintEvent` — the container (flat `surface`,
// level0, `corner-none`), the deprecated 1 px divider at its bottom (Compose's
// default tab row still paints one), and the indicator above the divider. The
// tabs themselves are child widgets; a Qt child always paints over its
// parent, so during a press the tab's ripple covers the indicator's 3 px —
// Compose places the indicator on top, and the difference is a transient
// overlap at the bottom edge. Recorded in porting-todo.md.
//
// ## Keyboard
//
// Compose makes the row a `selectableGroup`; Left/Right move the selection,
// Home/End jump to the ends. That is behaviour the spec page does not spell
// out and the export publishes no rows for — recorded as a convention.

#include "core/MdTabsTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdTab;
class MdTabsStyle;

class QT_MD3_EXPORT MdTabs : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::MdTabsVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::MdTabsLayout layout READ layout WRITE setLayout NOTIFY layoutChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)

public:
    explicit MdTabs(QWidget *parent = nullptr);
    ~MdTabs() override;

    // --- items ---------------------------------------------------------------
    void addTab(MdTab *tab);
    void insertTab(int index, MdTab *tab);
    void removeTab(MdTab *tab);
    void clearTabs();

    MdTab *tabAt(int index) const;
    int indexOf(const MdTab *tab) const;
    int count() const { return int(m_tabs.size()); }

    // --- selection -------------------------------------------------------------
    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    // --- axes ------------------------------------------------------------------
    MdTabsVariant variant() const { return m_variant; }
    void setVariant(MdTabsVariant variant);

    MdTabsLayout layout() const { return m_layout; }
    void setLayout(MdTabsLayout layout);

    // --- tokens ------------------------------------------------------------------
    const MdTabsTokens &tokens() const;
    void invalidateTokens();

    /// Instance-level `md.comp.*` overrides.
    MdComponentTokens &componentTokens()
    {
        invalidateTokens();
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- indicator animation state ---------------------------------------------
    /// The indicator's offset and width right now, in content coordinates.
    /// Between states they run on the spatial default spring, as Compose's
    /// `TabIndicatorOffsetNode` does. Exposed for the tests.
    qreal indicatorOffset() const { return m_indicatorFrom + (m_indicatorTo - m_indicatorFrom) * m_indicatorProgress; }
    qreal indicatorWidth() const { return m_widthFrom + (m_widthTo - m_widthFrom) * m_indicatorProgress; }

    /// The scrollable row's current scroll offset in content coordinates.
    qreal scrollOffset() const { return m_scrollOffset; }

    // --- geometry ------------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool hasHeightForWidth() const override { return false; }

signals:
    void currentChanged(int index);
    void variantChanged(md::MdTabsVariant variant);
    void layoutChanged(md::MdTabsLayout layout);
    void tabsChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onIndicatorTick();

private:
    void init();
    void applyToTab(MdTab *tab);
    void applyToAllTabs();
    void placeChildren();
    void restartIndicatorAnimation(bool initial = false);
    void restartScrollAnimation();
    void updateIndicatorTargets(bool initial, bool markRun);
    bool tabsWidthKnown() const;

    MdTabsVariant m_variant = MdTabsVariant::Primary;
    MdTabsLayout m_layout = MdTabsLayout::Fixed;
    int m_currentIndex = -1;
    QList<MdTab *> m_tabs;
    bool m_updatingSelection = false;

    qreal m_indicatorFrom = 0.0;
    qreal m_indicatorTo = 0.0;
    qreal m_widthFrom = 0.0;
    qreal m_widthTo = 0.0;
    qreal m_indicatorProgress = 1.0;
    bool m_indicatorHasRun = false;
    QElapsedTimer m_indicatorClock;

    // Scrollable rows only: the offset the content is drawn at, and its
    // spring. Wheel scrolling lands instantly; a selection change animates.
    qreal m_scrollOffset = 0.0;
    qreal m_scrollFrom = 0.0;
    qreal m_scrollTo = 0.0;
    QElapsedTimer m_scrollClock;

    mutable MdTabsTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    QTimer *m_indicatorTimer = nullptr;
};

} // namespace md

#endif // MD_TABS_H
