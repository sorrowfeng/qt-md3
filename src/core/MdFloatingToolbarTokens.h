#ifndef MD_FLOATING_TOOLBAR_TOKENS_H
#define MD_FLOATING_TOOLBAR_TOKENS_H

// The published `md.comp.toolbar.floating.*`, `md.comp.toolbar.floating.fab.*`,
// `md.comp.toolbar.standard.*` and `md.comp.toolbar.vibrant.*` token sets.
//
// Every number and every role below is transcribed from the authoritative
// sources, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-toolbar-floating.scss
//     tokens/versions/latest/sass/_md-comp-toolbar-floating-fab.scss
//     tokens/versions/latest/sass/_md-comp-toolbar-standard.scss
//     tokens/versions/latest/sass/_md-comp-toolbar-vibrant.scss
//   androidx/androidx
//     compose/material3/.../FloatingToolbar.kt               (behaviour)
//     compose/material3/.../tokens/FloatingToolbarTokens.kt   (cross-check)
//
// Token export version 34.0.21. Compose's `FloatingToolbarTokens` is
// VERSION 12_0_0 and is *older* than the export: it lacks the horizontal /
// vertical split and the elevation row entirely. Every value it does carry
// agrees with the export row for row; where the two disagree the port follows
// the export and says so below.
//
// Eight facts of this variant worth keeping visible:
//
//   * **The floating toolbar is the one variant that *is* a Compose
//     component.** m3.material.io's note reads: "On Jetpack Compose, the
//     floating toolbar is a separate component from the docked toolbar and
//     bottom app bar." `FloatingToolbar.kt` is 2000 lines and it, not
//     material-web (which ships no toolbars implementation at all), is the
//     behaviour source.
//   * **`container.elevation` is published and unused.** The export carries
//     `md.comp.toolbar.floating.container.elevation: level3`, and Compose's
//     `FloatingToolbarDefaults.ContainerExpandedElevation` is
//     `ElevationTokens.Level0` with the comment "TODO read from token". The
//     published value is carried here and is what this port paints; the
//     divergence is recorded in docs/porting-todo.md.
//   * **`container.shape` is `corner-full`, and it never morphs.** Every other
//     shape-bearing component in this library has a pressed shape; a floating
//     toolbar keeps its pill through the press, because the press lands on the
//     *items*, not on the container.
//   * **The standard / vibrant colour schemes are not nested under
//     `floating`.** Their rows live at `md.comp.toolbar.standard.*` and
//     `md.comp.toolbar.vibrant.*`; the `md.comp.toolbar.floating.standard.*` /
//     `.floating.vibrant.*` rows are deprecated duplicates of the same values,
//     carried here so a 2024 theme still resolves.
//   * **The selected group publishes colours but no opacities, and no disabled
//     row.** Both are real gaps in the export, not transcription slips: the
//     selected state-layer opacities are absent from all four scss files, and
//     the disabled rows exist only unselected. This port carries the
//     unselected opacities into the selected group and the unselected disabled
//     colours into `selected[Disabled]`, which is what Compose's
//     `!enabled -> disabled` precedence produces anyway. Recorded in
//     docs/porting-todo.md.
//   * **The adjacent FAB is two size sets, not one.** A floating toolbar's FAB
//     is 56 px with the toolbar expanded and 80 px with it collapsed —
//     `FabSizeRange = FabBaselineTokens.ContainerWidth..FabMediumTokens.
//     ContainerWidth` — so the FAB *grows* as the toolbar shrinks. Its shape
//     and elevation change with it (`corner-large` / level1 expanded,
//     `corner-large-increased` / level2 collapsed).
//   * **The cross extent is read from the split rows, not the single one.**
//     Compose's `FloatingToolbarDefaults.ContainerSize` is
//     `FloatingToolbarTokens.ContainerHeight`, i.e. the *deprecated*
//     `container.height`; the export has superseded that row with
//     `horizontal.container.height` / `vertical.container.width` and says so in
//     its own comment. `containerCrossExtent()` reads the pair, and the single
//     row is carried unread — the same treatment `docked.container.min-spacing`
//     gets. All three are 64, so nothing observable differs until a theme moves
//     one.
//   * **`container.between-space` is read here and not in Compose.** The row is
//     declared in `FloatingToolbarTokens` and referenced nowhere: upstream's
//     toolbar items are arranged by the caller's `Row`, so Compose ships no
//     spacing at all. `MdFloatingToolbar` owns its children's arrangement, so it
//     applies the row — see the field's own comment.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// The two published colour schemes of a floating toolbar.
///
/// m3.material.io's configuration table: "Color — Standard (default)", which
/// "Available as bottom app bar" in M3, and "Vibrant", which is M3 Expressive
/// only.
enum class MdToolbarColorScheme
{
    Standard,
    Vibrant,
    Count,
};

