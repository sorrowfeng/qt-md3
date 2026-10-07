// ColorSpec2021 and the contrast levels.
//
// A contrast level is a *solve*, not a table. That makes verification two
// separate questions, and conflating them is the mistake this suite exists to
// prevent:
//
//   1. `md.sys.color` — the six token sets material-web publishes under
//      tokens/versions/latest/sass — is authored static data on the M3
//      reference palette. It is what `MdColorScheme::baseline()` must return,
//      verbatim, and it is *not* the output of the dynamic solver. See
//      baselineSchemeIsThePublishedTokenSet() and
//      dynamicTonalSpotIsNotTheStaticBaseline().
//
//   2. A dynamic scheme is solved, and the authority for it is
//      material-color-utilities' own test suite. mcuSchemeExpectationsMatch()
//      replays 367 of its assertions across nine variants, both modes and the
//      minimum/standard/maximum contrast levels.
//
// The remaining slots cover the algebra the solver is built out of — curves,
// ratios, tone-delta pairs — where a mistake would be invisible in the
// end-to-end numbers but would silently narrow the contrast range.

#include "core/MdColorMath.h"
#include "core/MdColorScheme.h"
#include "core/MdColorSpec.h"
#include "core/MdDynamicColor.h"
#include "core/MdTypes.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtTest/QtTest>

#include <algorithm>

using namespace md;

namespace {

constexpr Argb kDefaultSeed = 0xFF6750A4u;

QString roleLabel(ColorRole role)
{
    return colorRoleName(role);
}

/// role token name -> ColorRole, built from colorRoleName() so the two can
/// never drift apart.
QHash<QString, ColorRole> roleNamesByName()
{
    QHash<QString, ColorRole> names;
    for (int i = 0; i < int(ColorRole::Count); ++i) {
        names.insert(colorRoleName(ColorRole(i)), ColorRole(i));
    }
    return names;
}

/// One of the six published `md.sys.color` token sets, read from the vendored
/// fixture. Empty when the fixture is missing, which the caller must treat as a
/// failure rather than a skip — a missing reference is how this kind of
/// verification silently stops verifying anything.
QHash<ColorRole, Argb> readPublishedTokens(const QString &variant)
{
    QHash<ColorRole, Argb> tokens;
    const QDir dir(QStringLiteral(QT_MD3_TOKEN_FIXTURES));
    QFile file(dir.filePath(QStringLiteral("md-sys-color-%1.txt").arg(variant)));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return tokens;
    }
    const QHash<QString, ColorRole> names = roleNamesByName();
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        // `<role> <#rrggbb>` with an optional trailing `md.ref.palette` name.
        if (parts.size() < 2) {
            continue;
        }
        const auto role = names.constFind(parts.at(0));
        if (role == names.constEnd()) {
            continue;
        }
        // The fixture stores "#rrggbb", so the parsed value has no alpha. Force
        // it opaque here rather than masking at every comparison site: a role
        // colour is opaque by definition, and comparing 0x00RRGGBB against
        // 0xFFRRGGBB produces a "mismatch" whose two printed values are
        // identically the same colour.
        tokens.insert(*role, MdColorMath::argbFromHex(parts.at(1)) | 0xFF000000u);
    }
    return tokens;
}

ContrastLevel levelOf(const QString &variant)
{
    if (variant.contains(QStringLiteral("medium"))) {
        return ContrastLevel::Medium;
    }
    if (variant.contains(QStringLiteral("high"))) {
        return ContrastLevel::High;
    }
    return ContrastLevel::Standard;
}

ThemeMode modeOf(const QString &variant)
{
    return variant.startsWith(QStringLiteral("dark")) ? ThemeMode::Dark : ThemeMode::Light;
}

QStringList allVariants()
{
    return {QStringLiteral("light-standard"),
            QStringLiteral("light-medium"),
            QStringLiteral("light-high"),
            QStringLiteral("dark-standard"),
            QStringLiteral("dark-medium"),
            QStringLiteral("dark-high")};
}

/// Every mismatch between the vendored published table and the solver, as
/// "role (published #hex, solver #hex)" strings.
QStringList compareAgainstPublished(const QString &variant, bool *fixtureLoaded)
{
    const QHash<ColorRole, Argb> published = readPublishedTokens(variant);
    if (fixtureLoaded) {
        *fixtureLoaded = !published.isEmpty();
    }
    QStringList mismatches;
    if (published.isEmpty()) {
        return mismatches;
    }

    const MdDynamicScheme scheme = MdDynamicScheme::create(
        kDefaultSeed, modeOf(variant), SchemeVariant::TonalSpot, levelOf(variant));
    for (auto it = published.constBegin(); it != published.constEnd(); ++it) {
        const Argb resolved = MdColorSpec2021::color(scheme, it.key());
        if (resolved != it.value()) {
            mismatches.append(QStringLiteral("%1 (%2 vs %3)")
                                  .arg(roleLabel(it.key()),
                                       MdColorMath::prettyHex(it.value()),
                                       MdColorMath::prettyHex(resolved)));
        }
    }
    mismatches.sort();
    return mismatches;
}

} // namespace

