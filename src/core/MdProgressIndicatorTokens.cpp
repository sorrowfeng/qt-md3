#include "MdProgressIndicatorTokens.h"

#include "MdCompTokenParse.h"

namespace md {

namespace {

using namespace comptoken;

} // namespace

MdProgressIndicatorTokens MdProgressIndicatorTokens::resolve(
    ProgressIndicatorShape shape, const MdComponentTokens *overrides)
{
    MdProgressIndicatorTokens tokens;

    if (shape == ProgressIndicatorShape::Linear) {
        // --- md.comp.progress-indicator.linear.* ----------------------------
        tokens.linearHeight =
            lengthOverride(overrides, plainKeys("progress-indicator.linear", "height"),
                           tokens.linearHeight);
        tokens.linearActiveIndicatorThickness = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear", "active-indicator.thickness"),
            tokens.linearActiveIndicatorThickness);
        tokens.linearTrackThickness =
            lengthOverride(overrides,
                           plainKeys("progress-indicator.linear", "track.thickness"),
                           tokens.linearTrackThickness);
        tokens.linearTrackActiveIndicatorSpace = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear", "track-active-indicator-space"),
            tokens.linearTrackActiveIndicatorSpace);
        tokens.linearStopIndicatorSize =
            lengthOverride(overrides,
                           plainKeys("progress-indicator.linear", "stop-indicator.size"),
                           tokens.linearStopIndicatorSize);
        tokens.linearStopIndicatorTrailingSpace = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear", "stop-indicator.trailing-space"),
            tokens.linearStopIndicatorTrailingSpace);
        tokens.linearWaveAmplitude = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear", "active-indicator.wave.amplitude"),
            tokens.linearWaveAmplitude);
        tokens.linearWaveWavelength = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear", "active-indicator.wave.wavelength"),
            tokens.linearWaveWavelength);
        tokens.linearIndeterminateWaveWavelength = lengthOverride(
            overrides,
            plainKeys("progress-indicator.linear",
                      "indeterminate.active-indicator.wave.wavelength"),
            tokens.linearIndeterminateWaveWavelength);
        tokens.linearWithWaveHeight =
            lengthOverride(overrides,
                           plainKeys("progress-indicator.linear", "with-wave.height"),
                           tokens.linearWithWaveHeight);
        return tokens;
    }

    // --- md.comp.progress-indicator.circular.* ------------------------------
    tokens.circularSize =
        lengthOverride(overrides, plainKeys("progress-indicator.circular", "size"),
                       tokens.circularSize);
    tokens.circularActiveIndicatorThickness = lengthOverride(
        overrides,
        plainKeys("progress-indicator.circular", "active-indicator.thickness"),
        tokens.circularActiveIndicatorThickness);
    tokens.circularTrackThickness =
        lengthOverride(overrides,
                       plainKeys("progress-indicator.circular", "track.thickness"),
                       tokens.circularTrackThickness);
    tokens.circularTrackActiveIndicatorSpace = lengthOverride(
        overrides,
        plainKeys("progress-indicator.circular", "track-active-indicator-space"),
        tokens.circularTrackActiveIndicatorSpace);
    tokens.circularWaveAmplitude = lengthOverride(
        overrides,
        plainKeys("progress-indicator.circular", "active-indicator.wave.amplitude"),
        tokens.circularWaveAmplitude);
    tokens.circularWaveWavelength = lengthOverride(
        overrides,
        plainKeys("progress-indicator.circular", "active-indicator.wave.wavelength"),
        tokens.circularWaveWavelength);
    tokens.circularWithWaveSize =
        lengthOverride(overrides,
                       plainKeys("progress-indicator.circular", "with-wave.size"),
                       tokens.circularWithWaveSize);
    return tokens;
}

} // namespace md
