// TestMd3ProgressIndicator — the merged progress-indicator family against
// its token sources.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-progress-indicator{,-linear,-circular}.scss
// (export 34.0.21, which deprecated the two per-shape sets), with the
// animation keyframes from the material-web internal SCSS — itself
// transplanted from MDC, and cited as such.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdProgressIndicatorStyle.h"
#include "widgets/MdProgressIndicator.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QColor pixelColorAt(MdProgressIndicator &indicator, const QPointF &position)
{
    QImage image(int(indicator.width()), int(indicator.height()),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    indicator.render(&painter);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

bool closeTo(qreal actual, qreal expected, qreal epsilon = 1.0e-4)
{
    return qAbs(actual - expected) <= epsilon;
}

} // namespace

class TestMd3ProgressIndicator : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void theMergedExportRows();
    void theDeprecatedRowsAreStillTranscribed();
    void theFourColorRolesComeFromTheDeprecatedSets();
    void aProgressIndicatorIsNeverInteractive();
    void theValueModelClampsToZeroOne();
    void linearDeterminateGeometry();
    void theStopIndicatorFollowsWithTheGap();
    void theStopIndicatorDisappearsNearCompletion();
    void linearIndeterminateKeyframes();
    void circularIndeterminateComposition();
    void theFourColorCycleInterpolatesColours();
    void determinateChangesTransition();
    void determinateSpringSettlesAndIsInterruptible();
    void renderSmokeLinear();
    void renderSmokeCircular();
    void waveRowsAreTranscribedAndOptIn();
    void waveUsesWithWaveGeometry();
    void wavePhaseScrollsOneWavelengthPerSecond();
    void indicatorAmplitudeFollowsComposeRule();
    void linearWaveSegmentPeaksAtPublishedAmplitude();
    void waveRenderSmoke();

private:
    MdProgressIndicator *m_linear = nullptr;
};

void TestMd3ProgressIndicator::initTestCase()
{
    MdTheme::instance();

    m_linear = new MdProgressIndicator;
    m_linear->resize(100, 4);
    m_linear->show();
}

void TestMd3ProgressIndicator::theMergedExportRows()
{
    // --- md.comp.progress-indicator.linear.* -------------------------------
    const MdProgressIndicatorTokens linear =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Linear);
    QCOMPARE(linear.linearHeight, 4.0);
    QCOMPARE(linear.linearActiveIndicatorThickness, 4.0);
    QCOMPARE(linear.linearTrackThickness, 4.0);
    QCOMPARE(linear.linearTrackActiveIndicatorSpace, 4.0);
    QCOMPARE(linear.linearStopIndicatorSize, 4.0);
    QCOMPARE(linear.linearStopIndicatorTrailingSpace, 0.0);
    // Wave rows: published Expressive tokens, carried but not rendered (the
    // registered gap in porting-todo.md).
    QCOMPARE(linear.linearWaveAmplitude, 3.0);
    QCOMPARE(linear.linearWaveWavelength, 40.0);
    QCOMPARE(linear.linearIndeterminateWaveWavelength, 20.0);
    QCOMPARE(linear.linearWithWaveHeight, 10.0);

    // --- md.comp.progress-indicator.circular.* -----------------------------
    const MdProgressIndicatorTokens circular =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Circular);
    QCOMPARE(circular.circularSize, 40.0);
    QCOMPARE(circular.circularActiveIndicatorThickness, 4.0);
    QCOMPARE(circular.circularTrackThickness, 4.0);
    QCOMPARE(circular.circularTrackActiveIndicatorSpace, 4.0);
    QCOMPARE(circular.circularWaveAmplitude, 1.6);
    QCOMPARE(circular.circularWaveWavelength, 15.0);
    QCOMPARE(circular.circularWithWaveSize, 48.0);

    // --- the shared base rows ----------------------------------------------
    QCOMPARE(int(linear.activeIndicatorColor), int(ColorRole::Primary));
    QCOMPARE(int(linear.stopIndicatorColor), int(ColorRole::Primary));
    QCOMPARE(int(linear.trackColor), int(ColorRole::SecondaryContainer));
    QCOMPARE(int(linear.activeIndicatorShape), int(ShapeCorner::Full));
    QCOMPARE(int(linear.stopIndicatorShape), int(ShapeCorner::Full));
    QCOMPARE(int(linear.trackShape), int(ShapeCorner::Full));

    // Timing constants [material-web internal SCSS, MDC heritage].
    QCOMPARE(MdProgressIndicatorStyle::kLinearDeterminateDurationMs, 250);
    QCOMPARE(MdProgressIndicatorStyle::kCircularDeterminateDurationMs, 500);
    QCOMPARE(MdProgressIndicatorStyle::kLinearIndeterminateDurationMs, 2000);
    QCOMPARE(MdProgressIndicatorStyle::kLinearFourColorDurationMs, 4000);
    QCOMPARE(MdProgressIndicatorStyle::kCircularArcDurationMs, 1333);
    QCOMPARE(MdProgressIndicatorStyle::kCircularCycleDurationMs, 5332);
    QVERIFY(closeTo(MdProgressIndicatorStyle::kCircularLinearRotateDurationMs,
                    1333.0 * 360.0 / 306.0));
}

