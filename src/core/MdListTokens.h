#ifndef MD_LIST_TOKENS_H
#define MD_LIST_TOKENS_H

// The published `md.comp.list.*` token sets — the container and its items.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-list.scss              (all rows)
//     list/internal/_list.scss, list/internal/listitem/_list-item.scss
//       (the two layout facts the export does not carry: the container's
//        `padding: 8px 0` and the 16px baseline gap)
//     tokens/versions/latest/sass/_md-sys-state*.scss             (opacities)
//     tokens/versions/latest/sass/_md-sys-state-focus-indicator.scss
//
// Token export version 34.0.21.
//
// Behaviour facts the export does not state, taken from the two behaviour
// sources and annotated where they are used:
//
//   [compose]  androidx Compose Material3 ListItem.kt / ListItemDefaults.kt /
//              tokens/ListTokens.kt — the line-count rule (three-line when
//              overline *and* supporting, or a multiline supporting text;
//              two-line when either; else one-line), the 8/12 dp vertical
//              padding ladder, the per-state shape morph
//              (`ListItemShapes.shapeForInteraction`: pressed > dragged >
//              selected > focused > hovered > base) animated on the
//              FastSpatial spring, the colour animation on DefaultEffects,
//              the dragged elevation level4, and `segmentedShapes(index,
//              count)` replacing the first/last items' *outer* corner pair
//              with the list's `container.shape`.
//   [spec]     m3.material.io/components/lists/specs — the measurement table:
//              label / leading / trailing elements are centred until the item
//              is 88dp or taller and top-aligned from there; leading *icons*
//              are always top-aligned with 8dp (12dp at 88dp+) top padding;
//              label left padding 16dp; leading element left padding 16dp;
//              trailing element right padding 24dp; 48dp targets; and the
//              expressive shape morph in words ("unselected 4dp inner, 16dp
//              outer; selected 16dp all around").
//
// Divergences recorded in docs/porting-todo.md rather than resolved silently:
//
//   * **The between-space is 12px here, 16px in material-web's SCSS.** The
//     export publishes `between-space: 12px` and Compose uses that row
//     (`ItemBetweenSpace`) as the slot spacing; material-web's
//     `_list-item.scss` hardcodes `gap: 16px` instead. Two sources against
//     one hardcode, so the token wins.
//   * **The trailing element's right padding is the token's 16px, not the
//     spec's 24dp** (`trailing-space: 16px`). The spec number is recorded.
//   * **The divider rows are deprecated in the export** ("use the standalone
//     divider component token md.comp.divider.thickness"). They are carried
//     here for completeness and are *not* rendered by MdList — insert an
//     MdDivider between items instead, which is what the deprecation asks
//     for.
//   * **The container's 8px vertical padding is not a token row** — it is
//     `padding: 8px 0` in `_list.scss`. Carried as a value so a theme can
//     retune it; the export publishes no key to override it through.
//   * **material-web fades the *whole* disabled item** (`opacity: 0.38` on
//     `.list-item.disabled`) where the export publishes per-element opacities
//     (label 0.38, state layer 0.1, and a container colour *only* for the
//     selected family). Compose is a third reading: it has no
//     `selectedDisabled` rows, so its disabled branch takes the plain
//     container and a disabled *selected* item would lose its
//     secondary-container. This port follows the export — a disabled selected
//     item keeps the selected container and composites on-surface at 0.38 over
//     it — which is also where material-web lands.
//   * **material-web hardcodes the list item's focus ring shape to 8px**
//     (`md-focus-ring { shape: 8px }`); here the ring follows the container's
//     own per-corner radii, the way Compose and the spec describe it — a
//     square Standard item therefore gets a square ring.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QtGlobal>