class TestMd3Contrast : public QObject
{
    Q_OBJECT

private slots:
    void contrastLevelValuesMatchMaterialColorUtilities();
    void contrastCurveHitsItsCornersAndInterpolates();
    void lighterAndDarkerInvertTheRatioEquation();
    void unreachableRatiosAreReportedNotClamped();
    void foregroundToneFollowsTheBackground();
    void monochromePinsTheEndsOfTheScale();
    void contrastLevelsReachTheirFloorOrTheirGamutLimit();
    void aNominalToneIsKeptWhenItAlreadyClearsItsFloor();
    void aPairCanLoseRatioWhenTheForegroundSaturates();
    void highContrastMeetsItsAccessibilityTarget();
    void resizedSurfacesDeepenWithContrast();
    void resolvedRolesAreOpaqueAndDistinct();
    void toneDeltaPairsKeepTheirDistance();
    void vendoredPublishedTokenFixturesArePresent();
    void mcuSchemeExpectationsMatch();
    void dynamicTonalSpotIsNotTheStaticBaseline();
    void baselineSchemeIsThePublishedTokenSet();
};

void TestMd3Contrast::contrastLevelValuesMatchMaterialColorUtilities()
{
    // ContrastCurve documents its four corners as -1.0, 0.0, 0.5 and 1.0.
    QCOMPARE(contrastLevelValue(ContrastLevel::Reduced), -1.0);
    QCOMPARE(contrastLevelValue(ContrastLevel::Standard), 0.0);
    QCOMPARE(contrastLevelValue(ContrastLevel::Medium), 0.5);
    QCOMPARE(contrastLevelValue(ContrastLevel::High), 1.0);
}

void TestMd3Contrast::contrastCurveHitsItsCornersAndInterpolates()
{
    const MdContrastCurve curve(87.0, 87.0, 80.0, 75.0); // surface-dim, light

    QCOMPARE(curve.get(-1.0), 87.0);
    QCOMPARE(curve.get(0.0), 87.0);
    QCOMPARE(curve.get(0.5), 80.0);
    QCOMPARE(curve.get(1.0), 75.0);

    // Below -1.0 and above 1.0 clamp to the ends rather than extrapolating.
    QCOMPARE(curve.get(-3.0), 87.0);
    QCOMPARE(curve.get(4.0), 75.0);

    // Linear between the corners.
    QVERIFY(qFuzzyCompare(curve.get(0.25), 83.5));
    QVERIFY(qFuzzyCompare(curve.get(0.75), 77.5));

    // A curve whose corners are all equal is flat everywhere, which is how the
    // spec pins roles that must not move with contrast.
    const MdContrastCurve flat(96.0, 96.0, 96.0, 95.0);
    QCOMPARE(flat.get(0.0), 96.0);
    QCOMPARE(flat.get(0.4), 96.0);
    QCOMPARE(flat.get(1.0), 95.0);
}

void TestMd3Contrast::lighterAndDarkerInvertTheRatioEquation()
{
    // lighter(t, r) must itself sit at ratio r from t, to within the 0.4
    // luminance tolerance the algorithm deliberately adds.
    for (double tone : {10.0, 30.0, 50.0, 70.0, 90.0}) {
        for (double ratio : {3.0, 4.5, 7.0}) {
            const double up = MdContrast::lighter(tone, ratio);
            if (up >= 0.0) {
                QVERIFY2(up >= tone, qPrintable(QStringLiteral("lighter() must not go down")));
                QVERIFY(MdContrast::ratioOfTones(up, tone) >= ratio - 0.05);
            }
            const double down = MdContrast::darker(tone, ratio);
            if (down >= 0.0) {
                QVERIFY2(down <= tone, qPrintable(QStringLiteral("darker() must not go up")));
                QVERIFY(MdContrast::ratioOfTones(down, tone) >= ratio - 0.05);
            }
        }
    }

    // T100 has nothing lighter, so the request is refused rather than clamped.
    QCOMPARE(MdContrast::lighter(100.0, 4.5), -1.0);
    QCOMPARE(MdContrast::darker(0.0, 4.5), -1.0);
}

