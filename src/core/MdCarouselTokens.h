// *****************************************************************************
// MdCarouselTokens — the token set for the carousel family, transcribed from
// material-web's `_md-comp-carousel-item.scss` (tokens 34.0.21) and the
// Compose M3 carousel package's defaults (Keylines.kt / Carousel.kt).
// *****************************************************************************
#pragma once

#include "MdCompTokenParse.h"
#include "MdTypes.h"

namespace md
{

/// The carousel family's state rows and scroll constants. The item container
/// is a plain surface at level 0 that lifts to level 1 on hover; the shape is
/// the extra-large corner on every item regardless of its keyline width.
class QT_MD3_EXPORT MdCarouselTokens
{
public:
    /// Builds the resolved token set, applying any per-component overrides on
    /// top of the export values.
    static MdCarouselTokens resolve(const MdComponentTokens *overrides = nullptr);

    // --- Container (Enabled) -------------------------------------------------
    ColorRole containerColor = ColorRole::Surface;   // $surface
    qreal containerElevation = 0.0;                  // md-sys-elevation.$level0
    qreal hoverContainerElevation = 1.0;             // $level1
    qreal pressedContainerElevation = 0.0;           // $level0 (ripple repeats it)
    ColorRole containerShadowColor = ColorRole::Shadow;
    qreal containerShapeRadius = 28.0;               // $corner-extra-large

    // --- Outline (the with-outline variant) ----------------------------------
    bool withOutline = false;                        // the caller opts into the row set
    qreal outlineWidth = 1.0;                        // $with-outline-outline-width
    ColorRole outlineColor = ColorRole::Outline;     // $outline
    qreal disabledOutlineOpacity = 0.12;             // $with-outline-disabled-outline-opacity

    // --- Disabled -------------------------------------------------------------
    qreal disabledContainerOpacity = 0.38;           // $disabled-container-opacity

    // --- State layer (hover / focus / pressed all carry one) ------------------
    ColorRole stateLayerColor = ColorRole::OnSurface; // $on-surface
    qreal hoverStateLayerOpacity = 0.08;              // md-sys-state.$hover-state-layer-opacity
    qreal focusStateLayerOpacity = 0.12;              // $focus-state-layer-opacity
    qreal pressedStateLayerOpacity = 0.12;            // $pressed-state-layer-opacity

    // --- Focus indicator -------------------------------------------------------
    ColorRole focusIndicatorColor = ColorRole::Secondary; // $secondary
    qreal focusIndicatorThickness = 3.0;                  // focus-indicator.$thickness
    qreal focusIndicatorOffset = 2.0;                     // focus-indicator.$outer-offset

    // --- Scroll / layout constants (Compose defaults + the specs page) --------
    qreal itemSpacing = 8.0;         // specs "Padding between elements"; Compose defaults 0
    qreal minSmallItemSize = 40.0;   // CarouselDefaults.MinSmallItemSize
    qreal maxSmallItemSize = 56.0;   // CarouselDefaults.MaxSmallItemSize
    qreal anchorSize = 10.0;         // CarouselDefaults.AnchorSize
    qreal sidePadding = 16.0;        // specs leading/trailing padding (multi-browse)
    qreal crossPadding = 8.0;        // specs top/bottom padding
    qreal mediumItemFlexPercentage = 0.1;      // Arrangement.kt MediumItemFlexPercentage
    qreal mediumLargeItemDiffThreshold = 0.85; // CarouselDefaults.MediumLargeItemDiffThreshold
    qreal fullScreenItemSpacing = 16.0;        // specs full-screen "Padding between elements"

    /// The deprecated surface-tint row, kept as the record only
    /// (md.comp.carousel-item.container.surface-tint-layer.color).
    static constexpr bool kSurfaceTintDeprecated = true;
};

} // namespace md
