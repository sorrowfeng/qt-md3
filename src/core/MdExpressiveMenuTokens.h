#ifndef MD_EXPRESSIVE_MENU_TOKENS_H
#define MD_EXPRESSIVE_MENU_TOKENS_H

// MdExpressiveMenuTokens — `md.comp.menus.*` at export version 34.0.21.
//
// ## What the export publishes
//
// Three files, not one. `_md-comp-menus.scss` carries the *shared* Expressive
// geometry (44 px items, 2 px gap, 2 px group padding, corner-large container
// and active item, per-position item shapes, 20 px icons, label-large /
// body-small type). `_md-comp-menus-standard.scss` and
// `_md-comp-menus-vibrant.scss` carry the two colour families:
//
//   * **StandardMenu** — `surface-container-low` base with `on-surface`
//     content and a `tertiary-container` selected row;
//   * **VibrantMenu** — `tertiary-container` base with `on-tertiary-container`
//     content and a `tertiary` selected row.
//
// The latest sass export publishes **StandardMenu + VibrantMenu only** — there
// is no `_md-comp-menus-segmented.scss`. The third family is not invented here.
//
// ## Why this is a sibling of `MdMenuTokens`, not a mode of it
//
// `md.comp.menu.*` (classic) and `md.comp.menus.*` (Expressive) are disjoint
// namespaces with different geometry (48 vs 44 px items, corner-extra-small vs
// corner-large surfaces) and different colour families. The classic export is
// already ported through `MdMenuTokens`; this struct is the Expressive set.
//
// ## Divergences pinned here rather than smoothed
//
// * **SegmentedMenu is absent from the export** — recorded, not invented.
// * **The horizontal / horizontal-icon-only layouts** are carried as their own
//   `horizontal.*` / `horizontal.icon-only.*` rows and resolved by
//   `layoutMetricsFor()` / `horizontalItemShapeFor()` rather than mixed into
//   the vertical fields. Two gaps in that family are recorded in
//   porting-todo: the export publishes no `horizontal.menu-item.height` (the
//   item's cross extent is derived from its content + the published top/bottom
//   spaces) and no resting shape row on the text side (only the interaction
//   rows and the icon-only resting selected row exist).
// * **`menu-item.first/last-child.inner-corner.corner-size` is corner-extra-
//   small** — the Expressive group's inner corners. Carried as
//   `innerCornerSize` and read by the group-shape resolver.
// * **`container-surface-tint` is carried, not drawn** — the elevation paints
//   through `MdElevation`, the project's standing rule for tonal surfaces.
// * **The icon-button sub-slots** (the trailing action inside a menu item) are
//   carried as colour rows; the menu item's own trailing icon uses the item's
//   trailing-icon rows, not the icon-button family.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which Expressive colour family the menu resolves.
enum class MdExpressiveMenuVariant {
    /// `md.comp.menus.standard.*` — surface-container-low base.
    Standard,
    /// `md.comp.menus.vibrant.*` — tertiary-container base.
    Vibrant,
    Count,
};

constexpr int expressiveMenuVariantCount = int(MdExpressiveMenuVariant::Count);

/// Which side of the item colour tables a paint reads.
enum class MdExpressiveMenuSelection {
    Unselected,
    Selected,
    Count,
};

constexpr int expressiveMenuSelectionCount = int(MdExpressiveMenuSelection::Count);

/// Which axis `MdExpressiveMenu` arranges its items along.
enum class MdExpressiveMenuOrientation {
    /// The stacked column — `menu-item.height`, `gap`, `group.padding`.
    Vertical,
    /// The horizontal bar — `horizontal.*` rows; with `iconOnly` the
    /// `horizontal.icon-only.*` ones instead.
    Horizontal,
    Count,
};

constexpr int expressiveMenuOrientationCount = int(MdExpressiveMenuOrientation::Count);

