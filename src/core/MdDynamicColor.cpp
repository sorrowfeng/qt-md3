#include "MdDynamicColor.h"

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

// dislike.cc
bool isDisliked(const MdHct &hct)
{
    const double roundedHue = std::round(hct.hue());
    return roundedHue >= 90.0 && roundedHue <= 111.0 && std::round(hct.chroma()) > 16.0
           && std::round(hct.tone()) < 65.0;
}

MdHct fixIfDisliked(const MdHct &hct)
{
    return isDisliked(hct) ? MdHct(hct.hue(), hct.chroma(), 70.0) : hct;
}

// temperature_cache.cc, reduced to the operations the content and fidelity
// variants need. Hues are kept as integer indices so palette lookups stay
// exact instead of relying on floating-point map keys.
class TemperatureCache
{
public:
    explicit TemperatureCache(const MdHct &input)
        : m_input(input)
    {
        m_hcts.reserve(361);
        m_temps.reserve(361);
        for (int hue = 0; hue <= 360; ++hue) {
            m_hcts.emplace_back(double(hue), input.chroma(), input.tone());
            m_temps.push_back(rawTemperature(m_hcts.back()));
        }
        m_inputTemp = rawTemperature(input);
    }

    MdHct complement()
    {
        const double coldestTemp = temperatureOf(coldestIndex());
        const double warmestTemp = temperatureOf(warmestIndex());
        const double range = warmestTemp - coldestTemp;

        const double coldestHue = hueOfIndex(coldestIndex());
        const double warmestHue = hueOfIndex(warmestIndex());
        const bool startIsColdestToWarmest = isBetween(m_input.hue(), coldestHue, warmestHue);
        const double startHue = startIsColdestToWarmest ? warmestHue : coldestHue;
        const double endHue = startIsColdestToWarmest ? coldestHue : warmestHue;

        double smallestError = 1000.0;
        MdHct answer = m_hcts.at(int(std::round(m_input.hue())));
        const double complementRelativeTemp = 1.0 - relativeTemperature();

        for (double addend = 0.0; addend <= 360.0; addend += 1.0) {
            const double hue = MdColorMath::sanitizeDegreesDouble(startHue + addend);
            if (!isBetween(hue, startHue, endHue)) {
                continue;
            }
            const MdHct possible = m_hcts.at(int(std::round(hue)));
            const double relativeTemp = (temperatureOf(indexOfHue(hue)) - coldestTemp) / range;
            const double error = std::abs(complementRelativeTemp - relativeTemp);
            if (error < smallestError) {
                smallestError = error;
                answer = possible;
            }
        }
        return answer;
    }

    std::vector<MdHct> analogous(int count, int divisions)
    {
        const int startHue = int(std::round(m_input.hue()));
        const MdHct startHct = m_hcts.at(startHue);
        double lastTemp = relativeTemperatureOfHue(startHue);

        std::vector<MdHct> allColors;
        allColors.push_back(startHct);

        double absoluteTotalTempDelta = 0.0;
        for (int i = 0; i < 360; ++i) {
            const int hue = MdColorMath::sanitizeDegreesInt(startHue + i);
            const double temp = relativeTemperatureOfHue(hue);
            absoluteTotalTempDelta += std::abs(temp - lastTemp);
            lastTemp = temp;
        }

        int hueAddend = 1;
        const double tempStep = absoluteTotalTempDelta / double(divisions);
        double totalTempDelta = 0.0;
        lastTemp = relativeTemperatureOfHue(startHue);
        while (int(allColors.size()) < divisions) {
            const int hue = MdColorMath::sanitizeDegreesInt(startHue + hueAddend);
            const MdHct hct = m_hcts.at(hue);
            const double temp = relativeTemperatureOfHue(hue);
            totalTempDelta += std::abs(temp - lastTemp);

            double desiredTotalTempDeltaForIndex = double(allColors.size()) * tempStep;
            bool indexSatisfied = totalTempDelta >= desiredTotalTempDeltaForIndex;
            int indexAddend = 1;
            while (indexSatisfied && int(allColors.size()) < divisions) {
                allColors.push_back(hct);
                desiredTotalTempDeltaForIndex =
                    double(allColors.size() + indexAddend) * tempStep;
                indexSatisfied = totalTempDelta >= desiredTotalTempDeltaForIndex;
                ++indexAddend;
            }
            lastTemp = temp;
            ++hueAddend;

            if (hueAddend > 360) {
                while (int(allColors.size()) < divisions) {
                    allColors.push_back(hct);
                }
                break;
            }
        }

        std::vector<MdHct> answers;
        answers.push_back(m_input);

        const int ccwCount = int(std::floor((double(count) - 1.0) / 2.0));
        for (int i = 1; i < ccwCount + 1; ++i) {
            int index = -i;
            while (index < 0) {
                index = int(allColors.size()) + index;
            }
            if (index >= int(allColors.size())) {
                index = index % int(allColors.size());
            }
            answers.insert(answers.begin(), allColors.at(size_t(index)));
        }

        const int cwCount = count - ccwCount - 1;
        for (int i = 1; i < cwCount + 1; ++i) {
            int index = i;
            if (index >= int(allColors.size())) {
                index = index % int(allColors.size());
            }
            answers.push_back(allColors.at(size_t(index)));
        }
        return answers;
    }

private:
    // index 0..360 = hue; 361 = the input colour
    static constexpr int kInputIndex = 361;

