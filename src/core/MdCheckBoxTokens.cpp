#include "MdCheckBoxTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = navItemStateCount;

/// The unselected side. No container row exists (the fill is transparent), the
/// outline rests on `on-surface-variant` and lifts to `on-surface` under every
/// interaction, the pressed state layer is the `primary` special case, and the
/// deprecated icon rows are carried `on-surface`.
void fillUnselected(MdCheckBoxTokens *tokens)
{
    MdNavigationColourSlot *box = tokens->box[int(MdCheckBoxSelection::Unselected)];
    MdNavigationColourSlot *outline = tokens->outline[int(MdCheckBoxSelection::Unselected)];
    MdNavigationColourSlot *checkmark = tokens->checkmark[int(MdCheckBoxSelection::Unselected)];
    MdNavigationColourSlot *stateLayer = tokens->stateLayer[int(MdCheckBoxSelection::Unselected)];

    for (int i = 0; i < stateCount; ++i) {
        outline[i] = {ColorRole::OnSurfaceVariant, 1.0};
        checkmark[i] = {ColorRole::OnSurface, 1.0}; // deprecated; carried, not read
    }
    for (int i : {int(MdNavigationItemState::Hovered), int(MdNavigationItemState::Focused),
                  int(MdNavigationItemState::Pressed)}) {
        outline[i] = {ColorRole::OnSurface, 1.0};
        stateLayer[i] = {ColorRole::OnSurface, 1.0};
    }
    // The pressed layer's own colour row: the colour the box is about to earn.
    stateLayer[int(MdNavigationItemState::Pressed)] = {ColorRole::Primary, 1.0};
    // Disabled: the outline keeps its 2 px width but reads `on-surface` at the
    // 0.38 container opacity; no state layer, no fill.
    outline[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                     tokens->unselectedDisabledContainerOpacity};
    checkmark[int(MdNavigationItemState::Disabled)] = {ColorRole::Count, 1.0};
}

/// The selected side. The fill and the border resolve to the same `primary`
/// (Compose's `drawBox` collapses the pair), the check rides `on-primary`, and
/// the pressed state layer is the `on-surface` special case.
void fillSelected(MdCheckBoxTokens *tokens)
{
    MdNavigationColourSlot *box = tokens->box[int(MdCheckBoxSelection::Selected)];
    MdNavigationColourSlot *checkmark = tokens->checkmark[int(MdCheckBoxSelection::Selected)];
    MdNavigationColourSlot *stateLayer = tokens->stateLayer[int(MdCheckBoxSelection::Selected)];

    for (int i = 0; i < stateCount; ++i) {
        box[i] = {ColorRole::Primary, 1.0};
        checkmark[i] = {ColorRole::OnPrimary, 1.0};
        stateLayer[i] = {ColorRole::Primary, 1.0};
    }
    // The pressed layer's own colour row.
    stateLayer[int(MdNavigationItemState::Pressed)] = {ColorRole::OnSurface, 1.0};
    // Disabled: the whole box reads `on-surface` at the 0.38 container
    // opacity, the check `surface` at full strength, and no state layer.
    box[int(MdNavigationItemState::Disabled)] = {ColorRole::OnSurface,
                                                 tokens->selectedDisabledContainerOpacity};
    checkmark[int(MdNavigationItemState::Disabled)] = {ColorRole::Surface, 1.0};
    stateLayer[int(MdNavigationItemState::Disabled)] = {ColorRole::Count, 1.0};
}

