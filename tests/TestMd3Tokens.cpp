// Token audit: every number in MdTokens is compared against the value
// published by the authoritative source. This is the correctness floor for the
// whole library — if these fail, nothing painted above them can be trusted.

#include "core/MdColorMath.h"
#include "core/MdColorScheme.h"
#include "core/MdDynamicColor.h"
#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTokens.h"
#include "core/MdTypeScale.h"

#include <QtTest/QtTest>

using namespace md;

class TestMd3Tokens : public QObject
{
    Q_OBJECT

private slots:
    // --- md.ref.palette.* -------------------------------------------------
    void referencePaletteMatchesSource();
    void referencePaletteCoversEveryTone();
    void referenceLookupByName();

    // --- md.sys.shape.corner.* -------------------------------------------
    void shapeScaleMatchesSource();

    // --- md.sys.elevation.level* -----------------------------------------
    void elevationMatchesSource();

    // --- md.sys.motion.* -------------------------------------------------
    void motionDurationsMatchSource();
    void motionEasingsMatchSource();
    void springsMatchComposeTokens();

    // --- md.sys.state.* --------------------------------------------------
    void stateLayerOpacitiesMatchSource();

    // --- md.sys.typescale.* ----------------------------------------------
    void baselineTypeScaleMatchesSource();
    void emphasizedTypeScaleMatchesCompose();
    void lineHeightAdaptsToScript();

    // --- colour science ---------------------------------------------------
    void hctRoundTrips();
    void hctMatchesKnownValue();
    void contrastRatioIsSane();

    // --- schemes ----------------------------------------------------------
    void baselineLightSchemeMatchesSource();
    void baselineDarkSchemeMatchesSource();
    void dynamicSchemeSitsOnDocumentedTones();
    void monochromeSchemeDropsChroma();
    void harmonizeKeepsChromaAndNudgesHue();
};

namespace {

QString roleHex(const MdColorScheme &scheme, ColorRole role)
{
    return scheme.hex(role);
}

/// L* (tone) of a QColor, via the same maths the palette uses.
double toneOf(const QColor &color)
{
    return MdColorMath::lstarFromArgb(MdColorMath::argbFromHex(color.name(QColor::HexRgb)));
}

} // namespace

void TestMd3Tokens::referencePaletteMatchesSource()
{
    // material-web tokens/versions/v0_192/_md-ref-palette.scss
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("primary"), 40).name(), QStringLiteral("#6750a4"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("primary"), 80).name(), QStringLiteral("#d0bcff"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("secondary"), 40).name(), QStringLiteral("#625b71"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("tertiary"), 40).name(), QStringLiteral("#7d5260"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("error"), 40).name(), QStringLiteral("#b3261e"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral"), 98).name(), QStringLiteral("#fef7ff"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral"), 6).name(), QStringLiteral("#141218"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral"), 4).name(), QStringLiteral("#0f0d13"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral"), 24).name(), QStringLiteral("#3b383e"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral-variant"), 30).name(),
             QStringLiteral("#49454f"));
    QCOMPARE(MdReferenceTokens::toneColor(QStringLiteral("neutral-variant"), 90).name(),
             QStringLiteral("#e7e0ec"));
}

void TestMd3Tokens::referencePaletteCoversEveryTone()
{
    // The brief requires the full 0/10/../100 ladder for the six palettes.
    const QList<int> primary = MdReferenceTokens::tones(QStringLiteral("primary"));
    QVERIFY(primary.contains(0));
    QVERIFY(primary.contains(10));
    QVERIFY(primary.contains(99));
    QVERIFY(primary.contains(100));

    // neutral additionally publishes the M3 surface tones.
    const QList<int> neutral = MdReferenceTokens::tones(QStringLiteral("neutral"));
    for (int tone : {4, 6, 12, 17, 22, 24, 87, 92, 94, 96, 98}) {
        QVERIFY2(neutral.contains(tone), qPrintable(QStringLiteral("missing neutral%1").arg(tone)));
    }

    QCOMPARE(MdReferenceTokens::paletteNames().size(), 6);
}