void TestMd3ProgressIndicator::theDeprecatedRowsAreStillTranscribed()
{
    // The `thick.*` rows are deprecated *as a variant* ("no longer tokenized
    // as a variant, but rather a sample configuration in code") — carried for
    // completeness, never exposed as an API.
    const MdProgressIndicatorTokens tokens =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Linear);
    QCOMPARE(tokens.linearThickHeight, 8.0);
    QCOMPARE(tokens.linearThickActiveIndicatorThickness, 8.0);
    QCOMPARE(tokens.linearThickTrackThickness, 8.0);
    QCOMPARE(tokens.linearThickTrackActiveIndicatorSpace, 4.0);
    QCOMPARE(tokens.linearThickStopIndicatorSize, 4.0);
    QCOMPARE(tokens.linearThickStopIndicatorTrailingSpace, 2.0);
    QCOMPARE(tokens.linearThickWithWaveHeight, 14.0);
    QCOMPARE(tokens.circularThickSize, 52.0);
    QCOMPARE(tokens.circularThickActiveIndicatorThickness, 8.0);
    QCOMPARE(tokens.circularThickTrackThickness, 8.0);
    QCOMPARE(tokens.circularThickTrackActiveIndicatorSpace, 4.0);

    // The deprecated base metrics of the merged set.
    QCOMPARE(tokens.activeIndicatorTrackSpace, 4.0);
    QCOMPARE(tokens.baseActiveIndicatorThickness, 4.0);
    QCOMPARE(tokens.baseStopIndicatorSize, 4.0);
    QCOMPARE(tokens.baseTrackThickness, 4.0);
}

void TestMd3ProgressIndicator::theFourColorRolesComeFromTheDeprecatedSets()
{
    // The old `_md-comp-{linear,circular}-progress-indicator` sets are
    // deprecated in favour of the merged one; their four-color rows are the
    // only source for the four-color API and both shapes publish the same
    // four roles.
    const MdProgressIndicatorTokens tokens =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Linear);
    QCOMPARE(int(tokens.fourColorOne), int(ColorRole::Primary));
    QCOMPARE(int(tokens.fourColorTwo), int(ColorRole::PrimaryContainer));
    QCOMPARE(int(tokens.fourColorThree), int(ColorRole::Tertiary));
    QCOMPARE(int(tokens.fourColorFour), int(ColorRole::TertiaryContainer));
}

void TestMd3ProgressIndicator::aProgressIndicatorIsNeverInteractive()
{
    // The merged export publishes no state rows at all — like the badge
    // family, the interaction contract is "nothing happens".
    QCOMPARE(int(m_linear->focusPolicy()), int(Qt::NoFocus));

    MdProgressIndicator circular(ProgressIndicatorShape::Circular);
    QCOMPARE(int(circular.focusPolicy()), int(Qt::NoFocus));
}

void TestMd3ProgressIndicator::theValueModelClampsToZeroOne()
{
    // Compose's contract: the fraction is clamped to 0..1. material-web's
    // CSS would happily scaleX past 100 %; the clamp wins, recorded as such.
    MdProgressIndicator indicator;
    indicator.setMax(1.0);
    indicator.setValue(0.5);
    QCOMPARE(indicator.fraction(), 0.5);
    indicator.setValue(2.0);
    QCOMPARE(indicator.fraction(), 1.0);
    indicator.setValue(-1.0);
    QCOMPARE(indicator.fraction(), 0.0);
    indicator.setMax(0.0);
    QCOMPARE(indicator.fraction(), 0.0);
}

