// TestMd3ScrollBar — Stage 2 Qt extension (Appendix D, P0).
//
// MD3 has no scrollbar family; this suite pins the derived overlay spec the
// header documents:
//
//   * thickness is a live Q_PROPERTY (4..24, default 8) and the style
//     honours it when computing the handle rect;
//   * visibleMode (Auto / Always / Never) controls chrome; Never paints
//     nothing even though the bar still scrolls;
//   * lifted is the hover/drag readout the 0.38 / 0.46 / 0.54 alpha ladder
//     is keyed on;
//   * the bar installs MdScrollBarStyle on itself so callers do not have to.
//
// Pixel assertions go through QWidget::render (as in the other suites) so
// they do not depend on the font machinery.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdScrollBarStyle.h"
#include "widgets/MdScrollBar.h"

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

int paintedColumnExtent(const QImage &image, int column)
{
    int rows = 0;
    for (int y = 0; y < image.height(); ++y) {
        if (image.pixelColor(column, y).alpha() > 8) {
            ++rows;
        }
    }
    return rows;
}

} // namespace

class TestMd3ScrollBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultProperties()
    {
        MdScrollBar bar(Qt::Vertical);
        QCOMPARE(bar.thickness(), 8);
        QCOMPARE(bar.visibleMode(), MdScrollBarVisibility::Auto);
        QVERIFY(!bar.isLifted());
    }

    void thicknessIsLiveAndClamped()
    {
        MdScrollBar bar(Qt::Vertical);
        QSignalSpy spy(&bar, &MdScrollBar::thicknessChanged);

        bar.setThickness(12);
        QCOMPARE(bar.thickness(), 12);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.takeFirst().at(0).toInt(), 12);

        bar.setThickness(2);
        QCOMPARE(bar.thickness(), 4); // clamped to the minimum
        bar.setThickness(99);
        QCOMPARE(bar.thickness(), 24); // clamped to the maximum

        // Idempotent writes do not re-emit.
        spy.clear();
        bar.setThickness(24);
        QCOMPARE(spy.count(), 0);
    }

    void visibleModeIsLive()
    {
        MdScrollBar bar(Qt::Vertical);
        QSignalSpy spy(&bar, &MdScrollBar::visibleModeChanged);

        bar.setVisibleMode(MdScrollBarVisibility::Always);
        QCOMPARE(bar.visibleMode(), MdScrollBarVisibility::Always);
        QCOMPARE(spy.count(), 1);

        bar.setVisibleMode(MdScrollBarVisibility::Never);
        QCOMPARE(bar.visibleMode(), MdScrollBarVisibility::Never);
        QCOMPARE(spy.count(), 2);
    }

    void styleInstallsItself()
    {
        MdScrollBar bar(Qt::Vertical);
        QVERIFY(bar.style() == MdScrollBarStyle::shared());
    }

    void handleRespectsThickness()
    {
        const QRect bar(0, 0, 16, 200);
        const QRect thin = MdScrollBarStyle::handleRectFor(
            bar, Qt::Vertical, 0, 100, 25, 50, false, 4);
        const QRect wide = MdScrollBarStyle::handleRectFor(
            bar, Qt::Vertical, 0, 100, 25, 50, false, 12);
        QCOMPARE(thin.width(), 4);
        QCOMPARE(wide.width(), 12);
        // Both stay centred on the same track line.
        QCOMPARE(thin.center().x(), wide.center().x());
    }

    void neverModePaintsNothing()
    {
        MdScrollBar bar(Qt::Vertical);
        bar.setRange(0, 100);
        bar.setPageStep(25);
        bar.resize(16, 200);
        bar.setVisibleMode(MdScrollBarVisibility::Never);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        const QImage image = renderWidget(bar);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 8) {
                    QFAIL("MdScrollBarVisibility::Never must paint no chrome");
                }
            }
        }
    }

    void alwaysModePaintsAtRest()
    {
        MdScrollBar bar(Qt::Vertical);
        bar.setRange(0, 100);
        bar.setPageStep(25);
        bar.resize(16, 200);
        bar.setVisibleMode(MdScrollBarVisibility::Always);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        const QImage image = renderWidget(bar);
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixelColor(x, y).alpha() > 8) {
                    ++painted;
                }
            }
        }
        QVERIFY2(painted > 0, "Always mode must paint the handle at rest");
    }

    void liftedReadsHoverAndDrag()
    {
        MdScrollBar bar(Qt::Vertical);
        QSignalSpy spy(&bar, &MdScrollBar::liftedChanged);

        bar.resize(16, 200);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        QTest::mouseMove(&bar, QPoint(8, 100));
        QCoreApplication::processEvents();
        // enterEvent is synthesised by the window system on move; some
        // platforms need an explicit enter. Force it through the public
        // event path so the test is platform-independent.
        QEvent enter(QEvent::Enter);
        QCoreApplication::sendEvent(&bar, &enter);
        QVERIFY(bar.isLifted());

        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(&bar, &leave);
        QVERIFY(!bar.isLifted());
    }

    void alphaLadderIsUnchanged()
    {
        // The derived spec's published ladder — keep it stable.
        QCOMPARE(MdScrollBarStyle::idleAlpha(), 0.38);
        QCOMPARE(MdScrollBarStyle::hoverAlpha(), 0.46);
        QCOMPARE(MdScrollBarStyle::pressedAlpha(), 0.54);
        QCOMPARE(MdScrollBarStyle::disabledAlpha(), 0.12);
    }
};

QTEST_MAIN(TestMd3ScrollBar)
#include "TestMd3ScrollBar.moc"