void TestMd3Contrast::unreachableRatiosAreReportedNotClamped()
{
    // The unsafe variants exist precisely so a caller always gets a usable tone.
    QCOMPARE(MdContrast::lighterUnsafe(100.0, 4.5), 100.0);
    QCOMPARE(MdContrast::darkerUnsafe(0.0, 4.5), 0.0);

    // Out-of-range input is rejected by the safe form and saturated by the
    // unsafe one, never silently reinterpreted.
    QCOMPARE(MdContrast::lighter(-5.0, 4.5), -1.0);
    QCOMPARE(MdContrast::darker(140.0, 4.5), -1.0);
}

void TestMd3Contrast::foregroundToneFollowsTheBackground()
{
    // A light background takes a dark foreground, and vice versa.
    QVERIFY(MdContrast::foregroundTone(98.0, 4.5) < 50.0);
    QVERIFY(MdContrast::foregroundTone(6.0, 4.5) > 50.0);

    // The two predicates are not the same question and must not be conflated:
    // tonePrefersLightForeground asks "would a viewer expect light text here",
    // toneAllowsLightForeground asks "can it actually carry the 4.5 ratio".
    QVERIFY(MdContrast::tonePrefersLightForeground(55.0));
    QVERIFY(!MdContrast::toneAllowsLightForeground(55.0));
    QVERIFY(MdContrast::toneAllowsLightForeground(49.0));
    QVERIFY(!MdContrast::tonePrefersLightForeground(60.0));

    QCOMPARE(MdContrast::enableLightForeground(55.0), 49.0);
    QCOMPARE(MdContrast::enableLightForeground(70.0), 70.0);
}

void TestMd3Contrast::monochromePinsTheEndsOfTheScale()
{
    for (ThemeMode mode : {ThemeMode::Light, ThemeMode::Dark}) {
        const MdDynamicScheme scheme = MdDynamicScheme::create(
            kDefaultSeed, mode, SchemeVariant::Monochrome, ContrastLevel::Standard);
        const bool dark = mode == ThemeMode::Dark;

        // Monochrome has no accent chroma, so primary is pushed to the extreme
        // of the scale and on-primary takes the opposite end.
        QCOMPARE(scheme.resolvedTone(ColorRole::Primary), dark ? 100.0 : 0.0);
        QCOMPARE(scheme.resolvedTone(ColorRole::OnPrimary), dark ? 10.0 : 90.0);
        QCOMPARE(scheme.resolvedTone(ColorRole::Surface), dark ? 6.0 : 98.0);
    }
}

void TestMd3Contrast::contrastLevelsReachTheirFloorOrTheirGamutLimit()
{
    // What a contrast level actually promises.
    //
    // A role's ContrastCurve is a *floor*, not a target. ColorSpec2021's
    // getTone, case 2 (single colour against a background) reads:
    //
    //     if (ratioOfTones(bgTone, answer) >= desiredRatio) {
    //       // Don't "improve" what's good enough.
    //     } else {
    //       answer = foregroundTone(bgTone, desiredRatio);
    //     }
    //
    // so a nominal tone that already clears the floor is left exactly where it
    // is, and a tone that cannot clear it is pushed as far as the scale allows.
    // That gives a complete, checkable contract: for every role pair and every
    // level, the achieved ratio either meets the floor, or the floor is
    // unreachable and the ratio sits at the best the gamut can do.
    //
    // This replaces an earlier, wrong assertion that every pair's ratio must
    // rise monotonically with the level. It does not, and it need not - see
    // aPairCanLoseRatioWhenTheForegroundSaturates().
    struct Pair {
        ColorRole foreground;
        ColorRole background;
        MdContrastCurve curve; // the published curve for `foreground`
    };
    const Pair pairs[] = {
        {ColorRole::OnSurface, ColorRole::Surface, {4.5, 7.0, 11.0, 21.0}},
        {ColorRole::OnSurfaceVariant, ColorRole::Surface, {3.0, 4.5, 7.0, 11.0}},
        {ColorRole::OnPrimary, ColorRole::Primary, {4.5, 7.0, 11.0, 21.0}},
        {ColorRole::OnPrimaryContainer, ColorRole::PrimaryContainer,
         {3.0, 4.5, 7.0, 11.0}},
        {ColorRole::OnSecondaryContainer, ColorRole::SecondaryContainer,
         {3.0, 4.5, 7.0, 11.0}},
        {ColorRole::Outline, ColorRole::Surface, {1.5, 3.0, 4.5, 7.0}},
    };

    for (int level = 0; level < int(ContrastLevel::Count); ++level) {
        const auto contrast = ContrastLevel(level);
        const double levelValue = contrastLevelValue(contrast);
        for (ThemeMode mode : {ThemeMode::Light, ThemeMode::Dark}) {
            const MdDynamicScheme scheme =
                MdDynamicScheme::create(kDefaultSeed, mode, SchemeVariant::TonalSpot, contrast);
            for (const Pair &pair : pairs) {
                const Argb fg = MdColorSpec2021::color(scheme, pair.foreground);
                const Argb bg = MdColorSpec2021::color(scheme, pair.background);
                const double achieved = MdColorMath::contrastRatio(fg, bg);
                const double floor = pair.curve.get(levelValue);

                // Best ratio any foreground tone can reach against this
                // background. The extreme tones bracket it because the ratio is
                // monotone in the tone distance.
                const double bgTone = scheme.resolvedTone(pair.background);
                const double ceiling = std::max(MdContrast::ratioOfTones(0.0, bgTone),
                                                MdContrast::ratioOfTones(100.0, bgTone));

                const QString label =
                    QStringLiteral("%1 %2/%3 at %4: achieved %5, floor %6, ceiling %7 "
                                   "(T%8/T%9)")
                        .arg(themeModeName(mode),
                             roleLabel(pair.foreground),
                             roleLabel(pair.background),
                             contrastLevelName(contrast))
                        .arg(achieved, 0, 'f', 3)
                        .arg(floor, 0, 'f', 3)
                        .arg(ceiling, 0, 'f', 3)
                        .arg(scheme.resolvedTone(pair.foreground), 0, 'f', 0)
                        .arg(bgTone, 0, 'f', 0);

                QVERIFY2(achieved >= floor - 0.1 || achieved >= ceiling - 0.1,
                         qPrintable(label));
            }
        }
    }
}