/// How a floating toolbar is laid out.
///
/// m3.material.io's configuration table: "Floating toolbar layout — Horizontal
/// (default) / Vertical".
enum class MdToolbarOrientation
{
    Horizontal,
    Vertical,
    Count,
};

/// Where the adjacent FAB sits, along the toolbar's main axis.
///
/// Compose splits the concept in two — `FloatingToolbarHorizontalFabPosition`
/// {Start, End} and `FloatingToolbarVerticalFabPosition` {Top, Bottom} — but
/// each pair is the same two places on the main axis, so one enum serves both
/// orientations: for a horizontal toolbar `Start` / `End` are the leading and
/// trailing edges, for a vertical one the top and the bottom. The defaults
/// agree as well (horizontal = End, vertical = Bottom), which is why `End` is
/// the default here: the FAB sits at the end of the toolbar.
enum class MdToolbarFabPosition
{
    Start,
    End,
    Count,
};

/// The five states a toolbar item's colour table is published for.
///
/// The *order matters*: `Disabled` is tested first, exactly as Compose's
/// `containerColor` / `contentColor` when-clauses do (`!enabled -> … -> dragged
/// -> … -> selected`), so a disabled selected item paints disabled. This is the
/// same asymmetry MdListTokens documents.
enum class MdToolbarItemState
{
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Disabled,
    Count,
};

/// One colour slot of a toolbar item.
///
/// A slot may be *absent* (`role == ColorRole::Count`), which is how the
/// published table records "this scheme does not paint this part": only the
/// selected group publishes a container colour, because an unselected item has
/// no container behind it.
struct QT_MD3_EXPORT MdToolbarColourSlot
{
    ColorRole role = ColorRole::Count;
    /// Multiplier on top of the role's colour, for the disabled opacities.
    qreal opacity = 1.0;

    bool isPresent() const { return role != ColorRole::Count; }
};

/// The resolved colours of one toolbar item in one interaction state.
struct QT_MD3_EXPORT MdToolbarItemColours
{
    /// `[selected.]button.container.color`. Selected only.
    MdToolbarColourSlot container;
    /// `[selected.][state.]icon.color`.
    MdToolbarColourSlot icon;
    /// `[selected.][state.]label-text.color`.
    MdToolbarColourSlot labelText;
    /// `[selected.][state.]state-layer.color`.
    ColorRole stateLayer = ColorRole::Count;

    /// The state-layer opacities. Unselected publishes all three
    /// (`md-sys-state.$hover/focus/pressed-state-layer-opacity`); selected
    /// publishes none, so these are carried across — see the header comment.
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    /// The opacity that applies for `state`, ignoring `Dragged` (a toolbar
    /// item does not drag).
    qreal stateLayerOpacityFor(MdToolbarItemState state) const;

    bool paintsContainer() const { return container.isPresent(); }
};

/// One colour scheme's worth of rows.
struct QT_MD3_EXPORT MdToolbarScheme
{
    /// `md.comp.toolbar.<scheme>.container.color`.
    ColorRole containerColor = ColorRole::SurfaceContainer;
    /// `md.comp.toolbar.<scheme>.button.container.color`.
    ColorRole buttonContainerColor = ColorRole::SurfaceContainer;
    /// `md.comp.toolbar.<scheme>.container.shape` — `corner-full` for both
    /// schemes; the shape is a layout row, repeated here because the export
    /// publishes it per scheme.
    ShapeCorner containerShape = ShapeCorner::Full;

    /// The unselected table, indexed by `MdToolbarItemState`.
    MdToolbarItemColours unselected[int(MdToolbarItemState::Count)];
    /// The selected table. `selected[Disabled]` repeats the unselected
    /// disabled rows because the export publishes none — see the header.
    MdToolbarItemColours selected[int(MdToolbarItemState::Count)];

