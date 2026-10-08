#ifndef MD_FAB_MENU_TOKENS_H
#define MD_FAB_MENU_TOKENS_H

// The published `md.comp.fab-menu.*` token set, resolved for one colour
// group.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-fab-menu.scss                    (common)
//     tokens/versions/latest/sass/_md-comp-fab-menu-<group>-container.scss  (list items)
//     tokens/versions/latest/sass/_md-comp-fab-menu-<group>-close-button.scss
//     tokens/versions/latest/sass/_md-sys-state*.scss                       (opacities)
//
// Token export version 34.0.21.
//
// Facts of the export worth keeping visible (also recorded in
// docs/porting-todo.md):
//
//   * **No motion rows.** The export publishes no durations, easings or
//     stagger values for the open/close animation. The spec page says only
//     "the FAB menu animates from the top trailing edge of the FAB". The
//     reveal here uses the Compose M3 Expressive `SpatialDefault` spring
//     with a 40 ms per-item stagger — recorded as a sourced-from-Compose
//     convention, not a token fact.
//   * **No disabled rows** — as with every family since the buttons, the
//     disabled values (on-surface @12% container, on-surface @38% icon and
//     label, level0) come from the official spec page's disabled state
//     table.
//   * **No focus-indicator rows of its own** — the close button and list
//     items carry focused state-layer colours but no
//     `focus.indicator.*` rows, so the indicator spec falls back to the
//     shared `md-sys-state-focus-indicator` values (secondary, 3 px,
//     offset 2) every other family uses.
//   * The colour rows are *flat* — hovered/focused/pressed restate the
//     enabled roles in every set.
//   * The spec page adds two structural facts the export cannot express:
//     the menu has **up to six items**, and on the web the component
//     "inherits its states and specs from the baseline menu", with a
//     FAB-to-menu gap of **4 dp recommended**.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// The two element shapes of a FAB menu, each with its own published token
/// set: the 56 px close button and the 56 px list items.
enum class FabMenuElement {
    CloseButton,
    ListItem,
    Count,
};

/// One element's resolved rows. Colours are flat (one interactive row +
/// disabled); elevation is where the states differ.
struct QT_MD3_EXPORT MdFabMenuElementTokens
{
    // --- metrics (md.comp.fab-menu.* / md.comp.fab-menu.<group>) -----------
    qreal containerHeight = 56.0;
    /// 0 means content-derived (the list item: leading + icon + gap + label
    /// + trailing); the close button publishes a square 56 px.
    qreal containerWidth = 0.0;
    qreal iconSize = 24.0;
    qreal iconLabelSpace = 8.0;
    qreal leadingSpace = 0.0;
    qreal trailingSpace = 0.0;

    // --- shape (md.comp.fab-menu.*.container.shape) -------------------------
    /// Both elements publish corner-full.
    ShapeCorner containerShape = ShapeCorner::Full;

    // --- label text (md.comp.fab-menu.menu-item label type scale) -----------
    /// The common set applies `md.sys.typescale.title-medium` to the item
    /// label; the close button has no label.
    TypeStyle labelStyle = TypeStyle::TitleMedium;

    // --- colours (flat interactive row) -------------------------------------
    /// `container.color`; `ColorRole::Count` means absent.
    ColorRole container = ColorRole::Count;
    /// `icon.color` == `label-text.color` in every set.
    ColorRole content = ColorRole::Count;
    /// `hovered.state-layer.color` (== focused == pressed).
    ColorRole stateLayer = ColorRole::Count;

    // --- elevation per state ------------------------------------------------
    /// The close button rests at level3 and climbs to level4 on hover; the
    /// list items stay at level0 in every state.
    ElevationLevel enabledElevation = ElevationLevel::Level0;
    ElevationLevel hoveredElevation = ElevationLevel::Level0;
    ElevationLevel focusedElevation = ElevationLevel::Level0;
    ElevationLevel pressedElevation = ElevationLevel::Level0;
    ElevationLevel disabledElevation = ElevationLevel::Level0;
};

/// Everything needed to paint and lay out one FAB menu.
struct QT_MD3_EXPORT MdFabMenuTokens
{
    // --- spacing (md.comp.fab-menu.*) ---------------------------------------
    /// `close-button.between-space`: close button to first item.
    qreal closeButtonBetweenSpace = 8.0;
    /// `menu-item.between-space`: between neighbouring items.
    qreal menuItemBetweenSpace = 4.0;

    // --- focus indicator (md-sys-state-focus-indicator, the shared fallback)
    ColorRole focusIndicator = ColorRole::Secondary;
    qreal focusIndicatorThickness = 3.0;
    qreal focusIndicatorOffset = 2.0;

    // --- elements ------------------------------------------------------------
    MdFabMenuElementTokens closeButton;
    MdFabMenuElementTokens listItem;

    /// Build the published set for one colour group.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace with
    /// element-qualified keys winning:
    ///
    ///   md.comp.fab-menu.close-button.container.height   (wins)
    ///   md.comp.fab-menu.container.height                (fallback)
    ///
    /// Colours are not overridable — same rule as the other families.
    static MdFabMenuTokens resolve(FabMenuVariant variant,
                                   const MdComponentTokens *overrides = nullptr);
};

/// The spec-page disabled row's opacities, shared by both elements (the
/// export publishes no disabled rows). Free functions so the transcription
/// has exactly one home.
QT_MD3_EXPORT qreal fabMenuDisabledContainerOpacity();
QT_MD3_EXPORT qreal fabMenuDisabledContentOpacity();

} // namespace md

#endif // MD_FAB_MENU_TOKENS_H