void TestMd3Tokens::referenceLookupByName()
{
    QCOMPARE(MdReferenceTokens::referenceColor(QStringLiteral("primary40")).name(),
             QStringLiteral("#6750a4"));
    // Must not mistake "neutral-variant90" for "neutral" + "90".
    QCOMPARE(MdReferenceTokens::referenceColor(QStringLiteral("neutral-variant90")).name(),
             QStringLiteral("#e7e0ec"));
    QCOMPARE(MdReferenceTokens::referenceColor(QStringLiteral("white")).name(),
             QStringLiteral("#ffffff"));
    QVERIFY(!MdReferenceTokens::referenceColor(QStringLiteral("primary45")).isValid());
}

void TestMd3Tokens::shapeScaleMatchesSource()
{
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::None), 0.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::ExtraSmall), 4.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::Small), 8.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::Medium), 12.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::Large), 16.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::ExtraLarge), 28.0);
    // M3 Expressive steps (Compose ShapeTokens.kt).
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::LargeIncreased), 20.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::ExtraLargeIncreased), 32.0);
    QCOMPARE(MdSystemTokens::cornerRadius(ShapeCorner::ExtraExtraLarge), 48.0);
    QCOMPARE(MdSystemTokens::shapeScale().size(), 10);
}

void TestMd3Tokens::elevationMatchesSource()
{
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level0), 0.0);
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level1), 1.0);
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level2), 3.0);
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level3), 6.0);
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level4), 8.0);
    QCOMPARE(MdSystemTokens::elevationDp(ElevationLevel::Level5), 12.0);
}

void TestMd3Tokens::motionDurationsMatchSource()
{
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Short1), 50);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Short4), 200);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Medium1), 250);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Medium4), 400);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Long1), 450);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::Long4), 600);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::ExtraLong1), 700);
    QCOMPARE(MdSystemTokens::durationMs(MotionDuration::ExtraLong4), 1000);
}

void TestMd3Tokens::motionEasingsMatchSource()
{
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::Linear),
             QList<qreal>({0.0, 0.0, 1.0, 1.0}));
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::Standard),
             QList<qreal>({0.2, 0.0, 0.0, 1.0}));
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::StandardAccelerate),
             QList<qreal>({0.3, 0.0, 1.0, 1.0}));
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::StandardDecelerate),
             QList<qreal>({0.0, 0.0, 0.0, 1.0}));
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::EmphasizedAccelerate),
             QList<qreal>({0.3, 0.0, 0.8, 0.15}));
    QCOMPARE(MdSystemTokens::easingBezier(MotionEasing::EmphasizedDecelerate),
             QList<qreal>({0.05, 0.7, 0.1, 1.0}));

    // Endpoints must be exact for every curve.
    for (int i = 0; i < int(MotionEasing::Count); ++i) {
        const auto easing = static_cast<MotionEasing>(i);
        QCOMPARE(MdMotion::easedValue(easing, 0.0), 0.0);
        QCOMPARE(MdMotion::easedValue(easing, 1.0), 1.0);
    }
}

void TestMd3Tokens::springsMatchComposeTokens()
{
    // androidx Compose Material3 ExpressiveMotionTokens.kt
    qreal stiffness = 0.0;
    qreal damping = 0.0;

    MdSystemTokens::springParameters(MotionSpring::SpatialFast, &stiffness, &damping);
    QCOMPARE(stiffness, 800.0);
    QCOMPARE(damping, 0.6);

    MdSystemTokens::springParameters(MotionSpring::SpatialDefault, &stiffness, &damping);
    QCOMPARE(stiffness, 380.0);
    QCOMPARE(damping, 0.8);

    MdSystemTokens::springParameters(MotionSpring::SpatialSlow, &stiffness, &damping);
    QCOMPARE(stiffness, 200.0);
    QCOMPARE(damping, 0.8);

    MdSystemTokens::springParameters(MotionSpring::EffectsFast, &stiffness, &damping);
    QCOMPARE(stiffness, 3800.0);
    QCOMPARE(damping, 1.0);

    MdSystemTokens::springParameters(MotionSpring::EffectsDefault, &stiffness, &damping);
    QCOMPARE(stiffness, 1600.0);
    QCOMPARE(damping, 1.0);

    MdSystemTokens::springParameters(MotionSpring::EffectsSlow, &stiffness, &damping);
    QCOMPARE(stiffness, 800.0);
    QCOMPARE(damping, 1.0);

    // The solver must start at rest and converge.
    const MdSpring spatial = MdMotion::spring(MotionSpring::SpatialDefault);
    QCOMPARE(spatial.valueAt(0.0), 0.0);
    QVERIFY(qAbs(spatial.valueAt(2.0) - 1.0) < 0.01);
    QVERIFY(spatial.settlingDurationMs() > 0.0);
    QVERIFY(spatial.settlingDurationMs() < 3000.0);
}

