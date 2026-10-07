#include "MdDynamicColor.h"

#include "MdColorSpec.h"
#include "MdTemperatureCache.h"

#include <QtCore/QVector>

#include <algorithm>
#include <cmath>
#include <vector>

namespace md {

namespace {

// SPDX-License-Identifier: Apache-2.0
// Transcribed from material-color-utilities cpp/ (utils, blend, dislike,
// temperature, scheme/*).

double rotationDirection(double from, double to)
{
    const double increasingDifference = MdColorMath::sanitizeDegreesDouble(to - from);
    return increasingDifference <= 180.0 ? 1.0 : -1.0;
}

double getRotatedHue(const MdHct &source, const QVector<double> &hues, const QVector<double> &rotations)
{
    const double sourceHue = source.hue();
    if (rotations.size() == 1) {
        return MdColorMath::sanitizeDegreesDouble(sourceHue + rotations[0]);
    }
    const int size = hues.size();
    for (int i = 0; i <= size - 2; ++i) {
        if (hues[i] < sourceHue && sourceHue < hues[i + 1]) {
            return MdColorMath::sanitizeDegreesDouble(sourceHue + rotations[i]);
        }
    }
    return sourceHue;
}

// dislike.cc used to live here; it is now md::fixIfDisliked in MdColorSpec.h,
// because the ColorSpec2021 role table needs it too (tertiary-container under
// the fidelity variant).

// --- variant palette construction, from cpp/scheme/scheme_*.cc ------------

const QVector<double> kVibrantHues = {0, 41, 61, 101, 131, 181, 251, 301, 360};
const QVector<double> kVibrantSecondaryRotations = {18, 15, 10, 12, 15, 18, 15, 12, 12};
const QVector<double> kVibrantTertiaryRotations = {35, 30, 20, 25, 30, 35, 30, 25, 25};

const QVector<double> kExpressiveHues = {0, 21, 51, 121, 151, 191, 271, 321, 360};
const QVector<double> kExpressiveSecondaryRotations = {45, 95, 45, 20, 45, 90, 45, 45, 45};
const QVector<double> kExpressiveTertiaryRotations = {120, 120, 20, 45, 20, 15, 20, 120, 120};

struct PaletteSet {
    MdTonalPalette primary;
    MdTonalPalette secondary;
    MdTonalPalette tertiary;
    MdTonalPalette neutral;
    MdTonalPalette neutralVariant;
};

PaletteSet buildPalettes(const MdHct &source, SchemeVariant variant)
{
    const double hue = source.hue();
    const double chroma = source.chroma();
    PaletteSet set;

    switch (variant) {
    case SchemeVariant::TonalSpot:
        set.primary = MdTonalPalette(hue, 36.0);
        set.secondary = MdTonalPalette(hue, 16.0);
        set.tertiary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 60.0), 24.0);
        set.neutral = MdTonalPalette(hue, 6.0);
        set.neutralVariant = MdTonalPalette(hue, 8.0);
        break;
    case SchemeVariant::Vibrant:
        set.primary = MdTonalPalette(hue, 200.0);
        set.secondary = MdTonalPalette(getRotatedHue(source, kVibrantHues, kVibrantSecondaryRotations), 24.0);
        set.tertiary = MdTonalPalette(getRotatedHue(source, kVibrantHues, kVibrantTertiaryRotations), 32.0);
        set.neutral = MdTonalPalette(hue, 10.0);
        set.neutralVariant = MdTonalPalette(hue, 12.0);
        break;
    case SchemeVariant::Expressive:
        set.primary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 240.0), 40.0);
        set.secondary = MdTonalPalette(getRotatedHue(source, kExpressiveHues, kExpressiveSecondaryRotations), 24.0);
        set.tertiary = MdTonalPalette(getRotatedHue(source, kExpressiveHues, kExpressiveTertiaryRotations), 32.0);
        set.neutral = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 15.0), 8.0);
        set.neutralVariant = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 15.0), 12.0);
        break;
    case SchemeVariant::Content: {
        set.primary = MdTonalPalette(hue, chroma);
        set.secondary = MdTonalPalette(hue, std::fmax(chroma - 32.0, chroma * 0.5));
        set.tertiary = MdTonalPalette(
            fixIfDisliked(MdTemperatureCache(source).analogous(3, 6).at(2)));
        set.neutral = MdTonalPalette(hue, chroma / 8.0);
        set.neutralVariant = MdTonalPalette(hue, chroma / 8.0 + 4.0);
        break;
    }
    case SchemeVariant::Fidelity:
        set.primary = MdTonalPalette(hue, chroma);
        set.secondary = MdTonalPalette(hue, std::fmax(chroma - 32.0, chroma * 0.5));
        set.tertiary = MdTonalPalette(
            fixIfDisliked(MdTemperatureCache(source).complement()));
        set.neutral = MdTonalPalette(hue, chroma / 8.0);
        set.neutralVariant = MdTonalPalette(hue, chroma / 8.0 + 4.0);
        break;
    case SchemeVariant::Monochrome:
        set.primary = MdTonalPalette(hue, 0.0);
        set.secondary = MdTonalPalette(hue, 0.0);
        set.tertiary = MdTonalPalette(hue, 0.0);
        set.neutral = MdTonalPalette(hue, 0.0);
        set.neutralVariant = MdTonalPalette(hue, 0.0);
        break;
    case SchemeVariant::Neutral:
        set.primary = MdTonalPalette(hue, 12.0);
        set.secondary = MdTonalPalette(hue, 8.0);
        set.tertiary = MdTonalPalette(hue, 16.0);
        set.neutral = MdTonalPalette(hue, 2.0);
        set.neutralVariant = MdTonalPalette(hue, 2.0);
        break;
    case SchemeVariant::Rainbow:
        set.primary = MdTonalPalette(hue, 48.0);
        set.secondary = MdTonalPalette(hue, 16.0);
        set.tertiary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue + 60.0), 24.0);
        set.neutral = MdTonalPalette(hue, 0.0);
        set.neutralVariant = MdTonalPalette(hue, 0.0);
        break;
    case SchemeVariant::FruitSalad:
        set.primary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue - 50.0), 48.0);
        set.secondary = MdTonalPalette(MdColorMath::sanitizeDegreesDouble(hue - 50.0), 36.0);
        set.tertiary = MdTonalPalette(hue, 36.0);
        set.neutral = MdTonalPalette(hue, 10.0);
        set.neutralVariant = MdTonalPalette(hue, 16.0);
        break;
    }
    return set;
}

} // namespace