/// The error overlay. The export publishes enabled-state rows only; disabled
/// error boxes fall back to the base tables.
void fillError(MdCheckBoxTokens *tokens)
{
    MdNavigationColourSlot *errorBox = tokens->errorBox[int(MdCheckBoxSelection::Selected)];
    MdNavigationColourSlot *errorOutline = tokens->errorOutline[int(MdCheckBoxSelection::Unselected)];
    MdNavigationColourSlot *errorCheckmark = tokens->errorCheckmark[int(MdCheckBoxSelection::Selected)];

    for (int i : {int(MdNavigationItemState::Enabled), int(MdNavigationItemState::Hovered),
                  int(MdNavigationItemState::Focused), int(MdNavigationItemState::Pressed)}) {
        errorBox[i] = {ColorRole::Error, 1.0};
        errorOutline[i] = {ColorRole::Error, 1.0};
        errorCheckmark[i] = {ColorRole::OnError, 1.0};
        tokens->errorStateLayer[i] = {ColorRole::Error, 1.0};
    }
}

} // namespace

const MdNavigationColourSlot &
MdCheckBoxTokens::boxFor(bool error, MdCheckBoxSelection selection,
                         MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), checkBoxSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    if (error && errorBox[s][i].isPresent()) {
        return errorBox[s][i];
    }
    return box[s][i];
}

const MdNavigationColourSlot &
MdCheckBoxTokens::outlineFor(bool error, MdCheckBoxSelection selection,
                             MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), checkBoxSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    if (error && errorOutline[s][i].isPresent()) {
        return errorOutline[s][i];
    }
    return outline[s][i];
}

const MdNavigationColourSlot &
MdCheckBoxTokens::checkmarkFor(bool error, MdCheckBoxSelection selection,
                               MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), checkBoxSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    if (error && errorCheckmark[s][i].isPresent()) {
        return errorCheckmark[s][i];
    }
    return checkmark[s][i];
}

const MdNavigationColourSlot &
MdCheckBoxTokens::stateLayerFor(bool error, MdCheckBoxSelection selection,
                                MdNavigationItemState state) const
{
    const int s = qBound(0, int(selection), checkBoxSelectionCount - 1);
    const int i = qBound(0, int(state), stateCount - 1);
    if (error && errorStateLayer[i].isPresent()) {
        return errorStateLayer[i];
    }
    return stateLayer[s][i];
}

MdCheckBoxTokens MdCheckBoxTokens::resolve(const MdComponentTokens *overrides)
{
    MdCheckBoxTokens tokens;

    using namespace comptoken;

    tokens.containerSize = lengthOverride(
        overrides, plainKeys("checkbox", "container.size"), 18.0);
    // The shape row is a plain 2 px radius, so it reads through `lengthOverride`
    // rather than the ShapeCorner table.
    tokens.containerShapeRadius = lengthOverride(
        overrides, plainKeys("checkbox", "container.shape"), 2.0);
    tokens.iconSize = lengthOverride(overrides, plainKeys("checkbox", "icon.size"), 18.0);
    tokens.stateLayerSize = lengthOverride(
        overrides, plainKeys("checkbox", "state-layer.size"), 40.0);
    tokens.hoverStateLayerOpacity = lengthOverride(
        overrides, plainKeys("checkbox", "unselected.hover.state-layer.opacity"), 0.08);
    tokens.focusStateLayerOpacity = lengthOverride(
        overrides, plainKeys("checkbox", "unselected.focus.state-layer.opacity"), 0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("checkbox", "unselected.pressed.state-layer.opacity"), 0.12);
    tokens.selectedDisabledContainerOpacity = lengthOverride(
        overrides, plainKeys("checkbox", "selected.disabled.container.opacity"), 0.38);
    tokens.unselectedDisabledContainerOpacity = lengthOverride(
        overrides, plainKeys("checkbox", "unselected.disabled.container.opacity"), 0.38);
    tokens.focusIndicatorOuterOffset = lengthOverride(
        overrides, plainKeys("checkbox", "focus.indicator.outline.offset"), 2.0);
    tokens.focusIndicatorThickness = lengthOverride(
        overrides, plainKeys("checkbox", "focus.indicator.thickness"), 3.0);

    fillUnselected(&tokens);
    fillSelected(&tokens);
    fillError(&tokens);

    return tokens;
}

} // namespace md