namespace md {

/// The two published list styles. The style is a visual choice and does not
/// change behaviour [spec]:
///
///   Standard    baseline: square corners (corner-none), no shape morph.
///   Expressive  the recommended style: corner-extra-small at rest, morphing
///               through corner-medium (hover) to corner-large
///               (focus / press / select / drag).
enum class MdListVariant {
    Standard,
    Expressive,
    Count,
};

/// The states a list item's colour table is published for.
enum class MdListState {
    Enabled,
    Hovered,
    Focused,
    Pressed,
    Dragged,
    Disabled,
    Count,
};

/// One interaction state of one selection family: the state layer and every
/// content colour the export publishes for that state.
///
/// Rows the export does not publish for a state keep the *enabled* value —
/// that is what the export means, not an omission: e.g. there is no
/// `hover.overline.color`, so hovering does not change an overline.
struct QT_MD3_EXPORT MdListStateRow
{
    /// `state-layer.color` for this state.
    ColorRole stateLayer = ColorRole::OnSurface;
    /// `state-layer.opacity` — 0 for the enabled row, which has no layer.
    qreal stateLayerOpacity = 0.0;
    ColorRole labelText = ColorRole::OnSurface;
    ColorRole leadingIcon = ColorRole::OnSurfaceVariant;
    ColorRole trailingIcon = ColorRole::OnSurfaceVariant;
    ColorRole overline = ColorRole::OnSurfaceVariant;
    ColorRole supportingText = ColorRole::OnSurfaceVariant;
    ColorRole trailingSupportingText = ColorRole::OnSurfaceVariant;
};

/// The disabled rows, which publish per-element opacities rather than plain
/// colours: Compose composites the disabled colour over the enabled one.
struct QT_MD3_EXPORT MdListDisabledRow
{
    ColorRole container = ColorRole::OnSurface;
    /// `container.opacity` — the selected family publishes 0.38; the
    /// unselected family publishes no container row at all, so its container
    /// is left untouched (opacity 0 = "no override").
    qreal containerOpacity = 0.0;
    ColorRole labelText = ColorRole::OnSurface;
    ColorRole leadingIcon = ColorRole::OnSurface;
    ColorRole trailingIcon = ColorRole::OnSurface;
    ColorRole overline = ColorRole::OnSurface;
    ColorRole supportingText = ColorRole::OnSurface;
    ColorRole trailingSupportingText = ColorRole::OnSurface;
    ColorRole stateLayer = ColorRole::OnSurface;
    qreal labelTextOpacity = 0.38;
    qreal leadingIconOpacity = 0.38;
    qreal trailingIconOpacity = 0.38;
    qreal overlineOpacity = 0.38;
    qreal supportingTextOpacity = 0.38;
    qreal trailingSupportingTextOpacity = 0.38;
    qreal stateLayerOpacity = 0.1;
};

/// The five *flat* states of one selection family (unselected or selected).
/// The disabled state is published with per-element opacities instead and
/// lives in MdListDisabledRow.
struct QT_MD3_EXPORT MdListFamily
{
    /// The resting container colour — `container.color`.
    ColorRole container = ColorRole::Surface;
    MdListStateRow enabled;
    MdListStateRow hovered;
    MdListStateRow focused;
    MdListStateRow pressed;
    MdListStateRow dragged;

    /// The row for a flat state. `MdListState::Disabled` is not answerable
    /// here — ask MdListTokens::disabledRow() instead.
    const MdListStateRow &state(MdListState which) const;
};

/// The list container itself, `md.comp.list.*` outside `list-item.*`.
struct QT_MD3_EXPORT MdListContainerTokens
{
    /// `container.color`.
    ColorRole containerColor = ColorRole::Surface;
    /// `container.shape` — also the `segmentedShapes` outer-corner override.
    ShapeCorner shape = ShapeCorner::Large;

    /// `_list.scss` `padding: 8px 0` — carried, not a published row.
    qreal topPadding = 8.0;
    qreal bottomPadding = 8.0;

    /// `segmented.gap` — the space between segmented items.
    qreal segmentedGap = 2.0;

    // --- the deprecated divider rows (carried, not rendered) ---------------
    ColorRole dividerColor = ColorRole::Outline;
    qreal dividerHeight = 1.0;
    qreal dividerTopSpace = 0.0;
    qreal dividerBottomSpace = 0.0;
    qreal dividerLeadingSpace = 16.0;
    qreal dividerTrailingSpace = 16.0;

    // The `focus.indicator.*` rows live in MdListTokens, not here: the item
    // paints the ring, so the item's set is where they are consumed.

    static MdListContainerTokens resolve(const MdComponentTokens *overrides = nullptr);
};

/// Everything needed to paint and lay out one list item.
struct QT_MD3_EXPORT MdListTokens
{
    // --- container metrics --------------------------------------------------
    /// `list-item.one-line | two-line | three-line.container.height`.
    qreal oneLineHeight = 56.0;
    qreal twoLineHeight = 72.0;
    qreal threeLineHeight = 88.0;
    /// `list-item.top-space` / `bottom-space`.
    qreal topSpace = 10.0;
    qreal bottomSpace = 10.0;
    /// `list-item.leading-space` / `trailing-space`.
    qreal leadingSpace = 16.0;
    qreal trailingSpace = 16.0;
    /// `list-item.between-space` — the slot gap (see the header note).
    qreal betweenSpace = 12.0;

    // --- slots ---------------------------------------------------------------
    /// `list-item.leading-icon.size` / `leading-icon.expressive.size`.
    qreal leadingIconSize = 24.0;
    qreal leadingIconExpressiveSize = 20.0;
    /// `list-item.trailing-icon.size` / `trailing-icon.expressive.size`.
    qreal trailingIconSize = 24.0;
    qreal trailingIconExpressiveSize = 20.0;
    /// `list-item.leading-avatar.size` and its shape / colours / label type.
    qreal leadingAvatarSize = 40.0;
    ShapeCorner leadingAvatarShape = ShapeCorner::Full;
    ColorRole leadingAvatarColor = ColorRole::PrimaryContainer;
    ColorRole leadingAvatarLabelColor = ColorRole::OnPrimaryContainer;
    TypeStyle leadingAvatarLabelType = TypeStyle::TitleMedium;
    /// `list-item.leading-image.width` / `.height` and its shapes.
    qreal leadingImageWidth = 56.0;
    qreal leadingImageHeight = 56.0;
    ShapeCorner leadingImageShape = ShapeCorner::None;
    ShapeCorner leadingImageExpressiveShape = ShapeCorner::Small;
    /// `list-item.leading-video.width` / the small and large rows.
    qreal leadingVideoWidth = 100.0;
    qreal leadingVideoHeight = 56.0;
    qreal smallLeadingVideoWidth = 100.0;
    qreal smallLeadingVideoHeight = 56.0;
    qreal largeLeadingVideoWidth = 114.0;
    qreal largeLeadingVideoHeight = 64.0;
    ShapeCorner leadingVideoShape = ShapeCorner::Small;

