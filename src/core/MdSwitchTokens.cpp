#include "MdSwitchTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = navItemStateCount;

/// The unselected side. The track rests on `surface-container-highest` with a
/// 2 px `outline` border; the handle starts at `outline` and lifts to
/// `on-surface-variant` under interaction; the thumb icon rides the track's
/// own colour; the state layer is `on-surface` under hover/focus/press.
void fillUnselected(MdSwitchTokens *tokens)
{
    const int s = int(MdSwitchSelection::Unselected);
    MdNavigationColourSlot *track = tokens->track[s];
    MdNavigationColourSlot *trackOutline = tokens->trackOutline[s];
    MdNavigationColourSlot *handle = tokens->handle[s];
    MdNavigationColourSlot *icon = tokens->icon[s];
    MdNavigationColourSlot *stateLayer = tokens->stateLayer[s];

    for (int i = 0; i < stateCount; ++i) {
        track[i] = {ColorRole::SurfaceContainerHighest, 1.0};
        trackOutline[i] = {ColorRole::Outline, 1.0};
        icon[i] = {ColorRole::SurfaceContainerHighest, 1.0};
    }
    handle[int(MdNavigationItemState::Enabled)] = {ColorRole::Outline, 1.0};
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        handle[i] = {ColorRole::OnSurfaceVariant, 1.0};
        stateLayer[i] = {ColorRole::OnSurface, 1.0};
    }
    // Disabled: the track and its outline fade at their own 0.12, the handle
    // at 0.38, the icon at 0.38.
    track[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                   tokens->disabledTrackOpacity};
    trackOutline[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                          tokens->disabledTrackOpacity};
    handle[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                    tokens->disabledUnselectedHandleOpacity};
    icon[int(MdNavigationItemState::Disabled)] = {ColorRole::SurfaceContainerHighest,
                                                  tokens->disabledIconOpacity};
}

/// The selected side. The track is `primary`; the handle `on-primary` resting
/// and `primary-container` under interaction; the icon `primary`; the state
/// layer `primary` under hover/focus/press. No outline rows — the checked
/// border resolves transparent.
void fillSelected(MdSwitchTokens *tokens)
{
    const int s = int(MdSwitchSelection::Selected);
    MdNavigationColourSlot *track = tokens->track[s];
    MdNavigationColourSlot *handle = tokens->handle[s];
    MdNavigationColourSlot *icon = tokens->icon[s];
    MdNavigationColourSlot *stateLayer = tokens->stateLayer[s];

    for (int i = 0; i < stateCount; ++i) {
        track[i] = {ColorRole::Primary, 1.0};
        icon[i] = {ColorRole::Primary, 1.0};
    }
    handle[int(MdNavigationItemState::Enabled)] = {ColorRole::OnPrimary, 1.0};
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        handle[i] = {ColorRole::PrimaryContainer, 1.0};
        stateLayer[i] = {ColorRole::Primary, 1.0};
    }
    // Disabled: the track fades at 0.12; the handle keeps **full strength** on
    // its `surface` row (`disabled.selected.handle.opacity` is 1); the icon
    // fades at 0.38.
    track[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                   tokens->disabledTrackOpacity};
    handle[int(MdNavigationItemState::Disabled)] = {ColorRole::Surface,
                                                    tokens->disabledSelectedHandleOpacity};
    icon[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                  tokens->disabledIconOpacity};
}

} // namespace

const MdNavigationColourSlot &MdSwitchTokens::trackFor(MdSwitchSelection selection,
                                                       MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), switchSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return track[s][i];
}

const MdNavigationColourSlot &MdSwitchTokens::trackOutlineFor(MdSwitchSelection selection,
                                                              MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), switchSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return trackOutline[s][i];
}

const MdNavigationColourSlot &MdSwitchTokens::handleFor(MdSwitchSelection selection,
                                                        MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), switchSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return handle[s][i];
}

const MdNavigationColourSlot &MdSwitchTokens::iconFor(MdSwitchSelection selection,
                                                      MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), switchSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return icon[s][i];
}

const MdNavigationColourSlot &MdSwitchTokens::stateLayerFor(MdSwitchSelection selection,
                                                            MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), switchSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    return stateLayer[s][i];
}

MdSwitchTokens MdSwitchTokens::resolve(const MdComponentTokens *overrides)
{
    MdSwitchTokens tokens;

    using namespace comptoken;

    tokens.trackWidth = lengthOverride(overrides, plainKeys("switch", "track.width"), 52.0);
    tokens.trackHeight = lengthOverride(overrides, plainKeys("switch", "track.height"), 32.0);
    tokens.trackOutlineWidth =
        lengthOverride(overrides, plainKeys("switch", "track.outline.width"), 2.0);
    tokens.unselectedHandleSize = lengthOverride(
        overrides, plainKeys("switch", "unselected.handle.width"), 16.0);
    tokens.selectedHandleSize =
        lengthOverride(overrides, plainKeys("switch", "selected.handle.width"), 24.0);
    tokens.pressedHandleSize =
        lengthOverride(overrides, plainKeys("switch", "pressed.handle.width"), 28.0);
    tokens.withIconHandleSize =
        lengthOverride(overrides, plainKeys("switch", "with-icon.handle.width"), 24.0);
    tokens.iconSize = lengthOverride(overrides, plainKeys("switch", "selected.icon.size"), 16.0);
    tokens.stateLayerSize =
        lengthOverride(overrides, plainKeys("switch", "state-layer.size"), 40.0);
    tokens.disabledTrackOpacity =
        lengthOverride(overrides, plainKeys("switch", "disabled.track.opacity"), 0.12);
    tokens.disabledUnselectedHandleOpacity = lengthOverride(
        overrides, plainKeys("switch", "disabled.unselected.handle.opacity"), 0.38);
    tokens.disabledSelectedHandleOpacity = lengthOverride(
        overrides, plainKeys("switch", "disabled.selected.handle.opacity"), 1.0);
    tokens.disabledIconOpacity = lengthOverride(
        overrides, plainKeys("switch", "disabled.unselected.icon.opacity"), 0.38);
    tokens.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("switch", "unselected.hover.state-layer.opacity"), 0.08);
    tokens.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("switch", "unselected.focus.state-layer.opacity"), 0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("switch", "unselected.pressed.state-layer.opacity"), 0.12);
    tokens.focusIndicatorOuterOffset = lengthOverride(
        overrides, plainKeys("switch", "focus.indicator.offset"), 2.0);
    tokens.focusIndicatorThickness = lengthOverride(
        overrides, plainKeys("switch", "focus.indicator.thickness"), 3.0);

    fillUnselected(&tokens);
    fillSelected(&tokens);

    return tokens;
}

} // namespace md
