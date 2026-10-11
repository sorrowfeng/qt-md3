#include "MdDatePickerDockedTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

constexpr int stateCount = datePickerDockedStateCount;
using Slot = MdNavigationColourSlot;

void fillDateTables(MdDatePickerDockedTokens *tokens)
{
    const int e = int(MdDatePickerDockedState::Enabled);
    const int h = int(MdDatePickerDockedState::Hovered);
    const int f = int(MdDatePickerDockedState::Focused);
    const int p = int(MdDatePickerDockedState::Pressed);
    const int d = int(MdDatePickerDockedState::Disabled);

    // Selected: primary fill, on-primary label, on-primary state layers.
    for (int i = 0; i < stateCount; ++i) {
        tokens->dateSelected[i] = {ColorRole::OnPrimary, i == d ? 0.38 : 1.0};
        tokens->dateToday[i] = {ColorRole::Primary, i == d ? 0.38 : 1.0};
        tokens->dateUnselected[i] = {ColorRole::OnSurface, i == d ? 0.38 : 1.0};
    }
    // The selected *container* is primary; the slots above carry the label.
    // Paint reads `dateSelected[Enabled]` as the fill and OnPrimary as the
    // label — carried as one slot pair, the modal set's shape.
    Q_UNUSED(e);
    Q_UNUSED(h);
    Q_UNUSED(f);
    Q_UNUSED(p);
}

void fillMenuButton(MdDatePickerDockedTokens *tokens)
{
    for (int i = 0; i < stateCount; ++i) {
        const bool disabled = i == int(MdDatePickerDockedState::Disabled);
        // Enabled / hover / focus / pressed all read on-surface-variant.
        tokens->menuButtonLabel[i] = {ColorRole::OnSurfaceVariant, disabled ? 0.38 : 1.0};
        tokens->menuButtonIcon[i] = {ColorRole::OnSurfaceVariant, disabled ? 0.38 : 1.0};
        tokens->menuButtonStateLayer[i] = {ColorRole::OnSurfaceVariant, disabled ? 0.0 : 1.0};
    }
}

void fillMenuList(MdDatePickerDockedTokens *tokens)
{
    for (int i = 0; i < stateCount; ++i) {
        tokens->menuListLabel[i] = {ColorRole::OnSurface, 1.0};
        // The selected leading icon is on-surface at rest and lifts to
        // on-surface-variant under the interactive states.
        const bool lifted = i == int(MdDatePickerDockedState::Hovered)
            || i == int(MdDatePickerDockedState::Focused)
            || i == int(MdDatePickerDockedState::Pressed);
        tokens->menuListSelectedIcon[i] = {lifted ? ColorRole::OnSurfaceVariant : ColorRole::OnSurface,
                                           1.0};
    }
}

} // namespace

MdDatePickerDockedTokens MdDatePickerDockedTokens::resolve(const MdComponentTokens *overrides)
{
    MdDatePickerDockedTokens tokens;

    using namespace comptoken;

    tokens.containerWidth =
        lengthOverride(overrides, plainKeys("date-picker.docked", "container.width"), 360.0);
    tokens.containerHeight =
        lengthOverride(overrides, plainKeys("date-picker.docked", "container.height"), 456.0);
    tokens.headerHeight =
        lengthOverride(overrides, plainKeys("date-picker.docked", "header.height"), 64.0);
    tokens.dateCellWidth =
        lengthOverride(overrides, plainKeys("date-picker.docked", "date.container.width"), 48.0);
    tokens.dateCellHeight =
        lengthOverride(overrides, plainKeys("date-picker.docked", "date.container.height"), 48.0);
    tokens.dateStateLayerWidth = lengthOverride(
        overrides, plainKeys("date-picker.docked", "date.state-layer.width"), 40.0);
    tokens.dateStateLayerHeight = lengthOverride(
        overrides, plainKeys("date-picker.docked", "date.state-layer.height"), 40.0);
    tokens.dateTodayOutlineWidth = lengthOverride(
        overrides, plainKeys("date-picker.docked", "date.today.container.outline.width"), 1.0);
    tokens.dateOutsideMonthLabelOpacity = lengthOverride(
        overrides,
        plainKeys("date-picker.docked", "date.unselected.outside-month.label-text.opacity"), 0.38);
    tokens.menuButtonHeight = lengthOverride(
        overrides, plainKeys("date-picker.docked", "menu-button.container.height"), 40.0);
    tokens.menuButtonIconSize =
        lengthOverride(overrides, plainKeys("date-picker.docked", "menu-button.icon.size"), 18.0);
    tokens.menuListItemHeight = lengthOverride(
        overrides, plainKeys("date-picker.docked", "menu.list-item.container.height"), 48.0);
    tokens.menuListSelectedIconSize = lengthOverride(
        overrides, plainKeys("date-picker.docked", "menu.list-item.selected.leading-icon.size"),
        24.0);

    tokens.hoverStateLayerOpacity =
        lengthOverride(overrides, plainKeys("date-picker.docked", "date.hover.state-layer.opacity"),
                       0.08);
    tokens.focusStateLayerOpacity =
        lengthOverride(overrides, plainKeys("date-picker.docked", "date.focus.state-layer.opacity"),
                       0.12);
    tokens.pressedStateLayerOpacity = lengthOverride(
        overrides, plainKeys("date-picker.docked", "date.pressed.state-layer.opacity"), 0.12);

    fillDateTables(&tokens);
    fillMenuButton(&tokens);
    fillMenuList(&tokens);

    return tokens;
}

} // namespace md
