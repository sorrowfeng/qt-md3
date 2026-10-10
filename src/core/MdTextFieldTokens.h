#ifndef MD_TEXT_FIELD_TOKENS_H
#define MD_TEXT_FIELD_TOKENS_H

// MdTextFieldTokens — `md.comp.filled-text-field.*` + `md.comp.outlined-text-field.*`
// at export version 34.0.21.
//
// ## What the export publishes
//
// Two full sets (filled and outlined) sharing one structure: the container
// (56 px; filled surface-container-highest with corner-extra-small-top,
// outlined transparent with a 1 px outline), the active indicator (1 px
// resting / 1 px hover / 2 px focus, primary / on-surface-variant by state),
// the input text (body-large, on-surface), the label (body-large resting,
// body-small populated; on-surface-variant, primary under focus), the
// supporting text (body-small, on-surface-variant), the leading / trailing
// icons (24 px, on-surface-variant) and the error overlay (error colour on
// the indicator / label / supporting text / trailing icon).
//
// ## Compose's behaviour this widget ports
//
//   * **the label floats.** At rest it sits in the input line at body-large;
//     once the field is populated or focused it rises to body-small on the
//     container's top edge.
//   * **the active indicator is the state's spine.** 1 px on-surface-variant
//     at rest, on-surface on hover, 2 px primary under focus, error when the
//     error state is set.
//   * **the disabled rows fade by element** — container 0.04, indicator 0.38,
//     text and icons 0.38.
//
// ## Divergences pinned here rather than smoothed
//
// * **The `focus-active-indicator-thickness` row reads
//   `md-sys-state-focus-indicator.thickness`** — the system 3 px — while the
//   focus indicator height is the 2 px `focus-active-indicator-height`. The
//   export publishes both; the indicator uses the 2 px row, the focus ring
//   uses the system row.
// * **The error state is one bool, not a colour table.** The export publishes
//   a full error overlay family; the port exposes `error` and resolves the
//   error rows when it is set.
// * **The outlined set's `container-shape` is corner-extra-small (4 px)** on
//   all four corners, while filled is corner-extra-small-top (4 px top only).

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which of the two published sets the field renders.
enum class MdTextFieldVariant {
    Filled,
    Outlined,
    Count,
};

/// The interaction states the colour tables are indexed by.
enum class MdTextFieldState {
    Enabled,
    Hovered,
    Focused,
    Disabled,
    Count,
};

constexpr int textFieldStateCount = int(MdTextFieldState::Count);

/// One resolved text field.
struct QT_MD3_EXPORT MdTextFieldTokens
{
    // --- container ---------------------------------------------------------
    /// `container-height: 56px`.
    qreal containerHeight = 56.0;
    /// Filled: `container-color: surface-container-highest`,
    /// `container-shape: corner-extra-small-top` (4 px top corners only).
    /// Outlined: transparent with a 1 px `active-indicator-color` outline and
    /// `corner-extra-small` (4 px all round).
    ColorRole containerColor = ColorRole::SurfaceContainerHighest;
    qreal containerRadius = 4.0;
    bool containerRadiusTopOnly = true;
    bool outlined = false;
    qreal outlineWidth = 1.0;

    // --- active indicator ---------------------------------------------------
    /// `active-indicator-height: 1px`, `hover: 1px`, `focus-active-indicator-
    /// height: 2px`, `disabled-active-indicator-height: 1px`.
    qreal indicatorHeight = 1.0;
    qreal hoverIndicatorHeight = 1.0;
    qreal focusIndicatorHeight = 2.0;
    /// The indicator colour by state: on-surface-variant resting, on-surface
    /// hover, primary focus, on-surface @0.38 disabled, error when errored.
    MdNavigationColourSlot indicator[textFieldStateCount];

    // --- text ---------------------------------------------------------------
    /// `input-text-color: on-surface` (body-large).
    ColorRole inputTextColor = ColorRole::OnSurface;
    /// `input-text-placeholder-color: on-surface-variant`.
    ColorRole placeholderColor = ColorRole::OnSurfaceVariant;
    /// `label-text-color: on-surface-variant` — `focus-label-text-color:
    /// primary` under focus.
    MdNavigationColourSlot label[textFieldStateCount];
    /// `supporting-text-color: on-surface-variant` (body-small).
    MdNavigationColourSlot supportingText[textFieldStateCount];

    // --- icons ----------------------------------------------------------------
    /// `leading-icon-size: 24px`, `trailing-icon-size: 24px`.
    qreal iconSize = 24.0;
    MdNavigationColourSlot leadingIcon[textFieldStateCount];
    MdNavigationColourSlot trailingIcon[textFieldStateCount];

    // --- error overlay -----------------------------------------------------------
    /// The error family: `error-active-indicator-color`, `error-label-text-color`,
    /// `error-supporting-text-color`, `error-trailing-icon-color` — all `error`.
    ColorRole errorColor = ColorRole::Error;

    // --- disabled opacities ---------------------------------------------------
    /// `disabled-container-opacity: 0.04`, the rest 0.38.
    qreal disabledContainerOpacity = 0.04;
    qreal disabledContentOpacity = 0.38;

    // --- state-layer opacities -------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;

    // --- system focus-indicator rows ----------------------------------------
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    /// Resolve the token set for one variant.
    static MdTextFieldTokens resolve(MdTextFieldVariant variant);
};

} // namespace md

#endif // MD_TEXT_FIELD_TOKENS_H
