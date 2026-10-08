#ifndef MD_EXTENDED_FAB_TOKENS_H
#define MD_EXTENDED_FAB_TOKENS_H

// The published `md.comp.extended-fab.*` token set, resolved for one
// (colour set, size, lowered) combination.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-extended-fab.scss           (base metrics)
//     tokens/versions/latest/sass/_md-comp-extended-fab-<size>.scss    (3 sizes)
//     tokens/versions/latest/sass/_md-comp-extended-fab-<set>.scss     (6 colour sets)
//     tokens/versions/latest/sass/_md-sys-state*.scss                  (opacities,
//                                                                       focus indicator)
//
// Token export version 34.0.21.
//
// Facts of the export worth keeping visible (also recorded in
// docs/porting-todo.md):
//
//   * **No surface colour set.** The FAB family publishes surface/primary/
//     secondary/tertiary; the extended FAB ships primary/secondary/tertiary
//     plus three *-container sets and nothing surface-based. The spec page
//     agrees, so this is a family difference, not an export gap.
//   * **The base metric set matches no size table.** The unprefixed file
//     publishes 56 px, label-large text and a 16/12/20 space rhythm, while
//     the small size table publishes 56 px with title-medium text and a
//     16/8/16 rhythm. We ship the three explicit size tables
//     (small/medium/large = 56/80/96 px) and treat the base rows as the
//     legacy standard-M3 default they are.
//   * The colour rows are *flat* — the non-deprecated hovered/focused/pressed
//     rows restate the enabled roles, so the per-state colour table is one
//     interactive row plus a disabled row. (The colour sets also carry
//     *deprecated* focus rows pointing at the `*-container` roles; the
//     non-deprecated `focused-*` rows point at the interactive roles and are
//     the ones used here.) Elevation is where the states differ — hover
//     climbs one level, exactly as the FAB family does.
//   * The export publishes **no disabled rows at all**. The disabled values
//     here (on-surface @12% container, on-surface @38% icon and label,
//     level0) come from the official spec page's disabled state table, the
//     same disabled row every push-button family uses.
//   * No pressed container shape is published — like the FAB family, the
//     container never morphs on press; the ripple is the whole response.
//   * Width is *not* a token: the container is leading-space + icon +
//     icon-label-space + label + trailing-space wide. The layout derives it,
//     which is the entire point of an "extended" FAB.

#include "MdFabTokens.h"
#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to paint and lay out one extended FAB.
struct QT_MD3_EXPORT MdExtendedFabTokens
{
    // --- metrics (md.comp.extended-fab.<size>.*) ---------------------------
    qreal containerHeight = 56.0;
    qreal iconSize = 24.0;
    /// The gap between the icon and the label.
    qreal iconLabelSpace = 8.0;
    qreal leadingSpace = 16.0;
    qreal trailingSpace = 16.0;

    // --- shape (md.comp.extended-fab.<size>.container.shape) ----------------
    /// small → corner-large (16 px), medium → corner-large-increased (20 px),
    /// large → corner-extra-large (28 px). Static for the life of a press —
    /// see the file comment.
    ShapeCorner containerShape = ShapeCorner::Large;

    // --- label text (md.comp.extended-fab.<size>.label-text.type-scale) -----
    /// small → title-medium, medium → title-large, large → headline-small.
    TypeStyle labelStyle = TypeStyle::TitleMedium;

    // --- focus indicator (md-sys-state-focus-indicator) ---------------------
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- colours + elevation ----------------------------------------------
    /// Same shape as the FAB family — flat colour rows, per-state elevation —
    /// so the FAB structs are reused verbatim.
    MdFabFamily family;

    /// Build the published set for one combination.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, with
    /// the more qualified key winning (size segment over the bare set).
    /// Colours are not overridable — same rule as the button and FAB
    /// families, recorded in docs/porting-todo.md. Shape values accept both
    /// spellings ("full" / "corner-full").
    static MdExtendedFabTokens resolve(ExtendedFabVariant variant,
                                       ExtendedFabSize size,
                                       bool lowered,
                                       const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_EXTENDED_FAB_TOKENS_H
