#ifndef MD_TIME_PICKER_TOKENS_H
#define MD_TIME_PICKER_TOKENS_H

// MdTimePickerTokens — `md.comp.time-picker.*` at export version 34.0.21.
//
// ## What the export publishes
//
// The container rows (surface-container-high at level 3, corner-extra-large),
// the clock dial (256 px, corner-full, surface-container-highest; the 48 px
// selector handle around an 8 px centre on a 2 px track, all `primary`), the
// time selectors (96×80 corner-small, display-large; selected primary-container
// / on-primary-container, unselected surface-container-highest / on-surface),
// the period selector (216×38 horizontal or 52×80 vertical, corner-small,
// outline; selected tertiary-container / on-tertiary-container, unselected
// on-surface-variant), the headline (label-medium, on-surface-variant) and the
// state-layer opacities (hover 0.08 / focus 0.12 / pressed 0.12) on the two
// selector families.
//
// ## Compose's behaviour this widget ports
//
//   * **the dial is the interaction.** The handle follows the pointer around
//     the 256 px circle and snaps to the nearest hour (or 5-minute slot on the
//     minutes face); the numbers' selected/unselected colours follow.
//   * **the time selectors are the input mode.** Clicking a selector switches
//     the dial between hours and minutes; the period selector flips AM/PM.
//   * **the headline types the mode** ("Select time" / the locale's AM-PM
//     prompt) in label-medium.
//
// ## Divergences pinned here rather than smoothed
//
// * **`clock-dial.color.ignore` / `clock-dial.shape.ignore` are the typo'd
//   deprecated pair** — carried, not read; `clock-dial.color` / `shape` win.
// * **`surface-tint-layer-color` is carried, not drawn** — the container's
//   level 3 elevation paints through `MdElevation`, which is the project's
//   standing rule for tonal surfaces.
// * **The dialog chrome is the host's.** This family paints the picker face
//   (headline, selectors, dial); the surrounding dialog is `MdDialog`.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// The dial's face: hours or minutes.
enum class MdTimePickerFace {
    Hours,
    Minutes,
    Count,
};

/// The AM/PM period.
enum class MdTimePeriod {
    Am,
    Pm,
    Count,
};

/// The interaction states the selector colour tables are indexed by.
enum class MdTimeSelectorState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

constexpr int timeSelectorStateCount = int(MdTimeSelectorState::Count);

/// One resolved time picker.
struct QT_MD3_EXPORT MdTimePickerTokens
{
    // --- container ---------------------------------------------------------
    /// `container.color: surface-container-high`.
    ColorRole containerColor = ColorRole::SurfaceContainerHigh;
    /// `container.elevation: level3`.
    ElevationLevel containerElevation = ElevationLevel::Level3;
    /// `container.shape: corner-extra-large` (28 px).
    qreal containerRadius = 28.0;

    // --- clock dial ----------------------------------------------------------
    /// `clock-dial.container.size: 256px`.
    qreal dialSize = 256.0;
    /// `clock-dial.color: surface-container-highest`.
    ColorRole dialColor = ColorRole::SurfaceContainerHighest;
    /// `clock-dial.shape: corner-full`.
    qreal dialRadius = 128.0;
    /// `clock-dial.selector.handle.container.size: 48px`.
    qreal selectorHandleSize = 48.0;
    /// `clock-dial.selector.center.container.size: 8px`.
    qreal selectorCenterSize = 8.0;
    /// `clock-dial.selector.track.container.width: 2px`.
    qreal selectorTrackWidth = 2.0;
    /// The three selector rows are all `primary`.
    ColorRole selectorColor = ColorRole::Primary;
    /// `clock-dial.selected.label-text.color: on-primary`.
    ColorRole dialSelectedLabel = ColorRole::OnPrimary;
    /// `clock-dial.unselected.label-text.color: on-surface`.
    ColorRole dialUnselectedLabel = ColorRole::OnSurface;

    // --- time selectors -------------------------------------------------------
    /// `time-selector.container.width: 96px`, `container.height: 80px`.
    qreal timeSelectorWidth = 96.0;
    qreal timeSelectorHeight = 80.0;
    /// `time-selector.container.shape: corner-small` (8 px).
    qreal timeSelectorRadius = 8.0;
    /// `time-selector.24h-vertical.container.width: 114px` — the 24h mode's
    /// wider single selector.
    qreal timeSelector24hWidth = 114.0;
    /// The selected row: primary-container / on-primary-container.
    MdNavigationColourSlot timeSelected[timeSelectorStateCount];
    /// The unselected row: surface-container-highest / on-surface.
    MdNavigationColourSlot timeUnselected[timeSelectorStateCount];

    // --- period selector ---------------------------------------------------------
    /// `period-selector.horizontal.container.*: 216×38`, `vertical: 52×80`.
    qreal periodWidth = 216.0;
    qreal periodHeight = 38.0;
    qreal periodVerticalWidth = 52.0;
    qreal periodVerticalHeight = 80.0;
    /// `period-selector.container.shape: corner-small` (8 px).
    qreal periodRadius = 8.0;
    /// `period-selector.outline.width: 1px`, `outline.color: outline`.
    qreal periodOutlineWidth = 1.0;
    ColorRole periodOutlineColor = ColorRole::Outline;
    /// The selected row: tertiary-container / on-tertiary-container.
    MdNavigationColourSlot periodSelected[timeSelectorStateCount];
    /// The unselected row: on-surface-variant.
    MdNavigationColourSlot periodUnselected[timeSelectorStateCount];

    // --- headline -------------------------------------------------------------
    /// `headline.color: on-surface-variant` (label-medium).
    ColorRole headlineColor = ColorRole::OnSurfaceVariant;

    // --- state-layer opacities (shared by both selector families) ----------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- system focus-indicator rows ----------------------------------------
    /// The export publishes no focus-indicator rows; the ring carries the
    /// system values around the focused selector.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    static MdTimePickerTokens resolve();
};

} // namespace md

#endif // MD_TIME_PICKER_TOKENS_H
