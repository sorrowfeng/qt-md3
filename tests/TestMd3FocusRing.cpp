// MdFocusRing — stroke geometry, the two-phase grow/settle timeline and the
// outward/inward placement variants.
//
// The focus indicator is pure decoration, so an off-by-half-a-pixel here is
// invisible in review but wrong against the spec. These tests pin the numbers.

#include "TestMd3Common.h"

#include "core/MdFocusRing.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

#include <limits>

using namespace md;

class TestMd3FocusRing : public QObject
{
    Q_OBJECT

private slots:
    void specMatchesTokens();
    void animationSplitsIntoQuarterAndThreeQuarters();
    void widthGrowsThenSettles();
    void outwardVariantSitsOutsideWithAGap();
    void inwardVariantSitsInside();
    void radiiFollowTheComponentShape();
    void paintsAVisibleRing();
    void controllerRestartsOnFocus();
};

void TestMd3FocusRing::specMatchesTokens()
{
    // tokens/_md-comp-focus-ring.scss.
    const MdFocusRingSpec spec;
    QVERIFY(qFuzzyCompare(spec.width, 3.0));
    QVERIFY(qFuzzyCompare(spec.activeWidth, 8.0));
    QVERIFY(qFuzzyCompare(spec.outwardOffset, 2.0));
    QVERIFY(qFuzzyCompare(spec.inwardOffset, 0.0));
    QCOMPARE(spec.durationMs, 600); // md.sys.motion.duration-long4
    QCOMPARE(spec.easing, MotionEasing::Emphasized);
    QCOMPARE(spec.shape, ShapeCorner::Full);
    QVERIFY(!spec.inward);
}

void TestMd3FocusRing::animationSplitsIntoQuarterAndThreeQuarters()
{
    const MdFocusRingSpec spec;
    QCOMPARE(MdFocusRing::growMs(spec), 150);
    QCOMPARE(MdFocusRing::settleMs(spec), 450);
    QCOMPARE(MdFocusRing::totalMs(spec), 600);
}