void TestMd3ProgressIndicator::linearDeterminateGeometry()
{
    m_linear->setIndeterminate(false);
    m_linear->setBuffer(0.0);
    m_linear->setMax(1.0);
    m_linear->setValue(0.5);
    // Let the 250 ms transition land.
    QTest::qWait(350);
    QCOMPARE(m_linear->displayFraction(), 0.5);

    const MdProgressIndicatorStyle::Layout layout =
        MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QCOMPARE(layout.track, QRectF(0.0, 0.0, 100.0, 4.0));
    QCOMPARE(layout.activeIndicator, QRectF(0.0, 0.0, 100.0, 4.0));
    QCOMPARE(layout.trackScale, 1.0);
    // min-width: 80 px [material-web css] over the 4 px token height.
    QCOMPARE(m_linear->sizeHint(), QSize(80, 4));
}

void TestMd3ProgressIndicator::theStopIndicatorFollowsWithTheGap()
{
    // [active bar][gap 4][stop dot 4] — the dot's trailing space is 0, so the
    // track continues immediately after it.
    m_linear->setValue(0.5);
    QTest::qWait(350);
    const MdProgressIndicatorStyle::Layout layout =
        MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QVERIFY(layout.stopIndicator.isValid());
    QCOMPARE(layout.stopIndicator, QRectF(54.0, 0.0, 4.0, 4.0));

    // With a buffer set, the track scales to the buffer fraction and the
    // dots region opens between the scaled track's end and the widget edge.
    m_linear->setBuffer(0.8);
    const MdProgressIndicatorStyle::Layout buffered =
        MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QCOMPARE(buffered.trackScale, 0.8);
    QCOMPARE(buffered.bufferDots, QRectF(80.0, 0.0, 20.0, 4.0));
    m_linear->setBuffer(0.0);
}

void TestMd3ProgressIndicator::theStopIndicatorDisappearsNearCompletion()
{
    // Complete: the bar fills the track — no gap, no dot. Near-complete:
    // when the gap+dot would not fit, the dot drops instead of overflowing.
    m_linear->setValue(1.0);
    QTest::qWait(350);
    MdProgressIndicatorStyle::Layout layout =
        MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QVERIFY(!layout.stopIndicator.isValid());

    // 0.97 * 100 + 4 + 4 > 100 → no room for the dot.
    m_linear->setValue(0.97);
    QTest::qWait(350);
    layout = MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QVERIFY(!layout.stopIndicator.isValid());

    // 0.9 * 100 + 4 + 4 <= 100 → the dot is there.
    m_linear->setValue(0.9);
    QTest::qWait(350);
    layout = MdProgressIndicatorStyle::layoutFor(*m_linear, m_linear->tokens());
    QVERIFY(layout.stopIndicator.isValid());
}

void TestMd3ProgressIndicator::linearIndeterminateKeyframes()
{
    // The MDC keyframes, evaluated as pure functions of the cycle position.
    const auto frameAt = [](qreal p) {
        return MdProgressIndicatorStyle::linearIndeterminateFrame(p);
    };

    // Cycle start: both bars at scale 0.08, at their first translate.
    MdProgressIndicatorStyle::LinearIndeterminateFrame frame = frameAt(0.0);
    QVERIFY(closeTo(frame.primaryTranslate, 0.0));
    QVERIFY(closeTo(frame.primaryScale, 0.08));
    QVERIFY(closeTo(frame.secondaryTranslate, 0.0));
    QVERIFY(closeTo(frame.secondaryScale, 0.08));

    // primary translate: 20 % → 0, 59.15 % → 83.6714 %, 100 % → 200.611 %.
    frame = frameAt(0.20);
    QVERIFY(closeTo(frame.primaryTranslate, 0.0));
    frame = frameAt(0.5915);
    QVERIFY(closeTo(frame.primaryTranslate, 0.836714));
    frame = frameAt(1.0);
    QVERIFY(closeTo(frame.primaryTranslate, 2.00611));

    // primary scale: 36.65 % → 0.08, 69.15 % → 0.661479, 100 % → 0.08.
    frame = frameAt(0.3665);
    QVERIFY(closeTo(frame.primaryScale, 0.08));
    frame = frameAt(0.6915);
    QVERIFY(closeTo(frame.primaryScale, 0.661479));

    // secondary translate: 25 % → 37.6519 %, 48.35 % → 84.3862 %,
    // 100 % → 160.278 %.
    frame = frameAt(0.25);
    QVERIFY(closeTo(frame.secondaryTranslate, 0.376519));
    frame = frameAt(0.4835);
    QVERIFY(closeTo(frame.secondaryTranslate, 0.843862));
    frame = frameAt(1.0);
    QVERIFY(closeTo(frame.secondaryTranslate, 1.60278));

    // secondary scale: 19.15 % → 0.457104, 44.15 % → 0.72796, 100 % → 0.08.
    frame = frameAt(0.1915);
    QVERIFY(closeTo(frame.secondaryScale, 0.457104));
    frame = frameAt(0.4415);
    QVERIFY(closeTo(frame.secondaryScale, 0.72796));
    frame = frameAt(1.0);
    QVERIFY(closeTo(frame.secondaryScale, 0.08));
}

