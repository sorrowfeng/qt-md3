#ifndef MD_TEMPERATURE_CACHE_H
#define MD_TEMPERATURE_CACHE_H

// TemperatureCache — design utilities built on colour temperature theory.
//
// Port of material-color-utilities' `cpp/temperature/temperature_cache.{h,cc}`
// (which implements the same algorithm as `java/temperature/TemperatureCache.java`).
//
// Only two variants need it, and they need it badly:
//
//   * `content`  — tertiary = fixIfDisliked(getAnalogousColors(3, 6).get(2))
//   * `fidelity` — tertiary = fixIfDisliked(getComplement())
//
// Every other variant derives its tertiary from a plain hue rotation, so a
// defect here is invisible unless you look at those two specifically. That is
// exactly why this module has its own test: tests/TestMd3TemperatureCache.cpp
// pins the raw temperature, the complement and the analogous colours against
// the numbers upstream asserts in temperature_cache_test.cc, so a regression
// shows up as a temperature failing rather than as a strange tertiary hue three
// layers up.
//
// All 361 hues at the input's chroma and tone are precomputed in the
// constructor, together with the input itself.

#include "MdColorMath.h"
#include "QtMd3Export.h"

#include <QtCore/QVector>

namespace md {

class QT_MD3_EXPORT MdTemperatureCache
{
public:
    explicit MdTemperatureCache(const MdHct &input);

    /// A colour that complements the input aesthetically: equally cool-warm in
    /// the opposite direction. Same chroma and tone as the input.
    MdHct complement() const;

    /// `count` colours equidistant in temperature and adjacent in hue, with the
    /// input in the middle. `divisions` is how many slices the colour wheel is
    /// cut into; when `divisions < count` colours repeat.
    QVector<MdHct> analogous(int count, int divisions) const;

    /// The input's temperature relative to every colour sharing its chroma and
    /// tone, on a 0..1 scale.
    double relativeTemperature() const;

    /// Raw temperature of any colour.
    ///
    /// Uses Lab, not HCT: the hue term is the Lab hue angle, so it is not
    /// interchangeable with `MdHct::hue()`. Negative is cool, positive is warm;
    /// bounds are about -9.66 and 8.61 at the chroma limits of sRGB.
    static double rawTemperature(const MdHct &color);

private:
    /// Index 0..360 = hue, 361 = the input colour. Keeping the input out of the
    /// hue grid keeps the endpoints exact instead of relying on the input
    /// landing on an integer hue.
    static constexpr int kInputIndex = 361;

    double hueOfIndex(int index) const;
    double temperatureOf(int index) const;
    int coldestIndex() const;
    int warmestIndex() const;
    double relativeTemperatureOfHue(int hue) const;

    static bool isBetween(double angle, double a, double b);

    MdHct m_input;
    QVector<MdHct> m_hcts;
    QVector<double> m_temps;
    double m_inputTemp = 0.0;
};

} // namespace md

#endif // MD_TEMPERATURE_CACHE_H
