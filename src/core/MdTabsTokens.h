#ifndef MD_TABS_TOKENS_H
#define MD_TABS_TOKENS_H

// MdTabsTokens — the MD3 tabs, both published families.
//
// ## Two families, one component name
//
// The export ships `md.comp.primary-navigation-tab.*` and
// `md.comp.secondary-navigation-tab.*` at 34.0.21, and the two differ in more
// than numbers:
//
//   * **the indicator's shape** — primary publishes `active-indicator.shape:
//     3px 3px 0px 0px` (a pill rounded on its top corners only); secondary
//     publishes no shape row, so its 2 px indicator is a plain rectangle;
//   * **the indicator's width semantics** — Compose's `PrimaryTabRow` builds
//     its indicator with `matchContentSize = true`, so the primary indicator's
//     width *animates* to the selected tab's content width; `SecondaryTabRow`
//     uses `matchContentSize = false`, so its indicator spans the whole tab;
//   * **the state-layer rows** — primary keeps `active.*` and `inactive.*`
//     tables (the inactive pressed layer is `primary`, where the inactive
//     hover and focus layers are `on-surface`); secondary publishes one shared
//     table, all `on-surface`.
//
// material-web ships neither family as a component (only the two token files);
// the numbers come from the export and the behaviour from Compose's
// `TabRow.kt` — the same standing as the app bars.
//
// ## Divergences pinned here rather than smoothed
//
// * **the icon+label container height: 64, not 72.** The export and the spec
//   page agree on `with-icon-and-label-text.container.height: 64px`, and
//   Compose *declares* `IconAndLabelTextContainerHeight = 64` — but its
//   `Tab.kt` hard-codes `LargeTabHeight = 72.dp` in the behaviour, a residue of
//   the M2 tab. Three sources against one hard-coded number: 64 wins, the 72
//   is recorded here.
// * **the secondary indicator height: 2, not 3.** The secondary export
//   publishes `active-indicator-height: 2px`; Compose's
//   `SecondaryNavigationTabTokens` declares no height row at all, so its
//   `SecondaryIndicator` falls back to the *primary* default of 3 through a
//   default parameter. The export's own row wins.
// * **the divider rows are deprecated** ("use standalone divider component"),
//   but Compose's default tab row still paints a divider at the bottom, so the
//   rows are carried and painted.
// * **no disabled rows** in either family, and Compose's `Tab` writes no
//   disabled colour either — the system 0.38 disabled alpha is applied to the
//   content, as the navigation families do.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which published family the tabs are drawn from.
enum class MdTabsVariant {
    Primary,
    Secondary,
    Count,
};

constexpr int tabsVariantCount = int(MdTabsVariant::Count);

/// Whether the row divides its width evenly between the tabs (`Fixed`) or lays
/// them out at their content width inside a scrolling viewport (`Scrollable`).
/// Compose: `PrimaryTabRow` vs `PrimaryScrollableTabRow`.
enum class MdTabsLayout {
    Fixed,
    Scrollable,
    Count,
};

constexpr int tabsLayoutCount = int(MdTabsLayout::Count);

/// Whether a tab paints its icon above its label (the default `Tab`) or
/// leading it (Compose's `LeadingIconTab`, 48 px tall either way).
enum class MdTabIconPosition {
    Top,
    Start,
    Count,
};

constexpr int tabIconPositionCount = int(MdTabIconPosition::Count);

/// One family's rows. Both families resolve into this same struct.
struct QT_MD3_EXPORT MdTabsVariantTokens
{
    // --- container ----------------------------------------------------------
    qreal containerHeight = 48.0;
    /// The height a tab takes when it shows icon *and* label. The secondary
    /// export publishes no such row (its gallery usage is text-only) and
    /// inherits the primary's 64 — see the header note on the 72 divergence.
    qreal iconLabelTextContainerHeight = 64.0;
    qreal iconSize = 24.0;
    /// Primary 3, secondary 2.
    qreal activeIndicatorHeight = 3.0;
    /// Primary publishes `3px 3px 0px 0px` — rounded on top only. Secondary
    /// publishes no shape row, so its indicator is square.
    bool activeIndicatorTopRounded = true;
    /// Deprecated; carried and painted (Compose paints a bottom divider too).
    qreal dividerHeight = 1.0;

    ColorRole containerColor = ColorRole::Surface;
    ElevationLevel containerElevation = ElevationLevel::Level0;
    ShapeCorner containerShape = ShapeCorner::None;
    /// Secondary only (`container.shadow-color`); the primary publishes none.
    ColorRole containerShadowColor = ColorRole::Shadow;
    ColorRole dividerColor = ColorRole::SurfaceVariant;
    ColorRole activeIndicatorColor = ColorRole::Primary;

    // --- focus indicator ------------------------------------------------------
    /// Both families read the shared focus-indicator rows: `secondary`, the
    /// system inner offset and thickness.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- state-layer opacities ------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- content colours, indexed by state ------------------------------------
    // The primary family's tables differ between active and inactive tabs —
    // including the one-row special case where the *inactive pressed* layer is
    // `primary`. The secondary family's tables are shared, so its resolve
    // fills both sides with the same values.
    MdNavigationColourSlot activeLabel[int(MdNavigationItemState::Count)];
    MdNavigationColourSlot inactiveLabel[int(MdNavigationItemState::Count)];
    MdNavigationColourSlot activeIcon[int(MdNavigationItemState::Count)];
    MdNavigationColourSlot inactiveIcon[int(MdNavigationItemState::Count)];
    /// The state layer behind an active (selected) tab.
    MdNavigationColourSlot activeStateLayer[int(MdNavigationItemState::Count)];
    /// The state layer behind an inactive tab.
    MdNavigationColourSlot inactiveStateLayer[int(MdNavigationItemState::Count)];

    /// `label-text-*`: `title-small` in both families.
    TypeStyle labelTextType = TypeStyle::TitleSmall;

    // --- Compose behaviour constants (not export rows) -------------------------
    /// `Tab.kt`'s `HorizontalTextPadding`: the horizontal padding around the
    /// label, and the amount subtracted from a tab's width to get the
    /// indicator's content width.
    qreal horizontalTextPadding = 16.0;
    /// `Tab.kt`'s `TextDistanceFromLeadingIcon` — the gap between a leading
    /// icon and its label.
    qreal textDistanceFromLeadingIcon = 8.0;
    /// `TabRowDefaults.ScrollableTabRowEdgeStartPadding`.
    qreal scrollableEdgePadding = 52.0;
    /// `TabRowDefaults.ScrollableTabRowMinTabWidth`.
    qreal scrollableMinTabWidth = 90.0;
    /// The smallest width the indicator is allowed to take — Compose's
    /// "enforce minimum touch target of 24.dp".
    qreal indicatorMinimumWidth = 24.0;

    const MdNavigationColourSlot &activeLabelFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &inactiveLabelFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &activeIconFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &inactiveIconFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &activeStateLayerFor(MdNavigationItemState state) const;
    const MdNavigationColourSlot &inactiveStateLayerFor(MdNavigationItemState state) const;
};

struct QT_MD3_EXPORT MdTabsTokens
{
    MdTabsVariantTokens variant[tabsVariantCount];

    const MdTabsVariantTokens &forVariant(MdTabsVariant variant) const;

    /// Resolves both families. Colours are not overridable library-wide; the
    /// lengths, shapes and opacities are read back under each family's own
    /// component name.
    static MdTabsTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_TABS_TOKENS_H