// ---------------------------------------------------------------------------
// MdDynamicScheme
// ---------------------------------------------------------------------------

MdDynamicScheme MdDynamicScheme::create(Argb seed, ThemeMode mode, SchemeVariant variant, ContrastLevel contrast)
{
    MdDynamicScheme scheme;
    scheme.m_sourceColor = seed;
    scheme.m_sourceHct = MdHct(seed);
    scheme.m_mode = mode;
    scheme.m_variant = variant;
    scheme.m_contrast = contrast;

    const PaletteSet palettes = buildPalettes(scheme.m_sourceHct, variant);
    scheme.m_primary = palettes.primary;
    scheme.m_secondary = palettes.secondary;
    scheme.m_tertiary = palettes.tertiary;
    scheme.m_neutral = palettes.neutral;
    scheme.m_neutralVariant = palettes.neutralVariant;
    // The error palette is always hue 25 / chroma 84 (DynamicScheme's default).
    scheme.m_error = MdTonalPalette(25.0, 84.0);
    return scheme;
}

Argb MdDynamicScheme::color(ColorRole role) const
{
    // The tone table alone cannot express a contrast level; hand over to the
    // ColorSpec2021 solver, which re-solves every role against its background.
    return MdColorSpec2021::color(*this, role);
}

double MdDynamicScheme::resolvedTone(ColorRole role) const
{
    return MdColorSpec2021::tone(*this, role);
}

// ---------------------------------------------------------------------------
// MdDynamicColor
// ---------------------------------------------------------------------------

Argb MdDynamicColor::harmonize(Argb designColor, Argb sourceColor)
{
    MdHct fromHct(designColor);
    const MdHct toHct(sourceColor);
    const double differenceDegrees = MdColorMath::diffDegrees(fromHct.hue(), toHct.hue());
    const double rotationDegrees = std::min(differenceDegrees * 0.5, 15.0);
    const double outputHue = MdColorMath::sanitizeDegreesDouble(
        fromHct.hue() + rotationDegrees * rotationDirection(fromHct.hue(), toHct.hue()));
    fromHct.setHue(outputHue);
    return fromHct.toInt();
}

Argb MdDynamicColor::hctHue(Argb from, Argb to, double amount)
{
    // blend.cc: BlendCam16Ucs then BlendHctHue — interpolate in CAM16-UCS and
    // graft the blended hue onto the source colour.
    const MdCam16 fromCam = MdColorMath::camFromInt(from);
    const MdCam16 toCam = MdColorMath::camFromInt(to);
    const double jstar = fromCam.jstar + (toCam.jstar - fromCam.jstar) * amount;
    const double astar = fromCam.astar + (toCam.astar - fromCam.astar) * amount;
    const double bstar = fromCam.bstar + (toCam.bstar - fromCam.bstar) * amount;
    const Argb ucs = MdColorMath::intFromCam(MdColorMath::camFromUcs(jstar, astar, bstar));

    const MdHct ucsHct(ucs);
    MdHct fromHct(from);
    fromHct.setHue(ucsHct.hue());
    return fromHct.toInt();
}

QColor MdDynamicColor::roleColor(const QColor &seed,
                                 ThemeMode mode,
                                 ColorRole role,
                                 SchemeVariant variant,
                                 ContrastLevel contrast)
{
    const MdDynamicScheme scheme =
        MdDynamicScheme::create(MdColorMath::argbFromHex(seed.name(QColor::HexRgb)), mode, variant, contrast);
    return QColor::fromRgba(QRgb(scheme.color(role)));
}

} // namespace md