    /// The colours for one (selected, state) pair. Never returns a dangling
    /// reference.
    const MdToolbarItemColours &item(bool selected, MdToolbarItemState state) const;
};

/// The adjacent floating action button's two size sets.
struct QT_MD3_EXPORT MdToolbarFabTokens
{
    /// `md.comp.toolbar.floating.fab.between-space` — the gap between the
    /// toolbar and the FAB. Compose spells the same 8 px
    /// `FloatingToolbarDefaults.ToolbarToFabGap` with a "TODO Load this from
    /// the component tokens?" comment; the export does have the row.
    qreal betweenSpace = 8.0;

    // --- expanded (the toolbar is open, the FAB is small) ------------------
    /// `md.comp.toolbar.floating.fab.container.width` / `.height`.
    qreal expandedSize = 56.0;
    /// `md.comp.toolbar.floating.fab.icon.size`.
    qreal expandedIconSize = 24.0;
    /// `md.comp.toolbar.floating.fab.container.shape`.
    ShapeCorner expandedShape = ShapeCorner::Large;
    /// `md.comp.toolbar.floating.fab.container.elevation`.
    ElevationLevel expandedElevation = ElevationLevel::Level1;

    // --- collapsed (the toolbar is shut, the FAB is large) -----------------
    /// `md.comp.toolbar.floating.fab.medium.container.width` / `.height` —
    /// the export's word for the collapsed size is "medium".
    qreal collapsedSize = 80.0;
    /// `md.comp.toolbar.floating.fab.medium.icon.size`.
    qreal collapsedIconSize = 28.0;
    /// `md.comp.toolbar.floating.fab.medium.container.shape`.
    ShapeCorner collapsedShape = ShapeCorner::LargeIncreased;
    /// `md.comp.toolbar.floating.fab.medium.container.elevation`.
    ElevationLevel collapsedElevation = ElevationLevel::Level2;

    /// `md.comp.toolbar.floating.fab.standard.container.color` /
    /// `.standard.icon.color`.
    ColorRole standardContainerColor = ColorRole::SecondaryContainer;
    ColorRole standardIconColor = ColorRole::OnSecondaryContainer;
    /// `md.comp.toolbar.floating.fab.vibrant.container.color` /
    /// `.vibrant.icon.color`.
    ColorRole vibrantContainerColor = ColorRole::TertiaryContainer;
    ColorRole vibrantIconColor = ColorRole::OnTertiaryContainer;

    /// The FAB's size for a given expansion progress: `1` is the toolbar fully
    /// expanded (FAB at `expandedSize`), `0` fully collapsed (FAB at
    /// `collapsedSize`). Compose lerps `FabSizeRange` by `1 - progress`.
    qreal sizeFor(qreal expansionProgress) const;

    /// The FAB's rounded corner for a given expansion progress.
    ShapeCorner shapeFor(qreal expansionProgress) const;
};

/// Everything needed to lay out and paint one floating toolbar.
struct QT_MD3_EXPORT MdFloatingToolbarTokens
{
    // --- container geometry (md.comp.toolbar.floating.*) -------------------
    /// `md.comp.toolbar.floating.container.height` — a deprecated row
    /// ("Deprecating this for a vertical and horizontal variant"). Carried, and
    /// deliberately *not* what `containerCrossExtent()` returns; see the header
    /// comment.
    qreal containerHeight = 64.0;
    /// `md.comp.toolbar.floating.horizontal.container.height`.
    qreal horizontalContainerHeight = 64.0;
    /// `md.comp.toolbar.floating.vertical.container.width`.
    qreal verticalContainerWidth = 64.0;

    /// `md.comp.toolbar.floating.container.leading-space` — the content
    /// padding's start. 8, chosen so that 8 + a 48 px icon button + 8 = the
    /// 64 px container height.
    qreal containerLeadingSpace = 8.0;
    /// `md.comp.toolbar.floating.container.trailing-space`.
    qreal containerTrailingSpace = 8.0;
    /// `md.comp.toolbar.floating.container.between-space` — the gap between
    /// *items* inside the toolbar. Not the toolbar-to-FAB gap; that is
    /// `MdToolbarFabTokens::betweenSpace`.
    ///
    /// **This is the one published row this port reads and Compose does not.**
    /// `FloatingToolbarTokens.ContainerBetweenSpace` exists in Compose and is
    /// referenced nowhere: a `HorizontalFloatingToolbar`'s items are arranged by
    /// the *caller's* `Row`, so upstream ships no spacing of its own and the row
    /// is advice rather than behaviour. `MdFloatingToolbar` owns the arrangement
    /// of its children, so it applies the row — the pill is
    /// `8 + n * item + (n - 1) * 4 + 8` long. Recorded in docs/porting-todo.md
    /// and pinned by `TestMd3Toolbar::betweenSpaceSeparatesTheSlots`.
    qreal containerBetweenSpace = 4.0;

