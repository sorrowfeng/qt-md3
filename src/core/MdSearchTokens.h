#ifndef MD_SEARCH_TOKENS_H
#define MD_SEARCH_TOKENS_H

// MdSearchTokens — `md.comp.search-bar.*` + `md.comp.search-view.*` at export
// version 34.0.21.
//
// ## What the export publishes
//
// The bar (56 px, surface-container-high at level 3, corner-full; 24 px
// icons at 16 px side spaces, body-large input text, on-surface icons and
// text) and the view (a docked form with corner-extra-large, a full-screen
// form with corner-none, surface-container-low background, an outline
// divider, a 56 px docked / 72 px full-screen header). The contained variant
// adds the avatar row (30 px avatar, 48 px target) and the spring row
// (`spring-fast-spatial`).
//
// ## Compose's behaviour this widget ports
//
//   * **the bar is the entry point.** Clicking the bar expands it into the
//     view (the contained spring, `spring-fast-spatial`).
//   * **the view is the search surface.** Docked floats at corner-extra-large;
//     full-screen takes the window at corner-none.
//
// ## Divergences pinned here rather than smoothed
//
// * **`container-surface-tint-layer-color` is carried, not drawn** — the
//   elevation paints through `MdElevation`.
// * **`focus-indicator-*` rows are the system set** (secondary, outer offset,
//   thickness) — carried as the focus ring's spec.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which of the two published surfaces the search family renders.
enum class MdSearchSurface {
    /// The 56 px entry-point bar.
    Bar,
    /// The docked view (corner-extra-large).
    DockedView,
    /// The full-screen view (corner-none).
    FullScreenView,
    Count,
};

/// The interaction states the colour tables are indexed by.
enum class MdSearchState {
    Enabled,
    Hovered,
    Pressed,
    Disabled,
    Count,
};

constexpr int searchStateCount = int(MdSearchState::Count);

/// One resolved search surface.
struct QT_MD3_EXPORT MdSearchTokens
{
    // --- bar / header -------------------------------------------------------
    /// `container-height: 56px` (bar and docked header), 72 px full-screen.
    qreal containerHeight = 56.0;
    qreal fullScreenHeaderHeight = 72.0;
    /// `container-color: surface-container-high`, `container-elevation: level3`,
    /// `container-shape: corner-full` (the bar).
    ColorRole containerColor = ColorRole::SurfaceContainerHigh;
    ElevationLevel containerElevation = ElevationLevel::Level3;
    qreal containerRadius = 28.0;
    /// The view's shapes: docked corner-extra-large (28), full-screen
    /// corner-none (0).
    qreal dockedRadius = 28.0;
    qreal fullScreenRadius = 0.0;
    /// `contained-background-color: surface-container-low` (the view's
    /// background), `divider-color: outline`.
    ColorRole viewBackgroundColor = ColorRole::SurfaceContainerLow;
    ColorRole dividerColor = ColorRole::Outline;

    // --- icons / text ---------------------------------------------------------
    /// `icon-size: 24px`, `leading-space: 16px`, `trailing-space: 16px`.
    qreal iconSize = 24.0;
    qreal leadingSpace = 16.0;
    qreal trailingSpace = 16.0;
    /// `input-text-color: on-surface` (body-large).
    ColorRole inputTextColor = ColorRole::OnSurface;
    /// `leading-icon-color: on-surface`, `header-leading-icon-color: on-surface`.
    ColorRole leadingIconColor = ColorRole::OnSurface;
    /// `header-supporting-text-color: on-surface-variant`.
    ColorRole supportingTextColor = ColorRole::OnSurfaceVariant;

    // --- state layers ------------------------------------------------------------
    /// `hover-state-layer-color: on-surface`, `pressed-state-layer-color: on-surface`.
    MdNavigationColourSlot stateLayer[searchStateCount];

    // --- contained variant ----------------------------------------------------------
    /// `avatar-size: 30px`, `contained-avatar-target-size: 48px`.
    qreal avatarSize = 30.0;
    qreal avatarTargetSize = 48.0;

    // --- state-layer opacities ---------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal pressedStateLayerOpacity = 0.12;

    // --- system focus-indicator rows ----------------------------------------
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    static MdSearchTokens resolve(MdSearchSurface surface);
};

} // namespace md

#endif // MD_SEARCH_TOKENS_H