void TestMd3Contrast::aNominalToneIsKeptWhenItAlreadyClearsItsFloor()
{
    // The "don't improve what's good enough" half of the rule, stated as a
    // concrete case. At the standard level on-primary-container's floor against
    // primary-container is only 4.5, but its nominal tone 30 against
    // primary-container's tone 90 already gives about 7.3. The solver must
    // leave it at 30 rather than lightening it toward 4.5.
    const MdDynamicScheme standard = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::Standard);
    QCOMPARE(standard.resolvedTone(ColorRole::OnPrimaryContainer), 30.0);
    QCOMPARE(standard.resolvedTone(ColorRole::PrimaryContainer), 90.0);
    QVERIFY(MdColorMath::contrastRatio(
                MdColorSpec2021::color(standard, ColorRole::OnPrimaryContainer),
                MdColorSpec2021::color(standard, ColorRole::PrimaryContainer))
            > 7.0);

    // Nothing is touched, so the standard level reproduces the nominal tones
    // for the whole container ladder. These are the values the published token
    // sets use, which is a second, independent confirmation: the light sets
    // name `on-secondary-container` and `on-tertiary-container` as
    // `secondary30` and `tertiary30`.
    QVERIFY(qAbs(standard.resolvedTone(ColorRole::OnSecondaryContainer) - 30.0) < 0.1);
    QVERIFY(qAbs(standard.resolvedTone(ColorRole::OnTertiaryContainer) - 30.0) < 0.1);
    QVERIFY(qAbs(standard.resolvedTone(ColorRole::OnErrorContainer) - 30.0) < 0.1);
}

