// TestMd3LoadingIndicator — the Expressive loading indicator family.
//
// Pins, field by field:
//
//   * the token export (34.0.21): the metric rows, the colour roles, the
//     corner-full container shape, the deprecated container colour's
//     transcription, and the variant-qualified override keys;
//   * the shape engine: normalized bounds, the MaterialShapes catalogue
//     geometry (vertex counts, near-constant circle), and the radial
//     morph's endpoint contracts;
//   * the Compose-port animation math as pure functions: the closed-form
//     spring, the 650 ms morph grid, the quarter-turn steps over the 4666 ms
//     linear spin, and the determinate half-turn sweep;
//   * the widget contract: token-driven size hint, the non-interactivity
//     contract, repaints only while an indeterminate cycle is showing, and
//     contained render smoke checks.

#include "TestMd3Common.h"

#include "core/MdLoadingIndicatorTokens.h"
#include "core/MdMaterialShapes.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdLoadingIndicatorStyle.h"
#include "widgets/MdLoadingIndicator.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtTest/QTest>

#include <algorithm>
#include <cmath>

namespace md {
namespace {

bool closeTo(double a, double b, double eps = 1e-6)
{
    return std::abs(a - b) <= eps;
}

bool pointsClose(const QPointF &a, const QPointF &b, double eps = 1e-6)
{
    return std::abs(a.x() - b.x()) <= eps && std::abs(a.y() - b.y()) <= eps;
}

int colorDistance(const QColor &a, const QColor &b)
{
    return std::abs(a.red() - b.red()) + std::abs(a.green() - b.green())
        + std::abs(a.blue() - b.blue());
}

bool colorsClose(const QColor &a, const QColor &b, int tolerance)
{
    return colorDistance(a, b) <= tolerance;
}

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------

class TestMd3LoadingIndicator : public QObject
{
    Q_OBJECT

private slots:
    void tokenExportRows();
    void tokenVariantOverrides();
    void shapeEngineNormalizedBounds();
    void shapeCatalogueGeometry();
    void morphEndpoints();
    void morphIntermediateStaysWithin();
    void springMath();
    void indeterminateFrames();
    void determinateFrames();
    void shapeScaleFactor();
    void widgetContract();
    void widgetRepaints();
    void widgetPainting();
};

void TestMd3LoadingIndicator::tokenExportRows()
{
    // md.comp.loading-indicator.*, export 34.0.21.
    const MdLoadingIndicatorTokens plain =
        MdLoadingIndicatorTokens::resolve(LoadingIndicatorVariant::Plain);
    QCOMPARE(plain.activeIndicatorSize, 38.0);
    QCOMPARE(plain.containerWidth, 48.0);
    QCOMPARE(plain.containerHeight, 48.0);
    QCOMPARE(plain.containerShape, ShapeCorner::Full);
    QCOMPARE(plain.activeIndicatorColor, ColorRole::Primary);
    QCOMPARE(plain.containedActiveIndicatorColor, ColorRole::OnPrimaryContainer);
    QCOMPARE(plain.containedContainerColor, ColorRole::PrimaryContainer);
    // The deprecated row is transcribed, never exposed as the API default.
    QCOMPARE(plain.deprecatedContainerColor, ColorRole::SecondaryContainer);

    // The contained variant shares the metric rows (the export publishes no
    // contained metrics) and differs only in the colour mapping, which the
    // painter reads by variant.
    const MdLoadingIndicatorTokens contained =
        MdLoadingIndicatorTokens::resolve(LoadingIndicatorVariant::Contained);
    QCOMPARE(contained.activeIndicatorSize, plain.activeIndicatorSize);
    QCOMPARE(contained.containerWidth, plain.containerWidth);
    QCOMPARE(contained.containerHeight, plain.containerHeight);
}

void TestMd3LoadingIndicator::tokenVariantOverrides()
{
    // Variant-qualified keys win over the base ones; lengths only.
    MdComponentTokens &global = MdComponentTokens::global();
    global.setValue(QStringLiteral("md.comp.loading-indicator.active-indicator.size"),
                    QStringLiteral("40px"));
    global.setValue(QStringLiteral("md.comp.loading-indicator.container.width"),
                    QStringLiteral("56px"));
    global.setValue(QStringLiteral("md.comp.loading-indicator.contained.container.width"),
                    QStringLiteral("64px"));

    const MdLoadingIndicatorTokens plain =
        MdLoadingIndicatorTokens::resolve(LoadingIndicatorVariant::Plain);
    QCOMPARE(plain.activeIndicatorSize, 40.0);
    QCOMPARE(plain.containerWidth, 56.0);
    QCOMPARE(plain.containerHeight, 48.0);

    const MdLoadingIndicatorTokens contained =
        MdLoadingIndicatorTokens::resolve(LoadingIndicatorVariant::Contained);
    QCOMPARE(contained.activeIndicatorSize, 40.0);
    QCOMPARE(contained.containerWidth, 64.0);
    QCOMPARE(contained.containerHeight, 48.0);

    global.remove(QStringLiteral("md.comp.loading-indicator.active-indicator.size"));
    global.remove(QStringLiteral("md.comp.loading-indicator.container.width"));
    global.remove(QStringLiteral("md.comp.loading-indicator.contained.container.width"));
}

// ---------------------------------------------------------------------------
// Shape engine
// ---------------------------------------------------------------------------

void TestMd3LoadingIndicator::shapeEngineNormalizedBounds()
{
    const QList<MdRoundedPolygon> shapes = MdMaterialShapes::indeterminatePolygons();
    QCOMPARE(shapes.size(), 7);
    for (const MdRoundedPolygon &shape : shapes) {
        // The reference normalizes against the *control-point* hull, so the
        // true curve may sit a hair inside the unit square — a small
        // tolerance, not an exact 0..1 pin.
        double bounds[4];
        shape.calculateBounds(bounds);
        QVERIFY(bounds[0] > -0.05);
        QVERIFY(bounds[1] > -0.05);
        QVERIFY(bounds[2] < 1.05);
        QVERIFY(bounds[3] < 1.05);
        // Max bounds (the rotation-holding square) contain the bounds.
        double maxBounds[4];
        shape.calculateMaxBounds(maxBounds);
        QVERIFY(maxBounds[0] <= bounds[0] + 1e-6);
        QVERIFY(maxBounds[1] <= bounds[1] + 1e-6);
        QVERIFY(maxBounds[2] >= bounds[2] - 1e-6);
        QVERIFY(maxBounds[3] >= bounds[3] - 1e-6);
    }
}

void TestMd3LoadingIndicator::shapeCatalogueGeometry()
{
    // softBurst: 10 repetitions × 2 points = 20 outline points — the cubic
    // count reflects it (20 corners + 20 edges, minus the closing join).
    const MdRoundedPolygon softBurst = MdMaterialShapes::softBurst();
    QVERIFY(softBurst.cubics().size() >= 38);

    // pentagon: mirrored single repetition = 5 points.
    const MdRoundedPolygon pentagon = MdMaterialShapes::pentagon();
    QVERIFY(pentagon.cubics().size() >= 8);

    // The circle approximates its radius: the outline's distance from the
    // center is near-constant (the 10-vertex rounding construction).
    const MdRoundedPolygon circle = MdMaterialShapes::circle();
    const MdMorph self(circle, circle);
    const QVector<QPointF> samples = self.samplesAt(0.0);
    QVERIFY(samples.size() >= 8);
    qreal minRadius = 1e9;
    qreal maxRadius = 0.0;
    const QPointF c = circle.center();
    for (const QPointF &sample : samples) {
        const qreal r = std::hypot(sample.x() - c.x(), sample.y() - c.y());
        minRadius = std::min(minRadius, r);
        maxRadius = std::max(maxRadius, r);
    }
    QVERIFY(maxRadius - minRadius < 0.01);
}

void TestMd3LoadingIndicator::morphEndpoints()
{
    // progress 0 reproduces the start outline; progress 1 the end outline.
    const MdRoundedPolygon start = MdMaterialShapes::softBurst();
    const MdRoundedPolygon end = MdMaterialShapes::cookie9Sided();
    const MdMorph morph(start, end);

    const QVector<QPointF> atStart = morph.samplesAt(0.0);
    const QVector<QPointF> atEnd = morph.samplesAt(1.0);
    QCOMPARE(atStart.size(), morph.sampleCount());
    QCOMPARE(atEnd.size(), morph.sampleCount());

    const MdMorph startOnly(start, start);
    const MdMorph endOnly(end, end);
    const QVector<QPointF> startSamples = startOnly.samplesAt(0.0);
    const QVector<QPointF> endSamples = endOnly.samplesAt(0.0);
    for (int i = 0; i < morph.sampleCount(); i += 8) {
        QVERIFY(pointsClose(atStart[i], startSamples[i]));
        QVERIFY(pointsClose(atEnd[i], endSamples[i]));
    }
}

void TestMd3LoadingIndicator::morphIntermediateStaysWithin()
{
    // Interpolated outlines never leave the unit square's slack neighbourhood
    // (both shapes are normalized).
    const MdMorph morph(MdMaterialShapes::pentagon(), MdMaterialShapes::pill());
    for (int step = 0; step <= 10; ++step) {
        const QPainterPath path = morph.pathAt(step / 10.0);
        const QRectF bounds = path.boundingRect();
        QVERIFY(bounds.left() > -0.05);
        QVERIFY(bounds.top() > -0.05);
        QVERIFY(bounds.right() < 1.05);
        QVERIFY(bounds.bottom() < 1.05);
    }
}

// ---------------------------------------------------------------------------
// Animation math (Compose port, pure functions)
// ---------------------------------------------------------------------------

void TestMd3LoadingIndicator::springMath()
{
    // Zero at t=0, converged at the 650 ms snap, and overshooting on the
    // way (ζ = 0.6) — the bounce the reference shows.
    QVERIFY(closeTo(MdLoadingIndicatorStyle::springValue(0.0), 0.0, 1e-9));
    QVERIFY(MdLoadingIndicatorStyle::springValue(0.65) > 1.0 - 0.01);

    qreal overshoot = 0.0;
    for (int step = 0; step <= 650; ++step) {
        overshoot = std::max(overshoot, MdLoadingIndicatorStyle::springValue(step / 1000.0));
    }
    QVERIFY(overshoot > 1.0);
    QVERIFY(overshoot < 1.2); // a gentle bounce
}

void TestMd3LoadingIndicator::indeterminateFrames()
{
    // The morph grid: slot 0 starts at morph 0 with rotation target 90°.
    const int morphCount = 7;
    const MdLoadingIndicatorStyle::IndeterminateFrame frame0 =
        MdLoadingIndicatorStyle::indeterminateFrame(0, morphCount);
    QCOMPARE(frame0.morphIndex, 0);
    QVERIFY(closeTo(frame0.morphProgress, 0.0, 1e-9));
    QVERIFY(closeTo(frame0.rotationDeg, 90.0, 1e-9));

    // Slot 1: index advanced, target angle advanced a quarter turn, and the
    // global spin has run 650 ms of its 4666 ms turn. At exactly the slot
    // boundary the in-slot spring time is 0 again — continuous with the
    // previous slot, whose spring had converged to the same shape.
    const MdLoadingIndicatorStyle::IndeterminateFrame frame1 =
        MdLoadingIndicatorStyle::indeterminateFrame(650, morphCount);
    QCOMPARE(frame1.morphIndex, 1);
    QVERIFY(closeTo(frame1.morphProgress, 0.0, 1e-9));
    const qreal expected1 = 90.0 * 0.0 + 180.0 + 650.0 / 4666.0 * 360.0;
    QVERIFY(closeTo(frame1.rotationDeg, expected1, 1e-6));

    // The morph index wraps over the circular sequence.
    const MdLoadingIndicatorStyle::IndeterminateFrame frame7 =
        MdLoadingIndicatorStyle::indeterminateFrame(650 * 7, morphCount);
    QCOMPARE(frame7.morphIndex, 0);

    // The rotation composes: in-morph quarter turn + stepped target + the
    // linear global spin. Mid-slot the spring has already overshot past 1 —
    // the shape bounce the reference shows.
    const MdLoadingIndicatorStyle::IndeterminateFrame mid =
        MdLoadingIndicatorStyle::indeterminateFrame(325, morphCount);
    QVERIFY(mid.morphProgress > 1.0); // the overshoot
    QVERIFY(mid.morphProgress < 1.1);
    const qreal spin = 325.0 / 4666.0 * 360.0;
    const qreal expected = mid.morphProgress * 90.0 + 90.0 + spin;
    QVERIFY(closeTo(mid.rotationDeg, expected, 1e-6));
}

void TestMd3LoadingIndicator::determinateFrames()
{
    const MdLoadingIndicatorStyle::DeterminateFrame atZero =
        MdLoadingIndicatorStyle::determinateFrame(0.0, 1);
    QCOMPARE(atZero.morphIndex, 0);
    QVERIFY(closeTo(atZero.adjustedProgress, 0.0, 1e-9));
    QVERIFY(closeTo(atZero.rotationDeg, 0.0, 1e-9));

    const MdLoadingIndicatorStyle::DeterminateFrame atHalf =
        MdLoadingIndicatorStyle::determinateFrame(0.5, 1);
    QCOMPARE(atHalf.morphIndex, 0);
    QVERIFY(closeTo(atHalf.adjustedProgress, 0.5, 1e-9));
    QVERIFY(closeTo(atHalf.rotationDeg, -90.0, 1e-9));

    const MdLoadingIndicatorStyle::DeterminateFrame atOne =
        MdLoadingIndicatorStyle::determinateFrame(1.0, 1);
    QVERIFY(closeTo(atOne.adjustedProgress, 1.0, 1e-9));
    QVERIFY(closeTo(atOne.rotationDeg, -180.0, 1e-9));

    // Out-of-range progress coerces.
    const MdLoadingIndicatorStyle::DeterminateFrame clampedHigh =
        MdLoadingIndicatorStyle::determinateFrame(1.5, 1);
    QVERIFY(closeTo(clampedHigh.adjustedProgress, 1.0, 1e-9));
    const MdLoadingIndicatorStyle::DeterminateFrame clampedLow =
        MdLoadingIndicatorStyle::determinateFrame(-0.5, 1);
    QVERIFY(closeTo(clampedLow.adjustedProgress, 0.0, 1e-9));
}

void TestMd3LoadingIndicator::shapeScaleFactor()
{
    // The factor is the shapes' fit into their rotation square, times the
    // active indicator's share of the container: 38/48.
    const QList<MdRoundedPolygon> shapes = MdMaterialShapes::indeterminatePolygons();
    const MdLoadingIndicatorTokens tokens;
    const qreal factor = MdLoadingIndicatorStyle::shapeScaleFactor(shapes, tokens);
    QVERIFY(factor > 0.0);
    QVERIFY(factor < 38.0 / 48.0); // the fit term is ≤ 1
    QVERIFY(factor > 0.5); // the shapes are close to their max bounds

    // The circle alone fills its rotation square exactly.
    const QList<MdRoundedPolygon> circleOnly = { MdMaterialShapes::circle() };
    QVERIFY(closeTo(MdLoadingIndicatorStyle::shapeScaleFactor(circleOnly, tokens), 38.0 / 48.0,
                    1e-4));
}

// ---------------------------------------------------------------------------
// Widget
// ---------------------------------------------------------------------------

void TestMd3LoadingIndicator::widgetContract()
{
    MdLoadingIndicator indicator;
    QVERIFY(indicator.isIndeterminate());
    QVERIFY(indicator.isRunning());
    QCOMPARE(indicator.variant(), LoadingIndicatorVariant::Plain);
    QCOMPARE(indicator.progress(), 0.0);

    // The container token drives the size hint.
    QCOMPARE(indicator.sizeHint(), QSize(48, 48));
    QCOMPARE(indicator.minimumSizeHint(), QSize(48, 48));

    // Not interactive — no state rows at all.
    QCOMPARE(indicator.focusPolicy(), Qt::NoFocus);
    QCOMPARE(indicator.accessibleName(), QStringLiteral("loading"));

    // Progress coerces into 0..1.
    indicator.setProgress(1.5);
    QCOMPARE(indicator.progress(), 1.0);
    indicator.setProgress(-1.0);
    QCOMPARE(indicator.progress(), 0.0);

    // Variant change re-resolves tokens.
    indicator.setVariant(LoadingIndicatorVariant::Contained);
    QCOMPARE(indicator.variant(), LoadingIndicatorVariant::Contained);
    indicator.setVariant(LoadingIndicatorVariant::Plain);
}

void TestMd3LoadingIndicator::widgetRepaints()
{
    // The animation state observable: the cycle is active while the
    // indicator is running, indeterminate, and visible — and stops the
    // moment any of those stop. (Offscreen platforms never deliver paint
    // events, so repaint counting is not an option here; the painting
    // itself is smoke-checked in widgetPainting via render().)
    MdLoadingIndicator indicator;
    indicator.resize(48, 48);
    QVERIFY(!indicator.isAnimating()); // never shown yet

    indicator.show();
    QVERIFY(QTest::qWaitForWindowExposed(&indicator));
    QVERIFY(indicator.isAnimating());

    indicator.setIndeterminate(false);
    QVERIFY(!indicator.isAnimating()); // determinate is stateless
    indicator.setIndeterminate(true);
    QVERIFY(indicator.isAnimating());

    indicator.setRunning(false);
    QVERIFY(!indicator.isAnimating());
    indicator.setRunning(true);
    QVERIFY(indicator.isAnimating());

    indicator.hide();
    QVERIFY(!indicator.isAnimating());
    indicator.setRunning(true);
    QVERIFY(!indicator.isAnimating()); // still hidden
}

void TestMd3LoadingIndicator::widgetPainting()
{
    // Contained render smoke test: the container disc paints in the
    // primary-container colour, the morphed shape in on-primary-container.
    MdLoadingIndicator indicator(LoadingIndicatorVariant::Contained);
    indicator.setIndeterminate(false); // deterministic frame
    indicator.resize(48, 48);
    QImage image(48, 48, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    indicator.render(&painter);
    painter.end();

    const QColor container = MdTheme::instance().color(ColorRole::PrimaryContainer);
    // A pixel just inside the disc's corner arc belongs to the container.
    bool foundContainer = false;
    for (int d = 1; d < 10 && !foundContainer; ++d) {
        if (colorsClose(image.pixelColor(d, 48 - 1 - d), container, 24)) {
            foundContainer = true;
        }
    }
    QVERIFY(foundContainer);

    // The indicator's colour appears somewhere inside.
    const QColor indicatorColor = MdTheme::instance().color(ColorRole::OnPrimaryContainer);
    bool foundIndicator = false;
    for (int y = 10; y < 38 && !foundIndicator; ++y) {
        for (int x = 10; x < 38; ++x) {
            if (colorsClose(image.pixelColor(x, y), indicatorColor, 40)) {
                foundIndicator = true;
                break;
            }
        }
    }
    QVERIFY(foundIndicator);
}

} // namespace
} // namespace md

QTEST_MAIN(md::TestMd3LoadingIndicator)
#include "TestMd3LoadingIndicator.moc"
