#include "MdSliderTokens.h"

namespace md {

namespace {

/// The five Expressive size sets: `md.comp.slider.<size>.*` overriding the
/// base `md.comp.slider.*` rows. xsmall and small publish no icon rows.
struct SizeRow {
    qreal trackHeight;
    qreal handleHeight;
    qreal trackEndRadius;
    qreal iconSize;
    qreal iconPadding;
};

const SizeRow &sizeRow(MdSliderSize size)
{
    static const SizeRow rows[sliderSizeCount] = {
        {16.0, 44.0, 8.0, 0.0, 0.0},   // xsmall
        {24.0, 44.0, 8.0, 0.0, 0.0},   // small
        {40.0, 44.0, 12.0, 24.0, 6.0}, // medium
        {56.0, 68.0, 16.0, 24.0, 6.0}, // large
        {96.0, 108.0, 28.0, 32.0, 8.0} // xlarge
    };
    const int s = int(size);
    return rows[s >= 0 && s < sliderSizeCount ? s : 0];
}

/// The colour tables. Every enabled interaction resolves the same rows —
/// the export publishes `primary` for the focus / hover / pressed handle and
/// track rows alike — and the disabled rows carry their own opacities.
void fillColours(MdSliderTokens *tokens)
{
    for (int i = 0; i < sliderInteractionCount; ++i) {
        const bool disabled = MdSliderInteraction(i) == MdSliderInteraction::Disabled;

        tokens->activeTrack[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledActiveTrackOpacity}
            : MdNavigationColourSlot{ColorRole::Primary, 1.0};
        tokens->inactiveTrack[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledInactiveTrackOpacity}
            : MdNavigationColourSlot{ColorRole::SecondaryContainer, 1.0};
        tokens->handle[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledHandleOpacity}
            : MdNavigationColourSlot{ColorRole::Primary, 1.0};
        tokens->activeStopIndicator[i] = disabled
            ? MdNavigationColourSlot{ColorRole::InverseOnSurface,
                                     tokens->disabledStopIndicatorOpacity}
            : MdNavigationColourSlot{ColorRole::OnPrimary, 1.0};
        tokens->inactiveStopIndicator[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledStopIndicatorOpacity}
            : MdNavigationColourSlot{ColorRole::OnSecondaryContainer, 1.0};

        // The tick marks ride the deprecated with-tick-marks family at 0.38.
        tokens->activeTickMark[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledStopIndicatorOpacity}
            : MdNavigationColourSlot{ColorRole::OnPrimary, 0.38};
        tokens->inactiveTickMark[i] = disabled
            ? MdNavigationColourSlot{ColorRole::OnSurface, tokens->disabledStopIndicatorOpacity}
            : MdNavigationColourSlot{ColorRole::OnSurfaceVariant, 0.38};
    }
}

} // namespace

MdSliderTokens MdSliderTokens::resolve(MdSliderSize size)
{
    MdSliderTokens tokens;
    const SizeRow &row = sizeRow(size);

    tokens.trackHeight = row.trackHeight;
    tokens.handleHeight = row.handleHeight;
    tokens.trackEndRadius = row.trackEndRadius;
    tokens.iconSize = row.iconSize;
    tokens.iconPadding = row.iconPadding;

    fillColours(&tokens);
    return tokens;
}

} // namespace md
