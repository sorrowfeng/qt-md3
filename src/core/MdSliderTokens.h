#ifndef MD_SLIDER_TOKENS_H
#define MD_SLIDER_TOKENS_H

// MdSliderTokens — `md.comp.slider.*` at export version 34.0.21.
//
// ## What the export publishes
//
// The metric rows (the 4 px handle with its per-state widths 4/4/2/2/4, the
// 44 px handle height, the 16 px track, the 4 px stop indicators, the 40 px
// state layer), five Expressive size sets (`xsmall` … `xlarge`) overriding the
// track heights (16/24/40/56/96), the handle heights (44/44/44/68/108) and the
// track end radii (8/8/12/16/28), the disabled opacities (active track 0.38,
// inactive track 0.12, handle 0.38), the value-indicator rows (inverse-surface
// container, inverse-on-surface label on `label-large`), the with-tick-marks
// rows and the state-layer rows.
//
// ## Compose's behaviour this widget ports
//
//   * **the handle is a vertical pill, not a circle.** `handle.width` 4 px,
//     `handle.height` 44 px — a rounded bar sitting on the track. The width
//     narrows to 2 px under focus and press (`focus.handle.width` /
//     `pressed.handle.width`); hover keeps 4 px. Disabled keeps 4 px.
//   * **no colour animation.** Track and handle colours resolve by state and
//     land this frame (the same standing as chips and switch).
//   * **the value indicator scales in from the handle** on the emphasized
//     easing over `duration-short2`, transform-origin bottom-centre — the
//     material-web `.label` contract. It shows while the handle is focused,
//     hovered or pressed (material-web's `:focus-within` / `.hover` / `:active`
//     triple).
//
// ## Divergences pinned here rather than smoothed
//
// * **`inactive.track.color` is `secondary-container` in the export** while
//   the material-web theming table claims `surface-container-highest`. The
//   export wins (it is the value layer of record); Compose's
//   `SliderTokens.InactiveTrackColor` agrees with the export. Recorded in
//   porting-todo.
// * **`stop-indicator.trailing-space` is 4 px in the export** and 6 dp in
//   Compose (`SliderTokens.StopIndicatorTrailingSpace`). The export wins.
// * **The deprecated rows are carried, not read**: `active-container-opacity`,
//   `inactive-container-opacity`, `label-container-height`, the whole
//   `label.label-text.*` family, `state-layer-size`, `with-overlap.*`,
//   `with-tick-marks.*` opacities, `handle-elevation` / `handle-shadow-color`
//   (the handle shadow is level 1 in the export's own `handle.elevation`, read
//   through `MdElevation`).
// * **The range slider is one widget, not two.** material-web ships `range`
//   with `valueStart` / `valueEnd`; this port carries the same pair on
//   `MdSlider` and paints two handles. Compose separates `RangeSlider`; the
//   single-widget contract is the material-web one.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// The Expressive size scale of the slider — five published sets.
enum class MdSliderSize {
    XSmall,
    Small,
    Medium,
    Large,
    XLarge,
    Count,
};

constexpr int sliderSizeCount = int(MdSliderSize::Count);

/// The interaction states the slider's colour tables are indexed by. The
/// export publishes Enabled / Hovered / Focused / Pressed / Disabled rows —
/// no dragged.
enum class MdSliderInteraction {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

constexpr int sliderInteractionCount = int(MdSliderInteraction::Count);

/// One resolved slider — the size's metric row plus the shared colour tables.
struct QT_MD3_EXPORT MdSliderTokens
{
    // --- per-size metrics (from `md.comp.slider.<size>.*`) -----------------
    /// Track height: 16 / 24 / 40 / 56 / 96.
    qreal trackHeight = 16.0;
    /// Handle height: 44 / 44 / 44 / 68 / 108.
    qreal handleHeight = 44.0;
    /// The track's outer end radius: 8 / 8 / 12 / 16 / 28.
    qreal trackEndRadius = 8.0;
    /// Icon size: xsmall and small publish none (0); medium and large 24;
    /// xlarge 32.
    qreal iconSize = 0.0;
    /// Icon padding: xsmall and small publish none (0); medium and large 6;
    /// xlarge 8.
    qreal iconPadding = 0.0;

