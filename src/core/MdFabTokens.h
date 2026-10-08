#ifndef MD_FAB_TOKENS_H
#define MD_FAB_TOKENS_H

// The published `md.comp.fab.*` token set, resolved for one
// (variant, size, lowered) combination.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-fab.scss               (base metrics)
//     tokens/versions/latest/sass/_md-comp-fab-<size>.scss        (3 sizes)
//     tokens/versions/latest/sass/_md-comp-fab-<variant>.scss     (4 colour sets)
//     tokens/versions/latest/sass/_md-sys-state*.scss             (opacities,
//                                                                  focus indicator)
//
// Token export version 34.0.21.
//
// Three facts of the export worth keeping visible:
//
//   * The colour rows are *flat* — the non-deprecated hovered/focused/pressed
//     rows restate the enabled roles, so the per-state colour table is one
//     interactive row plus a disabled row. Elevation is where the states
//     actually differ (hovered climbs one level).
//   * The export publishes **no disabled rows at all**. The disabled values
//     here (on-surface @12% container, on-surface @38% icon, level0) come from
//     the official spec page's disabled state table, the same disabled row
//     every push-button family uses. Recorded in docs/porting-todo.md.
//   * No pressed container shape is published — a FAB does not morph its
//     corners on press (unlike the button/icon-button families, whose pressed
//     shape tokens drive their spring morphs). The press response is the
//     ripple alone, which is also what the "Pressed (ripple)" row labels in
//     the export mean.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// One colour slot of a FAB: a role plus an opacity multiplier, where
/// "absent" is a real value.
struct QT_MD3_EXPORT MdFabColourSlot
{
    /// `ColorRole::Count` means "this state does not paint this part".
    ColorRole role = ColorRole::Count;
    qreal opacity = 1.0;

    bool isPresent() const { return role != ColorRole::Count; }
};

/// The colours one interaction state paints with.
struct QT_MD3_EXPORT MdFabStateColours
{
    /// `container.color`.
    MdFabColourSlot container;
    /// `icon.color`.
    MdFabColourSlot icon;
    /// `state-layer.color`. The hover / keyboard-focus overlay source; the
    /// press overlay is the ripple, not this layer.
    ColorRole stateLayer = ColorRole::Count;
};

/// The five states a FAB's token table is published for.
enum class MdFabState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

/// Colours and elevation across all five states.
struct QT_MD3_EXPORT MdFabFamily
{
    MdFabStateColours enabled;
    MdFabStateColours hovered;
    MdFabStateColours focused;
    MdFabStateColours pressed;
    MdFabStateColours disabled;

    /// `container.elevation` per state — the *non-deprecated* rows, which is
    /// why pressed repeats enabled rather than the deprecated
    /// `pressed.container.elevation` (level3 raised / level1 lowered — the
    /// same values either way).
    ElevationLevel enabledElevation = ElevationLevel::Level3;
    ElevationLevel hoveredElevation = ElevationLevel::Level4;
    ElevationLevel focusedElevation = ElevationLevel::Level3;
    ElevationLevel pressedElevation = ElevationLevel::Level3;
    ElevationLevel disabledElevation = ElevationLevel::Level0;

    const MdFabStateColours &state(MdFabState which) const;
    ElevationLevel elevation(MdFabState which) const;
};

/// Everything needed to paint and lay out one FAB.
struct QT_MD3_EXPORT MdFabTokens
{
    // --- metrics (md.comp.fab.<size>.*) -----------------------------------
    qreal containerHeight = 56.0;
    qreal containerWidth = 56.0;
    qreal iconSize = 24.0;

    // --- shape (md.comp.fab.<size>.container.shape) ------------------------
    /// small → corner-medium (12 px), medium → corner-large (16 px),
    /// large → corner-extra-large (28 px). Static for the life of a press —
    /// see the file comment.
    ShapeCorner containerShape = ShapeCorner::Large;

    // --- focus indicator (md-sys-state-focus-indicator) ---------------------
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours + elevation ----------------------------------------------
    MdFabFamily family;

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, with
    /// the more qualified key winning:
    ///
    ///   md.comp.fab.<size>.container.height        (wins)
    ///   md.comp.fab.container.height               (fallback)
    ///   published value                            (default)
    ///
    /// Colours are not overridable — same rule as the button families,
    /// recorded in docs/porting-todo.md. Shape values accept both spellings
    /// ("full" / "corner-full").
    static MdFabTokens resolve(FabVariant variant,
                               FabSize size,
                               bool lowered,
                               const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_FAB_TOKENS_H
