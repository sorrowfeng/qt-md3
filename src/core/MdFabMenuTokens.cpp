#include "MdFabMenuTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

// The `md.comp.*` override parsing itself lives in MdCompTokenParse.h, shared
// with the other component resolvers so a length or a shape name can only be
// parsed one way across the whole library.
using namespace comptoken;

// ---------------------------------------------------------------------------
// md.comp.fab-menu.<group> — colours
//
// Each group is published twice: `*-container` (the list items) and
// `*-close-button` (the pure colour). Both sets' non-deprecated rows restate
// the enabled roles for hovered/focused/pressed, so the interactive row is
// transcribed once per element and the state-layer opacity stays
// md.sys.state's business.
// ---------------------------------------------------------------------------

struct GroupRow
{
    FabMenuVariant variant;

    ColorRole closeButtonContainer; ///< `*-close-button.container.color`
    ColorRole closeButtonContent;   ///< its icon (== state layer) role
    ColorRole itemContainer;        ///< `*-container.container.color`
    ColorRole itemContent;          ///< its icon (== label == state layer) role
};

const GroupRow kGroupRows[] = {
    // md.comp.fab-menu.primary[-container].*
    {FabMenuVariant::Primary, ColorRole::Primary, ColorRole::OnPrimary,
     ColorRole::PrimaryContainer, ColorRole::OnPrimaryContainer},

    // md.comp.fab-menu.secondary[-container].*
    {FabMenuVariant::Secondary, ColorRole::Secondary, ColorRole::OnSecondary,
     ColorRole::SecondaryContainer, ColorRole::OnSecondaryContainer},

    // md.comp.fab-menu.tertiary[-container].*
    {FabMenuVariant::Tertiary, ColorRole::Tertiary, ColorRole::OnTertiary,
     ColorRole::TertiaryContainer, ColorRole::OnTertiaryContainer},
};

const GroupRow &groupRow(FabMenuVariant variant)
{
    for (const GroupRow &row : kGroupRows) {
        if (row.variant == variant) {
            return row;
        }
    }
    return kGroupRows[0]; // Primary, the first published group.
}

// The spec-page disabled state table — the same disabled row every push
// button family shows, filling the export's gap.
constexpr qreal kDisabledContainerOpacity = 0.12;
constexpr qreal kDisabledContentOpacity = 0.38;

MdFabMenuElementTokens applyOverrides(MdFabMenuElementTokens element, FabMenuElement role,
                                      const MdComponentTokens *overrides)
{
    const char *segment = role == FabMenuElement::CloseButton ? "close-button" : "menu-item";

    element.containerHeight = lengthOverride(
        overrides, sizeKeys("fab-menu", segment, "container.height"), element.containerHeight);
    if (element.containerWidth > 0.0) {
        element.containerWidth = lengthOverride(
            overrides, sizeKeys("fab-menu", segment, "container.width"), element.containerWidth);
    }
    element.iconSize = lengthOverride(overrides, sizeKeys("fab-menu", segment, "icon.size"),
                                      element.iconSize);
    if (role == FabMenuElement::ListItem) {
        element.iconLabelSpace = lengthOverride(
            overrides, sizeKeys("fab-menu", segment, "icon.label.space"), element.iconLabelSpace);
        element.leadingSpace = lengthOverride(
            overrides, sizeKeys("fab-menu", segment, "leading.space"), element.leadingSpace);
        element.trailingSpace = lengthOverride(
            overrides, sizeKeys("fab-menu", segment, "trailing.space"), element.trailingSpace);
    }
    element.containerShape = shapeOverride(
        overrides, sizeKeys("fab-menu", segment, "container.shape"), element.containerShape);
    return element;
}

} // namespace

MdFabMenuTokens MdFabMenuTokens::resolve(FabMenuVariant variant,
                                         const MdComponentTokens *overrides)
{
    const GroupRow &group = groupRow(variant);
    MdFabMenuTokens tokens;

    // --- spacing: md.comp.fab-menu.* ----------------------------------------
    tokens.closeButtonBetweenSpace = lengthOverride(
        overrides, plainKeys("fab-menu", "close-button.between-space"), 8.0);
    tokens.menuItemBetweenSpace = lengthOverride(
        overrides, plainKeys("fab-menu", "menu-item.between-space"), 4.0);

    // --- focus indicator: the shared md-sys-state-focus-indicator fallback --
    tokens.focusIndicator = ColorRole::Secondary;
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, plainKeys("fab-menu", "focus.indicator.thickness"), 3.0);
    tokens.focusIndicatorOffset = lengthOverride(
        overrides, plainKeys("fab-menu", "focus.indicator.outline.offset"), 2.0);

    // --- close button: md.comp.fab-menu.<group>.close-button ----------------
    // 56 × 56, icon 20, corner-full; rests at level3, hovers at level4
    // (focused/pressed level3), exactly the raised-FAB ladder.
    MdFabMenuElementTokens close;
    close.containerHeight = 56.0;
    close.containerWidth = 56.0;
    close.iconSize = 20.0;
    close.containerShape = ShapeCorner::Full;
    close.container = group.closeButtonContainer;
    close.content = group.closeButtonContent;
    close.stateLayer = group.closeButtonContent;
    close.enabledElevation = ElevationLevel::Level3;
    close.hoveredElevation = ElevationLevel::Level4;
    close.focusedElevation = ElevationLevel::Level3;
    close.pressedElevation = ElevationLevel::Level3;
    close.disabledElevation = ElevationLevel::Level0;
    tokens.closeButton = applyOverrides(close, FabMenuElement::CloseButton, overrides);

    // --- list item: md.comp.fab-menu.<group>-container ----------------------
    // 56 tall, content-derived wide; icon 24, title-medium label, L0 in
    // every state.
    MdFabMenuElementTokens item;
    item.containerHeight = 56.0;
    item.containerWidth = 0.0;
    item.iconSize = 24.0;
    item.iconLabelSpace = 8.0;
    item.leadingSpace = 24.0;
    item.trailingSpace = 24.0;
    item.containerShape = ShapeCorner::Full;
    item.labelStyle = TypeStyle::TitleMedium;
    item.container = group.itemContainer;
    item.content = group.itemContent;
    item.stateLayer = group.itemContent;
    item.enabledElevation = ElevationLevel::Level0;
    item.hoveredElevation = ElevationLevel::Level0;
    item.focusedElevation = ElevationLevel::Level0;
    item.pressedElevation = ElevationLevel::Level0;
    item.disabledElevation = ElevationLevel::Level0;
    tokens.listItem = applyOverrides(item, FabMenuElement::ListItem, overrides);

    return tokens;
}

// The disabled row, shared by both elements; exposed through the element
// struct's colour slots by the style, kept here as the single transcription.
qreal fabMenuDisabledContainerOpacity()
{
    return kDisabledContainerOpacity;
}

qreal fabMenuDisabledContentOpacity()
{
    return kDisabledContentOpacity;
}

} // namespace md
