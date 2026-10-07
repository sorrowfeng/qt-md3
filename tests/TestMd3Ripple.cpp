// MdRipple — geometry, timing and shape clipping.
//
// These are not visual-diff tests. They pin the geometry and timing constants
// published by material-web's ripple handler, plus a render smoke check so a
// style that silently paints nothing cannot pass.

#include "TestMd3Common.h"

#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

#include <cmath>

using namespace md;

class TestMd3Ripple : public QObject
{
    Q_OBJECT

private slots:
    void constantsMatchMaterialWeb();
    void geometryMatchesDetermineRippleSize();
    void geometrySurvivesDegenerateBounds();
    void progressIsMonotonicAndClamped();
    void frameTravelsFromPressPointToCenter();
    void releaseIsDeferredToMinimumPress();
    void fadeOutIsLinearOver375ms();
    void paintsInsideClipPathOnly();
    void controllerTracksPressAndRelease();
};

void TestMd3Ripple::constantsMatchMaterialWeb()
{
    // ripple/internal/ripple.ts and ripple/internal/_ripple.scss.
    QCOMPARE(MdRipple::PressGrowMs, 450);
    QCOMPARE(MdRipple::MinimumPressMs, 225);
    QCOMPARE(MdRipple::FadeOutMs, 375);
    QVERIFY(qFuzzyCompare(MdRipple::InitialOriginScale, 0.2));
    QVERIFY(qFuzzyCompare(MdRipple::Padding, 10.0));
    QVERIFY(qFuzzyCompare(MdRipple::SoftEdgeMinimumSize, 75.0));
    QVERIFY(qFuzzyCompare(MdRipple::SoftEdgeContainerRatio, 0.35));
    // tokens/_md-comp-ripple.scss points at the pressed state-layer opacity.
    QVERIFY(qFuzzyCompare(MdStateLayer::opacity(MdRipple::OverlayKind), 0.12));
}

void TestMd3Ripple::geometryMatchesDetermineRippleSize()
{
    // 200 x 50: maxDim 200, softEdge max(0.35 * 200, 75) = 75,
    // initialSize floor(200 * 0.2) = 40, maxRadius hypot(200, 50) + 10.
    const MdRipple::Geometry wide = MdRipple::geometryFor(QSizeF(200.0, 50.0));
    QVERIFY(wide.isValid());
    QVERIFY(qFuzzyCompare(wide.softEdgeSize, 75.0));
    QVERIFY(qFuzzyCompare(wide.initialSize, 40.0));
    const qreal expectedMaxRadius = std::hypot(200.0, 50.0) + 10.0;
    QVERIFY(std::abs(wide.finalRadius - (expectedMaxRadius + 75.0) / 2.0) < 1e-9);

    // The circle must always be able to reach the farthest corner of the
    // component, otherwise a corner press would leave an unpainted wedge.
    QVERIFY(wide.finalRadius * 2.0 >= std::hypot(200.0, 50.0));

    // Small box: the 75px soft-edge floor dominates.
    const MdRipple::Geometry small = MdRipple::geometryFor(QSizeF(40.0, 40.0));
    QVERIFY(qFuzzyCompare(small.softEdgeSize, 75.0));
    QVERIFY(qFuzzyCompare(small.initialSize, 8.0));

    // A very large box makes the 70px gradient band visible, so the soft edge
    // starts later than the 65% floor.
    const MdRipple::Geometry huge = MdRipple::geometryFor(QSizeF(2100.0, 100.0));
    QVERIFY(qFuzzyCompare(huge.initialSize, 420.0));
    QVERIFY(huge.softEdgeStart > 0.65);
    QVERIFY(std::abs(huge.softEdgeStart - (1.0 - 140.0 / 420.0)) < 1e-9);
}