/// Effective row / container geometry for one layout orientation.
///
/// `Vertical` reads `menu-item.*`, `gap` and `group.padding`. `Horizontal`
/// reads `horizontal.*` (`horizontal.icon-only.*` when `iconOnly`). Every
/// number is a published row; nothing here is estimated.
struct QT_MD3_EXPORT MdExpressiveMenuLayoutMetrics
{
    /// Item-internal padding.
    qreal itemLeadingSpace = 16.0;
    qreal itemTrailingSpace = 16.0;
    qreal itemTopSpace = 8.0;
    qreal itemBottomSpace = 8.0;
    /// Icon → label gap inside the item (`menu-item.between-space`).
    qreal itemIconTextSpacing = 12.0;
    /// Gap between adjacent rows (Vertical `gap`) or items (Horizontal
    /// `horizontal.menu-item.between-space` / `horizontal.icon-only.gap`).
    qreal itemGap = 2.0;
    /// Container inset around the group. Vertical is `group.padding` on all
    /// four sides; Horizontal is `horizontal.container.top/bottom-space` on
    /// the cross axis and **0** on the main axis — the export publishes no
    /// leading/trailing container row, and 0 is what nests the first/last
    /// item's corner-full cap exactly inside the container's cap.
    qreal containerLeadingSpace = 2.0;
    qreal containerTrailingSpace = 2.0;
    qreal containerTopSpace = 2.0;
    qreal containerBottomSpace = 2.0;
    /// The menu surface's corner (`container.shape` / `horizontal.container.shape`).
    ShapeCorner containerShape = ShapeCorner::Large;
};

/// One resolved Expressive menu. Colour tables are indexed
/// `[selection][interaction]`.
struct QT_MD3_EXPORT MdExpressiveMenuTokens
{
    // --- shared geometry (`_md-comp-menus.scss`) ------------------------------------
    /// `container.shape: corner-large` (16 px) and `active.container.shape` —
    /// the pressed item morphs to the same corner.
    ShapeCorner containerShape = ShapeCorner::Large;
    ShapeCorner activeContainerShape = ShapeCorner::Large;
    /// `container.elevation: level2`.
    ElevationLevel containerElevation = ElevationLevel::Level2;
    /// `gap: 2px` between items; `group.padding: 2px` around the group.
    qreal itemGap = 2.0;
    qreal groupPadding = 2.0;
    /// `menu-item.height: 44px`.
    qreal itemHeight = 44.0;
    /// `menu-item.leading-icon.size` / `trailing-icon.size`: 20 px.
    qreal leadingIconSize = 20.0;
    qreal trailingIconSize = 20.0;
    /// `menu-item.between-space: 12px` (icon ↔ text).
    qreal itemIconTextSpacing = 12.0;
    /// `menu-item.leading-space: 16px`, `trailing-space: 16px`.
    qreal itemLeadingSpace = 16.0;
    qreal itemTrailingSpace = 16.0;
    /// `menu-item.top-space: 8px`, `bottom-space: 8px`.
    qreal itemTopSpace = 8.0;
    qreal itemBottomSpace = 8.0;

    // --- horizontal layout (`_md-comp-menus.scss` `horizontal.*`) ----------------------
    /// `horizontal.container.shape: corner-full` — the horizontal surface is
    /// a pill, not the vertical corner-large box.
    ShapeCorner horizontalContainerShape = ShapeCorner::Full;
    /// `horizontal.container.top-space` / `bottom-space: 8px`.
    qreal horizontalContainerTopSpace = 8.0;
    qreal horizontalContainerBottomSpace = 8.0;
    /// `horizontal.menu-item.between-space: 12px` — the gap between items.
    qreal horizontalItemBetweenSpace = 12.0;
    /// `horizontal.menu-item.leading-space` / `trailing-space: 12px`.
    qreal horizontalItemLeadingSpace = 12.0;
    qreal horizontalItemTrailingSpace = 12.0;
    /// `horizontal.menu-item.top-space` / `bottom-space: 6px`.
    qreal horizontalItemTopSpace = 6.0;
    qreal horizontalItemBottomSpace = 6.0;

    // --- horizontal icon-only (`horizontal.icon-only.*`) ---------------------------------
    /// `horizontal.icon-only.gap: 4px` — the gap between icon-only items.
    qreal horizontalIconOnlyGap = 4.0;
    /// `horizontal.icon-only.menu-item.leading/trailing/top/bottom-space: 16px`.
    qreal horizontalIconOnlyItemLeadingSpace = 16.0;
    qreal horizontalIconOnlyItemTrailingSpace = 16.0;
    qreal horizontalIconOnlyItemTopSpace = 16.0;
    qreal horizontalIconOnlyItemBottomSpace = 16.0;

