#include "MdColorSpec.h"

#include <algorithm>
#include <cmath>

namespace md {

namespace {

// MathUtils.lerp — spelled exactly as upstream so the interpolation of a
// contrast curve is bit-identical.
double lerp(double start, double stop, double amount)
{
    return (1.0 - amount) * start + amount * stop;
}

double clampDouble(double min, double max, double input)
{
    if (input < min) {
        return min;
    }
    if (input > max) {
        return max;
    }
    return input;
}

// ColorSpec2021's constants, named rather than inlined so the provenance is
// visible at each use.
constexpr double kContrastRatioEpsilon = 0.04;
constexpr double kLuminanceGamutMapTolerance = 0.4;

} // namespace

// ---------------------------------------------------------------------------
// DislikeAnalyzer
// ---------------------------------------------------------------------------

bool isDislikedHct(const MdHct &hct)
{
    const double roundedHue = std::round(hct.hue());
    return roundedHue >= 90.0 && roundedHue <= 111.0 && std::round(hct.chroma()) > 16.0
           && std::round(hct.tone()) < 65.0;
}

MdHct fixIfDisliked(const MdHct &hct)
{
    return isDislikedHct(hct) ? MdHct(hct.hue(), hct.chroma(), 70.0) : hct;
}

// ---------------------------------------------------------------------------
// MdContrastCurve
// ---------------------------------------------------------------------------

double MdContrastCurve::get(double contrastLevel) const
{
    if (contrastLevel <= -1.0) {
        return low;
    }
    if (contrastLevel < 0.0) {
        return lerp(low, normal, contrastLevel - -1.0);
    }
    if (contrastLevel < 0.5) {
        return lerp(normal, medium, (contrastLevel - 0.0) / 0.5);
    }
    if (contrastLevel < 1.0) {
        return lerp(medium, high, (contrastLevel - 0.5) / 0.5);
    }
    return high;
}

double contrastLevelValue(ContrastLevel level)
{
    switch (level) {
    case ContrastLevel::Standard:
        return 0.0;
    case ContrastLevel::Medium:
        return 0.5;
    case ContrastLevel::High:
        return 1.0;
    case ContrastLevel::Reduced:
        return -1.0;
    case ContrastLevel::Count:
        break;
    }
    return 0.0;
}

// ---------------------------------------------------------------------------
// MdContrast
// ---------------------------------------------------------------------------

double MdContrast::ratioOfYs(double y1, double y2)
{
    const double lighter = std::max(y1, y2);
    const double darker = (lighter == y2) ? y1 : y2;
    return (lighter + 5.0) / (darker + 5.0);
}

double MdContrast::ratioOfTones(double t1, double t2)
{
    return ratioOfYs(MdColorMath::yFromLstar(t1), MdColorMath::yFromLstar(t2));
}

double MdContrast::lighter(double tone, double ratio)
{
    if (tone < 0.0 || tone > 100.0) {
        return -1.0;
    }
    // Invert the contrast-ratio equation to get the lighter Y.
    const double darkY = MdColorMath::yFromLstar(tone);
    const double lightY = ratio * (darkY + 5.0) - 5.0;
    if (lightY < 0.0 || lightY > 100.0) {
        return -1.0;
    }
    const double realContrast = ratioOfYs(lightY, darkY);
    const double delta = std::abs(realContrast - ratio);
    if (realContrast < ratio && delta > kContrastRatioEpsilon) {
        return -1.0;
    }

    const double returnValue = MdColorMath::lstarFromY(lightY) + kLuminanceGamutMapTolerance;
    if (returnValue < 0.0 || returnValue > 100.0) {
        return -1.0;
    }
    return returnValue;
}

double MdContrast::darker(double tone, double ratio)
{
    if (tone < 0.0 || tone > 100.0) {
        return -1.0;
    }
    const double lightY = MdColorMath::yFromLstar(tone);
    const double darkY = ((lightY + 5.0) / ratio) - 5.0;
    if (darkY < 0.0 || darkY > 100.0) {
        return -1.0;
    }
    const double realContrast = ratioOfYs(lightY, darkY);
    const double delta = std::abs(realContrast - ratio);
    if (realContrast < ratio && delta > kContrastRatioEpsilon) {
        return -1.0;
    }

    const double returnValue = MdColorMath::lstarFromY(darkY) - kLuminanceGamutMapTolerance;
    if (returnValue < 0.0 || returnValue > 100.0) {
        return -1.0;
    }
    return returnValue;
}

double MdContrast::lighterUnsafe(double tone, double ratio)
{
    const double lighterSafe = lighter(tone, ratio);
    return lighterSafe < 0.0 ? 100.0 : lighterSafe;
}

double MdContrast::darkerUnsafe(double tone, double ratio)
{
    const double darkerSafe = darker(tone, ratio);
    return std::max(0.0, darkerSafe);
}

double MdContrast::foregroundTone(double bgTone, double ratio)
{
    const double lighterTone = lighterUnsafe(bgTone, ratio);
    const double darkerTone = darkerUnsafe(bgTone, ratio);
    const double lighterRatio = ratioOfTones(lighterTone, bgTone);
    const double darkerRatio = ratioOfTones(darkerTone, bgTone);
    const bool preferLighter = tonePrefersLightForeground(bgTone);

    if (preferLighter) {
        // "Negligible difference" covers the case where the incoming ratio is
        // already higher than both options can reach, so neither passes and the
        // choice would otherwise flip on noise.
        const bool negligibleDifference = std::abs(lighterRatio - darkerRatio) < 0.1
                                          && lighterRatio < ratio && darkerRatio < ratio;
        if (lighterRatio >= ratio || lighterRatio >= darkerRatio || negligibleDifference) {
            return lighterTone;
        }
        return darkerTone;
    }
    return (darkerRatio >= ratio || darkerRatio >= lighterRatio) ? darkerTone : lighterTone;
}

bool MdContrast::tonePrefersLightForeground(double tone)
{
    // DynamicColor.tonePrefersLightForeground: round(tone) < 60. Note this is a
    // *different* threshold from MdColorMath::tonePrefersLightForeground, which
    // answers the narrower "is this surface dark enough for white text"
    // question at 49.5. Both are upstream values; do not merge them.
    return std::round(tone) < 60.0;
}

bool MdContrast::toneAllowsLightForeground(double tone)
{
    return std::round(tone) <= 49.0;
}

double MdContrast::enableLightForeground(double tone)
{
    if (tonePrefersLightForeground(tone) && !toneAllowsLightForeground(tone)) {
        return 49.0;
    }
    return tone;
}

// ---------------------------------------------------------------------------
// The ColorSpec2021 role table
// ---------------------------------------------------------------------------

namespace {

enum class Slot { Primary, Secondary, Tertiary, Neutral, NeutralVariant, Error };

/// The static half of a role definition: everything except its tone, which also
/// depends on the variant and the source colour.
struct RoleSpec
{
    Slot slot = Slot::Primary;
    bool isBackground = false;
    /// ColorRole::Count means "this role has no background".
    ColorRole background = ColorRole::Count;
    ColorRole secondBackground = ColorRole::Count;
    bool hasCurve = false;
    MdContrastCurve curve;
    bool hasDeltaPair = false;
    MdToneDeltaPair deltaPair;
};

/// Shorthand for the six `ContrastCurve`s the spec reuses.
constexpr MdContrastCurve kCurveOnSurface{4.5, 7.0, 11.0, 21.0};
constexpr MdContrastCurve kCurveOnSurfaceVariant{3.0, 4.5, 7.0, 11.0};
constexpr MdContrastCurve kCurveContainer{1.0, 1.0, 3.0, 4.5};
constexpr MdContrastCurve kCurveAccent{3.0, 4.5, 7.0, 7.0};
constexpr MdContrastCurve kCurveOutline{1.5, 3.0, 4.5, 7.0};
constexpr MdContrastCurve kCurveOutlineVariant{1.0, 1.0, 3.0, 4.5};
constexpr MdContrastCurve kCurveOnBackground{3.0, 3.0, 4.5, 7.0};

/// A container/on-container pair anchored to an accent role. All four accent
/// families in the spec use the same shape.
MdToneDeltaPair containerPair(ColorRole container, ColorRole accent)
{
    MdToneDeltaPair pair;
    pair.roleA = container;
    pair.roleB = accent;
    pair.delta = 10.0;
    pair.polarity = MdTonePolarity::Nearer;
    pair.stayTogether = false;
    pair.constraint = MdDeltaConstraint::Exact;
    return pair;
}

/// The `*-fixed` families pair a tone with its dimmed variant and must stay
/// together, so the pair is symmetric rather than directional.
MdToneDeltaPair fixedPair(ColorRole base, ColorRole dim)
{
    MdToneDeltaPair pair;
    pair.roleA = base;
    pair.roleB = dim;
    pair.delta = 10.0;
    pair.polarity = MdTonePolarity::Lighter;
    pair.stayTogether = true;
    pair.constraint = MdDeltaConstraint::Exact;
    return pair;
}

/// highestSurface(s) — the tonal surface a role is measured against.
ColorRole highestSurface(bool isDark)
{
    return isDark ? ColorRole::SurfaceBright : ColorRole::SurfaceDim;
}

RoleSpec specFor(ColorRole role, bool isDark)
{
    RoleSpec spec;
    const ColorRole high = highestSurface(isDark);
    const auto withCurve = [&spec](MdContrastCurve curve) -> RoleSpec & {
        spec.hasCurve = true;
        spec.curve = curve;
        return spec;
    };
    const auto withBackground = [&spec](ColorRole background) -> RoleSpec & {
        spec.background = background;
        return spec;
    };
    const auto withPair = [&spec](MdToneDeltaPair pair) -> RoleSpec & {
        spec.hasDeltaPair = true;
        spec.deltaPair = pair;
        return spec;
    };

    switch (role) {
    // --- Surfaces ------------------------------------------------------
    case ColorRole::Background:
        spec.slot = Slot::Neutral;
        spec.isBackground = true;
        break;
    case ColorRole::OnBackground:
        spec.slot = Slot::Neutral;
        withCurve(kCurveOnBackground);
        withBackground(ColorRole::Background);
        break;
    case ColorRole::Surface:
        spec.slot = Slot::Neutral;
        spec.isBackground = true;
        break;
    case ColorRole::SurfaceDim:
        spec.slot = Slot::Neutral;
        spec.isBackground = true;
        break;
    case ColorRole::SurfaceBright:
        spec.slot = Slot::Neutral;
        spec.isBackground = true;
        break;
    case ColorRole::SurfaceContainerLowest:
    case ColorRole::SurfaceContainerLow:
    case ColorRole::SurfaceContainer:
    case ColorRole::SurfaceContainerHigh:
    case ColorRole::SurfaceContainerHighest:
    case ColorRole::SurfaceVariant:
    case ColorRole::InverseSurface:
    case ColorRole::SurfaceTint:
        spec.slot = (role == ColorRole::SurfaceVariant)          ? Slot::NeutralVariant
                    : (role == ColorRole::SurfaceTint)           ? Slot::Primary
                                                                 : Slot::Neutral;
        spec.isBackground = true;
        break;
    case ColorRole::OnSurface:
        spec.slot = Slot::Neutral;
        withCurve(kCurveOnSurface);
        withBackground(high);
        break;
    case ColorRole::OnSurfaceVariant:
        spec.slot = Slot::NeutralVariant;
        withCurve(kCurveOnSurfaceVariant);
        withBackground(high);
        break;
    case ColorRole::InverseOnSurface:
        spec.slot = Slot::Neutral;
        withCurve(kCurveOnSurface);
        withBackground(ColorRole::InverseSurface);
        break;
    case ColorRole::Outline:
        spec.slot = Slot::NeutralVariant;
        withCurve(kCurveOutline);
        withBackground(high);
        break;
    case ColorRole::OutlineVariant:
        spec.slot = Slot::NeutralVariant;
        withCurve(kCurveOutlineVariant);
        withBackground(high);
        break;
    case ColorRole::Shadow:
    case ColorRole::Scrim:
        spec.slot = Slot::Neutral;
        break;

    // --- Primaries -----------------------------------------------------
    case ColorRole::Primary:
        spec.slot = Slot::Primary;
        spec.isBackground = true;
        withCurve(kCurveAccent);
        withBackground(high);
        withPair(containerPair(ColorRole::PrimaryContainer, ColorRole::Primary));
        break;
    case ColorRole::OnPrimary:
        spec.slot = Slot::Primary;
        withCurve(kCurveOnSurface);
        withBackground(ColorRole::Primary);
        break;
    case ColorRole::PrimaryContainer:
        spec.slot = Slot::Primary;
        spec.isBackground = true;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(containerPair(ColorRole::PrimaryContainer, ColorRole::Primary));
        break;
    case ColorRole::OnPrimaryContainer:
        spec.slot = Slot::Primary;
        withCurve(kCurveOnSurfaceVariant);
        withBackground(ColorRole::PrimaryContainer);
        break;
    case ColorRole::InversePrimary:
        spec.slot = Slot::Primary;
        withCurve(kCurveAccent);
        withBackground(ColorRole::InverseSurface);
        break;

    // --- Secondaries ---------------------------------------------------
    case ColorRole::Secondary:
        spec.slot = Slot::Secondary;
        spec.isBackground = true;
        withCurve(kCurveAccent);
        withBackground(high);
        withPair(containerPair(ColorRole::SecondaryContainer, ColorRole::Secondary));
        break;
    case ColorRole::OnSecondary:
        spec.slot = Slot::Secondary;
        withCurve(kCurveOnSurface);
        withBackground(ColorRole::Secondary);
        break;
    case ColorRole::SecondaryContainer:
        spec.slot = Slot::Secondary;
        spec.isBackground = true;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(containerPair(ColorRole::SecondaryContainer, ColorRole::Secondary));
        break;
    case ColorRole::OnSecondaryContainer:
        spec.slot = Slot::Secondary;
        withCurve(kCurveOnSurfaceVariant);
        withBackground(ColorRole::SecondaryContainer);
        break;

    // --- Tertiaries ----------------------------------------------------
    case ColorRole::Tertiary:
        spec.slot = Slot::Tertiary;
        spec.isBackground = true;
        withCurve(kCurveAccent);
        withBackground(high);
        withPair(containerPair(ColorRole::TertiaryContainer, ColorRole::Tertiary));
        break;
    case ColorRole::OnTertiary:
        spec.slot = Slot::Tertiary;
        withCurve(kCurveOnSurface);
        withBackground(ColorRole::Tertiary);
        break;
    case ColorRole::TertiaryContainer:
        spec.slot = Slot::Tertiary;
        spec.isBackground = true;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(containerPair(ColorRole::TertiaryContainer, ColorRole::Tertiary));
        break;
    case ColorRole::OnTertiaryContainer:
        spec.slot = Slot::Tertiary;
        withCurve(kCurveOnSurfaceVariant);
        withBackground(ColorRole::TertiaryContainer);
        break;

    // --- Errors --------------------------------------------------------
    case ColorRole::Error:
        spec.slot = Slot::Error;
        spec.isBackground = true;
        withCurve(kCurveAccent);
        withBackground(high);
        withPair(containerPair(ColorRole::ErrorContainer, ColorRole::Error));
        break;
    case ColorRole::OnError:
        spec.slot = Slot::Error;
        withCurve(kCurveOnSurface);
        withBackground(ColorRole::Error);
        break;
    case ColorRole::ErrorContainer:
        spec.slot = Slot::Error;
        spec.isBackground = true;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(containerPair(ColorRole::ErrorContainer, ColorRole::Error));
        break;
    case ColorRole::OnErrorContainer:
        spec.slot = Slot::Error;
        withCurve(kCurveOnSurfaceVariant);
        withBackground(ColorRole::ErrorContainer);
        break;

    // --- Fixed families ------------------------------------------------
    case ColorRole::PrimaryFixed:
    case ColorRole::SecondaryFixed:
    case ColorRole::TertiaryFixed: {
        spec.slot = (role == ColorRole::PrimaryFixed)     ? Slot::Primary
                    : (role == ColorRole::SecondaryFixed) ? Slot::Secondary
                                                          : Slot::Tertiary;
        spec.isBackground = true;
        const ColorRole dim = (role == ColorRole::PrimaryFixed) ? ColorRole::PrimaryFixedDim
                              : (role == ColorRole::SecondaryFixed)
                                  ? ColorRole::SecondaryFixedDim
                                  : ColorRole::TertiaryFixedDim;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(fixedPair(role, dim));
        break;
    }
    case ColorRole::PrimaryFixedDim:
    case ColorRole::SecondaryFixedDim:
    case ColorRole::TertiaryFixedDim: {
        spec.slot = (role == ColorRole::PrimaryFixedDim)     ? Slot::Primary
                    : (role == ColorRole::SecondaryFixedDim) ? Slot::Secondary
                                                             : Slot::Tertiary;
        spec.isBackground = true;
        const ColorRole base = (role == ColorRole::PrimaryFixedDim) ? ColorRole::PrimaryFixed
                               : (role == ColorRole::SecondaryFixedDim)
                                   ? ColorRole::SecondaryFixed
                                   : ColorRole::TertiaryFixed;
        withCurve(kCurveContainer);
        withBackground(high);
        withPair(fixedPair(base, role));
        break;
    }
    case ColorRole::OnPrimaryFixed:
    case ColorRole::OnSecondaryFixed:
    case ColorRole::OnTertiaryFixed: {
        const bool primary = role == ColorRole::OnPrimaryFixed;
        const bool secondary = role == ColorRole::OnSecondaryFixed;
        spec.slot = primary ? Slot::Primary : (secondary ? Slot::Secondary : Slot::Tertiary);
        withCurve(kCurveOnSurface);
        withBackground(primary ? ColorRole::PrimaryFixedDim
                               : (secondary ? ColorRole::SecondaryFixedDim
                                            : ColorRole::TertiaryFixedDim));
        spec.secondBackground = primary ? ColorRole::PrimaryFixed
                                : (secondary ? ColorRole::SecondaryFixed
                                             : ColorRole::TertiaryFixed);
        break;
    }
    case ColorRole::OnPrimaryFixedVariant:
    case ColorRole::OnSecondaryFixedVariant:
    case ColorRole::OnTertiaryFixedVariant: {
        const bool primary = role == ColorRole::OnPrimaryFixedVariant;
        const bool secondary = role == ColorRole::OnSecondaryFixedVariant;
        spec.slot = primary ? Slot::Primary : (secondary ? Slot::Secondary : Slot::Tertiary);
        withCurve(kCurveOnSurfaceVariant);
        withBackground(primary ? ColorRole::PrimaryFixedDim
                               : (secondary ? ColorRole::SecondaryFixedDim
                                            : ColorRole::TertiaryFixedDim));
        spec.secondBackground = primary ? ColorRole::PrimaryFixed
                                : (secondary ? ColorRole::SecondaryFixed
                                             : ColorRole::TertiaryFixed);
        break;
    }

    case ColorRole::Count:
        break;
    }
    return spec;
}

/// ColorSpec2021.findDesiredChromaByTone — walks away from `tone` while the
/// palette can still carry the requested chroma. Only the fidelity and content
/// variants need it.
double findDesiredChromaByTone(double hue, double chroma, double tone, bool byDecreasingTone)
{
    double answer = tone;

    MdHct closestToChroma(hue, chroma, tone);
    if (closestToChroma.chroma() < chroma) {
        double chromaPeak = closestToChroma.chroma();
        while (closestToChroma.chroma() < chroma) {
            answer += byDecreasingTone ? -1.0 : 1.0;
            const MdHct potentialSolution(hue, chroma, answer);
            if (chromaPeak > potentialSolution.chroma()) {
                break;
            }
            if (std::abs(potentialSolution.chroma() - chroma) < 0.4) {
                break;
            }

            const double potentialDelta = std::abs(potentialSolution.chroma() - chroma);
            const double currentDelta = std::abs(closestToChroma.chroma() - chroma);
            if (potentialDelta < currentDelta) {
                closestToChroma = potentialSolution;
            }
            chromaPeak = std::max(chromaPeak, potentialSolution.chroma());
        }
    }
    return answer;
}

/// The role table's tone function, evaluated for one scheme. This is the
/// `.setTone(...)` lambda from ColorSpec2021, in full.
double rawToneOf(const MdDynamicScheme &scheme, ColorRole role)
{
    const bool isDark = scheme.isDark();
    const double contrast = contrastLevelValue(scheme.contrastLevel());
    const bool monochrome = scheme.variant() == SchemeVariant::Monochrome;
    const bool fidelity = scheme.variant() == SchemeVariant::Fidelity
                          || scheme.variant() == SchemeVariant::Content;
    const double sourceTone = scheme.sourceHct().tone();

    switch (role) {
    case ColorRole::Background:
    case ColorRole::Surface:
        return isDark ? 6.0 : 98.0;
    case ColorRole::OnBackground:
        return isDark ? 90.0 : 10.0;
    case ColorRole::SurfaceDim:
        return isDark ? 6.0 : MdContrastCurve(87.0, 87.0, 80.0, 75.0).get(contrast);
    case ColorRole::SurfaceBright:
        return isDark ? MdContrastCurve(24.0, 24.0, 29.0, 34.0).get(contrast) : 98.0;
    case ColorRole::SurfaceContainerLowest:
        return isDark ? MdContrastCurve(4.0, 4.0, 2.0, 0.0).get(contrast) : 100.0;
    case ColorRole::SurfaceContainerLow:
        return isDark ? MdContrastCurve(10.0, 10.0, 11.0, 12.0).get(contrast)
                      : MdContrastCurve(96.0, 96.0, 96.0, 95.0).get(contrast);
    case ColorRole::SurfaceContainer:
        return isDark ? MdContrastCurve(12.0, 12.0, 16.0, 20.0).get(contrast)
                      : MdContrastCurve(94.0, 94.0, 92.0, 90.0).get(contrast);
    case ColorRole::SurfaceContainerHigh:
        return isDark ? MdContrastCurve(17.0, 17.0, 21.0, 25.0).get(contrast)
                      : MdContrastCurve(92.0, 92.0, 88.0, 85.0).get(contrast);
    case ColorRole::SurfaceContainerHighest:
        return isDark ? MdContrastCurve(22.0, 22.0, 26.0, 30.0).get(contrast)
                      : MdContrastCurve(90.0, 90.0, 84.0, 80.0).get(contrast);
    case ColorRole::OnSurface:
        return isDark ? 90.0 : 10.0;
    case ColorRole::SurfaceVariant:
        return isDark ? 30.0 : 90.0;
    case ColorRole::OnSurfaceVariant:
        return isDark ? 80.0 : 30.0;
    case ColorRole::InverseSurface:
        return isDark ? 90.0 : 20.0;
    case ColorRole::InverseOnSurface:
        return isDark ? 20.0 : 95.0;
    case ColorRole::Outline:
        return isDark ? 60.0 : 50.0;
    case ColorRole::OutlineVariant:
        return isDark ? 30.0 : 80.0;
    case ColorRole::Shadow:
    case ColorRole::Scrim:
        return 0.0;
    case ColorRole::SurfaceTint:
        return isDark ? 80.0 : 40.0;

    case ColorRole::Primary:
        return monochrome ? (isDark ? 100.0 : 0.0) : (isDark ? 80.0 : 40.0);
    case ColorRole::OnPrimary:
        return monochrome ? (isDark ? 10.0 : 90.0) : (isDark ? 20.0 : 100.0);
    case ColorRole::PrimaryContainer:
        if (fidelity) {
            return sourceTone;
        }
        if (monochrome) {
            return isDark ? 85.0 : 25.0;
        }
        return isDark ? 30.0 : 90.0;
    case ColorRole::OnPrimaryContainer:
        if (fidelity) {
            return MdContrast::foregroundTone(rawToneOf(scheme, ColorRole::PrimaryContainer), 4.5);
        }
        if (monochrome) {
            return isDark ? 0.0 : 100.0;
        }
        return isDark ? 90.0 : 30.0;
    case ColorRole::InversePrimary:
        return isDark ? 40.0 : 80.0;

    case ColorRole::Secondary:
        return isDark ? 80.0 : 40.0;
    case ColorRole::OnSecondary:
        return monochrome ? (isDark ? 10.0 : 100.0) : (isDark ? 20.0 : 100.0);
    case ColorRole::SecondaryContainer: {
        const double initialTone = isDark ? 30.0 : 90.0;
        if (monochrome) {
            return isDark ? 30.0 : 85.0;
        }
        if (!fidelity) {
            return initialTone;
        }
        return findDesiredChromaByTone(scheme.secondaryPalette().hue(),
                                       scheme.secondaryPalette().chroma(),
                                       initialTone,
                                       !isDark);
    }
    case ColorRole::OnSecondaryContainer:
        if (monochrome) {
            return isDark ? 90.0 : 10.0;
        }
        if (!fidelity) {
            return isDark ? 90.0 : 30.0;
        }
        return MdContrast::foregroundTone(rawToneOf(scheme, ColorRole::SecondaryContainer), 4.5);

    case ColorRole::Tertiary:
        return monochrome ? (isDark ? 90.0 : 25.0) : (isDark ? 80.0 : 40.0);
    case ColorRole::OnTertiary:
        return monochrome ? (isDark ? 10.0 : 90.0) : (isDark ? 20.0 : 100.0);
    case ColorRole::TertiaryContainer: {
        if (monochrome) {
            return isDark ? 60.0 : 49.0;
        }
        if (!fidelity) {
            return isDark ? 30.0 : 90.0;
        }
        const MdTonalPalette &palette = scheme.tertiaryPalette();
        const MdHct proposed(palette.hue(), palette.chroma(), sourceTone);
        return fixIfDisliked(proposed).tone();
    }
    case ColorRole::OnTertiaryContainer:
        if (monochrome) {
            return isDark ? 0.0 : 100.0;
        }
        if (!fidelity) {
            return isDark ? 90.0 : 30.0;
        }
        return MdContrast::foregroundTone(rawToneOf(scheme, ColorRole::TertiaryContainer), 4.5);

    case ColorRole::Error:
        return isDark ? 80.0 : 40.0;
    case ColorRole::OnError:
        return isDark ? 20.0 : 100.0;
    case ColorRole::ErrorContainer:
        return isDark ? 30.0 : 90.0;
    case ColorRole::OnErrorContainer:
        return monochrome ? (isDark ? 90.0 : 10.0) : (isDark ? 90.0 : 30.0);

    case ColorRole::PrimaryFixed:
        return monochrome ? 40.0 : 90.0;
    case ColorRole::PrimaryFixedDim:
        return monochrome ? 30.0 : 80.0;
    case ColorRole::OnPrimaryFixed:
        return monochrome ? 100.0 : 10.0;
    case ColorRole::OnPrimaryFixedVariant:
        return monochrome ? 90.0 : 30.0;

    case ColorRole::SecondaryFixed:
        return monochrome ? 80.0 : 90.0;
    case ColorRole::SecondaryFixedDim:
        return monochrome ? 70.0 : 80.0;
    case ColorRole::OnSecondaryFixed:
        return 10.0;
    case ColorRole::OnSecondaryFixedVariant:
        return monochrome ? 25.0 : 30.0;

    case ColorRole::TertiaryFixed:
        return monochrome ? 40.0 : 90.0;
    case ColorRole::TertiaryFixedDim:
        return monochrome ? 30.0 : 80.0;
    case ColorRole::OnTertiaryFixed:
        return monochrome ? 100.0 : 10.0;
    case ColorRole::OnTertiaryFixedVariant:
        return monochrome ? 90.0 : 30.0;

    case ColorRole::Count:
        break;
    }
    return 0.0;
}

const MdTonalPalette &paletteOf(const MdDynamicScheme &scheme, Slot slot)
{
    switch (slot) {
    case Slot::Primary:
        return scheme.primaryPalette();
    case Slot::Secondary:
        return scheme.secondaryPalette();
    case Slot::Tertiary:
        return scheme.tertiaryPalette();
    case Slot::Neutral:
        return scheme.neutralPalette();
    case Slot::NeutralVariant:
        return scheme.neutralVariantPalette();
    case Slot::Error:
        return scheme.errorPalette();
    }
    return scheme.neutralPalette();
}

/// ColorSpec2021.getTone, restated over qt-md3's role enum.
///
/// The recursion is real and bounded: a role looks up its background's tone,
/// and a background never looks back at the role that referenced it. The depth
/// is at most three (e.g. on-primary-container -> primary-container ->
/// surface-dim).
class Solver
{
public:
    explicit Solver(const MdDynamicScheme &scheme)
        : m_scheme(scheme)
    {
    }