void TestMd3Tokens::stateLayerOpacitiesMatchSource()
{
    QCOMPARE(MdSystemTokens::stateLayerOpacity(StateLayerKind::Hover), 0.08);
    QCOMPARE(MdSystemTokens::stateLayerOpacity(StateLayerKind::Focus), 0.12);
    QCOMPARE(MdSystemTokens::stateLayerOpacity(StateLayerKind::Pressed), 0.12);
    QCOMPARE(MdSystemTokens::stateLayerOpacity(StateLayerKind::Dragged), 0.16);
}

void TestMd3Tokens::baselineTypeScaleMatchesSource()
{
    const MdTypeStyleSpec display = MdTypeScale::spec(TypeStyle::DisplayLarge);
    QCOMPARE(display.size, 57.0);
    QCOMPARE(display.lineHeight, 64.0);
    QCOMPARE(display.tracking, -0.25);
    QCOMPARE(display.weight, 400);
    QCOMPARE(display.familyToken, QStringLiteral("brand"));

    const MdTypeStyleSpec body = MdTypeScale::spec(TypeStyle::BodyLarge);
    QCOMPARE(body.size, 16.0);
    QCOMPARE(body.lineHeight, 24.0);
    QCOMPARE(body.tracking, 0.5);
    QCOMPARE(body.weight, 400);

    const MdTypeStyleSpec bodyMedium = MdTypeScale::spec(TypeStyle::BodyMedium);
    QCOMPARE(bodyMedium.size, 14.0);
    QCOMPARE(bodyMedium.lineHeight, 20.0);
    QCOMPARE(bodyMedium.tracking, 0.25);

    const MdTypeStyleSpec labelLarge = MdTypeScale::spec(TypeStyle::LabelLarge);
    QCOMPARE(labelLarge.weight, 500);
    QCOMPARE(labelLarge.tracking, 0.1);

    const MdTypeStyleSpec labelSmall = MdTypeScale::spec(TypeStyle::LabelSmall);
    QCOMPARE(labelSmall.size, 11.0);
    QCOMPARE(labelSmall.tracking, 0.5);
}

void TestMd3Tokens::emphasizedTypeScaleMatchesCompose()
{
    // Compose Material3 — every emphasized style is heavier than its baseline.
    for (int i = 0; i < int(TypeStyle::Count); ++i) {
        const auto style = static_cast<TypeStyle>(i);
        const MdTypeStyleSpec baseline = MdTypeScale::spec(style, TypeEmphasis::Baseline);
        const MdTypeStyleSpec emphasized = MdTypeScale::spec(style, TypeEmphasis::Emphasized);
        QVERIFY2(emphasized.weight > baseline.weight,
                 qPrintable(QStringLiteral("style %1 is not heavier when emphasized")
                                .arg(typeStyleName(style))));
        // Size and line height are unchanged by emphasis.
        QCOMPARE(emphasized.size, baseline.size);
        QCOMPARE(emphasized.lineHeight, baseline.lineHeight);
    }

    QCOMPARE(MdTypeScale::spec(TypeStyle::TitleMedium, TypeEmphasis::Emphasized).weight, 700);
    QCOMPARE(MdTypeScale::spec(TypeStyle::LabelLarge, TypeEmphasis::Emphasized).weight, 700);
    QCOMPARE(MdTypeScale::spec(TypeStyle::DisplayLarge, TypeEmphasis::Emphasized).weight, 500);
}

