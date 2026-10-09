#ifndef MD_NAVIGATION_RAIL_TOKENS_H
#define MD_NAVIGATION_RAIL_TOKENS_H

// MdNavigationRailTokens — the MD3 navigation rail, both published families.
//
// ## Two families again, and the rail is where they actually meet
//
// Like the bar (see MdNavigationBarTokens.h for the full two-families story),
// the rail ships a baseline family (`md.comp.navigation-rail.*`) and a
// flexible one (`nav-rail-collapsed` / `nav-rail-expanded` / `nav-rail` +
// `nav-rail-item*`), both at export version 34.0.21. The spec's words are:
// "A new flexible navigation rail was introduced to replace the baseline
// navigation rail", and — the part that makes this family different — "the
// expanded navigation rail replaces the navigation drawer".
//
// The three sources line up the same way as the bar's:
//
//   * material-web consumes the baseline rows (`v0_192/navigation-rail`) and
//     ships no rail product at all;
//   * Compose's `NavigationRail.kt` is the baseline behaviour, and its
//     `WideNavigationRail.kt` (1711 lines) is the flexible one — the flexible
//     rail is where Compose *keeps investing*;
//   * the spec's Availability table reads `Web: Unavailable` for the flexible
//     rows.
//
// ## The item is the bar's item
//
// Compose is explicit about it: `WideNavigationRailItem` and
// `ShortNavigationBarItem` are both thin wrappers over the *one* expressive
// `NavigationItem` composable, differing in which padding constants they hand
// it. This port does the same thing with Qt means: the rail's item rows live
// in a `MdNavigationBarVariantTokens` (the `item` field below) that is pushed
// into a shared `MdNavigationBarItem`, whose geometry already handles both the
// `Top` (collapsed) and `Start` (expanded) arrangements. The colour tables
// agree with the bar's row for row in both families — `nav-rail.scss` against
// `nav-bar.scss`, `navigation-rail.scss` against `navigation-bar.scss` — so
// `resolve()` fills them with the shared `fillNavigationItemColours`.
//
// ## What the collapsed / expanded pair means
//
// The flexible rail has **one container token file with two widths**: 96
// collapsed (80 `narrow`, a row Compose declares and never reads), 220–360
// expanded, with the expanded width driven by the content (the widest item
// plus 20 px of trailing room) inside those bounds. `expanded` is therefore a
// *state* of the rail, not a variant — see `MdNavigationRail`. The modal rows
// (`modal-container-*`) belong to the same flexible family: the expanded modal
// rail is the drawer's replacement, arriving over a scrim.
//
// ## Behaviour rows Compose hard-codes
//
// The baseline publishes no spacing rows and Compose's `NavigationRail.kt`
// hard-codes all three: 4 px between items (`spacedBy(4)`), 4 px of item
// vertical padding, 8 px after the header. The flexible rail's spacing rows
// exist (`item-vertical-space: 4`, `between-item-space: 0px`) but its header
// gap reads the *baseline item* family's `header-space-minimum` (40) — a
// cross-file reference recorded below — and its expanded item padding is
// Compose's hard-coded 20 (numerically equal to the expanded family's
// `vertical-trailing-space`, from a different source).

#include "MdNavigationBarTokens.h"
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which published family the rail is drawn from. Same split as the bar's.
enum class MdNavigationRailVariant {
    Baseline,
    Flexible,
    Count,
};

constexpr int navRailVariantCount = int(MdNavigationRailVariant::Count);

/// Compose's `Arrangement.Vertical` for the items: `Top` stacks them under the
/// header; `Center` centres the run in the container's whole height.
enum class MdNavigationRailArrangement {
    Top,
    Center,
    Count,
};

constexpr int navRailArrangementCount = int(MdNavigationRailArrangement::Count);

/// One family's rows. The `item` field is pushed into the shared
/// `MdNavigationBarItem` — see the header note.
struct QT_MD3_EXPORT MdNavigationRailVariantTokens
{
    // --- container ----------------------------------------------------------
    /// Baseline 80, flexible collapsed 96.
    qreal containerWidth = 80.0;
    /// The narrow collapsed width. The baseline's *item* width in Compose
    /// (`NavigationRailItemWidth = NarrowContainerWidth`, 80); the flexible
    /// family publishes the row (80) and Compose reads it nowhere — carried.
    qreal narrowContainerWidth = 80.0;
    /// Expanded bounds. The baseline family publishes no expanded rows at all,
    /// so 0 here means "this rail does not expand" — `supportsExpanded()`.
    qreal expandedWidthMinimum = 0.0;
    qreal expandedWidthMaximum = 0.0;
    /// The flexible rail's 44 px top inset (`top-space`, Compose's
    /// `WNRTopPadding`). The baseline rail has no such row; its items start
    /// after a hard-coded 4 px container padding.
    qreal containerTopSpace = 0.0;
    /// The baseline rail's hard-coded 4 px vertical container padding.
    qreal containerVerticalPadding = 0.0;
    /// The gap between items. Baseline: Compose's hard-coded `spacedBy(4)`.
    /// Flexible collapsed: the published `item-vertical-space` (4). Expanded,
    /// the flexible family's own `between-item-space` (0) replaces it — the
    /// layout picks by state, not by variant.
    qreal itemVerticalSpace = 4.0;
    qreal expandedBetweenItemSpace = 0.0;
    /// The gap after the header. Baseline: Compose's hard-coded 8. Flexible:
    /// the *baseline item* family's `header-space-minimum` (40), which Compose's
    /// wide rail reads across files.
    qreal headerSpace = 8.0;
    /// Room left of the expanded rail's items (Compose's hard-coded
    /// `WNRItemHorizontalPadding`, 20 — numerically the expanded family's
    /// `vertical-trailing-space`, from a different source).
    qreal expandedItemPadding = 0.0;

    ColorRole containerColor = ColorRole::Surface;
    ElevationLevel containerElevation = ElevationLevel::Level0;
    ShapeCorner containerShape = ShapeCorner::None;

    /// The modal rows — the flexible family only. The modal expanded rail is
    /// the drawer's replacement and arrives over a scrim.
    ColorRole modalContainerColor = ColorRole::SurfaceContainer;
    ElevationLevel modalContainerElevation = ElevationLevel::Level2;
    ShapeCorner modalContainerShape = ShapeCorner::Large;

    bool supportsExpanded() const { return expandedWidthMinimum > 0.0; }

    // --- item ---------------------------------------------------------------
    /// Pushed into every `MdNavigationBarItem`. Collapsed, the rail's items
    /// are `Top`-arranged with `alwaysShowLabel = false`; expanded, they are
    /// `Start`-arranged and show their labels — the two states the shared
    /// item's geometry already carries.
    MdNavigationBarVariantTokens item;
};

struct QT_MD3_EXPORT MdNavigationRailTokens
{
    MdNavigationRailVariantTokens variant[navRailVariantCount];

    const MdNavigationRailVariantTokens &forVariant(MdNavigationRailVariant variant) const;

    /// Resolves both families. `item.colours` are not overridable per instance;
    /// lengths and shapes are read back from the store under the rail's own
    /// component names.
    static MdNavigationRailTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_NAVIGATION_RAIL_TOKENS_H