    // --- horizontal item shapes ---------------------------------------------------------
    /// `horizontal.menu-item.hovered/focused/pressed.shape: corner-medium`.
    ShapeCorner horizontalItemHoveredShape = ShapeCorner::Medium;
    ShapeCorner horizontalItemFocusedShape = ShapeCorner::Medium;
    ShapeCorner horizontalItemPressedShape = ShapeCorner::Medium;
    /// `horizontal.menu-item.selected.hovered/focused/pressed.shape: corner-full`.
    ShapeCorner horizontalItemSelectedHoveredShape = ShapeCorner::Full;
    ShapeCorner horizontalItemSelectedFocusedShape = ShapeCorner::Full;
    ShapeCorner horizontalItemSelectedPressedShape = ShapeCorner::Full;
    /// `horizontal.icon-only.menu-item.selected.shape: corner-full` — the
    /// family's only resting selected shape row.
    ShapeCorner horizontalIconOnlySelectedShape = ShapeCorner::Full;

    // --- per-position item shapes ----------------------------------------------------
    /// `menu-item.shape: corner-extra-small` (the neutral middle row).
    ShapeCorner itemShape = ShapeCorner::ExtraSmall;
    /// `menu-item.selected.shape: corner-medium`.
    ShapeCorner itemSelectedShape = ShapeCorner::Medium;
    /// `menu-item.first-child.shape` / `last-child.shape: corner-medium`.
    ShapeCorner itemFirstChildShape = ShapeCorner::Medium;
    ShapeCorner itemLastChildShape = ShapeCorner::Medium;
    /// `menu-item.first/last-child.inner-corner.corner-size: corner-extra-small`.
    ShapeCorner innerCornerSize = ShapeCorner::ExtraSmall;

    // --- type -------------------------------------------------------------------------
    /// `menu-item.label-text.*: label-large`.
    TypeStyle labelTextType = TypeStyle::LabelLarge;
    /// `menu-item.supporting-text.*: body-small`.
    TypeStyle supportingTextType = TypeStyle::BodySmall;
    /// `menu-item.trailing-supporting-text.*: label-large`.
    TypeStyle trailingSupportingTextType = TypeStyle::LabelLarge;

    // --- focus indicator --------------------------------------------------------------
    /// `menu-item.focus-indicator.color: secondary`, the system *inner* offset
    /// and thickness.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorInnerOffset = 3.0;
    qreal focusIndicatorThickness = 3.0;

    // --- variant container -------------------------------------------------------------
    /// Standard: `container-color: surface-container-low`. Vibrant:
    /// `container-color: tertiary-container`. Set by `resolve()`.
    ColorRole containerColor = ColorRole::SurfaceContainerLow;
    /// Standard: `container-shadow-color: shadow`. Vibrant publishes no row;
    /// the port keeps the system shadow colour.
    ColorRole containerShadowColor = ColorRole::Shadow;

    // --- colour tables, indexed [selection][interaction] --------------------------------
    /// The label text. Standard unselected `on-surface`; Vibrant unselected
    /// `on-tertiary-container`. Selected rows are the variant's selected
    /// content colour.
    MdNavigationColourSlot label[expressiveMenuSelectionCount][navItemStateCount];
    /// Leading / trailing icons. Standard unselected `on-surface-variant`
    /// (no interaction lift); Vibrant unselected lifts to `tertiary` under
    /// hover / focus / press.
    MdNavigationColourSlot icon[expressiveMenuSelectionCount][navItemStateCount];
    /// Supporting text (body-small). Standard `on-surface-variant`; Vibrant
    /// `on-tertiary-container`.
    MdNavigationColourSlot supportingText[expressiveMenuSelectionCount][navItemStateCount];
    /// Trailing supporting text (label-large).
    MdNavigationColourSlot trailingSupportingText[expressiveMenuSelectionCount][navItemStateCount];
    /// The state layer. Standard `on-surface`; Vibrant `on-tertiary-container`
    /// unselected / `on-tertiary` selected.
    MdNavigationColourSlot stateLayer[expressiveMenuSelectionCount][navItemStateCount];
    /// The item container. Unselected rows are the variant base (the item
    /// sits on the menu surface and is usually transparent — the export's
    /// `menu-item.container-color` is the surface colour). Selected rows are
    /// Standard `tertiary-container` / Vibrant `tertiary`.
    MdNavigationColourSlot itemContainer[expressiveMenuSelectionCount][navItemStateCount];