void TestMd3Contrast::aPairCanLoseRatioWhenTheForegroundSaturates()
{
    // A recorded surprise, pinned so that it is not "fixed".
    //
    // For light / on-primary-container against primary-container, the ratio at
    // the three levels runs about 7.3, then 5.2, then 9.0 - it goes *down* at
    // medium. That is correct, and here is the mechanism:
    //
    //   * primary-container is solved against the highest surface, whose own
    //     curve takes light surface-dim from T87 to T80 at medium and T75 at
    //     high. The container follows it down, from T90 to T46 to T31.
    //   * on-primary-container is then solved against that container. Its floor
    //     rises from 4.5 to 7.0, but the container at T46 leaves tone 100 as the
    //     only escape, and white on T46 is only about 5.2 - the floor is
    //     unreachable, so the solver takes the best available and saturates.
    //   * at high the container drops to T31, far enough that white clears 7.0
    //     again and the ratio recovers past its standard value.
    //
    // The tones are pinned exactly because they are the mechanism; the ratios
    // are pinned loosely because they depend on the rendered sRGB rounding.
    const MdDynamicScheme standard = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::Standard);
    const MdDynamicScheme medium = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::Medium);
    const MdDynamicScheme high = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::High);

    const auto toneOf = [](const MdDynamicScheme &scheme, ColorRole role) {
        return scheme.resolvedTone(role);
    };
    // Tones that come straight from the nominal table are exact. The ones the
    // solver moves are the output of foregroundTone(), so they are reals: the
    // medium container lands on 45.992..., not on 46.
    const auto near = [](double value, double expected) { return qAbs(value - expected) < 0.1; };

    QVERIFY(near(toneOf(standard, ColorRole::PrimaryContainer), 90.0));
    QVERIFY(near(toneOf(medium, ColorRole::PrimaryContainer), 46.0));
    QVERIFY(near(toneOf(high, ColorRole::PrimaryContainer), 31.0));

    // The foreground pins to the top of the scale as soon as the floor exceeds
    // what a lighter tone can buy.
    QVERIFY(near(toneOf(standard, ColorRole::OnPrimaryContainer), 30.0));
    QVERIFY(near(toneOf(medium, ColorRole::OnPrimaryContainer), 100.0));
    QVERIFY(near(toneOf(high, ColorRole::OnPrimaryContainer), 100.0));

    const auto ratioOf = [](const MdDynamicScheme &scheme) {
        return MdColorMath::contrastRatio(
            MdColorSpec2021::color(scheme, ColorRole::OnPrimaryContainer),
            MdColorSpec2021::color(scheme, ColorRole::PrimaryContainer));
    };
    QVERIFY2(ratioOf(medium) < ratioOf(standard),
             qPrintable(QStringLiteral("expected the medium level to dip: standard %1, medium %2")
                            .arg(ratioOf(standard), 0, 'f', 3)
                            .arg(ratioOf(medium), 0, 'f', 3)));
    QVERIFY2(ratioOf(high) > ratioOf(medium),
             qPrintable(QStringLiteral("expected the high level to recover: medium %1, high %2")
                            .arg(ratioOf(medium), 0, 'f', 3)
                            .arg(ratioOf(high), 0, 'f', 3)));

    // And the invariant that does hold: the dip never takes the pair below the
    // level-zero floor, so no contrast level is worse for accessibility than no
    // contrast level.
    QVERIFY(ratioOf(medium) >= 4.5);
    QVERIFY(ratioOf(high) >= 4.5);
}

void TestMd3Contrast::highContrastMeetsItsAccessibilityTarget()
{
    // on-surface's curve is (4.5, 7.0, 11.0, 21.0): the high level asks for the
    // maximum possible ratio. It will not always reach 21 (black on the
    // darkest surface cannot), but it must comfortably clear the medium target.
    for (ThemeMode mode : {ThemeMode::Light, ThemeMode::Dark}) {
        const MdDynamicScheme high = MdDynamicScheme::create(
            kDefaultSeed, mode, SchemeVariant::TonalSpot, ContrastLevel::High);
        const double ratio = MdColorMath::contrastRatio(
            MdColorSpec2021::color(high, ColorRole::OnSurface),
            MdColorSpec2021::color(high, ColorRole::Surface));
        QVERIFY2(ratio >= 11.0, qPrintable(QStringLiteral("%1: on-surface/surface at high "
                                                          "contrast is only %2")
                                               .arg(themeModeName(mode))
                                               .arg(ratio, 0, 'f', 3)));

        // on-primary-container's curve is (3.0, 4.5, 7.0, 11.0).
        const double pair = MdColorMath::contrastRatio(
            MdColorSpec2021::color(high, ColorRole::OnPrimaryContainer),
            MdColorSpec2021::color(high, ColorRole::PrimaryContainer));
        QVERIFY2(pair >= 7.0, qPrintable(QStringLiteral("%1: on-primary-container at high "
                                                        "contrast is only %2")
                                             .arg(themeModeName(mode))
                                             .arg(pair, 0, 'f', 3)));
    }
}

void TestMd3Contrast::resizedSurfacesDeepenWithContrast()
{
    // The container ladder is what makes raised surfaces readable, so higher
    // contrast has to spread the steps further apart, not merely shift them.
    const MdDynamicScheme standard = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::Standard);
    const MdDynamicScheme high = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::High);

    // Light mode: containers sit below surface and get darker as they rise.
    QCOMPARE(standard.resolvedTone(ColorRole::SurfaceContainerLowest), 100.0);
    QCOMPARE(standard.resolvedTone(ColorRole::SurfaceContainerHighest), 90.0);
    QCOMPARE(high.resolvedTone(ColorRole::SurfaceContainerLowest), 100.0);
    QCOMPARE(high.resolvedTone(ColorRole::SurfaceContainerHighest), 80.0);

    // surface-dim moves to make room; surface-bright is pinned in light mode.
    QCOMPARE(standard.resolvedTone(ColorRole::SurfaceDim), 87.0);
    QCOMPARE(high.resolvedTone(ColorRole::SurfaceDim), 75.0);
    QCOMPARE(standard.resolvedTone(ColorRole::SurfaceBright), 98.0);
    QCOMPARE(high.resolvedTone(ColorRole::SurfaceBright), 98.0);

    // Dark mode mirrors it on the bright side.
    const MdDynamicScheme darkHigh = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Dark, SchemeVariant::TonalSpot, ContrastLevel::High);
    QCOMPARE(darkHigh.resolvedTone(ColorRole::SurfaceBright), 34.0);
    QCOMPARE(darkHigh.resolvedTone(ColorRole::SurfaceContainerLowest), 0.0);
    QCOMPARE(darkHigh.resolvedTone(ColorRole::SurfaceContainerHighest), 30.0);
}