void TestMd3FocusRing::widthGrowsThenSettles()
{
    const MdFocusRingSpec spec;

    // A negative elapsed time means "settled, no animation".
    QVERIFY(qFuzzyCompare(MdFocusRing::widthAt(spec, -1), spec.width));
    QVERIFY(qFuzzyIsNull(MdFocusRing::widthAt(spec, 0)));
    // Peak at the end of the grow phase.
    QVERIFY(qFuzzyCompare(MdFocusRing::widthAt(spec, 150), spec.activeWidth));
    // Back to the resting width when the animation is over.
    QVERIFY(qFuzzyCompare(MdFocusRing::widthAt(spec, 600), spec.width));
    QVERIFY(qFuzzyCompare(MdFocusRing::widthAt(spec, 10'000), spec.width));

    // Monotonic rise, then monotonic fall — the emphasised easing is not
    // required to be monotonic in a strict mathematical sense, but the M3
    // curve is, and this catches a transposed control point.
    qreal previous = -1.0;
    for (int ms = 0; ms <= 150; ms += 5) {
        const qreal width = MdFocusRing::widthAt(spec, ms);
        QVERIFY(width >= previous - 1e-9);
        previous = width;
    }
    previous = std::numeric_limits<qreal>::max();
    for (int ms = 150; ms <= 600; ms += 5) {
        const qreal width = MdFocusRing::widthAt(spec, ms);
        QVERIFY(width <= previous + 1e-9);
        previous = width;
    }
}

void TestMd3FocusRing::outwardVariantSitsOutsideWithAGap()
{
    const MdFocusRingSpec spec;
    const QRectF bounds(10.0, 20.0, 100.0, 40.0);

    // Stroke centre line: gap (2) + half the stroke (1.5) = 3.5 outside.
    const QRectF ring = MdFocusRing::ringRect(bounds, spec);
    QVERIFY(qFuzzyCompare(ring.left(), bounds.left() - 3.5));
    QVERIFY(qFuzzyCompare(ring.top(), bounds.top() - 3.5));
    QVERIFY(qFuzzyCompare(ring.right(), bounds.right() + 3.5));
    QVERIFY(qFuzzyCompare(ring.bottom(), bounds.bottom() + 3.5));
    // offset() is the gap token on its own; the half-stroke that puts the
    // centre line at 3.5 is added by ringRect()/ringRadii().
    QVERIFY(qFuzzyCompare(spec.offset(), spec.outwardOffset));
}

void TestMd3FocusRing::inwardVariantSitsInside()
{
    MdFocusRingSpec spec;
    spec.inward = true;
    const QRectF bounds(10.0, 20.0, 100.0, 40.0);

    // `border: 3px` draws inside, so the centre line is 1.5 inside the edge.
    const QRectF ring = MdFocusRing::ringRect(bounds, spec);
    QVERIFY(qFuzzyCompare(ring.left(), bounds.left() + 1.5));
    QVERIFY(qFuzzyCompare(ring.right(), bounds.right() - 1.5));
    QVERIFY(qFuzzyCompare(spec.offset(), spec.inwardOffset));

    // A non-zero inward-offset must actually move the ring: the gap token is
    // shared with the outward variant, and a hard-coded `width / 2` here would
    // silently ignore it.
    MdFocusRingSpec inset;
    inset.inward = true;
    inset.inwardOffset = 4.0;
    const QRectF pushed = MdFocusRing::ringRect(bounds, inset);
    QVERIFY(qFuzzyCompare(pushed.left(), bounds.left() + 5.5));   // 4 + 1.5
    QVERIFY(qFuzzyCompare(pushed.right(), bounds.right() - 5.5));
    for (qreal radius : MdFocusRing::ringRadii({12.0, 12.0, 12.0, 12.0}, inset)) {
        QVERIFY(qFuzzyCompare(radius, 6.5));                      // 12 - 5.5
    }
}

void TestMd3FocusRing::radiiFollowTheComponentShape()
{
    const MdFocusRingSpec spec;
    const QList<qreal> componentRadii{12.0, 12.0, 12.0, 12.0};
    const QList<qreal> ringRadii = MdFocusRing::ringRadii(componentRadii, spec);
    QCOMPARE(ringRadii.size(), 4);
    for (qreal radius : ringRadii) {
        QVERIFY(qFuzzyCompare(radius, 15.5)); // 12 + 3.5
    }

    MdFocusRingSpec inward;
    inward.inward = true;
    for (qreal radius : MdFocusRing::ringRadii(componentRadii, inward)) {
        QVERIFY(qFuzzyCompare(radius, 10.5)); // 12 - 1.5
    }

    // Never negative, even for a fully round component.
    const QList<qreal> pill{20.0, 20.0, 20.0, 20.0};
    for (qreal radius : MdFocusRing::ringRadii(pill, inward)) {
        QVERIFY(radius >= 0.0);
    }

    // No radii in, no radii out: the caller asked for the `shape` fallback.
    QVERIFY(MdFocusRing::ringRadii(QList<qreal>(), spec).isEmpty());
}

void TestMd3FocusRing::paintsAVisibleRing()
{
    QImage image(120, 60, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    const QRectF bounds(10.0, 10.0, 100.0, 40.0);

    QPainter painter(&image);
    MdFocusRing::paint(&painter, bounds, QList<qreal>(), QColor(0, 0, 0),
                       MdFocusRingSpec(), -1);
    painter.end();
    const int settled = mdtest::paintedPixelCount(image, qRgb(255, 255, 255));
    QVERIFY(settled > 100);
    // The ring is outside the component, so the component interior and its
    // immediate surroundings must still be untouched.
    QCOMPARE(image.pixel(60, 30), qRgb(255, 255, 255));

    // The grown ring paints more than the settled one.
    QImage grown(120, 60, QImage::Format_ARGB32_Premultiplied);
    grown.fill(Qt::white);
    QPainter painter2(&grown);
    MdFocusRing::paint(&painter2, bounds, QList<qreal>(), QColor(0, 0, 0),
                       MdFocusRingSpec(), 150);
    painter2.end();
    QVERIFY(mdtest::paintedPixelCount(grown, qRgb(255, 255, 255)) > settled);

    // An invalid colour must not paint at all.
    QImage none(120, 60, QImage::Format_ARGB32_Premultiplied);
    none.fill(Qt::white);
    QPainter painter3(&none);
    MdFocusRing::paint(&painter3, bounds, QList<qreal>(), QColor(), MdFocusRingSpec(), -1);
    painter3.end();
    QCOMPARE(mdtest::paintedPixelCount(none, qRgb(255, 255, 255)), 0);
}

void TestMd3FocusRing::controllerRestartsOnFocus()
{
    MdFocusRingController controller;
    QSignalSpy spy(&controller, &MdFocusRingController::repaintRequested);
    QVERIFY(!controller.isAnimating());
    QVERIFY(qFuzzyCompare(controller.currentWidth(), 3.0));

    controller.start();
    QVERIFY(controller.isAnimating());
    QVERIFY(spy.count() >= 1);

    controller.stop();
    QVERIFY(!controller.isAnimating());
    QVERIFY(qFuzzyCompare(controller.currentWidth(), 3.0));

    // stop() while idle must not emit.
    const int before = spy.count();
    controller.stop();
    QCOMPARE(spy.count(), before);
}

QTEST_MAIN(TestMd3FocusRing)

#include "TestMd3FocusRing.moc"