    // --- disabled content opacities (both variants publish 0.38) -------------------------
    qreal disabledLabelTextOpacity = 0.38;
    qreal disabledIconOpacity = 0.38;
    qreal disabledSupportingTextOpacity = 0.38;
    qreal disabledTrailingSupportingTextOpacity = 0.38;

    // --- the icon-button sub-slots (carried; the item's trailing icon uses
    //     the item rows above) ------------------------------------------------------------
    ColorRole iconButtonContainerColor = ColorRole::SurfaceContainerLow;
    ColorRole iconButtonIconColor = ColorRole::OnSurfaceVariant;
    ColorRole iconButtonStateLayerColor = ColorRole::OnSurface;
    ColorRole iconButtonSelectedContainerColor = ColorRole::TertiaryContainer;
    ColorRole iconButtonSelectedIconColor = ColorRole::OnTertiaryContainer;

    // --- section label -------------------------------------------------------------------
    /// `section-label-text.color`. Standard `on-surface-variant`; Vibrant
    /// `on-tertiary-container`.
    ColorRole sectionLabelColor = ColorRole::OnSurfaceVariant;

    // --- system state-layer opacities -------------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- accessors ---------------------------------------------------------------------------
    const MdNavigationColourSlot &labelFor(MdExpressiveMenuSelection selection,
                                           MdNavigationItemState state) const;
    const MdNavigationColourSlot &iconFor(MdExpressiveMenuSelection selection,
                                          MdNavigationItemState state) const;
    const MdNavigationColourSlot &supportingTextFor(MdExpressiveMenuSelection selection,
                                                    MdNavigationItemState state) const;
    const MdNavigationColourSlot &trailingSupportingTextFor(MdExpressiveMenuSelection selection,
                                                            MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(MdExpressiveMenuSelection selection,
                                                MdNavigationItemState state) const;
    const MdNavigationColourSlot &itemContainerFor(MdExpressiveMenuSelection selection,
                                                   MdNavigationItemState state) const;

    /// The item's shape for a slot in a group of `count` rows at `index`.
    /// Selected → `itemSelectedShape`; first / last → the published medium
    /// corners; otherwise the neutral extra-small. `active` (pressed) morphs
    /// to `activeContainerShape`.
    ShapeCorner itemShapeFor(int index, int count, bool selected, bool active) const;

    /// The item's shape in the **horizontal** layout.
    ///
    /// Reads the `horizontal.menu-item.*.shape` interaction rows and the
    /// `horizontal.icon-only.menu-item.selected.shape` resting row. The export
    /// publishes no resting row on the text side; resting falls back to the
    /// family's uniform shape on that side (all three interaction rows are
    /// the same corner). Recorded in porting-todo.
    ShapeCorner horizontalItemShapeFor(MdExpressiveMenuSelection selection,
                                       MdNavigationItemState state, bool iconOnly) const;

    /// The row/container geometry for `orientation` (and the horizontal
    /// icon-only variant). Every value is a published row.
    MdExpressiveMenuLayoutMetrics layoutMetricsFor(MdExpressiveMenuOrientation orientation,
                                                   bool iconOnly = false) const;

    static MdExpressiveMenuTokens resolve(MdExpressiveMenuVariant variant,
                                          const MdComponentTokens *overrides = nullptr);
};

} // namespace md

Q_DECLARE_METATYPE(md::MdExpressiveMenuVariant)
Q_DECLARE_METATYPE(md::MdExpressiveMenuOrientation)

#endif // MD_EXPRESSIVE_MENU_TOKENS_H