void TestMd3Contrast::resolvedRolesAreOpaqueAndDistinct()
{
    for (int level = 0; level < int(ContrastLevel::Count); ++level) {
        const auto contrast = ContrastLevel(level);
        for (ThemeMode mode : {ThemeMode::Light, ThemeMode::Dark}) {
            const MdDynamicScheme scheme =
                MdDynamicScheme::create(kDefaultSeed, mode, SchemeVariant::Vibrant, contrast);
            for (int i = 0; i < int(ColorRole::Count); ++i) {
                const Argb argb = MdColorSpec2021::color(scheme, ColorRole(i));
                QVERIFY2(qAlpha(QRgb(argb)) == 0xFF,
                         qPrintable(QStringLiteral("%1 resolved with alpha %2")
                                        .arg(colorRoleName(ColorRole(i)))
                                        .arg(qAlpha(QRgb(argb)))));
            }
            // Scrim and shadow are black by definition.
            QCOMPARE(MdColorSpec2021::color(scheme, ColorRole::Scrim), 0xFF000000u);
            QCOMPARE(MdColorSpec2021::color(scheme, ColorRole::Shadow), 0xFF000000u);
        }
    }
}

void TestMd3Contrast::toneDeltaPairsKeepTheirDistance()
{
    // A container and its accent must stay at least 10 tones apart, and an
    // `on-` colour must not collide with the container it sits on. This is the
    // part of the algorithm most likely to be mis-ported, so it is checked
    // across every variant and level rather than just the default.
    const ColorRole pairs[][2] = {
        {ColorRole::PrimaryContainer, ColorRole::Primary},
        {ColorRole::SecondaryContainer, ColorRole::Secondary},
        {ColorRole::TertiaryContainer, ColorRole::Tertiary},
        {ColorRole::ErrorContainer, ColorRole::Error},
        {ColorRole::PrimaryFixed, ColorRole::PrimaryFixedDim},
    };

    for (int variant = 0; variant < int(SchemeVariant::Count); ++variant) {
        for (int level = 0; level < int(ContrastLevel::Count); ++level) {
            for (ThemeMode mode : {ThemeMode::Light, ThemeMode::Dark}) {
                const MdDynamicScheme scheme =
                    MdDynamicScheme::create(kDefaultSeed, mode, SchemeVariant(variant),
                                            ContrastLevel(level));
                for (const auto &pair : pairs) {
                    const double a = scheme.resolvedTone(pair[0]);
                    const double b = scheme.resolvedTone(pair[1]);
                    if (qFuzzyCompare(a + 1.0, b + 1.0)) {
                        continue; // monochrome collapses some pairs by design
                    }
                    QVERIFY2(qAbs(a - b) >= 10.0 - 1e-9,
                             qPrintable(QStringLiteral("%1/%2 %3/%4: T%5 vs T%6 is only %7 apart")
                                            .arg(schemeVariantName(SchemeVariant(variant)),
                                                 themeModeName(mode),
                                                 colorRoleName(pair[0]),
                                                 colorRoleName(pair[1]))
                                            .arg(a, 0, 'f', 1)
                                            .arg(b, 0, 'f', 1)
                                            .arg(qAbs(a - b), 0, 'f', 1)));
                }
            }
        }
    }
}

void TestMd3Contrast::vendoredPublishedTokenFixturesArePresent()
{
    for (const QString &variant : allVariants()) {
        bool loaded = false;
        compareAgainstPublished(variant, &loaded);
        QVERIFY2(loaded, qPrintable(QStringLiteral(
                              "%1 fixture missing; regenerate with "
                              "tools/update-sys-color-fixtures.py")
                              .arg(variant)));
    }
}