void TestMd3Ripple::geometrySurvivesDegenerateBounds()
{
    QVERIFY(!MdRipple::geometryFor(QSizeF(0.0, 0.0)).isValid());
    // A negative extent is a caller bug. Clamping it to zero would hand back a
    // geometry computed from a box that does not exist, so it is refused too.
    QVERIFY(!MdRipple::geometryFor(QSizeF(-5.0, 20.0)).isValid());
    QVERIFY(!MdRipple::geometryFor(QSizeF(20.0, -5.0)).isValid());
    // One axis collapsed means there is no area to cover either.
    QVERIFY(!MdRipple::geometryFor(QSizeF(0.0, 40.0)).isValid());

    const MdRipple::Geometry tiny = MdRipple::geometryFor(QSizeF(1.0, 1.0));
    QVERIFY(tiny.isValid());
    QVERIFY(tiny.initialSize >= 1.0);
    QVERIFY(std::isfinite(tiny.finalRadius));
}

void TestMd3Ripple::progressIsMonotonicAndClamped()
{
    QVERIFY(qFuzzyIsNull(MdRipple::progressAt(0)));
    QVERIFY(qFuzzyIsNull(MdRipple::progressAt(-10)));
    QVERIFY(qFuzzyCompare(MdRipple::progressAt(MdRipple::PressGrowMs), 1.0));
    QVERIFY(qFuzzyCompare(MdRipple::progressAt(MdRipple::PressGrowMs + 500), 1.0));

    qreal previous = -1.0;
    for (int ms = 0; ms <= MdRipple::PressGrowMs; ms += 15) {
        const qreal value = MdRipple::progressAt(ms);
        QVERIFY(value >= previous);
        QVERIFY(value >= 0.0 && value <= 1.0);
        previous = value;
    }
}

void TestMd3Ripple::frameTravelsFromPressPointToCenter()
{
    const QSizeF bounds(200.0, 50.0);
    const MdRipple::Geometry geometry = MdRipple::geometryFor(bounds);
    const QPointF press(20.0, 10.0);

    // At t = 0 the circle sits on the press point at its start radius.
    const QPointF start = MdRipple::centerAt(geometry, press, bounds, 0.0);
    QVERIFY(qFuzzyCompare(start.x(), press.x()));
    QVERIFY(qFuzzyCompare(start.y(), press.y()));
    QVERIFY(qFuzzyCompare(MdRipple::radiusAt(geometry, 0.0), geometry.initialSize / 2.0));

    // At t = 1 it is centred in the component at the full radius.
    const QPointF end = MdRipple::centerAt(geometry, press, bounds, 1.0);
    QVERIFY(qFuzzyCompare(end.x(), bounds.width() / 2.0));
    QVERIFY(qFuzzyCompare(end.y(), bounds.height() / 2.0));
    QVERIFY(qFuzzyCompare(MdRipple::radiusAt(geometry, 1.0), geometry.finalRadius));

    // A held press keeps the pressed opacity and stays valid past the growth.
    const MdRippleFrame held = MdRipple::frame(geometry, press, bounds, 500);
    QVERIFY(held.valid);
    QVERIFY(qFuzzyCompare(held.opacity, 0.12));
    QVERIFY(qFuzzyCompare(held.radius, geometry.finalRadius));

    // Out-of-range time yields nothing to paint.
    QVERIFY(!MdRipple::frame(geometry, press, bounds, -1).valid);
}

void TestMd3Ripple::releaseIsDeferredToMinimumPress()
{
    const QSizeF bounds(120.0, 48.0);
    const MdRipple::Geometry geometry = MdRipple::geometryFor(bounds);
    const QPointF press(60.0, 24.0);

    // Released after only 50 ms: the ripple is still held at full opacity
    // until MinimumPressMs, so a quick tap reads as a complete circle.
    QVERIFY(qFuzzyCompare(MdRipple::frame(geometry, press, bounds, 50, 50).opacity, 0.12));
    QVERIFY(qFuzzyCompare(MdRipple::frame(geometry, press, bounds, 200, 50).opacity, 0.12));
    QVERIFY(qFuzzyCompare(MdRipple::frame(geometry, press, bounds, 225, 50).opacity, 0.12));

    // Released after 300 ms: the fade starts immediately at 300 ms.
    QVERIFY(qFuzzyCompare(MdRipple::frame(geometry, press, bounds, 300, 300).opacity, 0.12));
    const MdRippleFrame mid = MdRipple::frame(geometry, press, bounds, 300 + 187, 300);
    QVERIFY(mid.opacity < 0.12);
    QVERIFY(mid.opacity > 0.0);

    QCOMPARE(MdRipple::lifetimeMs(50), 225 + 375);
    QCOMPARE(MdRipple::lifetimeMs(400), 400 + 375);
    QCOMPARE(MdRipple::lifetimeMs(-1), -1);
}

