#include "MdExpressiveMenuTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = navItemStateCount;

using Slot = MdNavigationColourSlot;

/// Shared disabled opacities: both variants publish 0.38 on every content slot.
void fillDisabledOpacities(MdExpressiveMenuTokens *tokens)
{
    tokens->disabledLabelTextOpacity = 0.38;
    tokens->disabledIconOpacity = 0.38;
    tokens->disabledSupportingTextOpacity = 0.38;
    tokens->disabledTrailingSupportingTextOpacity = 0.38;
}

/// StandardMenu (`_md-comp-menus-standard.scss`): surface-container-low base,
/// on-surface label, on-surface-variant icons with no interaction lift, the
/// on-surface state layer, and a tertiary-container selected row with
/// on-tertiary-container content. Disabled + selected publishes real rows
/// here (container at 0.38, content at 0.38).
void fillStandard(MdExpressiveMenuTokens *tokens)
{
    tokens->containerColor = ColorRole::SurfaceContainerLow;
    tokens->containerShadowColor = ColorRole::Shadow;
    tokens->sectionLabelColor = ColorRole::OnSurfaceVariant;
    tokens->iconButtonContainerColor = ColorRole::SurfaceContainerLow;
    tokens->iconButtonIconColor = ColorRole::OnSurfaceVariant;
    tokens->iconButtonStateLayerColor = ColorRole::OnSurface;
    tokens->iconButtonSelectedContainerColor = ColorRole::TertiaryContainer;
    tokens->iconButtonSelectedIconColor = ColorRole::OnTertiaryContainer;

    const int u = int(MdExpressiveMenuSelection::Unselected);
    const int s = int(MdExpressiveMenuSelection::Selected);

    for (int i = 0; i < stateCount; ++i) {
        const bool disabled = i == int(MdNavigationItemState::Disabled);

        // Unselected content.
        tokens->label[u][i] = {ColorRole::OnSurface, disabled ? 0.38 : 1.0};
        tokens->icon[u][i] = {ColorRole::OnSurfaceVariant, disabled ? 0.38 : 1.0};
        tokens->supportingText[u][i] = {ColorRole::OnSurfaceVariant, disabled ? 0.38 : 1.0};
        tokens->trailingSupportingText[u][i] = {ColorRole::OnSurfaceVariant, disabled ? 0.38 : 1.0};
        tokens->itemContainer[u][i] = {ColorRole::SurfaceContainerLow, 1.0};

        // Selected content: on-tertiary-container everywhere; disabled rows
        // are published at 0.38 rather than falling back.
        tokens->label[s][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->icon[s][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->supportingText[s][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->trailingSupportingText[s][i] = {ColorRole::OnTertiaryContainer,
                                                disabled ? 0.38 : 1.0};
        // Standard publishes `selected.disabled.container-opacity: 0.38`.
        tokens->itemContainer[s][i] = {ColorRole::TertiaryContainer, disabled ? 0.38 : 1.0};
    }

    // The state layer is one family (on-surface); the selected rows point at
    // on-tertiary-container with the same opacities.
    for (int i = 0; i < stateCount; ++i) {
        switch (MdNavigationItemState(i)) {
        case MdNavigationItemState::Hovered:
            tokens->stateLayer[u][i] = {ColorRole::OnSurface, 1.0};
            tokens->stateLayer[s][i] = {ColorRole::OnTertiaryContainer, 1.0};
            break;
        case MdNavigationItemState::Focused:
        case MdNavigationItemState::Pressed:
            tokens->stateLayer[u][i] = {ColorRole::OnSurface, 1.0};
            tokens->stateLayer[s][i] = {ColorRole::OnTertiaryContainer, 1.0};
            break;
        default:
            tokens->stateLayer[u][i] = {ColorRole::OnSurface, 0.0};
            tokens->stateLayer[s][i] = {ColorRole::OnTertiaryContainer, 0.0};
            break;
        }
    }
}

/// VibrantMenu (`_md-comp-menus-vibrant.scss`): tertiary-container base,
/// on-tertiary-container content, tertiary selected row with on-tertiary
/// content. The unselected icons *lift* to `tertiary` under hover / focus /
/// press — unlike Standard, where they stay put.
void fillVibrant(MdExpressiveMenuTokens *tokens)
{
    tokens->containerColor = ColorRole::TertiaryContainer;
    tokens->containerShadowColor = ColorRole::Shadow;
    tokens->sectionLabelColor = ColorRole::OnTertiaryContainer;
    tokens->iconButtonContainerColor = ColorRole::TertiaryContainer;
    tokens->iconButtonIconColor = ColorRole::OnTertiaryContainer;
    tokens->iconButtonStateLayerColor = ColorRole::OnTertiaryContainer;
    tokens->iconButtonSelectedContainerColor = ColorRole::Tertiary;
    tokens->iconButtonSelectedIconColor = ColorRole::OnTertiary;

    const int u = int(MdExpressiveMenuSelection::Unselected);
    const int s = int(MdExpressiveMenuSelection::Selected);

    for (int i = 0; i < stateCount; ++i) {
        const bool disabled = i == int(MdNavigationItemState::Disabled);
        const bool lifted = i == int(MdNavigationItemState::Hovered)
            || i == int(MdNavigationItemState::Focused)
            || i == int(MdNavigationItemState::Pressed);

        // Unselected: label / supporting text stay on-tertiary-container; the
        // icons lift to `tertiary` under the interactive states.
        tokens->label[u][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->icon[u][i] = {disabled || !lifted ? ColorRole::OnTertiaryContainer
                                                  : ColorRole::Tertiary,
                              disabled ? 0.38 : 1.0};
        tokens->supportingText[u][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->trailingSupportingText[u][i] = {ColorRole::OnTertiaryContainer, disabled ? 0.38 : 1.0};
        tokens->itemContainer[u][i] = {ColorRole::TertiaryContainer, 1.0};

        // Selected: `tertiary` fill with `on-tertiary` content. Vibrant
        // publishes selected disabled *content* opacities but no
        // `selected.disabled.container-opacity` row — the container stays at
        // 1.0 and only the content fades.
        tokens->label[s][i] = {ColorRole::OnTertiary, disabled ? 0.38 : 1.0};
        tokens->icon[s][i] = {ColorRole::OnTertiary, disabled ? 0.38 : 1.0};
        tokens->supportingText[s][i] = {ColorRole::OnTertiary, disabled ? 0.38 : 1.0};
        tokens->trailingSupportingText[s][i] = {ColorRole::OnTertiary, disabled ? 0.38 : 1.0};
        tokens->itemContainer[s][i] = {ColorRole::Tertiary, 1.0};
    }

    for (int i = 0; i < stateCount; ++i) {
        switch (MdNavigationItemState(i)) {
        case MdNavigationItemState::Hovered:
        case MdNavigationItemState::Focused:
        case MdNavigationItemState::Pressed:
            tokens->stateLayer[u][i] = {ColorRole::OnTertiaryContainer, 1.0};
            tokens->stateLayer[s][i] = {ColorRole::OnTertiary, 1.0};
            break;
        default:
            tokens->stateLayer[u][i] = {ColorRole::OnTertiaryContainer, 0.0};
            tokens->stateLayer[s][i] = {ColorRole::OnTertiary, 0.0};
            break;
        }
    }
}

} // namespace

const MdNavigationColourSlot &
MdExpressiveMenuTokens::labelFor(MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return label[int(selection)][i];
}

const MdNavigationColourSlot &
MdExpressiveMenuTokens::iconFor(MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return icon[int(selection)][i];
}

const MdNavigationColourSlot &MdExpressiveMenuTokens::supportingTextFor(
    MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return supportingText[int(selection)][i];
}

const MdNavigationColourSlot &MdExpressiveMenuTokens::trailingSupportingTextFor(
    MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return trailingSupportingText[int(selection)][i];
}

const MdNavigationColourSlot &MdExpressiveMenuTokens::stateLayerFor(
    MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return stateLayer[int(selection)][i];
}

const MdNavigationColourSlot &MdExpressiveMenuTokens::itemContainerFor(
    MdExpressiveMenuSelection selection, MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    return itemContainer[int(selection)][i];
}

ShapeCorner MdExpressiveMenuTokens::itemShapeFor(int index, int count, bool selected,
                                                 bool active) const
{
    // The pressed item morphs to the published active corner.
    if (active) {
        return activeContainerShape;
    }
    if (selected) {
        return itemSelectedShape;
    }
    if (count > 0 && index == 0) {
        return itemFirstChildShape;
    }
    if (count > 0 && index == count - 1) {
        return itemLastChildShape;
    }
    return itemShape;
}

MdExpressiveMenuTokens MdExpressiveMenuTokens::resolve(MdExpressiveMenuVariant variant,
                                                       const MdComponentTokens *overrides)
{
    MdExpressiveMenuTokens tokens;

    using namespace comptoken;

    // Shared geometry from `_md-comp-menus.scss`.
    tokens.itemHeight = lengthOverride(overrides, plainKeys("menus", "menu-item.height"), 44.0);
    tokens.itemGap = lengthOverride(overrides, plainKeys("menus", "gap"), 2.0);
    tokens.groupPadding = lengthOverride(overrides, plainKeys("menus", "group.padding"), 2.0);
    tokens.leadingIconSize =
        lengthOverride(overrides, plainKeys("menus", "menu-item.leading-icon.size"), 20.0);
    tokens.trailingIconSize =
        lengthOverride(overrides, plainKeys("menus", "menu-item.trailing-icon.size"), 20.0);
    tokens.itemIconTextSpacing =
        lengthOverride(overrides, plainKeys("menus", "menu-item.between-space"), 12.0);
    tokens.itemLeadingSpace =
        lengthOverride(overrides, plainKeys("menus", "menu-item.leading-space"), 16.0);
    tokens.itemTrailingSpace =
        lengthOverride(overrides, plainKeys("menus", "menu-item.trailing-space"), 16.0);

    fillDisabledOpacities(&tokens);

    switch (variant) {
    case MdExpressiveMenuVariant::Vibrant:
        fillVibrant(&tokens);
        break;
    case MdExpressiveMenuVariant::Standard:
    case MdExpressiveMenuVariant::Count:
        fillStandard(&tokens);
        break;
    }

    return tokens;
}

} // namespace md