void TestMd3Tokens::lineHeightAdaptsToScript()
{
    QCOMPARE(MdTypeScale::lineHeightMultiplier(ScriptCategory::Small), 1.0);
    QCOMPARE(MdTypeScale::lineHeightMultiplier(ScriptCategory::Medium), 1.07);

    // A Chinese label must not reuse the Latin line height.
    const qreal latin = MdTypeScale::lineHeight(TypeStyle::BodyMedium, TypeEmphasis::Baseline,
                                                ScriptCategory::Small);
    const qreal cjk = MdTypeScale::lineHeight(TypeStyle::BodyMedium, TypeEmphasis::Baseline,
                                              ScriptCategory::Medium);
    QCOMPARE(latin, 20.0);
    QVERIFY(cjk > latin);

    QCOMPARE(MdTypeScale::scriptCategoryForLanguage(QStringLiteral("zh-Hans")),
             ScriptCategory::Medium);
    QCOMPARE(MdTypeScale::scriptCategoryForLanguage(QStringLiteral("ja")),
             ScriptCategory::Medium);
    QCOMPARE(MdTypeScale::scriptCategoryForLanguage(QStringLiteral("en-US")),
             ScriptCategory::Small);
}

void TestMd3Tokens::hctRoundTrips()
{
    for (double hue : {0.0, 45.0, 120.0, 200.0, 275.0, 359.0}) {
        for (double chroma : {0.0, 16.0, 48.0}) {
            for (double tone : {0.0, 10.0, 50.0, 90.0, 100.0}) {
                const MdHct original(hue, chroma, tone);
                const MdHct roundTripped(original.toInt());
                // Hue is meaningless at zero chroma.
                if (chroma > 1.0) {
                    QVERIFY2(qAbs(MdColorMath::diffDegrees(original.hue(), roundTripped.hue())) < 1.5,
                             qPrintable(QStringLiteral("hue drift at h=%1 c=%2 t=%3: %4 vs %5")
                                            .arg(hue).arg(chroma).arg(tone)
                                            .arg(original.hue()).arg(roundTripped.hue())));
                }
                QVERIFY2(qAbs(original.tone() - roundTripped.tone()) < 0.6,
                         qPrintable(QStringLiteral("tone drift at h=%1 c=%2 t=%3: %4 vs %5")
                                        .arg(hue).arg(chroma).arg(tone)
                                        .arg(original.tone()).arg(roundTripped.tone())));
            }
        }
    }
}