void TestMd3ProgressIndicator::circularIndeterminateComposition()
{
    // The three composed rotations, evaluated as pure functions of elapsed
    // time (ms).
    using S = MdProgressIndicatorStyle;
    auto frame = S::circularIndeterminateFrame(0);
    // At t=0: no linear spin, no group rotation yet, the expand arc at its
    // 265° start plus each half's base (135° / 100°), the right half half a
    // cycle into the expansion (already down at 130°).
    QCOMPARE(frame.globalRotation, 0.0);
    QVERIFY(closeTo(frame.groupRotation, 0.0));
    QVERIFY(closeTo(frame.leftRotation, 265.0 + 135.0));
    QVERIFY(closeTo(frame.rightRotation, 130.0 + 100.0));

    // Exactly one segment boundary (666.5 ms): one full +135° group step.
    frame = S::circularIndeterminateFrame(667); // one tick past, eased ≈ 1
    QVERIFY(frame.groupRotation > 134.0 && frame.groupRotation < 136.0);

    // One arc period later the left half is back at its 265° start.
    frame = S::circularIndeterminateFrame(S::kCircularArcDurationMs);
    QVERIFY(closeTo(frame.leftRotation, 265.0 + 135.0));
    // ...and the right half is still delayed by half a period from it.
    QVERIFY(closeTo(frame.rightRotation, 130.0 + 100.0));

    // The linear spin advances 360° per ARCTIME*360/306 ms — a little more
    // than one turn of the group cycle.
    frame = S::circularIndeterminateFrame(qint64(S::kCircularLinearRotateDurationMs) - 1);
    QVERIFY(frame.globalRotation > 359.0);
}

void TestMd3ProgressIndicator::theFourColorCycleInterpolatesColours()
{
    using S = MdProgressIndicatorStyle;
    const MdProgressIndicatorTokens tokens =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Linear);
    const QColor one = MdTheme::instance().color(ColorRole::Primary);
    const QColor two = MdTheme::instance().color(ColorRole::PrimaryContainer);

    // Cycle start and the 15 % plateau: solid colour one.
    QCOMPARE(S::fourColorAt(tokens, ProgressIndicatorShape::Linear, 0.0), one);
    QCOMPARE(S::fourColorAt(tokens, ProgressIndicatorShape::Linear, 0.15), one);
    // CSS interpolates the animated *colour* between keyframes: halfway
    // between the 15 % and 25 % marks the colour is halfway between the two
    // roles — not an index jump.
    const QColor mid = S::fourColorAt(tokens, ProgressIndicatorShape::Linear, 0.20);
    QVERIFY(mid != one);
    QVERIFY(mid != two);
    // The 25–40 % plateau: solid colour two.
    QCOMPARE(S::fourColorAt(tokens, ProgressIndicatorShape::Linear, 0.30), two);
    // ...and the cycle wraps back to colour one at 100 %.
    QCOMPARE(S::fourColorAt(tokens, ProgressIndicatorShape::Linear, 1.0), one);
}

void TestMd3ProgressIndicator::determinateChangesTransition()
{
    // The value change springs the *display* fraction to the target instead
    // of jumping. The published CSS durations (250 ms linear / 500 ms
    // circular) are no longer the drive — the default-effects spring is.
    m_linear->setValue(0.5);
    QTest::qWait(350);
    QCOMPARE(m_linear->displayFraction(), 0.5);

    m_linear->setValue(1.0);
    // Early in the spring the display fraction has not reached the target.
    QTest::qWait(20);
    QVERIFY(m_linear->displayFraction() < 1.0);
    QTest::qWait(400);
    QCOMPARE(m_linear->displayFraction(), 1.0);
    QVERIFY(!m_linear->isValueTransitioning());
}

