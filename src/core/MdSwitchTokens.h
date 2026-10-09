#ifndef MD_SWITCH_TOKENS_H
#define MD_SWITCH_TOKENS_H

// MdSwitchTokens — `md.comp.switch.*` at export version 34.0.21.
//
// ## What the export publishes
//
// Metrics (track 52×32 with a 2 px corner-full outline, handle diameters 16
// unselected / 24 selected / 28 pressed / 24 with-icon, the 16 px thumb icon,
// the 40 px state layer), the focus-indicator rows (secondary, system outer
// offset / thickness), the disabled opacities and four colour tables indexed
// `[selection][interaction]`: track, track outline, handle and thumb icon —
// plus the state-layer rows that ride the *thumb* (Compose hangs the ripple on
// the thumb, an unbounded 40 px circle).
//
// ## Compose's behaviour this widget ports
//
//   * **the thumb travels and resizes on one spring.** `ThumbNode.measure`
//     computes both targets from the state — unchecked 16 px at the track's
//     left inner inset, checked 24 px at the far bound, pressed 28 px snapped
//     2 px inward — and animates size *and* offset on the fast spatial spring.
//     **While pressed both animations run `SnapSpec`** — the press lands
//     instantly; releasing springs back.
//   * **no colour animation.** `SwitchImpl` resolves
//     `colors.thumbColor(enabled, checked)` straight into the `Box` — a
//     selection's colours land this frame (the same standing as the chips).
//   * **the ripple is the thumb's** — `ripple(bounded = false, radius =
//     StateLayerSize / 2)` on the thumb Box; the state-layer colour rows are
//     tagged "(ripple)" against the track context but paint at the thumb.
//
// ## Divergences pinned here rather than smoothed
//
// * **The deprecated 20 px handle rows are carried, not read.**
//   `handle.height` / `handle.width` (20 px) predate the sizing rework; the
//   per-state rows (16/24/28/24) win.
// * **The inset-ring focus variant is opt-in upstream and not ported.**
//   Compose gates a `focusRingShape = trackShape` inset ring behind
//   `RippleThemeConfiguration.Focus.InsetRing`; the default paints the
//   outward system ring around the track, which is what this port does.
// * **The disabled selected handle opacity is 1, not 0.38.** The export's
//   `disabled.selected.handle.opacity` row — only the *icon* and the
//   *unselected handle* fade; the selected handle keeps full strength on its
//   `surface` row, and the *track* fades at its own 0.12.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which side of the colour tables a paint reads.
enum class MdSwitchSelection {
    Unselected,
    Selected,
    Count,
};

constexpr int switchSelectionCount = int(MdSwitchSelection::Count);

/// One resolved switch. The colour tables are indexed
/// `[selection][interaction]` (the five `MdNavigationItemState`s; the switch
/// publishes no dragged rows).
struct QT_MD3_EXPORT MdSwitchTokens
{
    // --- metrics ---------------------------------------------------------------
    qreal trackWidth = 52.0;
    qreal trackHeight = 32.0;
    /// `track.outline.width: 2px`.
    qreal trackOutlineWidth = 2.0;
    /// The handle diameters: resting unselected, resting selected, pressed and
    /// with-icon. Compose's `ThumbNode.measure` picks between them.
    qreal unselectedHandleSize = 16.0;
    qreal selectedHandleSize = 24.0;
    qreal pressedHandleSize = 28.0;
    qreal withIconHandleSize = 24.0;
    /// The thumb icon's size (`selected.icon.size` / `unselected.icon.size`,
    /// both 16).
    qreal iconSize = 16.0;
    /// The circular ripple bounds around the thumb.
    qreal stateLayerSize = 40.0;

    // --- colour tables, indexed [selection][interaction] ------------------------
    /// The track fill. Unselected rests on `surface-container-highest`
    /// (interaction rows unchanged); selected `primary`; disabled `on-surface`
    /// at the 0.12 track opacity on both sides.
    MdNavigationColourSlot track[switchSelectionCount][navItemStateCount];
    /// The 2 px track outline. Unselected `outline` for every enabled
    /// interaction, `on-surface` at 0.12 disabled; the selected side has no
    /// rows (the checked border resolves transparent — Compose's default
    /// `checkedBorderColor`).
    MdNavigationColourSlot trackOutline[switchSelectionCount][navItemStateCount];
    /// The handle. Unselected `outline` resting, `on-surface-variant` under
    /// interaction, `on-surface` at 0.38 disabled; selected `on-primary`
    /// resting, `primary-container` under interaction, `surface` at **full
    /// strength** disabled.
    MdNavigationColourSlot handle[switchSelectionCount][navItemStateCount];
    /// The thumb icon (the checkmark slot). Unselected
    /// `surface-container-highest` — the same row as the track; selected
    /// `primary`. Disabled 0.38 on both sides.
    MdNavigationColourSlot icon[switchSelectionCount][navItemStateCount];
    /// The state layer riding the thumb. Unselected `on-surface` under
    /// hover/focus/press; selected `primary`. No enabled row.
    MdNavigationColourSlot stateLayer[switchSelectionCount][navItemStateCount];

    // --- disabled opacities -------------------------------------------------------
    /// `disabled.track.opacity` — the *track* (and its outline) fade at 0.12,
    /// not the content's 0.38.
    qreal disabledTrackOpacity = 0.12;
    /// `disabled.handle.opacity` — the unselected handle (and the deprecated
    /// row's namesake).
    qreal disabledUnselectedHandleOpacity = 0.38;
    /// `disabled.selected.handle.opacity` — **1**: the selected handle keeps
    /// full strength on its `surface` row.
    qreal disabledSelectedHandleOpacity = 1.0;
    /// `disabled.*.icon.opacity` — the thumb icon, both sides.
    qreal disabledIconOpacity = 0.38;

    // --- the handle's elevation ------------------------------------------------------
    /// `handle.elevation: level1`, `disabled.handle.elevation: level0`, the
    /// shadow in `handle.shadow-color`.
    ElevationLevel handleElevation = ElevationLevel::Level1;
    ElevationLevel disabledHandleElevation = ElevationLevel::Level0;
    ColorRole handleShadowColor = ColorRole::Shadow;

    // --- focus indicator ---------------------------------------------------------------
    /// Around the *track* (`focus.indicator.offset` is the system outer
    /// offset; thickness the system thickness; colour secondary).
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorOuterOffset = 2.0;
    qreal focusIndicatorThickness = 3.0;

    // --- system state-layer opacities -----------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- Compose behaviour constants (not export rows) ---------------------------------------
    /// `minimumInteractiveComponentSize` — the touch target the 52×32 track
    /// centres in.
    qreal minimumInteractiveSize = 48.0;
    /// Compose's unbounded ripple radius (`StateLayerSize / 2`).
    qreal rippleRadius = 20.0;

    // --- accessors ------------------------------------------------------------------------------
    const MdNavigationColourSlot &trackFor(MdSwitchSelection selection,
                                           MdNavigationItemState state) const;
    const MdNavigationColourSlot &trackOutlineFor(MdSwitchSelection selection,
                                                  MdNavigationItemState state) const;
    const MdNavigationColourSlot &handleFor(MdSwitchSelection selection,
                                            MdNavigationItemState state) const;
    const MdNavigationColourSlot &iconFor(MdSwitchSelection selection,
                                          MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(MdSwitchSelection selection,
                                                MdNavigationItemState state) const;

    static MdSwitchTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_SWITCH_TOKENS_H
