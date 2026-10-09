#ifndef MD_RADIO_BUTTON_TOKENS_H
#define MD_RADIO_BUTTON_TOKENS_H

// MdRadioButtonTokens — `md.comp.radio-button.*` at export version 34.0.21.
//
// ## What the export publishes
//
// The leanest Selection export in the batch: two metric rows (`icon.size` 20,
// `state-layer.size` 40), one icon colour table and one state-layer colour
// table, both indexed `[selection][interaction]`, and the two disabled
// opacities. Nothing else — no container (the circle *is* the icon), no
// outline, no error variant, no drag rows, and **no focus-indicator rows**:
// the ring carries the system values (`md.sys.state.focus-indicator`,
// outer offset 2, thickness 3) with Compose's `focusRingShape = CircleShape`.
//
// ## The pressed state-layer special cases (checkbox-shaped)
//
// `unselected.pressed.state-layer.color` is `primary` and
// `selected.pressed.state-layer.color` is `on-surface` — the press layer is
// the colour the button is *about to earn* (or is about to leave), exactly the
// checkbox's special case. Hover and focus read each side's own colour
// (`on-surface` unselected, `primary` selected).
//
// ## Divergences pinned here rather than smoothed
//
// * **The dot and stroke sizes are Compose constants, not export rows.** The
//   export publishes only `icon.size`; Compose hardcodes
//   `RadioButtonDotSize` 12 dp (the dot draws at `dotRadius - strokeWidth/2`,
//   i.e. a 10 px filled circle), `RadioStrokeWidth` 2 dp and
//   `RadioButtonPadding` 2 dp. All three are carried here as behaviour
//   constants.
// * **One colour for stroke and dot.** Compose draws both circles in one
//   `animatedColor` — the icon table's row. There is no separate dot colour.
// * **The ripple is unbounded with a fixed radius.** Compose's
//   `ripple(bounded = false, radius = StateLayerSize / 2)` — a 20 px radius
//   circle that does not grow with the pointer. This port runs the standard
//   bounded ripple inside the 40 px layer bounds, clipped to its circle: the
//   growth curve past the clip is invisible, so the visible result matches
//   (the same standing as the checkbox's ripple).

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which side of the colour tables a paint reads.
enum class MdRadioButtonSelection {
    Unselected,
    Selected,
    Count,
};

constexpr int radioButtonSelectionCount = int(MdRadioButtonSelection::Count);

/// One resolved radio button. The two colour tables are indexed
/// `[selection][interaction]`.
struct QT_MD3_EXPORT MdRadioButtonTokens
{
    // --- metrics ---------------------------------------------------------------
    /// `icon.size: 20px` — the stroke circle's diameter.
    qreal iconSize = 20.0;
    /// The circular interaction area behind the icon.
    qreal stateLayerSize = 40.0;

    // --- colour tables, indexed [selection][interaction] ------------------------
    /// The icon colour — one colour for the stroke circle *and* the dot.
    /// Unselected rests on `on-surface-variant` and lifts to `on-surface`
    /// under interaction; selected is `primary` everywhere; disabled reads
    /// `on-surface` at 0.38 on both sides.
    MdNavigationColourSlot icon[radioButtonSelectionCount][navItemStateCount];
    /// The circular state layer. Unselected `on-surface` under hover/focus and
    /// `primary` when pressed; selected `primary` and `on-surface` pressed.
    /// No enabled/dragged/disabled rows — the export publishes none.
    MdNavigationColourSlot stateLayer[radioButtonSelectionCount][navItemStateCount];

    /// The disabled icon opacities (`disabled.*.icon.opacity`, both 0.38).
    qreal disabledIconOpacity = 0.38;

    // --- focus indicator ----------------------------------------------------------
    /// The export publishes no focus-indicator rows; the ring carries the
    /// system values and Compose's `CircleShape`.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- system state-layer opacities ----------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- Compose behaviour constants (not export rows) -------------------------------
    /// `RadioStrokeWidth` — the stroke circle's width (2 px).
    qreal strokeWidth = 2.0;
    /// `RadioButtonDotSize` — the dot's diameter *before* the stroke inset
    /// (12 px; the drawn dot radius is this/2 − strokeWidth/2, i.e. 5 px).
    qreal dotSize = 12.0;
    /// `RadioButtonPadding` — the gap between the icon canvas and the state
    /// layer's box (2 px).
    qreal padding = 2.0;
    /// `minimumInteractiveComponentSize` — the widget is this big when it can
    /// be clicked; the icon and state layer centre in it.
    qreal minimumInteractiveSize = 48.0;

    // --- accessors -----------------------------------------------------------------
    const MdNavigationColourSlot &iconFor(MdRadioButtonSelection selection,
                                          MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(MdRadioButtonSelection selection,
                                                MdNavigationItemState state) const;

    static MdRadioButtonTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_RADIO_BUTTON_TOKENS_H
