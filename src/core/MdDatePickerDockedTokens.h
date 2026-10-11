#ifndef MD_DATE_PICKER_DOCKED_TOKENS_H
#define MD_DATE_PICKER_DOCKED_TOKENS_H

// MdDatePickerDockedTokens — `md.comp.date-picker-docked.*` at export 34.0.21.
//
// ## What the export publishes
//
// The docked picker is the **inline** face of the date picker: a 360×456
// panel (`surface-container-high` at level 3, `corner-large`) with a 64 px
// header — not the modal 120 px one — carrying two 40 px menu buttons
// (month and year, `corner-full`, label-large, 18 px icons) and a calendar
// grid of 48 px date cells whose state layer is a 40 px `corner-full` pill
// inside the cell. Selected fills `primary` / `on-primary`; today carries a
// 1 px `primary` outline; unselected labels are `on-surface`; outside-month
// labels fade to 0.38.
//
// ## How this sits beside `MdDatePickerTokens`
//
// The modal set (`md.comp.date-picker-modal.*`) is already ported. The docked
// set is a **separate export namespace** with different geometry (456 vs 524
// height, 64 vs 120 header, 48 vs 40 date cells) and its own header content
// (menu buttons, not a headline). `MdDatePicker` carries both through
// `MdDatePickerVariant`; this struct is the docked half.
//
// ## Divergences pinned here rather than smoothed
//
// * **No `focus-indicator-*` rows exist in this export.** The modal set and
//   every button family publish a system ring; this one does not. The rows
//   are *absent*, not estimated — the docked face paints no focus ring of its
//   own, and the gap is recorded in porting-todo.
// * **`container-surface-tint-layer-color` is carried, not drawn** — the
//   elevation paints through `MdElevation`.
// * **The export publishes no grid gap row.** The 48 px date cell is the
//   slot; the 40 px state layer is the pill centred inside it (4 px inset
//   every side). The cells sit edge to edge.
// * **`menu.list-item.*` rows are the dropdown's list** (48 px, body-large,
//   `surface-variant` selected container). They are carried here; the port's
//   month / year popup reuses the classic `MdMenu` surface and is a recorded
//   divergence (the docked list-item set has no dedicated widget).
// * **Disabled menu-button rows publish 0.38 opacities** on `on-surface`;
//   the date-cell tables publish no disabled rows at all and fall back to
//   `on-surface` at 0.38, the modal set's standing rule.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// The interaction states the date-cell colour tables are indexed by.
enum class MdDatePickerDockedState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

constexpr int datePickerDockedStateCount = int(MdDatePickerDockedState::Count);

/// One resolved docked date picker.
struct QT_MD3_EXPORT MdDatePickerDockedTokens
{
    // --- container ---------------------------------------------------------
    /// `container.width: 360px`, `container.height: 456px`.
    qreal containerWidth = 360.0;
    qreal containerHeight = 456.0;
    /// `container.color: surface-container-high`, `container.elevation: level3`.
    ColorRole containerColor = ColorRole::SurfaceContainerHigh;
    ElevationLevel containerElevation = ElevationLevel::Level3;
    /// `container.shape: corner-large` (16 px) — not the modal extra-large.
    ShapeCorner containerShape = ShapeCorner::Large;

    // --- header -------------------------------------------------------------
    /// `header.height: 64px` (the modal header is 120).
    qreal headerHeight = 64.0;

    // --- date cells ----------------------------------------------------------
    /// `date.container.*: 48×48`, `date.state-layer.*: 40×40`.
    qreal dateCellWidth = 48.0;
    qreal dateCellHeight = 48.0;
    qreal dateStateLayerWidth = 40.0;
    qreal dateStateLayerHeight = 40.0;
    /// `date.container.shape: corner-full`, `date.state-layer.shape: corner-full`.
    ShapeCorner dateCellShape = ShapeCorner::Full;
    ShapeCorner dateStateLayerShape = ShapeCorner::Full;
    /// `date.today.container.outline.width: 1px`, `…color: primary`.
    qreal dateTodayOutlineWidth = 1.0;
    ColorRole dateTodayColor = ColorRole::Primary;
    /// `date.unselected.outside-month.label-text.opacity: 0.38`.
    qreal dateOutsideMonthLabelOpacity = 0.38;
    /// `date.label-text.type: body-large`.
    TypeStyle dateLabelType = TypeStyle::BodyLarge;
    /// `date-selected-container-color: primary`, `…label-text-color: on-primary`.
    MdNavigationColourSlot dateSelected[datePickerDockedStateCount];
    /// `date-today-*`: primary outline, primary label, primary state layers.
    MdNavigationColourSlot dateToday[datePickerDockedStateCount];
    /// `date-unselected-*`: on-surface label, on-surface-variant state layers.
    MdNavigationColourSlot dateUnselected[datePickerDockedStateCount];

    // --- weekday row ----------------------------------------------------------
    /// `weekdays.label-text.color: on-surface` (body-large).
    ColorRole weekdayColor = ColorRole::OnSurface;
    TypeStyle weekdayType = TypeStyle::BodyLarge;

    // --- menu buttons (month / year) -------------------------------------------
    /// `menu-button.container.height: 40px`, `menu-button.icon.size: 18px`,
    /// `menu-button.container.shape: corner-full`.
    qreal menuButtonHeight = 40.0;
    qreal menuButtonIconSize = 18.0;
    ShapeCorner menuButtonShape = ShapeCorner::Full;
    /// `menu-button.label-text.type: label-large`.
    TypeStyle menuButtonLabelType = TypeStyle::LabelLarge;
    /// Enabled / hover / focus / pressed all read `on-surface-variant` for
    /// both the label and the icon; disabled is `on-surface` at 0.38.
    MdNavigationColourSlot menuButtonLabel[datePickerDockedStateCount];
    MdNavigationColourSlot menuButtonIcon[datePickerDockedStateCount];
    MdNavigationColourSlot menuButtonStateLayer[datePickerDockedStateCount];

    // --- the dropdown's list items (`menu.list-item.*`) ---------------------------
    /// `menu.list-item.container.height: 48px`.
    qreal menuListItemHeight = 48.0;
    /// `menu.list-item.selected.leading-icon.size: 24px`.
    qreal menuListSelectedIconSize = 24.0;
    /// `menu.list-item.label-text.type: body-large`.
    TypeStyle menuListLabelType = TypeStyle::BodyLarge;
    /// `menu.list-item.label-text.color: on-surface` in every state.
    MdNavigationColourSlot menuListLabel[datePickerDockedStateCount];
    /// `menu.list-item.selected.container.color: surface-variant`.
    ColorRole menuListSelectedContainerColor = ColorRole::SurfaceVariant;
    /// `menu.list-item.selected.leading-icon.color: on-surface` (lifts to
    /// `on-surface-variant` under hover / focus / press).
    MdNavigationColourSlot menuListSelectedIcon[datePickerDockedStateCount];

    // --- state-layer opacities -------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    static MdDatePickerDockedTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_DATE_PICKER_DOCKED_TOKENS_H
