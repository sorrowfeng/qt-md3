#ifndef MD_CHECKBOX_TOKENS_H
#define MD_CHECKBOX_TOKENS_H

// MdCheckBoxTokens — `md.comp.checkbox.*` at export version 34.0.21.
//
// ## What the export publishes
//
// One checkbox, three axes of state that the rows cross-cut: **selection**
// (the `selected.*` / `unselected.*` / `error.*` prefixes), **interaction**
// (the `.hover.` / `.focus.` / `.pressed.` / `.disabled.` infixes) and the
// handful of metric rows. The boxes' own colour model is Compose's, because
// the export mostly publishes *outline* and *state-layer* rows and leaves the
// fill implicit:
//
//   * an **unchecked** box paints only its 2 px outline — the export has no
//     `unselected.container.color` row, and Compose resolves
//     `uncheckedBoxColor = Color.Transparent`;
//   * a **checked or indeterminate** box paints `selected.container.color`
//     (`primary`, `error` in the error variant) *and* the same colour as its
//     border — Compose's `drawBox` short-circuits the two to one filled round
//     rect when they are equal;
//   * **indeterminate is a selected state** — Compose's `boxColor` and
//     `borderColor` resolve `On` and `Indeterminate` to the same rows.
//
// ## The state-layer special cases
//
// `unselected.pressed.state-layer.color` is `primary` and
// `selected.pressed.state-layer.color` is `on-surface` — the press layer is
// the colour the box is *about to earn*, not the one it has (the same idea the
// tabs' inactive pressed layer carries). The error variant presses `error`.
//
// ## Divergences pinned here rather than smoothed
//
// * **The deprecated rows are carried, not read.** `unselected.*.icon.color`
//   and `disabled.*.icon.opacity/color` predate the rendering rework ("Checkbox
//   changed how rendering was specified"); the current model colours the
//   checkmark from `selected.icon.color` even at rest and folds the 0.38 into
//   the *container* opacity rows. The unselected rows are filled `on-surface`
//   into the checkmark table for the record.
// * **The error outline-width rows are carried as comments.** The export
//   publishes `unselected.error.*.outline.width: 2px` and
//   `selected.error.*.outline.width: 0px` marked "redundant" — the same numbers
//   the base width rows already carry.
// * **Compose's unchecked ripple colour is a bug.** With the styling fix on,
//   `indicatorColor(Off)` returns `uncheckedBoxColor`, which the default
//   colours set to `Color.Transparent` — an unchecked checkbox would ripple
//   invisibly. The export's state-layer rows win: a press ripples in the
//   pressed row's colour (`primary` unchecked, `on-surface` checked).
// * **The check proportions follow the styling fix.** Compose gates the check
//   path between the pre-fix fractions (left 0.2 / cross 0.4/0.7 / right 0.8)
//   and the fix's (0.25 / 0.4/0.65 / 0.75); the fix is current and wins.
// * **The focus ring's shape follows the component.** The export publishes no
//   `focus-indicator.shape` row, so material-web's rule applies — the ring's
//   radii follow the box's, offset by the gap. Compose overrides with a 25 %
   //  rounded rect instead; the difference is under 2 px at this size and is
//   recorded in porting-todo.md.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which side of the colour tables a paint reads. An indeterminate checkbox
/// resolves `Selected` — Compose's `On` and `Indeterminate` branches are one.
enum class MdCheckBoxSelection {
    Unselected,
    Selected,
    Count,
};

constexpr int checkBoxSelectionCount = int(MdCheckBoxSelection::Count);

/// One resolved checkbox. The four colour tables are indexed
/// `[selection][interaction]`; the error variant has its own overlay tables
/// and falls back to the base ones wherever the export publishes no row.
struct QT_MD3_EXPORT MdCheckBoxTokens
{
    // --- metrics ---------------------------------------------------------------
    qreal containerSize = 18.0;
    /// `container.shape: 2px` — the box's corner radius.
    qreal containerShapeRadius = 2.0;
    qreal iconSize = 18.0;
    /// Compose's `CheckboxDefaults.StrokeWidth` — the outline and the check
    /// stroke share it (2 px).
    qreal strokeWidth = 2.0;
    /// The circular interaction area behind the box.
    qreal stateLayerSize = 40.0;

