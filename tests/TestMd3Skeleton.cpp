// TestMd3Skeleton — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the skeleton semantics the header documents:
//   * surface-container-highest base fill;
//   * shimmer / shape / lines properties with NOTIFY;
//   * shape silhouette changes the painted blocks;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdSkeleton.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QImage renderWidget(QWidget &widget)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32);
}

} // namespace

class TestMd3Skeleton : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdSkeleton skeleton;
        QVERIFY(skeleton.isShimmering());
        QCOMPARE(skeleton.shape(), MdSkeletonShape::TextLine);
        QCOMPARE(skeleton.lines(), 3);
    }

    void propertiesNotify()
    {
        MdSkeleton skeleton;
        QSignalSpy shimmerSpy(&skeleton, &MdSkeleton::shimmeringChanged);
        QSignalSpy shapeSpy(&skeleton, &MdSkeleton::shapeChanged);
        QSignalSpy linesSpy(&skeleton, &MdSkeleton::linesChanged);

        skeleton.setShimmering(false);
        QCOMPARE(skeleton.isShimmering(), false);
        QCOMPARE(shimmerSpy.count(), 1);

        skeleton.setShape(MdSkeletonShape::Circle);
        QCOMPARE(skeleton.shape(), MdSkeletonShape::Circle);
        QCOMPARE(shapeSpy.count(), 1);

        skeleton.setLines(5);
        QCOMPARE(skeleton.lines(), 5);
        QCOMPARE(linesSpy.count(), 1);

        // Idempotent writes do not re-emit.
        skeleton.setShimmering(false);
        QCOMPARE(shimmerSpy.count(), 1);
    }

    void linesClamp()
    {
        MdSkeleton skeleton;
        skeleton.setLines(0);
        QCOMPARE(skeleton.lines(), 1);
    }

    void tokenColors()
    {
        MdSkeleton skeleton;
        QCOMPARE(skeleton.resolvedBaseColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainerHighest));
        QCOMPARE(skeleton.resolvedShimmerColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainerHigh));
    }

    void shapesRenderDifferently()
    {
        MdSkeleton rect(MdSkeletonShape::Rect);
        rect.resize(160, 80);
        rect.setShimmering(false);
        rect.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rect));
        const QImage rectImage = renderWidget(rect);

        MdSkeleton circle(MdSkeletonShape::Circle);
        circle.resize(160, 80);
        circle.setShimmering(false);
        circle.show();
        QVERIFY(QTest::qWaitForWindowExposed(&circle));
        const QImage circleImage = renderWidget(circle);

        QVERIFY2(mdtest::paintedPixelCount(rectImage, qRgba(0, 0, 0, 0)) > 0,
                 "MdSkeleton rect must paint");
        QVERIFY2(mdtest::paintedPixelCount(circleImage, qRgba(0, 0, 0, 0)) > 0,
                 "MdSkeleton circle must paint");
        QVERIFY(rectImage != circleImage);
    }

    void nonBlankRender()
    {
        MdSkeleton skeleton;
        skeleton.resize(200, 80);
        skeleton.show();
        QVERIFY(QTest::qWaitForWindowExposed(&skeleton));

        const QImage image = renderWidget(skeleton);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdSkeleton must paint its placeholder blocks");
    }

    void shimmerAnimationState()
    {
        MdSkeleton skeleton;
        skeleton.resize(80, 40);
        skeleton.show();
        QVERIFY(QTest::qWaitForWindowExposed(&skeleton));
        QVERIFY(skeleton.isAnimating());
        QVERIFY(skeleton.animationElapsedMs() >= 0);

        skeleton.setShimmering(false);
        QVERIFY(!skeleton.isAnimating());
    }
};

QTEST_MAIN(TestMd3Skeleton)
#include "TestMd3Skeleton.moc"