void TestMd3Contrast::mcuSchemeExpectationsMatch()
{
    // The decisive suite. Every row is an assertion material-color-utilities
    // makes about its own scheme output, flattened by
    // tools/update-mcu-scheme-fixtures.py: nine variants, both modes, and the
    // minimum / standard / maximum contrast levels. If the solver drifts in any
    // of tone selection, contrast curves, tone-delta pairs or dual backgrounds,
    // rows here start failing.
    QFile file(QStringLiteral(QT_MD3_TOKEN_FIXTURES "/mcu-scheme-expectations.txt"));
    QVERIFY2(file.open(QIODevice::ReadOnly | QIODevice::Text),
             "mcu-scheme-expectations.txt missing; regenerate with "
             "tools/update-mcu-scheme-fixtures.py");

    const QHash<QString, ColorRole> names = roleNamesByName();
    QHash<QString, MdDynamicScheme> schemes;
    QStringList failures;
    int checked = 0;
    int failedTotal = 0;

    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#'))) {
            continue;
        }
        const QStringList parts = line.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (parts.size() != 6) {
            continue;
        }
        const QString variantName = parts.at(0);
        const QString seedText = parts.at(1);
        const bool dark = parts.at(2) == QLatin1String("dark");
        const QString contrastText = parts.at(3);
        const QString camelRole = parts.at(4);
        const QString expectedText = parts.at(5);

        // camelCase -> md.sys token name, then through the shared lookup.
        QString token;
        for (const QChar &character : camelRole) {
            if (character.isUpper() && !token.isEmpty()) {
                token += QLatin1Char('-');
            }
            token += character.toLower();
        }
        const auto role = names.constFind(token);
        if (role == names.constEnd()) {
            continue;
        }

        SchemeVariant variant = SchemeVariant::TonalSpot;
        for (int i = 0; i < int(SchemeVariant::Count); ++i) {
            if (schemeVariantName(SchemeVariant(i)) == variantName) {
                variant = SchemeVariant(i);
                break;
            }
        }
        ContrastLevel contrast = ContrastLevel::Standard;
        const double contrastDouble = contrastText.toDouble();
        if (qFuzzyCompare(contrastDouble + 1.0, 0.5 + 1.0)) {
            contrast = ContrastLevel::Medium;
        } else if (qFuzzyCompare(contrastDouble + 2.0, 1.0 + 2.0)) {
            contrast = ContrastLevel::High;
        } else if (contrastDouble < -0.5) {
            contrast = ContrastLevel::Reduced;
        }

        const QString key = QStringLiteral("%1|%2|%3|%4")
                                .arg(variantName, seedText)
                                .arg(dark ? 1 : 0)
                                .arg(contrastText);
        auto cached = schemes.find(key);
        if (cached == schemes.end()) {
            cached = schemes.insert(key,
                                    MdDynamicScheme::create(
                                        MdColorMath::argbFromHex(seedText.mid(2)),
                                        dark ? ThemeMode::Dark : ThemeMode::Light,
                                        variant, contrast));
        }

        const Argb expected = MdColorMath::argbFromHex(expectedText);
        const Argb actual = MdColorSpec2021::color(*cached, *role);
        ++checked;
        if (actual != expected) {
            ++failedTotal;
            if (failures.size() < 20) {
                failures.append(QStringLiteral("%1 seed %2 %3 contrast %4 %5: %6 != %7")
                                    .arg(variantName, seedText, dark ? QStringLiteral("dark")
                                                                     : QStringLiteral("light"),
                                         contrastText, token,
                                         MdColorMath::prettyHex(actual),
                                         MdColorMath::prettyHex(expected)));
            }
        }
    }

    QVERIFY2(checked > 300,
             qPrintable(QStringLiteral("only %1 expectations were parsed; the fixture has been "
                                       "truncated")
                            .arg(checked)));
    QVERIFY2(failures.isEmpty(),
             qPrintable(QStringLiteral("%1 of %2 upstream expectations failed:\n  %3")
                            .arg(failedTotal)
                            .arg(checked)
                            .arg(failures.join(QStringLiteral("\n  ")))));
}