    /// `md.comp.toolbar.floating.container.external-padding` — a deprecated
    /// row ("Deprecating for a vertical and horizontal option"). Carried.
    qreal containerExternalPadding = 16.0;
    /// `md.comp.toolbar.floating.horizontal.container.external-space` — the
    /// offset from the screen edge for a horizontal toolbar.
    qreal horizontalContainerExternalSpace = 16.0;
    /// `md.comp.toolbar.floating.vertical.container.external-space` — 24, so a
    /// vertical toolbar sits further in than a horizontal one.
    qreal verticalContainerExternalSpace = 24.0;

    /// `md.comp.toolbar.floating.container.elevation` — `level3`. Published
    /// and (in Compose) unused; see the header comment.
    ElevationLevel containerElevation = ElevationLevel::Level3;
    /// `md.comp.toolbar.floating.container.shape` — `corner-full`.
    ShapeCorner containerShape = ShapeCorner::Full;

    // --- the deprecated `floating.standard.*` / `floating.vibrant.*` rows --
    /// `md.comp.toolbar.floating.standard.container.color` — the same value as
    /// `md.comp.toolbar.standard.container.color`.
    ColorRole floatingStandardContainerColor = ColorRole::SurfaceContainer;
    /// `md.comp.toolbar.floating.vibrant.container.color`.
    ColorRole floatingVibrantContainerColor = ColorRole::PrimaryContainer;
    /// `md.comp.toolbar.floating.vibrant.button.selected.container.color`.
    ColorRole floatingVibrantButtonSelectedContainerColor = ColorRole::SurfaceContainer;
    /// `…vibrant.button.selected.icon.color`.
    ColorRole floatingVibrantButtonSelectedIconColor = ColorRole::OnSurface;
    /// `…vibrant.button.selected.text.color`.
    ColorRole floatingVibrantButtonSelectedTextColor = ColorRole::OnSurface;
    /// `…vibrant.button.unselected.icon.color`.
    ColorRole floatingVibrantButtonUnselectedIconColor = ColorRole::OnPrimaryContainer;
    /// `…vibrant.button.unselected.text.color`.
    ColorRole floatingVibrantButtonUnselectedTextColor = ColorRole::OnPrimaryContainer;

    // --- the two colour schemes -------------------------------------------
    MdToolbarScheme standard;
    MdToolbarScheme vibrant;

    /// The adjacent FAB's rows.
    MdToolbarFabTokens fab;

    // --- Compose-derived constants (no token row) --------------------------
    /// `FloatingToolbarDefaults.ScrollDistanceThreshold` — the scroll distance
    /// that triggers an expand or a collapse for the vertical-nested-scroll
    /// modifier. Compose-only.
    qreal scrollDistanceThreshold = 40.0;

    /// The colour scheme's rows.
    const MdToolbarScheme &schemeFor(MdToolbarColorScheme scheme) const;

    /// The container's height for `orientation`, and — for a vertical toolbar
    /// — the width. A vertical toolbar is `verticalContainerWidth` wide and
    /// grows downward; a horizontal one is `horizontalContainerHeight` tall.
    qreal containerCrossExtent(MdToolbarOrientation orientation) const;

    /// The offset from the screen edge for `orientation`.
    qreal externalSpaceFor(MdToolbarOrientation orientation) const;

    /// Build the published set.
    ///
    /// `overrides` is consulted through the `md.comp.toolbar.*` key namespace.
    /// Both colour schemes are resolved in one call — the struct carries
    /// `standard` and `vibrant` alike — so there is no scheme argument here;
    /// a caller picks one with `schemeFor()`.
    static MdFloatingToolbarTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_FLOATING_TOOLBAR_TOKENS_H
