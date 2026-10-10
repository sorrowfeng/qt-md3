#ifndef MD_MENU_TOKENS_H
#define MD_MENU_TOKENS_H

// MdMenuTokens — `md.comp.menu.*` at export version 34.0.21.
//
// ## What the export publishes
//
// The menu surface (`container.*`: `surface-container`, elevation level 2,
// `corner-extra-small`) and its list items (`list-item.*`: 48 px tall,
// `on-surface` label-large text, `on-surface-variant` 24 px icons, the
// `secondary-container` selected side, 1 px `surface-variant` dividers and the
// 24 px cascading-menu indicator). The item's focus indicator rows point at
// the **inner** offset — the item ring draws inward, unlike the outward rings
// of the button families.
//
// ## Compose's behaviour this widget ports
//
//   * **the menu opens on a scale+alpha pair** — `DropdownMenuContent`
//     animates scale 0.8 ↔ 1.0 on the fast spatial spring and alpha 0 ↔ 1 on
//     fast effects, around the anchor corner;
//   * **the classic item geometry** — 48 px tall, horizontal padding 12,
//     min-width 112 / max-width 280, icon-text spacing 8, and the menu's
//     vertical padding 8;
//   * **no colour animation on the item** (the Expressive item animates its
//     container colour on fast effects; the export's classic rows resolve by
//     state — the classic wins, as everywhere);
//   * **the selected leading icon swap is a presence animation** (expand/
//     shrink horizontally + fade) — the Qt port swaps instantly; recorded.
//
// ## Divergences pinned here rather than smoothed
//
// * **The Expressive menu families are separate token sets** (`StandardMenu`,
//   `VibrantMenu`, `SegmentedMenu` — grouped menus with per-position item
//   shapes). Only the classic `md.comp.menu.*` export is ported; the Expressive
//   sets are recorded in porting-todo.
// * **Disabled + selected has no rows.** The export publishes the selected
//   container/label/icon colours for the enabled interactions only; a disabled
//   selected item falls back to the unselected disabled rows (its container
//   goes transparent). Recorded, not smoothed.
// * **The label-text type rows ride `label-large`** — resolved through the
//   shared type scale, carried as the item's type style.

#include "MdNavigationBarTokens.h" // MdNavigationColourSlot
#include "MdTokens.h"
#include "MdTypes.h"
#include "core/QtMd3Export.h"

namespace md {

/// Which side of the item colour tables a paint reads.
enum class MdMenuSelection {
    Unselected,
    Selected,
    Count,
};

constexpr int menuSelectionCount = int(MdMenuSelection::Count);

/// One resolved menu. The item colour tables are indexed
/// `[selection][interaction]`.
struct QT_MD3_EXPORT MdMenuTokens
{
    // --- the surface ------------------------------------------------------------
    /// `container.color: surface-container`.
    ColorRole containerColor = ColorRole::SurfaceContainer;
    /// `container.elevation: level2` — the shadow under the menu.
    ElevationLevel containerElevation = ElevationLevel::Level2;
    ColorRole containerShadowColor = ColorRole::Shadow;
    /// `container.shape: corner-extra-small` — through `MdShape::radius`.
    ShapeCorner containerShape = ShapeCorner::ExtraSmall;
    /// Compose's `DropdownMenuVerticalPadding` (8 px).
    qreal containerVerticalPadding = 8.0;
    /// `container.surface-tint-layer.color` — carried for the record; the
    /// paint draws the shadow straight from the elevation row.
    ColorRole containerSurfaceTint = ColorRole::SurfaceTint;