    // --- typography ----------------------------------------------------------
    TypeStyle labelTextType = TypeStyle::BodyLarge;
    TypeStyle overlineType = TypeStyle::LabelSmall;
    TypeStyle supportingTextType = TypeStyle::BodyMedium;
    TypeStyle trailingSupportingTextType = TypeStyle::LabelSmall;

    // --- alignment [spec] ----------------------------------------------------
    /// The height from which content is top-aligned instead of centred.
    qreal verticalAlignmentBreakpoint = 88.0;
    /// Leading-icon top padding, and its value at the breakpoint and above.
    qreal leadingIconTopPadding = 8.0;
    qreal leadingIconTopPaddingTall = 12.0;
    /// Targets — the accessibility minimum [spec].
    qreal targetSize = 48.0;

    // --- shapes --------------------------------------------------------------
    /// `container.shape` — the baseline outline (corner-none).
    ShapeCorner containerShape = ShapeCorner::None;
    /// `container.expressive.shape` and its per-state rows.
    ShapeCorner expressiveShape = ShapeCorner::ExtraSmall;
    ShapeCorner hoveredShape = ShapeCorner::Medium;
    ShapeCorner focusedShape = ShapeCorner::Large;
    ShapeCorner pressedShape = ShapeCorner::Large;
    ShapeCorner selectedShape = ShapeCorner::Large;
    ShapeCorner draggedShape = ShapeCorner::Large;
    /// `disabled.container.expressive.shape` — the unselected disabled row.
    ShapeCorner disabledShape = ShapeCorner::ExtraSmall;
    /// `selected.disabled.container.expressive.shape` — a disabled *selected*
    /// item keeps corner-large; the plain disabled row would drop it to
    /// extra-small, which is the unselected case only.
    ShapeCorner selectedDisabledShape = ShapeCorner::Large;

    // --- focus indicator -----------------------------------------------------
    /// `md.comp.list.focus.indicator.*` — borrowed from the list namespace for
    /// the same reason `listShape` is: the *item* paints this ring, so the
    /// rows belong where they are consumed.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    /// `focus.indicator.outline.offset` — a CSS `outline-offset` of -3px.
    qreal focusIndicatorOutlineOffset = -3.0;
    /// `focus.indicator.thickness` — md.sys.state.focus-indicator.thickness.
    qreal focusIndicatorThickness = 3.0;

    /// The ring's own inward gap, derived from the two rows above rather than
    /// hard-coded: a CSS `outline-offset` of -3px with a 3px outline puts the
    /// ring flush against the container edge, extending inward — which is the
    /// inward variant's zero gap plus the token thickness.
    qreal focusRingGap() const
    {
        return qMax(0.0, -focusIndicatorOutlineOffset - focusIndicatorThickness);
    }

    // --- elevation -----------------------------------------------------------
    /// `container.elevation` / `dragged.container.elevation`.
    ///
    /// Carried, not painted. A list item's container *is* its whole widget
    /// rect, so a shadow drawn around it would be clipped away by Qt, and a
    /// parent-side shadow would be covered by the neighbouring items'
    /// containers. Compose paints a dragged item in an overlay above the list;
    /// that overlay host does not exist here yet, so the dragged state is
    /// expressed through its shape row and its state layer instead — the
    /// limitation is recorded in docs/porting-todo.md.
    ElevationLevel containerElevation = ElevationLevel::Level0;
    ElevationLevel draggedElevation = ElevationLevel::Level4;

    // --- colours -------------------------------------------------------------
    /// The unselected family, and the selected one
    /// (`selected.*` rows plus `selected.container.color`).
    MdListFamily family;
    MdListFamily selectedFamily;
    MdListDisabledRow disabled;
    MdListDisabledRow selectedDisabled;

    /// The list's own `container.shape` — the segmented outer-corner
    /// override [compose] `segmentedShapes`.
    ShapeCorner listShape = ShapeCorner::Large;

    /// The disabled row for a selection mode.
    const MdListDisabledRow &disabledRow(bool selected) const
    {
        return selected ? selectedDisabled : disabled;
    }

    /// Build the published set for one style. `variant` selects the shape
    /// table (Expressive morphs, Standard stays square).
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace; the
    /// overridable rows are the metrics and the expressive shapes (colours
    /// are not overridable — the same rule as the other families).
    static MdListTokens resolve(MdListVariant variant, const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_LIST_TOKENS_H