void TestMd3Tokens::hctMatchesKnownValue()
{
    // CAM16 ground truth transcribed from material-color-utilities'
    // dart/test/hct_test.dart (cam_red / cam_green / cam_blue / cam_white /
    // cam_black). These six numbers per colour pin the whole appearance model:
    // the chromatic adaptation, the post-adaptation non-linearities and the
    // hue/colourfulness predictors all have to be right for them to agree.
    struct CamCase {
        const char *hex;
        double j;
        double chroma;
        double hue;
        double m;
        double s;
        double q;
    };
    static const CamCase kCases[] = {
        {"#ff0000", 46.445, 113.357, 27.408, 89.494, 91.889, 105.988},
        {"#00ff00", 79.331, 108.410, 142.139, 85.587, 78.604, 138.520},
        {"#0000ff", 25.465, 87.230, 282.788, 68.867, 93.674, 78.481},
        {"#ffffff", 100.0, 2.869, 209.492, 2.265, 12.068, 155.521},
        {"#000000", 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    };
    for (const CamCase &c : kCases) {
        const QString hex = QString::fromUtf8(c.hex);
        const MdCam16 cam = MdColorMath::camFromInt(MdColorMath::argbFromHex(hex));
        QVERIFY2(qAbs(cam.j - c.j) < 1e-3, qPrintable(QStringLiteral("%1 J %2").arg(hex).arg(cam.j)));
        QVERIFY2(qAbs(cam.chroma - c.chroma) < 1e-3,
                 qPrintable(QStringLiteral("%1 C %2").arg(hex).arg(cam.chroma)));
        QVERIFY2(qAbs(cam.hue - c.hue) < 1e-3,
                 qPrintable(QStringLiteral("%1 h %2").arg(hex).arg(cam.hue)));
        QVERIFY2(qAbs(cam.m - c.m) < 1e-3, qPrintable(QStringLiteral("%1 M %2").arg(hex).arg(cam.m)));
        QVERIFY2(qAbs(cam.s - c.s) < 1e-3, qPrintable(QStringLiteral("%1 s %2").arg(hex).arg(cam.s)));
        QVERIFY2(qAbs(cam.q - c.q) < 1e-3, qPrintable(QStringLiteral("%1 Q %2").arg(hex).arg(cam.q)));
    }

    // A neutral is *not* chroma 0 in CAM16: #ffffff legitimately reports 2.869.
    // Greys therefore sit below that floor rather than at exactly zero.
    QVERIFY(MdHct(0xFFFFFFFFu).chroma() < 2.9);
    QVERIFY(MdHct(0xFF000000u).chroma() < 1e-9);

    // The default M3 primary is the reference palette's primary40. Its HCT hue
    // is ~299, not the ~258 of the sRGB wheel: CAM16 hue and sRGB hue are
    // different quantities, which is exactly why the port has to be numeric.
    const Argb primary40 = MdColorMath::argbFromHex(QStringLiteral("#6750a4"));
    const MdHct hct(primary40);
    QCOMPARE(hct.toInt(), primary40);
    QVERIFY(qAbs(hct.tone() - MdColorMath::lstarFromArgb(primary40)) < 1e-9);
    QVERIFY(hct.chroma() > 45.0 && hct.chroma() < 50.0);
    QVERIFY(hct.hue() > 280.0 && hct.hue() < 320.0);
}

void TestMd3Tokens::contrastRatioIsSane()
{
    QVERIFY(qAbs(MdColorMath::contrastRatio(0xFFFFFFFFu, 0xFF000000u) - 21.0) < 0.5);
    QCOMPARE(MdColorMath::contrastRatio(0xFF123456u, 0xFF123456u), 1.0);

    // on-primary must clear WCAG AA against primary in both modes.
    const MdColorScheme light = MdColorScheme::baseline(ThemeMode::Light);
    QVERIFY(MdColorMath::contrastRatio(
                MdColorMath::argbFromHex(light.hex(ColorRole::Primary)),
                MdColorMath::argbFromHex(light.hex(ColorRole::OnPrimary)))
            >= 4.5);

    const MdColorScheme dark = MdColorScheme::baseline(ThemeMode::Dark);
    QVERIFY(MdColorMath::contrastRatio(
                MdColorMath::argbFromHex(dark.hex(ColorRole::Primary)),
                MdColorMath::argbFromHex(dark.hex(ColorRole::OnPrimary)))
            >= 4.5);
}

void TestMd3Tokens::baselineLightSchemeMatchesSource()
{
    const MdColorScheme scheme = MdColorScheme::baseline(ThemeMode::Light);
    QVERIFY(!scheme.isDynamic());
    QCOMPARE(scheme.colors().size(), int(ColorRole::Count));

    QCOMPARE(roleHex(scheme, ColorRole::Primary), QStringLiteral("#6750a4"));
    QCOMPARE(roleHex(scheme, ColorRole::OnPrimary), QStringLiteral("#ffffff"));
    QCOMPARE(roleHex(scheme, ColorRole::PrimaryContainer), QStringLiteral("#eaddff"));
    QCOMPARE(roleHex(scheme, ColorRole::OnPrimaryContainer), QStringLiteral("#21005d"));
    QCOMPARE(roleHex(scheme, ColorRole::Surface), QStringLiteral("#fef7ff"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerLowest), QStringLiteral("#ffffff"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainer), QStringLiteral("#f3edf7"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerHighest), QStringLiteral("#e6e0e9"));
    QCOMPARE(roleHex(scheme, ColorRole::OnSurfaceVariant), QStringLiteral("#49454f"));
    QCOMPARE(roleHex(scheme, ColorRole::Outline), QStringLiteral("#79747e"));
    // _md-sys-color.scss light: outline-variant = neutral-variant80 (#cac4d0).
    // (neutral-variant90 = #e7e0ec, which is a different token.)
    QCOMPARE(roleHex(scheme, ColorRole::OutlineVariant), QStringLiteral("#cac4d0"));
    QCOMPARE(roleHex(scheme, ColorRole::Error), QStringLiteral("#b3261e"));
    QCOMPARE(roleHex(scheme, ColorRole::InverseSurface), QStringLiteral("#322f35"));
    QCOMPARE(roleHex(scheme, ColorRole::InverseOnSurface), QStringLiteral("#f5eff7"));
    QCOMPARE(roleHex(scheme, ColorRole::Scrim), QStringLiteral("#000000"));
    QCOMPARE(roleHex(scheme, ColorRole::PrimaryFixed), QStringLiteral("#eaddff"));
    QCOMPARE(roleHex(scheme, ColorRole::PrimaryFixedDim), QStringLiteral("#d0bcff"));
    QCOMPARE(roleHex(scheme, ColorRole::OnPrimaryFixedVariant), QStringLiteral("#4f378b"));
}

void TestMd3Tokens::baselineDarkSchemeMatchesSource()
{
    const MdColorScheme scheme = MdColorScheme::baseline(ThemeMode::Dark);
    QVERIFY(scheme.isDark());

    QCOMPARE(roleHex(scheme, ColorRole::Primary), QStringLiteral("#d0bcff"));
    QCOMPARE(roleHex(scheme, ColorRole::OnPrimary), QStringLiteral("#381e72"));
    QCOMPARE(roleHex(scheme, ColorRole::PrimaryContainer), QStringLiteral("#4f378b"));
    QCOMPARE(roleHex(scheme, ColorRole::Surface), QStringLiteral("#141218"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerLowest), QStringLiteral("#0f0d13"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerLow), QStringLiteral("#1d1b20"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainer), QStringLiteral("#211f26"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerHigh), QStringLiteral("#2b2930"));
    QCOMPARE(roleHex(scheme, ColorRole::SurfaceContainerHighest), QStringLiteral("#36343b"));
    QCOMPARE(roleHex(scheme, ColorRole::OnSurfaceVariant), QStringLiteral("#cac4d0"));
    QCOMPARE(roleHex(scheme, ColorRole::Outline), QStringLiteral("#938f99"));
    QCOMPARE(roleHex(scheme, ColorRole::OutlineVariant), QStringLiteral("#49454f"));
    QCOMPARE(roleHex(scheme, ColorRole::Error), QStringLiteral("#f2b8b5"));
    QCOMPARE(roleHex(scheme, ColorRole::InverseSurface), QStringLiteral("#e6e0e9"));
    // The fixed roles do not move between modes.
    QCOMPARE(roleHex(scheme, ColorRole::PrimaryFixed), QStringLiteral("#eaddff"));
}

void TestMd3Tokens::dynamicSchemeSitsOnDocumentedTones()
{
    const QColor seed(QStringLiteral("#0061a4"));
    const MdColorScheme light = MdColorScheme::dynamic(seed, ThemeMode::Light);
    QVERIFY(light.isDynamic());

    // The scheme must place each role on the documented tone for its mode.
    QVERIFY(qAbs(toneOf(light.color(ColorRole::Primary)) - 40.0) < 0.6);
    QVERIFY(qAbs(toneOf(light.color(ColorRole::Surface)) - 98.0) < 0.6);
    QVERIFY(qAbs(toneOf(light.color(ColorRole::SurfaceContainer)) - 94.0) < 0.6);
    QVERIFY(qAbs(toneOf(light.color(ColorRole::OnSurfaceVariant)) - 30.0) < 0.6);
    QVERIFY(qAbs(toneOf(light.color(ColorRole::Outline)) - 50.0) < 0.6);

    const MdColorScheme dark = MdColorScheme::dynamic(seed, ThemeMode::Dark);
    QVERIFY(qAbs(toneOf(dark.color(ColorRole::Primary)) - 80.0) < 0.6);
    QVERIFY(qAbs(toneOf(dark.color(ColorRole::Surface)) - 6.0) < 0.6);
    QVERIFY(qAbs(toneOf(dark.color(ColorRole::OnSurfaceVariant)) - 80.0) < 0.6);
    QVERIFY(qAbs(toneOf(dark.color(ColorRole::SurfaceContainerHighest)) - 22.0) < 0.6);

    // Changing the seed must actually change the roles.
    const MdColorScheme other = MdColorScheme::dynamic(QColor(QStringLiteral("#7d5260")),
                                                      ThemeMode::Light);
    QVERIFY(other.color(ColorRole::Primary) != light.color(ColorRole::Primary));
}

void TestMd3Tokens::monochromeSchemeDropsChroma()
{
    // MCU ColorSpec2021::getPrimaryPalette: MONOCHROME uses chroma 0.0 for all
    // five palettes, so every role collapses onto the neutral tone ladder.
    const MdColorScheme scheme = MdColorScheme::dynamic(QColor(QStringLiteral("#0061a4")),
                                                        ThemeMode::Light,
                                                        SchemeVariant::Monochrome);
    const Argb primary = MdColorMath::argbFromHex(scheme.hex(ColorRole::Primary));

    // CAM16 does not put greys at chroma 0 (pure white is 2.869), so "grey"
    // here means "below the neutral floor", plus exactly equal sRGB channels.
    const MdHct primaryHct(primary);
    QVERIFY(primaryHct.chroma() < 2.9);
    QCOMPARE(qRed(QRgb(primary)), qGreen(QRgb(primary)));
    QCOMPARE(qGreen(QRgb(primary)), qBlue(QRgb(primary)));

    // primary is tone 40 of a chroma-0 palette, which is the plain L*=40 grey.
    QCOMPARE(primary, MdColorMath::intFromLstar(40.0));
    // ...and so are the other scheme families: no hue survives anywhere.
    QCOMPARE(scheme.color(ColorRole::Tertiary), scheme.color(ColorRole::Secondary));

    // The error palette is deliberately exempt: ColorSpec2021::getErrorPalette
    // returns empty for every variant, so MD3's error red (hue 25 / chroma 84)
    // is used unchanged even in a monochrome scheme.
    QCOMPARE(scheme.hex(ColorRole::ErrorContainer), QStringLiteral("#ffdad6"));
    QCOMPARE(scheme.hex(ColorRole::Error), QStringLiteral("#ba1a1a"));

    // The variant must actually change the result versus the default.
    const MdColorScheme spot = MdColorScheme::dynamic(QColor(QStringLiteral("#0061a4")),
                                                      ThemeMode::Light,
                                                      SchemeVariant::TonalSpot);
    QVERIFY(MdHct(MdColorMath::argbFromHex(spot.hex(ColorRole::Primary))).chroma() > 20.0);
}

void TestMd3Tokens::harmonizeKeepsChromaAndNudgesHue()
{
    // A neutral design colour must survive harmonization untouched.
    const Argb grey = MdColorMath::argbFromHex(QStringLiteral("#808080"));
    QCOMPARE(MdDynamicColor::harmonize(grey, MdColorMath::argbFromHex(QStringLiteral("#0061a4"))), grey);

    // A chromatic one moves toward the source, by at most 15 degrees.
    const Argb design = MdColorMath::argbFromHex(QStringLiteral("#e91e63"));
    const Argb source = MdColorMath::argbFromHex(QStringLiteral("#0061a4"));
    const MdHct before(design);
    const MdHct after(MdDynamicColor::harmonize(design, source));
    const double moved = MdColorMath::diffDegrees(before.hue(), after.hue());
    QVERIFY(moved > 0.0);
    QVERIFY(moved <= 15.0 + 1e-6);
    QVERIFY(qAbs(before.chroma() - after.chroma()) < 2.0);
}

QTEST_MAIN(TestMd3Tokens)
#include "TestMd3Tokens.moc"