void TestMd3ProgressIndicator::determinateSpringSettlesAndIsInterruptible()
{
    // Settles: a value change lands on the target and stops the transition.
    m_linear->setIndeterminate(false);
    m_linear->setValue(0.0);
    QTest::qWait(350);
    QCOMPARE(m_linear->displayFraction(), 0.0);

    m_linear->setValue(1.0);
    QVERIFY(m_linear->isValueTransitioning());
    QTest::qWait(400);
    QCOMPARE(m_linear->displayFraction(), 1.0);
    QVERIFY(!m_linear->isValueTransitioning());

    // Interruptible: a mid-flight retarget continues from the display
    // fraction currently on screen — it never jumps to the old target or
    // restarts from zero.
    m_linear->setValue(0.0);
    QTRY_VERIFY_WITH_TIMEOUT(m_linear->isValueTransitioning()
                                 && m_linear->displayFraction() < 1.0
                                 && m_linear->displayFraction() > 0.0,
                             250);
    const qreal mid = m_linear->displayFraction();
    QVERIFY(mid > 0.0);
    QVERIFY(mid < 1.0);
    m_linear->setValue(0.75);
    // The display fraction stays continuous across the retarget.
    QVERIFY(qAbs(m_linear->displayFraction() - mid) < 0.35);
    QTRY_COMPARE_WITH_TIMEOUT(m_linear->displayFraction(), 0.75, 800);
    QVERIFY(!m_linear->isValueTransitioning());
}

void TestMd3ProgressIndicator::renderSmokeLinear()
{
    m_linear->setIndeterminate(false);
    m_linear->setBuffer(0.0);
    m_linear->setValue(0.5);
    QTest::qWait(350);

    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    const QColor track = MdTheme::instance().color(ColorRole::SecondaryContainer);
    // Inside the active bar: primary. Beyond the stop dot: track.
    QCOMPARE(pixelColorAt(*m_linear, QPointF(5, 2)), primary);
    QCOMPARE(pixelColorAt(*m_linear, QPointF(56, 2)), primary); // the stop dot
    QCOMPARE(pixelColorAt(*m_linear, QPointF(95, 2)), track);
}

void TestMd3ProgressIndicator::renderSmokeCircular()
{
    MdProgressIndicator circular(ProgressIndicatorShape::Circular);
    circular.resize(40, 40);
    circular.show();
    circular.setValue(0.25);
    QTest::qWait(650); // the 500 ms dashoffset transition

    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    const QColor track = MdTheme::instance().color(ColorRole::SecondaryContainer);
    // The arc starts at 12 o'clock and runs clockwise: the top of the ring
    // is covered, the 9 o'clock point is bare track.
    QCOMPARE(pixelColorAt(circular, QPointF(20, 2)), primary);
    QCOMPARE(pixelColorAt(circular, QPointF(2, 20)), track);
}

void TestMd3ProgressIndicator::waveRowsAreTranscribedAndOptIn()
{
    const MdProgressIndicatorTokens linear =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Linear);
    QCOMPARE(linear.linearWaveAmplitude, 3.0);
    QCOMPARE(linear.linearWaveWavelength, 40.0);
    QCOMPARE(linear.linearIndeterminateWaveWavelength, 20.0);
    QCOMPARE(linear.linearWithWaveHeight, 10.0);

    const MdProgressIndicatorTokens circular =
        MdProgressIndicatorTokens::resolve(ProgressIndicatorShape::Circular);
    QCOMPARE(circular.circularWaveAmplitude, 1.6);
    QCOMPARE(circular.circularWaveWavelength, 15.0);
    QCOMPARE(circular.circularWithWaveSize, 48.0);

    // Opt-in: the non-wave baseline is the default.
    QVERIFY(!m_linear->isWave());
    m_linear->setWave(true);
    QVERIFY(m_linear->isWave());
    m_linear->setWave(false);
    QVERIFY(!m_linear->isWave());
}

void TestMd3ProgressIndicator::waveUsesWithWaveGeometry()
{
    MdProgressIndicator linear;
    linear.setWave(true);
    // `linear.with-wave-height` 10 px replaces the 4 px baseline height.
    QCOMPARE(linear.sizeHint().height(), 10);

    MdProgressIndicator circular(ProgressIndicatorShape::Circular);
    circular.setWave(true);
    // `circular.with-wave-size` 48 px replaces the 40 px baseline.
    QCOMPARE(circular.sizeHint().width(), 48);
    QCOMPARE(circular.sizeHint().height(), 48);
}