    // --- handle metrics ----------------------------------------------------
    /// `handle.width: 4px` — the resting width of the vertical pill.
    qreal handleWidth = 4.0;
    /// `hover.handle.width: 4px` — hover keeps the resting width.
    qreal hoverHandleWidth = 4.0;
    /// `focus.handle.width: 2px` — the handle narrows under keyboard focus.
    qreal focusHandleWidth = 2.0;
    /// `pressed.handle.width: 2px` — and under press.
    qreal pressedHandleWidth = 2.0;
    /// `disabled.handle.width: 4px` — disabled keeps the resting width.
    qreal disabledHandleWidth = 4.0;
    /// `active.handle.leading-space: 6px`.
    qreal handleLeadingSpace = 6.0;
    /// `active.handle.trailing-space: 6px`.
    qreal handleTrailingSpace = 6.0;
    /// `active.handle.padding: 6px`.
    qreal handlePadding = 6.0;

    // --- stop indicators / ticks -------------------------------------------
    /// `stop-indicator.size: 4px`.
    qreal stopIndicatorSize = 4.0;
    /// `stop-indicator.trailing-space: 4px` (Compose says 6 dp — recorded).
    qreal stopIndicatorTrailingSpace = 4.0;
    /// `with-tick-marks.container.size: 2px` — the tick mark's diameter.
    qreal tickMarkSize = 2.0;

    // --- state layer / value indicator -------------------------------------
    /// `state-layer-size: 40px` (deprecated row, carried).
    qreal stateLayerSize = 40.0;
    /// `value-indicator.active.bottom-space: 12px` — the gap under the label.
    qreal valueIndicatorBottomSpace = 12.0;
    /// `label-container-height: 28px` (deprecated row, carried) — the label's
    /// minimum size.
    qreal valueIndicatorMinSize = 28.0;

    // --- disabled opacities ------------------------------------------------
    /// `disabled.active-track-opacity: 0.38`.
    qreal disabledActiveTrackOpacity = 0.38;
    /// `disabled.inactive-track-opacity: 0.12`.
    qreal disabledInactiveTrackOpacity = 0.12;
    /// `disabled.handle-opacity: 0.38`.
    qreal disabledHandleOpacity = 0.38;
    /// `disabled.stop-indicator.container.opacity: 0.38` (deprecated row,
    /// carried for the disabled tick marks).
    qreal disabledStopIndicatorOpacity = 0.38;

    // --- colour tables, indexed [interaction] ------------------------------
    /// The active track. `primary` for every enabled interaction, `on-surface`
    /// at 0.38 disabled.
    MdNavigationColourSlot activeTrack[sliderInteractionCount];
    /// The inactive track. `secondary-container` for every enabled
    /// interaction, `on-surface` at 0.12 disabled.
    MdNavigationColourSlot inactiveTrack[sliderInteractionCount];
    /// The handle. `primary` for every enabled interaction (the focus/hover/
    /// pressed rows all resolve `primary`), `on-surface` at 0.38 disabled.
    MdNavigationColourSlot handle[sliderInteractionCount];
    /// The stop indicator on the active side (`on-primary`).
    MdNavigationColourSlot activeStopIndicator[sliderInteractionCount];
    /// The stop indicator on the inactive side (`on-secondary-container`).
    MdNavigationColourSlot inactiveStopIndicator[sliderInteractionCount];
    /// The tick mark on the active side (`on-primary` at 0.38).
    MdNavigationColourSlot activeTickMark[sliderInteractionCount];
    /// The tick mark on the inactive side (`on-surface-variant` at 0.38).
    MdNavigationColourSlot inactiveTickMark[sliderInteractionCount];

    // --- value indicator -----------------------------------------------------
    /// `value-indicator.container.color: inverse-surface`.
    ColorRole valueIndicatorContainer = ColorRole::InverseSurface;
    /// `value-indicator.label.label-text.color: inverse-on-surface`.
    ColorRole valueIndicatorLabel = ColorRole::InverseOnSurface;

    // --- handle elevation ----------------------------------------------------
    /// `handle.elevation: level1` — the only shadow in the family.
    ElevationLevel handleElevation = ElevationLevel::Level1;
    /// `disabled.handle.elevation: level0`.
    ElevationLevel disabledHandleElevation = ElevationLevel::Level0;

    // --- system focus-indicator rows ----------------------------------------
    /// The export publishes no focus-indicator rows; the ring carries the
    /// system values (secondary, outer offset 2, thickness 3) around the
    /// handle's state-layer circle.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- system state-layer opacities ----------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    /// Resolve the token set for one size: the size's metric row plus the
    /// shared colour tables.
    static MdSliderTokens resolve(MdSliderSize size);
};

} // namespace md

#endif // MD_SLIDER_TOKENS_H
