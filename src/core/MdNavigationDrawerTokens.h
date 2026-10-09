#ifndef MD_NAVIGATION_DRAWER_TOKENS_H
#define MD_NAVIGATION_DRAWER_TOKENS_H

// MdNavigationDrawerTokens — the MD3 navigation drawer, its one token family.
//
// "Navigation drawers provide access to destinations in your app" — a
// start-anchored sheet of destinations, headed by an optional headline. The
// spec ships **one** family here (`md.comp.navigation-drawer.*`, 64 rows at
// export version 34.0.21); there is no flexible second family. The M3
// Expressive update is blunt about the future: "the expanded navigation rail
// replaces the navigation drawer" — which is why `MdNavigationRail` carries
// the modal rows already. The drawer is ported as published.
//
// ## The item is NOT the shared expressive item
//
// Compose is explicit here too: `NavigationDrawerItem` is its own composable,
// not a wrapper over the shared `NavigationItem` — because the geometry is
// different in kind. The pill **is the item**: a full-width, 56 px-minimum
// full-corner row whose container colour *is* the selected state
// (`secondary-container` when selected, transparent when not), with no width
// animation and a `badge` slot at the trailing edge. So this family gets its
// own `MdNavigationDrawerItem` + `MdNavigationDrawerItemStyle`, and its own
// colour fill — the export's table disagrees with the bar's:
//
//   * every *active* row (icon / label / state layer, in all states) is
//     `on-secondary-container`, where the bar's active label is `on-surface`;
//   * the *inactive pressed state layer* is `on-secondary-container` — a
//     one-row special case, where hover and focus read `on-surface`;
//   * the label is `label-large` (prominent when selected), not
//     `label-medium`.
//
// ## Two container token sets, one file
//
// The variants are a single file's two container row groups: `standard` /
// `permanent` (`surface` at level0) and `modal` (`surface-container-low` at
// level1, arriving over a scrim). Dismissible vs permanent is behaviour —
// placement and gestures the host owns — not token rows, so the enum is
// `Modal` / `Standard` and nothing else.
//
// ## Rows carried and not painted
//
//   * **the scrim.** `scrim-color: neutral-variant20` at `scrim-opacity:
//     0.4` belongs to a window overlay, not a child widget: a modal drawer's
//     scrim covers its *parent*, which Qt does not let a child paint. The
//     colour and opacity are carried on the tokens (`scrimColor()`,
//     `scrimOpacity()`) for a host to build its overlay from — recorded in
//     docs/porting-todo.md, same grounds as the bar's elevation. Divergence
//     recorded: the library's `ColorRole::Scrim` resolves from `neutral0`,
//     where the export names `neutral-variant20`; the library has no
//     per-component colour rows, so the semantic role is what is carried.
//   * **the modal elevation.** level1 falls outside a child sheet's rect, as
//     the bar's level2 does.
//   * **the large-badge rows.** `large-badge-label-*` are published, and the
//     badge slot exists — as a text label painted in those rows. Compose's
//     badge is an arbitrary composable slot; a text is what the export's
//     rows describe.
//
// ## Behaviour rows Compose hard-codes
//
// The export publishes no content-spacing rows; `NavigationDrawerItem`'s Row
// is `padding(start = 16, end = 24)` with a 12 px icon-to-label gap (and 12
// before the badge), and `NavigationDrawerItemDefaults.ItemPadding` is
// `horizontal = 12` — the 12 that turns the 360 container into the 336 pill.
// All four are carried as the behaviour's numbers.

#include "MdNavigationBarTokens.h"
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which container row group the drawer draws from — see the header note.
enum class MdNavigationDrawerVariant {
    Modal,
    Standard,
    Count,
};

constexpr int navDrawerVariantCount = int(MdNavigationDrawerVariant::Count);

