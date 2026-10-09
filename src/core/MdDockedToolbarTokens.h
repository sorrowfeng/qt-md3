#ifndef MD_DOCKED_TOOLBAR_TOKENS_H
#define MD_DOCKED_TOOLBAR_TOKENS_H

// The published `md.comp.toolbar.docked.*` token set.
//
// Every number and every role below is transcribed from the authoritative
// sources, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-toolbar-docked.scss
//   androidx/androidx
//     compose/material3/.../tokens/DockedToolbarTokens.kt        (cross-check)
//     compose/material3/.../AppBar.kt        (the docked toolbar's *behaviour*)
//
// Token export version 34.0.21. Compose's `DockedToolbarTokens` agrees with
// the export row for row, including the deprecated colour row; where this port
// follows one source over the other it says so below.
//
// Five facts of this variant worth keeping visible:
//
//   * **The docked toolbar has no component of its own in Compose.** The
//     `tokens/` directory is the only place it exists there. Its behaviour is
//     realised as `FlexibleBottomAppBar` (`AppBar.kt`), which reads
//     `DockedToolbarTokens` for its height, its content padding and its
//     spacing — so that composable *is* the docked toolbar, and this port
//     models it as `MdDockedToolbar`.
//   * **m3.material.io lists it as a first-class variant, not a baseline.**
//     The spec's variant table reads: "Docked toolbar — M3: --, M3 Expressive:
//     Available", and the baseline row above it says the bottom app bar "is no
//     longer recommended. It should be replaced with the docked toolbar, which
//     is very similar and more flexible." Only `container.shape` differs from
//     the baseline (`corner-none` for both, in fact — the flexibility is in
//     the spacing and the collapse, not the silhouette).
//   * **`container.color` is a deprecated row.** The comment reads "Moving
//     this token to the color token set"; there is no replacement row here, so
//     the published value stands and is carried.
//   * **`container.min-spacing` is published and unused.** Compose's
//     `FlexibleHorizontalArrangement` is `Arrangement.spacedBy(maxSpacing,
//     Alignment.CenterHorizontally)` — it reads `ContainerMaxSpacing` (32) and
//     never `ContainerMinSpacing` (4). The row is carried so a theme that sets
//     it still resolves, and recorded in docs/porting-todo.md rather than
//     invented into the layout.
//   * **The whole bar collapses, not a row.** `BottomAppBarLayout` sets
//     `heightOffsetLimit = -placeable.height`, so a docked toolbar is
//     single-row and scrolls its *entire* height away. That is the opposite of
//     a two-row top app bar, which keeps its icon row; see `MdAppBarTokens::
//     heightFor` for the other half of the rule.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to lay out and paint one docked toolbar.
///
/// A docked toolbar is a single full-width row pinned to an edge: a 64 px
/// container, `corner-none`, `surface-container`, its content padded 16 px in
/// from each end and distributed with at least 32 px between items, centred.
/// It is the expressive replacement for the bottom app bar.
struct QT_MD3_EXPORT MdDockedToolbarTokens
{
    /// `md.comp.toolbar.docked.container.height`.
    qreal containerHeight = 64.0;
    /// `md.comp.toolbar.docked.container.leading-space` — the content's start
    /// padding (`BottomAppBarDefaults.FlexibleContentPadding`).
    qreal containerLeadingSpace = 16.0;
    /// `md.comp.toolbar.docked.container.trailing-space`.
    qreal containerTrailingSpace = 16.0;
    /// `md.comp.toolbar.docked.container.max-spacing` — the gap Compose's
    /// `spacedBy` uses.
    qreal containerMaxSpacing = 32.0;
    /// `md.comp.toolbar.docked.container.min-spacing` — published, and read by
    /// nothing in Compose. Carried, never used.
    qreal containerMinSpacing = 4.0;

    /// `md.comp.toolbar.docked.container.color` — a deprecated row ("Moving
    /// this token to the color token set") with no replacement here.
    ColorRole containerColor = ColorRole::SurfaceContainer;

    /// `md.comp.toolbar.docked.container.shape` — `corner-none`. Compose
    /// reaches the same value through `BottomAppBarTokens.ContainerShape`
    /// rather than this row; the values agree, the sources differ.
    ShapeCorner containerShape = ShapeCorner::None;

    /// `BottomAppBarDefaults.ContentPadding`'s start / top / end.
    ///
    /// The row pair above is start/end only; Compose lays the content out in a
    /// `height(containerHeight)` row with `verticalAlignment =
    /// CenterVertically`, so there is no vertical content padding — the 64 px
    /// container centres it. `contentTopSpace` exists so a theme can still
    /// retune it, and is not a token row.
    qreal contentTopSpace = 0.0;
    qreal contentBottomSpace = 0.0;

    /// Build the published set.
    ///
    /// `overrides` is consulted through the `md.comp.toolbar.docked.*` key
    /// namespace. Lengths and the shape are overridable; colours are not — the
    /// same rule the rest of the library follows, recorded in
    /// docs/porting-todo.md.
    static MdDockedToolbarTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_DOCKED_TOOLBAR_TOKENS_H
