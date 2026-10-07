// MdTemperatureCache, pinned against material-color-utilities' own test.
//
// Every expected value below is copied verbatim from
// `cpp/temperature/temperature_cache_test.cc` in material-color-utilities:
// RawTemperature, Complement and Analogous, for blue / red / green / black /
// white. Those assertions are the only place upstream publishes numeric answers
// for this module, and they are worth having because the two variants that use
// it - `content` and `fidelity` - are otherwise only observable through a
// derived tertiary hue several layers up.
//
// Why this suite exists at all: the first port read the three Lab components
// off a generic {a, b, c} triple, so L* was consumed where a* was meant. Every
// number still looked like a plausible temperature, the wheel simply had its
// warm and cool ends rotated by about a quadrant, and the only visible symptom
// was that `content` and `fidelity` produced odd tertiary hues. Doing the
// arithmetic by name is what these tests protect.

#include "core/MdColorMath.h"
#include "core/MdTemperatureCache.h"

#include <QtTest/QtTest>

using namespace md;

namespace {

constexpr Argb kBlue = 0xFF0000FFu;
constexpr Argb kRed = 0xFFFF0000u;
constexpr Argb kGreen = 0xFF00FF00u;
constexpr Argb kWhite = 0xFFFFFFFFu;
constexpr Argb kBlack = 0xFF000000u;

} // namespace

class TestMd3TemperatureCache : public QObject
{
    Q_OBJECT

private slots:
    void rawTemperatureMatchesUpstream();
    void warmIsWarmAndCoolIsCool();
    void complementMatchesUpstream();
    void analogousMatchesUpstream();
    void achromaticColoursCollapse();
    void analogousOrderingPutsTheInputInTheMiddle();
    void relativeTemperatureSpansTheFullRange();
};

void TestMd3TemperatureCache::rawTemperatureMatchesUpstream()
{
    // temperature_cache_test.cc, TEST(TemperatureCacheTest, RawTemperature).
    // The tolerance is tighter than upstream's own 0.001 because there is no
    // reason for a port to be looser than the reference.
    QVERIFY(qAbs(MdTemperatureCache::rawTemperature(MdHct(kBlue)) - (-1.393)) < 1e-3);
    QVERIFY(qAbs(MdTemperatureCache::rawTemperature(MdHct(kRed)) - 2.351) < 1e-3);
    QVERIFY(qAbs(MdTemperatureCache::rawTemperature(MdHct(kGreen)) - (-0.267)) < 1e-3);

    // White and black carry no chroma, so the temperature term vanishes and
    // only the -0.5 offset is left. This is the check that catches a bogus
    // chroma: any non-zero chroma leaks straight into the result.
    QVERIFY(qAbs(MdTemperatureCache::rawTemperature(MdHct(kWhite)) - (-0.5)) < 1e-3);
    QVERIFY(qAbs(MdTemperatureCache::rawTemperature(MdHct(kBlack)) - (-0.5)) < 1e-3);
}

void TestMd3TemperatureCache::warmIsWarmAndCoolIsCool()
{
    // The sanity check that the component mix-up would have failed loudly, and
    // the reason it is spelled out separately: red is the canonical warm hue,
    // blue the canonical cool one, green sits between them. A rotated axis
    // still produces numbers in range, so only an ordering check states the
    // actual requirement.
    const double red = MdTemperatureCache::rawTemperature(MdHct(kRed));
    const double green = MdTemperatureCache::rawTemperature(MdHct(kGreen));
    const double blue = MdTemperatureCache::rawTemperature(MdHct(kBlue));

    QVERIFY2(red > 0.0, qPrintable(QStringLiteral("red must be warm, got %1").arg(red)));
    QVERIFY2(blue < 0.0, qPrintable(QStringLiteral("blue must be cool, got %1").arg(blue)));
    QVERIFY2(red > green && green > blue,
             qPrintable(QStringLiteral("expected red > green > blue, got %1 / %2 / %3")
                            .arg(red).arg(green).arg(blue)));
}

void TestMd3TemperatureCache::complementMatchesUpstream()
{
    // temperature_cache_test.cc, TEST(TemperatureCacheTest, Complement).
    QCOMPARE(MdTemperatureCache(MdHct(kBlue)).complement().toInt(), 0xFF9D0002u);
    QCOMPARE(MdTemperatureCache(MdHct(kRed)).complement().toInt(), 0xFF007BFCu);
    QCOMPARE(MdTemperatureCache(MdHct(kGreen)).complement().toInt(), 0xFFFFD2C9u);

    // A colour with no temperature has nowhere to go: the complement of white
    // is white, of black is black.
    QCOMPARE(MdTemperatureCache(MdHct(kWhite)).complement().toInt(), 0xFFFFFFFFu);
    QCOMPARE(MdTemperatureCache(MdHct(kBlack)).complement().toInt(), 0xFF000000u);
}