/// One item's rows. The pill is the item itself, so there is no pill-in-item
/// arithmetic here: geometry is content inset inside the item's own rect.
struct QT_MD3_EXPORT MdNavigationDrawerItemTokens
{
    // --- geometry -----------------------------------------------------------
    /// `active-indicator-height`: 56, the row's minimum height.
    qreal activeIndicatorHeight = 56.0;
    /// `active-indicator-width`: 336 — carried, and derivable: the 360
    /// container minus the behaviour's 2 x 12 `itemPadding`. The item paints
    /// at whatever width its container gives it, which is what Compose's
    /// `fillMaxWidth` does.
    qreal activeIndicatorWidth = 336.0;
    qreal iconSize = 24.0;
    /// Compose's `ItemPadding` (horizontal = 12, hard-coded): the inset the
    /// owning drawer places the item at.
    qreal itemPadding = 12.0;
    /// Compose's content Row insets: `start = 16`, `end = 24` (hard-coded).
    qreal contentLeadingSpace = 16.0;
    qreal contentTrailingSpace = 24.0;
    /// Compose's hard-coded 12 between the icon and the label, and again
    /// between the label's box and the badge.
    qreal iconLabelSpace = 12.0;

    /// `label-text-*`: `label-large`. The selected label is
    /// `label-large-weight-prominent` — the emphasized cut of the same size.
    TypeStyle labelTextType = TypeStyle::LabelLarge;

    // --- shape and state layers ---------------------------------------------
    /// `active-indicator-shape`: `corner-full`. The pill is the item, so the
    /// radius resolves against the item's own size.
    ShapeCorner indicatorShape = ShapeCorner::Full;

    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- the focus ring -------------------------------------------------------
    /// `focus-indicator-*`: published by this family itself (the flexible bar
    /// inherits these; the drawer owns them).
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    /// `focus-indicator-outline-offset`: the system `inner-offset` (2) — the
    /// ring draws *inside* the pill.
    qreal focusIndicatorOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- colours ---------------------------------------------------------------
    /// The drawer's own table — see the header note for how it diverges from
    /// the bar's.
    MdNavigationItemColours unselected;
    MdNavigationItemColours selected;

    const MdNavigationItemColours &coloursFor(bool selected) const;
};

/// One variant's rows — a single file's two container row groups.
struct QT_MD3_EXPORT MdNavigationDrawerVariantTokens
{
    // --- container ------------------------------------------------------------
    /// `container-width`: 360. `container-height: 100%` is the parent's to
    /// answer, not a token.
    qreal containerWidth = 360.0;
    ColorRole containerColor = ColorRole::Surface;
    ElevationLevel containerElevation = ElevationLevel::Level0;
    /// `container-shape`: `corner-large-end` — the Large (16) radius on the
    /// **end** pair only (a start-anchored sheet). Directional shapes have no
    /// enum member, so the base radius is carried here and the style's
    /// `Layout.radii` puts it on the end pair, mirrored in RTL — the
    /// bottom-sheet's `corner-extra-large-top` precedent.
    ShapeCorner containerShape = ShapeCorner::Large;

    // --- the scrim (carried; see the header note) -------------------------------
    ColorRole scrimColor = ColorRole::Scrim;
    qreal scrimOpacity = 0.4;

    // --- headline and divider ----------------------------------------------------
    /// `headline-*`: `title-small` in `on-surface-variant`.
    ColorRole headlineColor = ColorRole::OnSurfaceVariant;
    TypeStyle headlineType = TypeStyle::TitleSmall;
    ColorRole dividerColor = ColorRole::Outline;

    MdNavigationDrawerItemTokens item;
};

struct QT_MD3_EXPORT MdNavigationDrawerTokens
{
    MdNavigationDrawerVariantTokens variant[navDrawerVariantCount];

    const MdNavigationDrawerVariantTokens &forVariant(MdNavigationDrawerVariant variant) const;

    /// Resolves both container row groups from the one
    /// `md.comp.navigation-drawer.*` file: the container rows are the only
    /// thing that differs. Colours are not overridable library-wide; lengths
    /// and shapes are read back under the `navigation-drawer` name.
    static MdNavigationDrawerTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_NAVIGATION_DRAWER_TOKENS_H