void TestMd3Ripple::fadeOutIsLinearOver375ms()
{
    const QSizeF bounds(120.0, 48.0);
    const MdRipple::Geometry geometry = MdRipple::geometryFor(bounds);
    const QPointF press(60.0, 24.0);

    // _ripple.scss: `transition: opacity 375ms linear`.
    const MdRippleFrame half = MdRipple::frame(geometry, press, bounds, 225 + 187, 0);
    QVERIFY(std::abs(half.opacity - 0.06) < 0.002);

    const MdRippleFrame done = MdRipple::frame(geometry, press, bounds, 225 + 375, 0);
    QVERIFY(!done.valid);
}

void TestMd3Ripple::paintsInsideClipPathOnly()
{
    QImage image(80, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    const QSizeF bounds(80.0, 40.0);
    const MdRipple::Geometry geometry = MdRipple::geometryFor(bounds);
    const MdRippleFrame frame =
        MdRipple::frame(geometry, QPointF(40.0, 20.0), bounds, MdRipple::PressGrowMs);

    QPainter painter(&image);
    // Clip to a 20px-radius rounded rect, leaving the corners untouched.
    const QPainterPath clip =
        MdShape::roundedRect(QRectF(0.0, 0.0, 80.0, 40.0), QList<qreal>{20.0, 20.0, 20.0, 20.0});
    MdRipple::paint(&painter, frame, clip, QColor(0, 0, 0));
    painter.end();

    QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 500);
    // The very corners are outside the clip path, so they stay white.
    QCOMPARE(image.pixel(0, 0), qRgb(255, 255, 255));
    QCOMPARE(image.pixel(79, 0), qRgb(255, 255, 255));
    QCOMPARE(image.pixel(0, 39), qRgb(255, 255, 255));
    // The centre is painted.
    QVERIFY(image.pixel(40, 20) != qRgb(255, 255, 255));

    // An empty clip path means "no clipping", not "draw nothing".
    QImage unclipped(80, 40, QImage::Format_ARGB32_Premultiplied);
    unclipped.fill(Qt::white);
    QPainter painter2(&unclipped);
    MdRipple::paint(&painter2, frame, QPainterPath(), QColor(0, 0, 0));
    painter2.end();
    QVERIFY(unclipped.pixel(0, 0) != qRgb(255, 255, 255));
}

void TestMd3Ripple::controllerTracksPressAndRelease()
{
    MdRippleController controller;
    controller.setBounds(QSizeF(100.0, 40.0));
    controller.setContentColor(QColor(0, 0, 0));

    QSignalSpy spy(&controller, &MdRippleController::repaintRequested);
    QVERIFY(!controller.isActive());
    // Idle means "nothing to paint". A valid frame here would let a caller that
    // paints on `frame.valid` draw a phantom ripple at the top-left corner
    // before the first press.
    QVERIFY(!controller.currentFrame().valid);

    controller.press(QPointF(10.0, 10.0));
    QVERIFY(controller.isActive());
    QVERIFY(spy.count() >= 1);
    QVERIFY(controller.currentFrame().valid);
    QVERIFY(controller.geometry().isValid());

    // Bounds changes mid-press recompute the geometry.
    controller.setBounds(QSizeF(300.0, 40.0));
    QVERIFY(qFuzzyCompare(controller.geometry().initialSize, 60.0));

    controller.cancel();
    QVERIFY(!controller.isActive());
    QVERIFY(!controller.currentFrame().valid);

    // Releasing without a press is a no-op, not a crash.
    controller.release();
    QVERIFY(!controller.isActive());

    // A press with no area must be ignored rather than start a zero-size ripple.
    controller.setBounds(QSizeF(0.0, 0.0));
    controller.press(QPointF(0.0, 0.0));
    QVERIFY(!controller.isActive());
    QVERIFY(!controller.currentFrame().valid);
}

QTEST_MAIN(TestMd3Ripple)

#include "TestMd3Ripple.moc"
