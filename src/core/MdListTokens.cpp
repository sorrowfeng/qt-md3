#include "MdListTokens.h"

#include "MdCompTokenParse.h"

namespace md {

const MdListStateRow &MdListFamily::state(MdListState which) const
{
    switch (which) {
    case MdListState::Hovered:
        return hovered;
    case MdListState::Focused:
        return focused;
    case MdListState::Pressed:
        return pressed;
    case MdListState::Dragged:
        return dragged;
    case MdListState::Enabled:
    case MdListState::Disabled:
    case MdListState::Count:
        break;
    }
    return enabled;
}

namespace {

using namespace comptoken;

/// `md.comp.list.<token>` — the container rows.
QStringList listKeys(const char *token)
{
    return {QStringLiteral("md.comp.list.%1").arg(QLatin1String(token))};
}

/// `md.comp.list.list-item.<token>` — the item rows.
QStringList itemKeys(const char *token)
{
    return {QStringLiteral("md.comp.list.list-item.%1").arg(QLatin1String(token))};
}

/// The state layers' opacities come from `md.sys.state.*`, exactly as the
/// export references them.
constexpr qreal kHoverOpacity = 0.08;
constexpr qreal kFocusOpacity = 0.12;
constexpr qreal kPressedOpacity = 0.12;
constexpr qreal kDraggedOpacity = 0.16;

} // namespace

MdListContainerTokens MdListContainerTokens::resolve(const MdComponentTokens *overrides)
{
    MdListContainerTokens tokens;

    using namespace comptoken;
    // `topPadding` / `bottomPadding` are deliberately absent: the export
    // publishes no `md.comp.list.container.top-padding` row — the 8px comes
    // from `_list.scss`'s `padding: 8px 0` and is carried, not looked up.
    tokens.segmentedGap = lengthOverride(overrides, listKeys("segmented.gap"),
                                         tokens.segmentedGap);
    tokens.shape = shapeOverride(overrides, listKeys("container.shape"), tokens.shape);

    return tokens;
}

MdListTokens MdListTokens::resolve(MdListVariant variant, const MdComponentTokens *overrides)
{
    MdListTokens tokens;

    using namespace comptoken;

    // --- metrics -----------------------------------------------------------
    tokens.oneLineHeight = lengthOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.one-line.container.height")},
        tokens.oneLineHeight);
    tokens.twoLineHeight = lengthOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.two-line.container.height")},
        tokens.twoLineHeight);
    tokens.threeLineHeight = lengthOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.three-line.container.height")},
        tokens.threeLineHeight);
    tokens.topSpace = lengthOverride(overrides, itemKeys("list.list-item.top-space"),
                                     tokens.topSpace);
    tokens.bottomSpace = lengthOverride(overrides, itemKeys("list.list-item.bottom-space"),
                                        tokens.bottomSpace);
    tokens.leadingSpace = lengthOverride(overrides, itemKeys("list.list-item.leading-space"),
                                         tokens.leadingSpace);
    tokens.trailingSpace =
        lengthOverride(overrides, itemKeys("list.list-item.trailing-space"),
                       tokens.trailingSpace);
    tokens.betweenSpace = lengthOverride(overrides, itemKeys("list.list-item.between-space"),
                                         tokens.betweenSpace);

    tokens.leadingIconSize =
        lengthOverride(overrides, itemKeys("list.list-item.leading-icon.size"),
                       tokens.leadingIconSize);
    tokens.leadingIconExpressiveSize =
        lengthOverride(overrides,
                       itemKeys("list.list-item.leading-icon.expressive.size"),
                       tokens.leadingIconExpressiveSize);
    tokens.trailingIconSize =
        lengthOverride(overrides, itemKeys("list.list-item.trailing-icon.size"),
                       tokens.trailingIconSize);
    tokens.trailingIconExpressiveSize =
        lengthOverride(overrides,
                       itemKeys("list.list-item.trailing-icon.expressive.size"),
                       tokens.trailingIconExpressiveSize);
    tokens.leadingAvatarSize =
        lengthOverride(overrides, itemKeys("list.list-item.leading-avatar.size"),
                       tokens.leadingAvatarSize);

    // Expressive items use the 20px icon rows; Standard keeps the 24px ones.
    if (variant == MdListVariant::Expressive) {
        tokens.leadingIconSize = tokens.leadingIconExpressiveSize;
        tokens.trailingIconSize = tokens.trailingIconExpressiveSize;
        tokens.leadingImageShape = tokens.leadingImageExpressiveShape;
    }

    // --- shapes -------------------------------------------------------------
    // The expressive rows, overridable individually so a theme can retune the
    // morph without touching the rest of the family.
    tokens.expressiveShape =
        shapeOverride(overrides,
                      {QStringLiteral("md.comp.list.list-item.container.expressive.shape"),
                       QStringLiteral("md.comp.list.list-item.container.shape")},
                      tokens.expressiveShape);
    tokens.hoveredShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.hovered.container.expressive.shape")},
        tokens.hoveredShape);
    tokens.focusedShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.focused.container.expressive.shape")},
        tokens.focusedShape);
    tokens.pressedShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.pressed.container.expressive.shape")},
        tokens.pressedShape);
    tokens.selectedShape = shapeOverride(
        overrides,
        {
            QStringLiteral("md.comp.list.list-item.selected.container.expressive.shape"),
            QStringLiteral("md.comp.list.list-item.selected.container.shape"),
        },
        tokens.selectedShape);
    tokens.draggedShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.dragged.container.expressive.shape")},
        tokens.draggedShape);
    tokens.disabledShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.disabled.container.expressive.shape")},
        tokens.disabledShape);
    tokens.selectedDisabledShape = shapeOverride(
        overrides,
        {QStringLiteral("md.comp.list.list-item.selected.disabled.container.expressive.shape")},
        tokens.selectedDisabledShape);
    tokens.containerShape =
        shapeOverride(overrides, {QStringLiteral("md.comp.list.list-item.container.shape")},
                      tokens.containerShape);
    tokens.listShape = shapeOverride(overrides, {QStringLiteral("md.comp.list.container.shape")},
                                     tokens.listShape);

    // --- the focus indicator (list-level rows) -------------------------------
    tokens.focusIndicatorOutlineOffset =
        lengthOverride(overrides, listKeys("focus.indicator.outline.offset"),
                       tokens.focusIndicatorOutlineOffset);
    tokens.focusIndicatorThickness =
        lengthOverride(overrides, listKeys("focus.indicator.thickness"),
                       tokens.focusIndicatorThickness);

    if (variant == MdListVariant::Standard) {
        // Only the *metrics* the style substitutes are flattened here: the
        // baseline item uses the 24px icons and a square media slot. The shape
        // rows above are left exactly as the export publishes them — the token
        // table is the export, and "Standard is square" is a *behaviour* rule
        // applied by MdListItemStyle::shapeFor, not a token rewrite.
        tokens.leadingIconSize = lengthOverride(
            overrides, itemKeys("list.list-item.leading-icon.size"), 24.0);
        tokens.trailingIconSize = lengthOverride(
            overrides, itemKeys("list.list-item.trailing-icon.size"), 24.0);
        tokens.leadingImageShape = ShapeCorner::None;
    }

    // --- the unselected family ---------------------------------------------
    tokens.family.container = ColorRole::Surface;
    tokens.family.enabled.stateLayer = ColorRole::OnSurface;
    tokens.family.enabled.stateLayerOpacity = 0.0; // the enabled row has no layer
    tokens.family.enabled.labelText = ColorRole::OnSurface;
    tokens.family.enabled.leadingIcon = ColorRole::OnSurfaceVariant;
    tokens.family.enabled.trailingIcon = ColorRole::OnSurfaceVariant;
    tokens.family.enabled.overline = ColorRole::OnSurfaceVariant;
    tokens.family.enabled.supportingText = ColorRole::OnSurfaceVariant;
    tokens.family.enabled.trailingSupportingText = ColorRole::OnSurfaceVariant;

    tokens.family.hovered = tokens.family.enabled;
    tokens.family.hovered.stateLayerOpacity = kHoverOpacity;
    tokens.family.focused = tokens.family.enabled;
    tokens.family.focused.stateLayerOpacity = kFocusOpacity;
    tokens.family.pressed = tokens.family.enabled;
    tokens.family.pressed.stateLayerOpacity = kPressedOpacity;
    tokens.family.dragged = tokens.family.enabled;
    tokens.family.dragged.stateLayerOpacity = kDraggedOpacity;

    // --- the selected family ------------------------------------------------
    tokens.selectedFamily.container = ColorRole::SecondaryContainer;
    tokens.selectedFamily.enabled.stateLayer = ColorRole::OnSurface;
    tokens.selectedFamily.enabled.stateLayerOpacity = 0.0;
    tokens.selectedFamily.enabled.labelText = ColorRole::OnSecondaryContainer;
    tokens.selectedFamily.enabled.leadingIcon = ColorRole::OnSecondaryContainer;
    tokens.selectedFamily.enabled.trailingIcon = ColorRole::OnSecondaryContainer;
    tokens.selectedFamily.enabled.overline = ColorRole::OnSecondaryContainer;
    tokens.selectedFamily.enabled.supportingText = ColorRole::OnSecondaryContainer;
    tokens.selectedFamily.enabled.trailingSupportingText = ColorRole::OnSecondaryContainer;

    // The interaction rows of the selected family keep the *enabled* content
    // colours for the label (the export publishes them) but the icons drop to
    // on-surface — the one place the selected rows differ from their
    // unselected counterparts' role.
    tokens.selectedFamily.hovered = tokens.selectedFamily.enabled;
    tokens.selectedFamily.hovered.stateLayerOpacity = kHoverOpacity;
    tokens.selectedFamily.hovered.leadingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.hovered.trailingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.focused = tokens.selectedFamily.enabled;
    tokens.selectedFamily.focused.stateLayerOpacity = kFocusOpacity;
    tokens.selectedFamily.focused.leadingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.focused.trailingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.pressed = tokens.selectedFamily.enabled;
    tokens.selectedFamily.pressed.stateLayerOpacity = kPressedOpacity;
    tokens.selectedFamily.pressed.leadingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.pressed.trailingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.dragged = tokens.selectedFamily.enabled;
    tokens.selectedFamily.dragged.stateLayerOpacity = kDraggedOpacity;
    tokens.selectedFamily.dragged.leadingIcon = ColorRole::OnSurface;
    tokens.selectedFamily.dragged.trailingIcon = ColorRole::OnSurface;

    // --- the disabled rows --------------------------------------------------
    tokens.disabled.container = ColorRole::Surface;
    tokens.disabled.containerOpacity = 0.0; // no container row — unchanged
    tokens.disabled.labelText = ColorRole::OnSurface;
    tokens.disabled.leadingIcon = ColorRole::OnSurface;
    tokens.disabled.trailingIcon = ColorRole::OnSurface;
    tokens.disabled.overline = ColorRole::OnSurface;
    tokens.disabled.supportingText = ColorRole::OnSurface;
    tokens.disabled.trailingSupportingText = ColorRole::OnSurface;
    tokens.disabled.stateLayer = ColorRole::OnSurface;

    tokens.selectedDisabled = tokens.disabled;
    // The selected disabled row *does* publish a container colour, composited
    // over the selected container at 0.38.
    tokens.selectedDisabled.container = ColorRole::OnSurface;
    tokens.selectedDisabled.containerOpacity = 0.38;
    tokens.selectedDisabled.labelText = ColorRole::OnSurface;
    tokens.selectedDisabled.leadingIcon = ColorRole::OnSurface;
    tokens.selectedDisabled.trailingIcon = ColorRole::OnSurface;
    tokens.selectedDisabled.overline = ColorRole::OnSurface;
    tokens.selectedDisabled.supportingText = ColorRole::OnSurface;
    tokens.selectedDisabled.trailingSupportingText = ColorRole::OnSurface;
    tokens.selectedDisabled.stateLayer = ColorRole::OnSurface;

    return tokens;
}

} // namespace md
