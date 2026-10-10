#include "MdTimePickerTokens.h"

namespace md {

namespace {

/// The two selector families' colour tables. The export publishes the
/// selected / unselected rows across Enabled / Hovered / Focused / Pressed —
/// the hover / focus / pressed label rows all resolve the resting colour, so
/// the only thing the states change is the state-layer colour — and no
/// disabled rows at all.
void fillSelectors(MdTimePickerTokens *tokens)
{
    for (int i = 0; i < timeSelectorStateCount; ++i) {
        const MdTimeSelectorState state = MdTimeSelectorState(i);
        const bool disabled = state == MdTimeSelectorState::Disabled;

        // Time selectors: primary-container / on-primary-container selected,
        // surface-container-highest / on-surface unselected.
        tokens->timeSelected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::PrimaryContainer, 1.0};
        tokens->timeUnselected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::SurfaceContainerHighest, 1.0};

        // Period selectors: tertiary-container / on-tertiary-container
        // selected, transparent / on-surface-variant unselected.
        tokens->periodSelected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::TertiaryContainer, 1.0};
        tokens->periodUnselected[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, 0.38}
            : MdNavigationColourSlot{ColorRole::OnSurfaceVariant, 1.0};
    }
}

} // namespace

MdTimePickerTokens MdTimePickerTokens::resolve()
{
    MdTimePickerTokens tokens;
    fillSelectors(&tokens);
    return tokens;
}

} // namespace md
