#include "MdTemperatureCache.h"

#include <algorithm>
#include <cmath>

namespace md {

namespace {

constexpr double kPi = 3.141592653589793;

} // namespace

MdTemperatureCache::MdTemperatureCache(const MdHct &input)
    : m_input(input)
{
    m_hcts.reserve(361);
    m_temps.reserve(361);
    for (int hue = 0; hue <= 360; ++hue) {
        m_hcts.append(MdHct(double(hue), input.chroma(), input.tone()));
        m_temps.append(rawTemperature(m_hcts.last()));
    }
    m_inputTemp = rawTemperature(input);
}

double MdTemperatureCache::rawTemperature(const MdHct &color)
{
    // temperature_cache.cc RawTemperature. Note this works in **Lab**, not HCT:
    // `hue` is the Lab hue angle, so it is unrelated to `color.hue()`.
    //
    // The components must be read by name. MdLab is {l, a, b}; reading a
    // generic triple as {a, b, _} silently swaps L* and a* here, which throws
    // the warm/cool axis off by roughly a quadrant and produces plausible but
    // wrong complements and analogous colours.
    const MdLab lab = MdTonalPalette::labFromArgb(color.toInt());
    const double hue = MdColorMath::sanitizeDegreesDouble(std::atan2(lab.b, lab.a) * 180.0 / kPi);
    const double chroma = std::hypot(lab.a, lab.b);
    return -0.5
           + 0.02 * std::pow(chroma, 1.07)
                 * std::cos(MdColorMath::sanitizeDegreesDouble(hue - 50.0) * kPi / 180.0);
}

MdHct MdTemperatureCache::complement() const
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

    // Find the colour in the opposite section sitting at the inverse percentile
    // of the input's temperature. That is the complement.
    for (double addend = 0.0; addend <= 360.0; addend += 1.0) {
        const double hue = MdColorMath::sanitizeDegreesDouble(startHue + addend);
        if (!isBetween(hue, startHue, endHue)) {
            continue;
        }
        const MdHct possible = m_hcts.at(int(std::round(hue)));
        const double relativeTemp = (temperatureOf(int(std::round(hue))) - coldestTemp) / range;
        const double error = std::abs(complementRelativeTemp - relativeTemp);
        if (error < smallestError) {
            smallestError = error;
            answer = possible;
        }
    }
    return answer;
}

QVector<MdHct> MdTemperatureCache::analogous(int count, int divisions) const
{
    // The starting hue is the hue of the input colour.
    const int startHue = int(std::round(m_input.hue()));
    const MdHct startHct = m_hcts.at(startHue);
    double lastTemp = relativeTemperatureOfHue(startHue);

    QVector<MdHct> allColors;
    allColors.append(startHct);

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
    while (allColors.size() < divisions) {
        const int hue = MdColorMath::sanitizeDegreesInt(startHue + hueAddend);
        const MdHct hct = m_hcts.at(hue);
        const double temp = relativeTemperatureOfHue(hue);
        totalTempDelta += std::abs(temp - lastTemp);

        double desiredTotalTempDeltaForIndex = double(allColors.size()) * tempStep;
        bool indexSatisfied = totalTempDelta >= desiredTotalTempDeltaForIndex;
        int indexAddend = 1;
        // Keep adding this hue until its temperature contribution is
        // insufficient. This keeps the result consistent when there are not
        // `divisions` discrete temperature steps around the wheel - white and
        // black have no analogues at all, so they just repeat.
        while (indexSatisfied && allColors.size() < divisions) {
            allColors.append(hct);
            desiredTotalTempDeltaForIndex =
                double(allColors.size() + indexAddend) * tempStep;
            indexSatisfied = totalTempDelta >= desiredTotalTempDeltaForIndex;
            ++indexAddend;
        }
        lastTemp = temp;
        ++hueAddend;

        if (hueAddend > 360) {
            while (allColors.size() < divisions) {
                allColors.append(hct);
            }
            break;
        }
    }

    QVector<MdHct> answers;
    answers.append(m_input);

    const int ccwCount = int(std::floor((double(count) - 1.0) / 2.0));
    for (int i = 1; i < ccwCount + 1; ++i) {
        int index = -i;
        while (index < 0) {
            index = allColors.size() + index;
        }
        if (index >= allColors.size()) {
            index = index % allColors.size();
        }
        answers.prepend(allColors.at(index));
    }

    const int cwCount = count - ccwCount - 1;
    for (int i = 1; i < cwCount + 1; ++i) {
        int index = i;
        if (index >= allColors.size()) {
            index = index % allColors.size();
        }
        answers.append(allColors.at(index));
    }
    return answers;
}

double MdTemperatureCache::hueOfIndex(int index) const
{
    return index == kInputIndex ? m_input.hue() : double(index);
}

double MdTemperatureCache::temperatureOf(int index) const
{
    return index == kInputIndex ? m_inputTemp : m_temps.at(index);
}

double MdTemperatureCache::relativeTemperatureOfHue(int hue) const
{
    return (m_temps.at(hue) - temperatureOf(coldestIndex()))
           / (temperatureOf(warmestIndex()) - temperatureOf(coldestIndex()));
}

double MdTemperatureCache::relativeTemperature() const
{
    const double range = temperatureOf(warmestIndex()) - temperatureOf(coldestIndex());
    const double differenceFromColdest = m_inputTemp - temperatureOf(coldestIndex());
    // No difference between warmest and coldest means there is only one colour:
    // at T100 or T0 everything collapses to white or black.
    if (range == 0.0) {
        return 0.5;
    }
    return differenceFromColdest / range;
}

int MdTemperatureCache::coldestIndex() const
{
    int best = kInputIndex;
    double bestTemp = m_inputTemp;
    for (int i = 0; i <= 360; ++i) {
        if (m_temps.at(i) < bestTemp) {
            bestTemp = m_temps.at(i);
            best = i;
        }
    }
    return best;
}

int MdTemperatureCache::warmestIndex() const
{
    int best = kInputIndex;
    double bestTemp = m_inputTemp;
    for (int i = 0; i <= 360; ++i) {
        if (m_temps.at(i) > bestTemp) {
            bestTemp = m_temps.at(i);
            best = i;
        }
    }
    return best;
}

bool MdTemperatureCache::isBetween(double angle, double a, double b)
{
    if (a < b) {
        return angle >= a && angle <= b;
    }
    return angle >= a || angle <= b;
}

} // namespace md