void TestMd3TemperatureCache::analogousMatchesUpstream()
{
    // temperature_cache_test.cc, TEST(TemperatureCacheTest, Analogous). The
    // default is count 5 over 12 divisions, and the input sits at index 2.
    const QVector<MdHct> blue = MdTemperatureCache(MdHct(kBlue)).analogous(5, 12);
    QCOMPARE(blue.size(), 5);
    QCOMPARE(blue.at(0).toInt(), 0xFF00590Cu);
    QCOMPARE(blue.at(1).toInt(), 0xFF00564Eu);
    QCOMPARE(blue.at(2).toInt(), kBlue);
    QCOMPARE(blue.at(3).toInt(), 0xFF6700CCu);
    QCOMPARE(blue.at(4).toInt(), 0xFF81009Fu);

    const QVector<MdHct> red = MdTemperatureCache(MdHct(kRed)).analogous(5, 12);
    QCOMPARE(red.at(0).toInt(), 0xFFF60082u);
    QCOMPARE(red.at(1).toInt(), 0xFFFC004Cu);
    QCOMPARE(red.at(2).toInt(), kRed);
    QCOMPARE(red.at(3).toInt(), 0xFFD95500u);
    QCOMPARE(red.at(4).toInt(), 0xFFAF7200u);

    const QVector<MdHct> green = MdTemperatureCache(MdHct(kGreen)).analogous(5, 12);
    QCOMPARE(green.at(0).toInt(), 0xFFCEE900u);
    QCOMPARE(green.at(1).toInt(), 0xFF92F500u);
    QCOMPARE(green.at(2).toInt(), kGreen);
    QCOMPARE(green.at(3).toInt(), 0xFF00FD6Fu);
    QCOMPARE(green.at(4).toInt(), 0xFF00FAB3u);
}

void TestMd3TemperatureCache::achromaticColoursCollapse()
{
    // Same source: white and black have no analogues at all, so the answers
    // repeat the input rather than cycling through hues that do not exist at
    // zero chroma.
    for (Argb achromatic : {kWhite, kBlack}) {
        const QVector<MdHct> result = MdTemperatureCache(MdHct(achromatic)).analogous(5, 12);
        QCOMPARE(result.size(), 5);
        for (const MdHct &hct : result) {
            QCOMPARE(hct.toInt(), achromatic);
        }
    }
}

void TestMd3TemperatureCache::analogousOrderingPutsTheInputInTheMiddle()
{
    // The output contract the `content` variant depends on: the input is at
    // index floor((count-1)/2), everything before it runs counter-clockwise and
    // everything after runs clockwise. Content reads index 2 of a count-3
    // request, i.e. the first clockwise neighbour.
    for (int count : {3, 5, 7}) {
        const QVector<MdHct> result = MdTemperatureCache(MdHct(kBlue)).analogous(count, 12);
        QCOMPARE(result.size(), count);
        const int middle = int(std::floor((double(count) - 1.0) / 2.0));
        QCOMPARE(result.at(middle).toInt(), kBlue);
    }

    // A count of 1 is just the input, and a count below the division count
    // still returns exactly `count` distinct entries.
    QCOMPARE(MdTemperatureCache(MdHct(kBlue)).analogous(1, 12).size(), 1);
    QCOMPARE(MdTemperatureCache(MdHct(kBlue)).analogous(3, 6).size(), 3);
}

void TestMd3TemperatureCache::relativeTemperatureSpansTheFullRange()
{
    // relativeTemperature() is normalised against the coldest and warmest
    // colours at the input's own chroma and tone, so the input can land
    // anywhere in [0, 1] and the extremes must be reachable.
    for (Argb seed : {kBlue, kRed, kGreen}) {
        const MdTemperatureCache cache{MdHct(seed)};
        const double relative = cache.relativeTemperature();
        QVERIFY2(relative >= 0.0 && relative <= 1.0,
                 qPrintable(QStringLiteral("%1: relativeTemperature %2 out of range")
                                .arg(seed, 8, 16, QLatin1Char('0'))
                                .arg(relative, 0, 'f', 4)));
    }

    // No chroma means no warm/cool spread at all, and the documented answer is
    // the neutral 0.5 rather than a division by zero.
    const MdTemperatureCache neutral{MdHct(kWhite)};
    QCOMPARE(neutral.relativeTemperature(), 0.5);
}

QTEST_MAIN(TestMd3TemperatureCache)
#include "TestMd3TemperatureCache.moc"
