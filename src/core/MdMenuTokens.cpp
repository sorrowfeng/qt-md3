#include "MdMenuTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = navItemStateCount;

/// The unselected side: `on-surface` label, `on-surface-variant` icons (no
/// interaction lift — the export's hover/focus/pressed icon rows carry the
/// same colour), the `on-surface` state layer under hover/focus/press, and no
/// container (a transparent item over the menu surface).
void fillUnselected(MdMenuTokens *tokens)
{
    const int s = int(MdMenuSelection::Unselected);
    MdNavigationColourSlot *label = tokens->label[s];
    MdNavigationColourSlot *icon = tokens->icon[s];
    MdNavigationColourSlot *stateLayer = tokens->stateLayer[s];
    MdNavigationColourSlot *itemContainer = tokens->itemContainer[s];

    for (int i : {int(MdNavigationItemState::Enabled), int(MdNavigationItemState::Hovered),
                  int(MdNavigationItemState::Focused), int(MdNavigationItemState::Pressed)}) {
        label[i] = {ColorRole::OnSurface, 1.0};
        icon[i] = {ColorRole::OnSurfaceVariant, 1.0};
    }
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        stateLayer[i] = {ColorRole::OnSurface, 1.0};
    }
    // Disabled: the content fades to `on-surface` at 0.38; no container.
    label[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                   tokens->disabledLabelTextOpacity};
    icon[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                  tokens->disabledIconOpacity};
    Q_UNUSED(itemContainer);
}

/// The selected side: `secondary-container` fill with `on-secondary-container`
/// content for the enabled interactions. Disabled has no rows — the fallback
/// to the unselected disabled rows happens in the accessors.
void fillSelected(MdMenuTokens *tokens)
{
    const int s = int(MdMenuSelection::Selected);
    MdNavigationColourSlot *label = tokens->label[s];
    MdNavigationColourSlot *icon = tokens->icon[s];
    MdNavigationColourSlot *itemContainer = tokens->itemContainer[s];

    for (int i : {int(MdNavigationItemState::Enabled), int(MdNavigationItemState::Hovered),
                  int(MdNavigationItemState::Focused), int(MdNavigationItemState::Pressed)}) {
        itemContainer[i] = {ColorRole::SecondaryContainer, 1.0};
        label[i] = {ColorRole::OnSecondaryContainer, 1.0};
        icon[i] = {ColorRole::OnSecondaryContainer, 1.0};
    }
}

} // namespace

const MdNavigationColourSlot &MdMenuTokens::labelFor(MdMenuSelection selection,
                                                     MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    if (selection == MdMenuSelection::Selected && i != int(MdNavigationItemState::Disabled)) {
        return label[int(MdMenuSelection::Selected)][i];
    }
    return label[int(MdMenuSelection::Unselected)][i];
}

const MdNavigationColourSlot &MdMenuTokens::iconFor(MdMenuSelection selection,
                                                    MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    if (selection == MdMenuSelection::Selected && i != int(MdNavigationItemState::Disabled)) {
        return icon[int(MdMenuSelection::Selected)][i];
    }
    return icon[int(MdMenuSelection::Unselected)][i];
}

const MdNavigationColourSlot &MdMenuTokens::stateLayerFor(MdMenuSelection selection,
                                                          MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    // The export publishes one state-layer family only (`on-surface` under the
    // unselected hover/focus/press states); the selected side has no rows, and
    // Compose's indication applies to selected items just the same — so the
    // state layer always reads the unselected side.
    Q_UNUSED(selection);
    return stateLayer[int(MdMenuSelection::Unselected)][i];
}

const MdNavigationColourSlot &MdMenuTokens::itemContainerFor(MdMenuSelection selection,
                                                             MdNavigationItemState state) const
{
    const int i = qBound(0, int(state), stateCount - 1);
    if (selection == MdMenuSelection::Selected && i != int(MdNavigationItemState::Disabled)) {
        return itemContainer[int(MdMenuSelection::Selected)][i];
    }
    return itemContainer[int(MdMenuSelection::Unselected)][i];
}

MdMenuTokens MdMenuTokens::resolve(const MdComponentTokens *overrides)
{
    MdMenuTokens tokens;

    using namespace comptoken;

    tokens.containerColor = ColorRole::SurfaceContainer;
    tokens.itemHeight =
        lengthOverride(overrides, plainKeys("menu", "list-item.container.height"), 48.0);
    tokens.iconSize = lengthOverride(
        overrides, plainKeys("menu", "list-item.with-leading-icon.leading-icon.size"), 24.0);
    tokens.dividerHeight =
        lengthOverride(overrides, plainKeys("menu", "divider.height"), 1.0);
    tokens.disabledLabelTextOpacity = lengthOverride(
        overrides, plainKeys("menu", "list-item.disabled.label-text.opacity"), 0.38);
    tokens.disabledIconOpacity = lengthOverride(
        overrides, plainKeys("menu", "list-item.with-leading-icon.disabled.leading-icon.opacity"),
        0.38);
    tokens.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("menu", "list-item.hover.state-layer.opacity"), 0.08);
    tokens.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("menu", "list-item.focus.state-layer.opacity"), 0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("menu", "list-item.pressed.state-layer.opacity"), 0.12);
    tokens.focusIndicatorInnerOffset = lengthOverride(
        overrides, plainKeys("menu", "focus.indicator.outline.offset"), 3.0);
    tokens.focusIndicatorThickness = lengthOverride(
        overrides, plainKeys("menu", "focus.indicator.thickness"), 3.0);

    fillUnselected(&tokens);
    fillSelected(&tokens);

    return tokens;
}

} // namespace md