    // --- colour tables, indexed [selection][interaction] ------------------------
    /// The box fill. Unselected rows are absent (a transparent fill — the
    /// export publishes no `unselected.container.color` row); selected rows are
    /// `primary`, the disabled one `on-surface` at the 0.38 container opacity.
    MdNavigationColourSlot box[checkBoxSelectionCount][navItemStateCount];
    /// The border stroke. Unselected `on-surface-variant` at rest, lifting to
    /// `on-surface` under interaction; selected rows are absent (width 0).
    MdNavigationColourSlot outline[checkBoxSelectionCount][navItemStateCount];
    /// The check stroke. Selected `on-primary` (disabled: `surface`); the
    /// unselected side carries the deprecated rows for the record.
    MdNavigationColourSlot checkmark[checkBoxSelectionCount][navItemStateCount];
    /// The circular state layer. Unselected `on-surface` under hover/focus and
    /// `primary` when pressed; selected `primary` and `on-surface` pressed.
    MdNavigationColourSlot stateLayer[checkBoxSelectionCount][navItemStateCount];

    // --- the error variant's own rows ------------------------------------------
    /// Selected: `error` for every enabled interaction. Unselected: absent (a
    /// transparent fill, like the base table).
    MdNavigationColourSlot errorBox[checkBoxSelectionCount][navItemStateCount];
    /// Unselected: `error` for every enabled interaction. Selected: absent
    /// (the outline width is 0 there).
    MdNavigationColourSlot errorOutline[checkBoxSelectionCount][navItemStateCount];
    /// Selected: `on-error`. Unselected: absent.
    MdNavigationColourSlot errorCheckmark[checkBoxSelectionCount][navItemStateCount];
    /// Hovered / Focused / Pressed: `error`. The error variant publishes no
    /// disabled rows — disabled falls back to the base tables.
    MdNavigationColourSlot errorStateLayer[navItemStateCount];

    /// The disabled opacities. `selected.disabled.container.opacity` folds the
    /// whole box to 38 %; `unselected.disabled.container.opacity` rides the
    /// outline (there is no unselected fill). Compose writes both as one
    /// `DisabledAlpha`-style pair on the resolved colours.
    qreal selectedDisabledContainerOpacity = 0.38;
    qreal unselectedDisabledContainerOpacity = 0.38;

    // --- focus indicator --------------------------------------------------------
    /// `focus-indicator.color: secondary`, `outline-offset: sys outer-offset`,
    /// `thickness: sys thickness` — an *outward* ring around the box.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- system state-layer opacities ------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- Compose behaviour constants (not export rows) --------------------------
    /// `SnapAnimationDelay` — an undo (anything → `Off`) holds the old visual
    /// for this long, then snaps.
    qreal snapAnimationDelayMs = 100.0;
    /// `minimumInteractiveComponentSize` — the widget is this big when it can
    /// be clicked; the box and state layer centre in it.
    qreal minimumInteractiveSize = 48.0;
    /// The check path, as fractions of the box size (`drawCheck`, styling fix).
    qreal checkLeftX = 0.25;
    qreal checkLeftY = 0.5;
    qreal checkCrossX = 0.4;
    qreal checkCrossY = 0.65;
    qreal checkRightX = 0.75;
    qreal checkRightY = 0.3;
    /// Compose's `focusRingShape = RoundedCornerShape(25)` — a per cent of the
    /// ring's size. Carried for the record; the paint follows the component's
    /// radii instead (see the header note).
    qreal focusRingRadiusPercent = 25.0;

    // --- accessors with the error fallback baked in ------------------------------
    const MdNavigationColourSlot &boxFor(bool error, MdCheckBoxSelection selection,
                                         MdNavigationItemState state) const;
    const MdNavigationColourSlot &outlineFor(bool error, MdCheckBoxSelection selection,
                                             MdNavigationItemState state) const;
    const MdNavigationColourSlot &checkmarkFor(bool error, MdCheckBoxSelection selection,
                                               MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(bool error, MdCheckBoxSelection selection,
                                                MdNavigationItemState state) const;

    static MdCheckBoxTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_CHECKBOX_TOKENS_H