void TestMd3Contrast::dynamicTonalSpotIsNotTheStaticBaseline()
{
    // These are two different things and must not be "reconciled":
    //
    //   * `md.sys.color` (the static baseline) is an authored token set built on
    //     the M3 reference palette, whose primary family carries the seed's own
    //     chroma. Its light primary for the default purple is #6750a4 and its
    //     error is #b3261e.
    //   * a dynamic scheme is *solved*. Tonal-spot specifies chroma 36 for the
    //     primary palette (ColorSpec2021.getPrimaryPalette), so the same seed
    //     yields #65558f, and the error palette is hue 25 / chroma 84, giving
    //     #ba1a1a. Both of those are exactly what upstream asserts in
    //     swift/Tests/.../SchemeTonalSpotTests.swift.
    //
    // So a mismatch here is the design, not a defect. The test pins it so that
    // someone "fixing" the dynamic path to reproduce the baseline is caught
    // immediately.
    const MdDynamicScheme scheme = MdDynamicScheme::create(
        kDefaultSeed, ThemeMode::Light, SchemeVariant::TonalSpot, ContrastLevel::Standard);

    QCOMPARE(MdColorSpec2021::color(scheme, ColorRole::Primary), 0xFF65558Fu);
    QCOMPARE(MdColorSpec2021::color(scheme, ColorRole::Error), 0xFFBA1A1Au);
    QCOMPARE(MdColorSpec2021::color(scheme, ColorRole::Surface), 0xFFFDF7FFu);

    // The static baseline, by contrast:
    const MdColorScheme baseline = MdColorScheme::baseline(ThemeMode::Light);
    QCOMPARE(baseline.color(ColorRole::Primary).rgb(), 0xFF6750A4u);
    QCOMPARE(baseline.color(ColorRole::Error).rgb(), 0xFFB3261Eu);

    // What the two *do* have to agree on is the hue family each role reads - the
    // tone may legitimately differ, the family may not. They are not bit-identical
    // hues: #6750a4 is an authored value from the reference palette, while
    // #65558f comes out of a solve that round-trips through tone/chroma and then
    // clips to the sRGB gamut at chroma 36, which nudges the hue slightly. A
    // fraction of a degree is expected; a different family is not.
    QCOMPARE_GT(MdHct(0xFF65558Fu).hue(), 297.0);
    QCOMPARE_LT(MdHct(0xFF65558Fu).hue(), 301.0);
    QCOMPARE_GT(MdHct(0xFF6750A4u).hue(), 297.0);
    QCOMPARE_LT(MdHct(0xFF6750A4u).hue(), 301.0);
    QVERIFY(qAbs(MdHct(0xFF65558Fu).hue() - MdHct(0xFF6750A4u).hue()) < 1.5);

    // The chroma, by contrast, is the whole difference and must not drift back:
    // 36 is what ColorSpec2021 prescribes for tonal-spot primary.
    QVERIFY(qAbs(MdDynamicScheme::create(kDefaultSeed, ThemeMode::Light,
                                        SchemeVariant::TonalSpot, ContrastLevel::Standard)
                     .primaryPalette()
                     .chroma()
                 - 36.0)
            < 1e-9);

    // And #6750a4 is genuinely the seed's own chroma, not an accident: rebuilding
    // the palette at the seed's own chroma and tone 40 reproduces it exactly.
    const MdHct seedHct(kDefaultSeed);
    QCOMPARE(MdTonalPalette(seedHct.hue(), seedHct.chroma()).tone(40.0), 0xFF6750A4u);
}

void TestMd3Contrast::baselineSchemeIsThePublishedTokenSet()
{
    // Every role of `MdColorScheme::baseline()` for all six published variants
    // must equal Google's own token file, role for role. `baseline()` is the
    // static system, so there is nothing to solve and nothing to interpret; a
    // mismatch means the table was transcribed or regenerated wrongly.
    for (const QString &variant : allVariants()) {
        const QHash<ColorRole, Argb> published = readPublishedTokens(variant);
        QVERIFY2(!published.isEmpty(),
                 qPrintable(QStringLiteral("%1 fixture missing").arg(variant)));

        const MdColorScheme scheme = MdColorScheme::baseline(modeOf(variant), levelOf(variant));
        QStringList mismatches;
        for (auto it = published.constBegin(); it != published.constEnd(); ++it) {
            const QColor actual = scheme.color(it.key());
            // readPublishedTokens() forces alpha opaque, so both sides are
            // 0xFFRRGGBB and a plain comparison is exact.
            if (!actual.isValid() || quint32(actual.rgb()) != it.value()) {
                mismatches.append(QStringLiteral("%1 (%2 != %3)")
                                      .arg(roleLabel(it.key()),
                                           actual.isValid()
                                               ? actual.name(QColor::HexRgb)
                                               : QStringLiteral("unset"),
                                           MdColorMath::prettyHex(it.value())));
            }
        }
        std::sort(mismatches.begin(), mismatches.end());
        QVERIFY2(mismatches.isEmpty(),
                 qPrintable(QStringLiteral("%1: %2 role(s) differ from the published set:\n  %3")
                                .arg(variant, QString::number(mismatches.size()),
                                     mismatches.join(QStringLiteral("\n  ")))));
    }
}

QTEST_MAIN(TestMd3Contrast)
#include "TestMd3Contrast.moc"