    double hueOfIndex(int index) const
    {
        return index == kInputIndex ? m_input.hue() : double(index);
    }

    double temperatureOf(int index) const
    {
        return index == kInputIndex ? m_inputTemp : m_temps.at(size_t(index));
    }

    int indexOfHue(double hue) const
    {
        return int(std::round(hue));
    }

    double rawTemperature(const MdHct &color) const
    {
        const MdVec3 lab = MdTonalPalette::labFromArgb(color.toInt());
        const double hue = MdColorMath::sanitizeDegreesDouble(std::atan2(lab.b, lab.a) * 180.0 / 3.141592653589793);
        const double chroma = std::hypot(lab.a, lab.b);
        return -0.5
               + 0.02 * std::pow(chroma, 1.07)
                     * std::cos(MdColorMath::sanitizeDegreesDouble(hue - 50.0) * 3.141592653589793 / 180.0);
    }

    double relativeTemperatureOfHue(int hue) const
    {
        const double range = temperatureOf(warmestIndex()) - temperatureOf(coldestIndex());
        const double differenceFromColdest = m_temps.at(size_t(hue)) - temperatureOf(coldestIndex());
        if (range == 0.0) {
            return 0.5;
        }
        return differenceFromColdest / range;
    }

    double relativeTemperature() const
    {
        const double range = temperatureOf(warmestIndex()) - temperatureOf(coldestIndex());
        const double differenceFromColdest = m_inputTemp - temperatureOf(coldestIndex());
        if (range == 0.0) {
            return 0.5;
        }
        return differenceFromColdest / range;
    }

    int coldestIndex() const
    {
        int best = kInputIndex;
        double bestTemp = m_inputTemp;
        for (int i = 0; i <= 360; ++i) {
            if (m_temps.at(size_t(i)) < bestTemp) {
                bestTemp = m_temps.at(size_t(i));
                best = i;
            }
        }
        return best;
    }

    int warmestIndex() const
    {
        int best = kInputIndex;
        double bestTemp = m_inputTemp;
        for (int i = 0; i <= 360; ++i) {
            if (m_temps.at(size_t(i)) > bestTemp) {
                bestTemp = m_temps.at(size_t(i));
                best = i;
            }
        }
        return best;
    }

    static bool isBetween(double angle, double a, double b)
    {
        if (a < b) {
            return angle >= a && angle <= b;
        }
        return angle >= a || angle <= b;
    }

    MdHct m_input;
    std::vector<MdHct> m_hcts;
    std::vector<double> m_temps;
    double m_inputTemp = 0.0;
};

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
        set.tertiary = MdTonalPalette(fixIfDisliked(TemperatureCache(source).analogous(3, 6).at(2)));
        set.neutral = MdTonalPalette(hue, chroma / 8.0);
        set.neutralVariant = MdTonalPalette(hue, chroma / 8.0 + 4.0);
        break;
    }
    case SchemeVariant::Fidelity:
        set.primary = MdTonalPalette(hue, chroma);
        set.secondary = MdTonalPalette(hue, std::fmax(chroma - 32.0, chroma * 0.5));
        set.tertiary = MdTonalPalette(fixIfDisliked(TemperatureCache(source).complement()));
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

enum class PaletteId { Primary, Secondary, Tertiary, Neutral, NeutralVariant, Error };

