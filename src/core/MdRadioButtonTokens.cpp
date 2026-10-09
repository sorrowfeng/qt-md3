#include "MdRadioButtonTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = navItemStateCount;

/// The unselected side: the icon rests on `on-surface-variant` and lifts to
/// `on-surface` under every interaction; the state layer is `on-surface` under
/// hover/focus and the `primary` pressed special case. Disabled reads
/// `on-surface` at the 0.38 icon opacity, no state layer.
void fillUnselected(MdRadioButtonTokens *tokens)
{
    MdNavigationColourSlot *icon = tokens->icon[int(MdRadioButtonSelection::Unselected)];
    MdNavigationColourSlot *stateLayer =
        tokens->stateLayer[int(MdRadioButtonSelection::Unselected)];

    icon[int(MdNavigationItemState::Enabled)] = {ColorRole::OnSurfaceVariant, 1.0};
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        icon[i] = {ColorRole::OnSurface, 1.0};
        stateLayer[i] = {ColorRole::OnSurface, 1.0};
    }
    // The pressed layer's own colour row: the colour the button is about to
    // earn.
    stateLayer[int(MdNavigationItemState::Pressed)] = {ColorRole::Primary, 1.0};
    icon[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                  tokens->disabledIconOpacity};
}

/// The selected side: the icon is `primary` in every enabled interaction and
/// the state layer is `primary` under hover/focus with the `on-surface`
/// pressed special case. Disabled reads `on-surface` at 0.38.
void fillSelected(MdRadioButtonTokens *tokens)
{
    MdNavigationColourSlot *icon = tokens->icon[int(MdRadioButtonSelection::Selected)];
    MdNavigationColourSlot *stateLayer =
        tokens->stateLayer[int(MdRadioButtonSelection::Selected)];

    for (int i = 0; i < stateCount; ++i) {
        icon[i] = {ColorRole::Primary, 1.0};
    }
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused)}) {
        stateLayer[i] = {ColorRole::Primary, 1.0};
    }
    stateLayer[int(MdNavigationItemState::Pressed)] = {ColorRole::OnSurface, 1.0};
    icon[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                  tokens->disabledIconOpacity};
}

} // namespace

const MdNavigationColourSlot &
MdRadioButtonTokens::iconFor(MdRadioButtonSelection selection, MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), radioButtonSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return icon[s][i];
}

const MdNavigationColourSlot &
MdRadioButtonTokens::stateLayerFor(MdRadioButtonSelection selection,
                                   MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), radioButtonSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return stateLayer[s][i];
}

MdRadioButtonTokens MdRadioButtonTokens::resolve(const MdComponentTokens *overrides)
{
    MdRadioButtonTokens tokens;

    using namespace comptoken;

    tokens.iconSize = lengthOverride(overrides, plainKeys("radio-button", "icon.size"), 20.0);
    tokens.stateLayerSize = lengthOverride(
        overrides, plainKeys("radio-button", "state-layer.size"), 40.0);
    tokens.disabledIconOpacity = lengthOverride(
        overrides, plainKeys("radio-button", "disabled.selected.icon.opacity"), 0.38);
    tokens.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("radio-button", "unselected.hover.state-layer.opacity"), 0.08);
    tokens.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("radio-button", "unselected.focus.state-layer.opacity"), 0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("radio-button", "unselected.pressed.state-layer.opacity"), 0.12);

    fillUnselected(&tokens);
    fillSelected(&tokens);

    return tokens;
}

} // namespace md
