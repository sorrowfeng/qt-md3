#include "MdDatePickerTokens.h"

namespace md {

namespace {

/// The date / year colour tables. The export publishes the selected /
/// unselected rows across Enabled / Hovered / Focused / Pressed — the hover /
/// focus / pressed label rows all resolve the resting colour — and no
/// disabled rows at all.
void fillTables(MdDatePickerTokens *tokens)
{
    for (int i = 0; i < datePickerStateCount; ++i) {
        const bool disabled = MdDatePickerState(i) == MdDatePickerState::Disabled;

        tokens->dateSelected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::Primary, 1.0};
        tokens->dateUnselected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::OnSurface, 1.0};
        tokens->yearSelected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::Primary, 1.0};
        tokens->yearUnselected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::OnSurfaceVariant, 1.0};
    }
}

} // namespace

MdDatePickerTokens MdDatePickerTokens::resolve()
{
    MdDatePickerTokens tokens;
    fillTables(&tokens);
    return tokens;
}

} // namespace md