void TestMd3ProgressIndicator::wavePhaseScrollsOneWavelengthPerSecond()
{
    // Default waveSpeed == wavelength → one full 0→1 cycle per second
    // (Compose updateOffsetAnimation: duration = (wavelength / waveSpeed)*1000).
    QVERIFY(closeTo(MdProgressIndicatorStyle::wavePhase(0, 40.0, 40.0), 0.0));
    QVERIFY(closeTo(MdProgressIndicatorStyle::wavePhase(500, 40.0, 40.0), 0.5));
    QVERIFY(closeTo(MdProgressIndicatorStyle::wavePhase(1000, 40.0, 40.0), 0.0)); // wraps
    // Twice the speed → half the period.
    QVERIFY(closeTo(MdProgressIndicatorStyle::wavePhase(250, 40.0, 80.0), 0.5));
    // No speed / no wavelength → no motion.
    QCOMPARE(MdProgressIndicatorStyle::wavePhase(500, 0.0, 40.0), 0.0);
    QCOMPARE(MdProgressIndicatorStyle::wavePhase(500, 40.0, 0.0), 0.0);
}

void TestMd3ProgressIndicator::indicatorAmplitudeFollowsComposeRule()
{
    // WavyProgressIndicatorDefaults.indicatorAmplitude:
    // 0 at or below 10 % and at or above 95 %, 1 in between; 1 while
    // indeterminate.
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.0, false), 0.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.1, false), 0.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.11, false), 1.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.5, false), 1.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.95, false), 0.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(1.0, false), 0.0);
    QCOMPARE(MdProgressIndicatorStyle::indicatorAmplitude(0.0, true), 1.0);
}

void TestMd3ProgressIndicator::linearWaveSegmentPeaksAtPublishedAmplitude()
{
    // The Compose quadratic chain peaks at (height - strokeWidth) / 2.
    // With the with-wave height 10 and thickness 4 that is exactly the
    // published `linear.wave-amplitude` 3 px.
    const qreal height = 10.0;
    const qreal stroke = 4.0;
    const qreal wavelength = 40.0;
    const qreal peak = (height - stroke) / 2.0;
    QCOMPARE(peak, 3.0);

    // Sample the segment and confirm the drawn peak matches, and that
    // amplitude 0 flattens onto the centreline.
    const QPainterPath full = MdProgressIndicatorStyle::linearWaveSegment(
        100.0, height, stroke, 0.0, 1.0, 1.0, 0.0, wavelength);
    QVERIFY(!full.isEmpty());
    qreal minY = 1e9;
    qreal maxY = -1e9;
    for (int i = 0; i <= 200; ++i) {
        const QPointF p = full.pointAtPercent(qreal(i) / 200.0);
        minY = qMin(minY, p.y());
        maxY = qMax(maxY, p.y());
    }
    // The wave sits on the centreline (height/2 = 5) and peaks ±3 around it.
    QVERIFY(closeTo((maxY - minY) / 2.0, peak, 0.15));
    QVERIFY(closeTo((maxY + minY) / 2.0, height / 2.0, 0.15));

    const QPainterPath flat = MdProgressIndicatorStyle::linearWaveSegment(
        100.0, height, stroke, 0.0, 1.0, 0.0, 0.0, wavelength);
    for (int i = 0; i <= 20; ++i) {
        QVERIFY(closeTo(flat.pointAtPercent(qreal(i) / 20.0).y(), height / 2.0, 0.01));
    }
}

void TestMd3ProgressIndicator::waveRenderSmoke()
{
    MdProgressIndicator wave;
    wave.setWave(true);
    wave.resize(100, 10);
    wave.show();
    wave.setValue(0.5);
    // Let the amplitude morph (effects-default spring) settle.
    QTest::qWait(600);

    const QColor primary = MdTheme::instance().color(ColorRole::Primary);
    const QColor track = MdTheme::instance().color(ColorRole::SecondaryContainer);
    // A pixel on the active wave should be primary; past the 50 % point the
    // track is secondary-container.
    const QColor mid = pixelColorAt(wave, QPointF(50, 5));
    const QColor far = pixelColorAt(wave, QPointF(95, 5));
    // The wave moves, so the exact mid pixel varies — just require that
    // *something* was painted (not the transparent default) and the far
    // side is track.
    QVERIFY(mid.alpha() > 0);
    QCOMPARE(far, track);
}

QTEST_MAIN(TestMd3ProgressIndicator)
#include "TestMd3ProgressIndicator.moc"
