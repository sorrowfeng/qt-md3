#ifndef MD_DIVIDER_TOKENS_H
#define MD_DIVIDER_TOKENS_H

// The published `md.comp.divider.*` token set.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-divider.scss             (all rows)
//
// Token export version 34.0.21.
//
// Three facts of this family worth keeping visible:
//
//   * The export is the **smallest token table in the library**: exactly two
//     rows — `thickness: 1px` and `color: outline-variant`. A divider is a
//     hairline that groups content, nothing more. There are no state rows
//     (no hover, no press, no focus indicator, no disabled row) and no
//     shape rows: non-interactivity is the contract, not a gap.
//   * The 16 px inset is **not a published token**. The m3.material.io
//     measurements list it ("Divider inset left margin 16dp"), but
//     material-web hardcodes it as `padding-inline-start: 16px` in
//     divider/internal/_divider.scss. It is tokenised here so themes can
//     retune it; the hardcoding is recorded in docs/porting-todo.md.
//   * Naming divergence between the sources: material-web's `[inset]`
//     attribute pads **both** inline edges, while the spec's "inset"
//     measurement is **start-only** (left 16 / right 0) and its
//     "middle-inset" is both. The widget's InsetMode exposes all four
//     combinations, so each source's vocabulary is reachable; see
//     MdDivider's header comment.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to paint and lay out one divider.
struct QT_MD3_EXPORT MdDividerTokens
{
    // --- metrics ----------------------------------------------------------
    /// `md.comp.divider.thickness` — the hairline's cross-axis size.
    qreal thickness = 1.0;
    /// The spec inset (not in the export): material-web's
    /// `padding-inline-start/end: 16px`, the m3.material.io "inset" margins.
    qreal inset = 16.0;

    // --- colours -----------------------------------------------------------
    /// `md.comp.divider.color`.
    ColorRole color = ColorRole::OutlineVariant;

    /// Build the published set.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace:
    ///
    ///   md.comp.divider.thickness
    ///   md.comp.divider.inset
    ///
    /// Colours are not overridable — same rule as the button families,
    /// recorded in docs/porting-todo.md.
    static MdDividerTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_DIVIDER_TOKENS_H
