#ifndef MD_DATE_PICKER_TOKENS_H
#define MD_DATE_PICKER_TOKENS_H

// MdDatePickerTokens — `md.comp.date-picker-modal.*` at export 34.0.21.
//
// ## What the export publishes
//
// The container (360×524, surface-container-high at level 3,
// corner-extra-large), the header (360×120, headline-large headline +
// label-large supporting text, on-surface-variant), the date cells (40×40
// corner-full state layers; selected primary / on-primary, today a 1 px
// primary outline with primary label, unselected on-surface), the weekday
// row (body-large, on-surface), the month subhead (title-small) and the year
// selection (72×36 corner-full; selected primary / on-primary,
// unselected on-surface-variant).
//
// ## Compose's behaviour this widget ports
//
//   * **the calendar grid is the interaction.** Clicking a date selects it
//     (primary fill); today carries the 1 px outline; the header's headline
//     and supporting text follow the selection.
//   * **the year list is the other face.** Clicking the header's supporting
//     text switches to the year selection list.
//   * **the range form** (the `range-selection-*` rows) fills the in-range
//     days with secondary-container / on-secondary-container and an
//     active-indicator pill on the endpoints.
//
// ## Divergences pinned here rather than smoothed
//
// * **The typo'd `*-state-layer-opcaity` rows are carried, not read** — the
//   correctly spelled `*-opacity` rows win.
// * **`container-surface-tint-layer-color` is carried, not drawn** — the
//   elevation paints through `MdElevation`.
// * **The docked variant (`md.comp.date-picker-docked.*`) is a separate
//   export** — only the modal set is ported here.
// * **The dialog chrome is the host's** — this family paints the picker face.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// The picker's face: the calendar grid or the year list.
enum class MdDatePickerFace {
    Calendar,
    Years,
    Count,
};

/// The interaction states the date / year colour tables are indexed by.
enum class MdDatePickerState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

constexpr int datePickerStateCount = int(MdDatePickerState::Count);

/// One resolved date picker.
struct QT_MD3_EXPORT MdDatePickerTokens
{
    // --- container ---------------------------------------------------------
    /// `container.width: 360px`, `container.height: 524px`.
    qreal containerWidth = 360.0;
    qreal containerHeight = 524.0;
    /// `container.color: surface-container-high`, `container.elevation: level3`.
    ColorRole containerColor = ColorRole::SurfaceContainerHigh;
    ElevationLevel containerElevation = ElevationLevel::Level3;
    /// `container.shape: corner-extra-large` (28 px).
    qreal containerRadius = 28.0;

    // --- header -------------------------------------------------------------
    /// `header-container.*: 360×120`.
    qreal headerHeight = 120.0;
    /// `header-headline.color: on-surface-variant` (headline-large).
    ColorRole headerHeadlineColor = ColorRole::OnSurfaceVariant;
    /// `header-supporting-text.color: on-surface-variant` (label-large).
    ColorRole headerSupportingColor = ColorRole::OnSurfaceVariant;

    // --- date cells ----------------------------------------------------------
    /// `date-container.* / date-state-layer.*: 40×40`, `date-container.shape:
    /// corner-full`.
    qreal dateCellSize = 40.0;
    /// `date-selected-container-color: primary`, `…label-text-color: on-primary`.
    MdNavigationColourSlot dateSelected[datePickerStateCount];
    /// `date-today-container-outline-color: primary`, `…label-text-color:
    /// primary` with a 1 px outline.
    qreal dateTodayOutlineWidth = 1.0;
    ColorRole dateTodayColor = ColorRole::Primary;
    /// `date-unselected-label-text-color: on-surface`.
    MdNavigationColourSlot dateUnselected[datePickerStateCount];

    // --- weekday row / month subhead ------------------------------------------
    /// `weekdays-label-text-color: on-surface` (body-large).
    ColorRole weekdayColor = ColorRole::OnSurface;
    /// `range-selection-month-subhead-color: on-surface-variant` (title-small).
    ColorRole monthSubheadColor = ColorRole::OnSurfaceVariant;

    // --- range selection ----------------------------------------------------------
    /// `range-selection-active-indicator.*: 40px`, `…container-color:
    /// secondary-container`, `…shape: corner-full`.
    qreal rangeIndicatorSize = 40.0;
    ColorRole rangeIndicatorColor = ColorRole::SecondaryContainer;
    /// `range-selection-date-in-range-label-text-color: on-secondary-container`.
    ColorRole rangeInLabelColor = ColorRole::OnSecondaryContainer;

    // --- year selection -----------------------------------------------------------
    /// `year-selection-year-container.*: 72×36`, `…state-layer-shape:
    /// corner-full`.
    qreal yearWidth = 72.0;
    qreal yearHeight = 36.0;
    /// `year-selected-container-color: primary`, `…label-text-color: on-primary`.
    MdNavigationColourSlot yearSelected[datePickerStateCount];
    /// `year-unselected-label-text-color: on-surface-variant`.
    MdNavigationColourSlot yearUnselected[datePickerStateCount];

    // --- state-layer opacities -------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- system focus-indicator rows ----------------------------------------
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    static MdDatePickerTokens resolve();
};

} // namespace md

#endif // MD_DATE_PICKER_TOKENS_H