    // --- the items ---------------------------------------------------------------
    /// `list-item.container.height: 48px`.
    qreal itemHeight = 48.0;
    /// Compose's `DropdownMenuItemHorizontalPadding` (12 px).
    qreal itemHorizontalPadding = 12.0;
    /// Compose's `DropdownMenuItemDefaultMinWidth` / `MaxWidth`.
    qreal itemMinWidth = 112.0;
    qreal itemMaxWidth = 280.0;
    /// Compose's `DropdownMenuIconTextPadding` (8 px).
    qreal itemIconTextSpacing = 8.0;
    /// The 24 px leading / trailing icon slots.
    qreal iconSize = 24.0;
    /// The label's type (`label-text.type: label-large`).
    TypeStyle labelTextType = TypeStyle::LabelLarge;
    /// `list-item.disabled.label-text.opacity` (0.38).
    qreal disabledLabelTextOpacity = 0.38;
    /// `list-item.with-leading-icon.disabled.leading-icon.opacity` (0.38).
    qreal disabledIconOpacity = 0.38;

    // --- colour tables, indexed [selection][interaction] ---------------------------
    /// The label text. `on-surface` in every enabled interaction; disabled
    /// `on-surface` at 0.38. The selected side reads `on-secondary-container`.
    MdNavigationColourSlot label[menuSelectionCount][navItemStateCount];
    /// The leading / trailing icon slots. Unselected `on-surface-variant` (no
    /// interaction lift — the export's hover/focus/pressed icon rows carry the
    /// same colour); disabled `on-surface` at 0.38. Selected
    /// `on-secondary-container`.
    MdNavigationColourSlot icon[menuSelectionCount][navItemStateCount];
    /// The state layer. `on-surface` under hover/focus/press; no enabled row.
    MdNavigationColourSlot stateLayer[menuSelectionCount][navItemStateCount];
    /// The selected item's container (`secondary-container`). The unselected
    /// side has no rows (a transparent item over the menu surface); the
    /// selected side covers the enabled interactions — disabled falls back to
    /// the unselected disabled rows.
    MdNavigationColourSlot itemContainer[menuSelectionCount][navItemStateCount];

    // --- the divider -----------------------------------------------------------------
    /// `divider.height: 1px`, `divider.color: surface-variant`. Compose's
    /// `HorizontalDividerPadding` (12 px horizontal, 2 px vertical).
    qreal dividerHeight = 1.0;
    ColorRole dividerColor = ColorRole::SurfaceVariant;
    qreal dividerHorizontalPadding = 12.0;
    qreal dividerVerticalPadding = 2.0;

    // --- the cascading-menu indicator ------------------------------------------------
    /// The trailing arrow of a submenu item (`cascading-menu-indicator.*`).
    ColorRole cascadingIndicatorColor = ColorRole::OnSurfaceVariant;

    // --- focus indicator ----------------------------------------------------------------
    /// Around the item — **inward** (`focus.indicator.outline.offset` is the
    /// system *inner* offset), secondary, the system thickness.
    ColorRole focusIndicatorColor = ColorRole::Secondary;
    qreal focusIndicatorInnerOffset = 3.0;
    qreal focusIndicatorThickness = 3.0;

    // --- system state-layer opacities ------------------------------------------------------
    qreal hoverStateLayerOpacity = 0.08;
    qreal focusStateLayerOpacity = 0.12;
    qreal pressedStateLayerOpacity = 0.12;

    // --- Compose behaviour constants (not export rows) ---------------------------------------
    /// The open/close animation: scale 0.8 ↔ 1.0 (fast spatial) and alpha
    /// 0 ↔ 1 (fast effects), around the anchor corner.
    qreal closedScale = 0.8;
    qreal closedAlpha = 0.0;
    /// Compose's `MenuHorizontalMargin` — the gap between the anchor and the
    /// menu surface.
    qreal menuHorizontalMargin = 8.0;

    // --- accessors ---------------------------------------------------------------------------
    const MdNavigationColourSlot &labelFor(MdMenuSelection selection,
                                           MdNavigationItemState state) const;
    const MdNavigationColourSlot &iconFor(MdMenuSelection selection,
                                          MdNavigationItemState state) const;
    const MdNavigationColourSlot &stateLayerFor(MdMenuSelection selection,
                                                MdNavigationItemState state) const;
    const MdNavigationColourSlot &itemContainerFor(MdMenuSelection selection,
                                                   MdNavigationItemState state) const;

    static MdMenuTokens resolve(const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_MENU_TOKENS_H