PaletteId paletteFor(ColorRole role)
{
    switch (role) {
    case ColorRole::Primary:
    case ColorRole::OnPrimary:
    case ColorRole::PrimaryContainer:
    case ColorRole::OnPrimaryContainer:
    case ColorRole::PrimaryFixed:
    case ColorRole::PrimaryFixedDim:
    case ColorRole::OnPrimaryFixed:
    case ColorRole::OnPrimaryFixedVariant:
    case ColorRole::InversePrimary:
    case ColorRole::SurfaceTint:
        return PaletteId::Primary;

    case ColorRole::Secondary:
    case ColorRole::OnSecondary:
    case ColorRole::SecondaryContainer:
    case ColorRole::OnSecondaryContainer:
    case ColorRole::SecondaryFixed:
    case ColorRole::SecondaryFixedDim:
    case ColorRole::OnSecondaryFixed:
    case ColorRole::OnSecondaryFixedVariant:
        return PaletteId::Secondary;

    case ColorRole::Tertiary:
    case ColorRole::OnTertiary:
    case ColorRole::TertiaryContainer:
    case ColorRole::OnTertiaryContainer:
    case ColorRole::TertiaryFixed:
    case ColorRole::TertiaryFixedDim:
    case ColorRole::OnTertiaryFixed:
    case ColorRole::OnTertiaryFixedVariant:
        return PaletteId::Tertiary;

    case ColorRole::Error:
    case ColorRole::OnError:
    case ColorRole::ErrorContainer:
    case ColorRole::OnErrorContainer:
        return PaletteId::Error;

    case ColorRole::SurfaceVariant:
    case ColorRole::OnSurfaceVariant:
    case ColorRole::Outline:
    case ColorRole::OutlineVariant:
        return PaletteId::NeutralVariant;

    default:
        // Background / surface family / inverse surface / scrim / shadow.
        return PaletteId::Neutral;
    }
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

double MdDynamicScheme::toneFor(ColorRole role, bool isDark)
{
    // The published md.sys.color mapping. Cross-checked against material-web's
    // tokens/versions/v0_192/_md-sys-color.scss, which is exactly this table
    // evaluated for the default seed — so the light and dark columns below
    // reproduce that file's reference names one for one.
    struct ToneRow {
        ColorRole role;
        int lightTone;
        int darkTone;
    };
    static const ToneRow kRows[] = {
        // Fixed roles are mode-independent.
        {ColorRole::PrimaryFixed, 90, 90},
        {ColorRole::PrimaryFixedDim, 80, 80},
        {ColorRole::OnPrimaryFixed, 10, 10},
        {ColorRole::OnPrimaryFixedVariant, 30, 30},
        {ColorRole::SecondaryFixed, 90, 90},
        {ColorRole::SecondaryFixedDim, 80, 80},
        {ColorRole::OnSecondaryFixed, 10, 10},
        {ColorRole::OnSecondaryFixedVariant, 30, 30},
        {ColorRole::TertiaryFixed, 90, 90},
        {ColorRole::TertiaryFixedDim, 80, 80},
        {ColorRole::OnTertiaryFixed, 10, 10},
        {ColorRole::OnTertiaryFixedVariant, 30, 30},
        {ColorRole::Scrim, 0, 0},
        {ColorRole::Shadow, 0, 0},

        {ColorRole::Primary, 40, 80},
        {ColorRole::OnPrimary, 100, 20},
        {ColorRole::PrimaryContainer, 90, 30},
        {ColorRole::OnPrimaryContainer, 10, 90},
        {ColorRole::InversePrimary, 80, 40},

        {ColorRole::Secondary, 40, 80},
        {ColorRole::OnSecondary, 100, 20},
        {ColorRole::SecondaryContainer, 90, 30},
        {ColorRole::OnSecondaryContainer, 10, 90},

        {ColorRole::Tertiary, 40, 80},
        {ColorRole::OnTertiary, 100, 20},
        {ColorRole::TertiaryContainer, 90, 30},
        {ColorRole::OnTertiaryContainer, 10, 90},

        {ColorRole::Error, 40, 80},
        {ColorRole::OnError, 100, 20},
        {ColorRole::ErrorContainer, 90, 30},
        {ColorRole::OnErrorContainer, 10, 90},

        {ColorRole::Background, 98, 6},
        {ColorRole::OnBackground, 10, 90},
        {ColorRole::Surface, 98, 6},
        {ColorRole::OnSurface, 10, 90},
        {ColorRole::SurfaceDim, 87, 6},
        {ColorRole::SurfaceBright, 98, 24},
        {ColorRole::SurfaceContainerLowest, 100, 4},
        {ColorRole::SurfaceContainerLow, 96, 10},
        {ColorRole::SurfaceContainer, 94, 12},
        {ColorRole::SurfaceContainerHigh, 92, 17},
        {ColorRole::SurfaceContainerHighest, 90, 22},
        {ColorRole::InverseSurface, 20, 90},
        {ColorRole::InverseOnSurface, 95, 20},

        {ColorRole::SurfaceVariant, 90, 30},
        {ColorRole::OnSurfaceVariant, 30, 80},
        {ColorRole::Outline, 50, 60},
        {ColorRole::OutlineVariant, 80, 30},

        {ColorRole::SurfaceTint, 40, 80},
    };

    for (const ToneRow &row : kRows) {
        if (row.role == role) {
            return double(isDark ? row.darkTone : row.lightTone);
        }
    }
    return 0.0;
}

Argb MdDynamicScheme::color(ColorRole role) const
{
    const double tone = toneFor(role, isDark());
    switch (paletteFor(role)) {
    case PaletteId::Primary: return m_primary.tone(tone);
    case PaletteId::Secondary: return m_secondary.tone(tone);
    case PaletteId::Tertiary: return m_tertiary.tone(tone);
    case PaletteId::Neutral: return m_neutral.tone(tone);
    case PaletteId::NeutralVariant: return m_neutralVariant.tone(tone);
    case PaletteId::Error: return m_error.tone(tone);
    }
    return 0xFF000000u;
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