    double tone(ColorRole role) const
    {
        const double contrastLevel = contrastLevelValue(m_scheme.contrastLevel());
        const bool decreasingContrast = contrastLevel < 0.0;
        const bool isDark = m_scheme.isDark();
        const RoleSpec spec = specFor(role, isDark);

        // --- Case 1: a tone-delta pair ---------------------------------
        if (spec.hasDeltaPair) {
            const MdToneDeltaPair &pair = spec.deltaPair;
            const MdTonePolarity polarity = pair.polarity;

            // Transcribed as written upstream, including the second clause
            // repeating `!scheme.isDark`. It reads like it should be
            // `DARKER && scheme.isDark`, but the published md.sys.color table
            // only comes out right with the upstream spelling, so it is kept
            // verbatim rather than "corrected".
            const bool aIsNearer = polarity == MdTonePolarity::Nearer
                                   || (polarity == MdTonePolarity::Lighter && !isDark)
                                   || (polarity == MdTonePolarity::Darker && !isDark);
            const ColorRole nearer = aIsNearer ? pair.roleA : pair.roleB;
            const ColorRole farther = aIsNearer ? pair.roleB : pair.roleA;
            const bool amNearer = role == nearer;
            const double expansionDir = isDark ? 1.0 : -1.0;

            double nTone = rawToneOf(m_scheme, nearer);
            double fTone = rawToneOf(m_scheme, farther);

            const RoleSpec nearerSpec = specFor(nearer, isDark);
            const RoleSpec fartherSpec = specFor(farther, isDark);

            // 1st round: solve each against the shared background.
            if (spec.background != ColorRole::Count && nearerSpec.hasCurve
                && fartherSpec.hasCurve) {
                const double bgTone = tone(spec.background);
                const double nContrast = nearerSpec.curve.get(contrastLevel);
                const double fContrast = fartherSpec.curve.get(contrastLevel);

                // A colour that is already good enough is left alone.
                if (MdContrast::ratioOfTones(bgTone, nTone) < nContrast) {
                    nTone = MdContrast::foregroundTone(bgTone, nContrast);
                }
                if (MdContrast::ratioOfTones(bgTone, fTone) < fContrast) {
                    fTone = MdContrast::foregroundTone(bgTone, fContrast);
                }

                if (decreasingContrast) {
                    nTone = MdContrast::foregroundTone(bgTone, nContrast);
                    fTone = MdContrast::foregroundTone(bgTone, fContrast);
                }
            }

            // 2nd and 3rd rounds: satisfy the delta.
            if ((fTone - nTone) * expansionDir < pair.delta) {
                fTone = clampDouble(0.0, 100.0, nTone + pair.delta * expansionDir);
                if ((fTone - nTone) * expansionDir < pair.delta) {
                    nTone = clampDouble(0.0, 100.0, fTone - pair.delta * expansionDir);
                }
            }

            // Avoid the 50-59 "awkward zone" where a tone is too light to read
            // as dark and too dark to read as light.
            if (nTone >= 50.0 && nTone < 60.0) {
                if (expansionDir > 0.0) {
                    nTone = 60.0;
                    fTone = std::max(fTone, nTone + pair.delta * expansionDir);
                } else {
                    nTone = 49.0;
                    fTone = std::min(fTone, nTone + pair.delta * expansionDir);
                }
            } else if (fTone >= 50.0 && fTone < 60.0) {
                if (pair.stayTogether) {
                    if (expansionDir > 0.0) {
                        nTone = 60.0;
                        fTone = std::max(fTone, nTone + pair.delta * expansionDir);
                    } else {
                        nTone = 49.0;
                        fTone = std::min(fTone, nTone + pair.delta * expansionDir);
                    }
                } else {
                    fTone = expansionDir > 0.0 ? 60.0 : 49.0;
                }
            }

            return amNearer ? nTone : fTone;
        }

        // --- Case 2: solve for itself ----------------------------------
        double answer = rawToneOf(m_scheme, role);
        if (spec.background == ColorRole::Count || !spec.hasCurve) {
            return answer;
        }

        const double bgTone = tone(spec.background);
        const double desiredRatio = spec.curve.get(contrastLevel);

        if (MdContrast::ratioOfTones(bgTone, answer) < desiredRatio) {
            answer = MdContrast::foregroundTone(bgTone, desiredRatio);
        }

        if (decreasingContrast) {
            answer = MdContrast::foregroundTone(bgTone, desiredRatio);
        }

        if (spec.isBackground && answer >= 50.0 && answer < 60.0) {
            answer = MdContrast::ratioOfTones(49.0, bgTone) >= desiredRatio ? 49.0 : 60.0;
        }

        if (spec.secondBackground == ColorRole::Count) {
            return answer;
        }

        // --- Case 3: two backgrounds -----------------------------------
        const double bgTone1 = tone(spec.background);
        const double bgTone2 = tone(spec.secondBackground);
        const double upper = std::max(bgTone1, bgTone2);
        const double lower = std::min(bgTone1, bgTone2);

        if (MdContrast::ratioOfTones(upper, answer) >= desiredRatio
            && MdContrast::ratioOfTones(lower, answer) >= desiredRatio) {
            return answer;
        }

        const double lightOption = MdContrast::lighter(upper, desiredRatio);
        const double darkOption = MdContrast::darker(lower, desiredRatio);

        const bool prefersLight = MdContrast::tonePrefersLightForeground(bgTone1)
                                  || MdContrast::tonePrefersLightForeground(bgTone2);
        if (prefersLight) {
            return lightOption == -1.0 ? 100.0 : lightOption;
        }
        const int availableCount = (lightOption != -1.0 ? 1 : 0) + (darkOption != -1.0 ? 1 : 0);
        if (availableCount == 1) {
            return lightOption != -1.0 ? lightOption : darkOption;
        }
        return darkOption == -1.0 ? 0.0 : darkOption;
    }

private:
    const MdDynamicScheme &m_scheme;
};

} // namespace

// ---------------------------------------------------------------------------
// MdColorSpec2021
// ---------------------------------------------------------------------------

double MdColorSpec2021::rawTone(const MdDynamicScheme &scheme, ColorRole role)
{
    return rawToneOf(scheme, role);
}

double MdColorSpec2021::tone(const MdDynamicScheme &scheme, ColorRole role)
{
    return Solver(scheme).tone(role);
}

Argb MdColorSpec2021::color(const MdDynamicScheme &scheme, ColorRole role)
{
    // getHct() upstream: find the tone for contrast, then rebuild the colour
    // from the palette so the intended chroma survives the tone change.
    const Slot slot = specFor(role, scheme.isDark()).slot;
    return paletteOf(scheme, slot).tone(tone(scheme, role));
}

} // namespace md
