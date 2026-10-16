// TestMd3Rate — Stage 2 Qt extension (Appendix D, P2).
//
// Pins the rate semantics the header documents:
//   * value / max / readOnly / halfSteps / glyph properties with NOTIFY;
//   * value clamps into [0, max];
//   * icon-button selected / unselected colour pair;
//   * keyboard and click commit emit rated;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdRate.h"

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

class TestMd3Rate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdRate rate;
        QCOMPARE(rate.value(), 0.0);
        QCOMPARE(rate.max(), 5);
        QVERIFY(!rate.isReadOnly());
        QVERIFY(!rate.halfSteps());
        QCOMPARE(rate.glyph(), MdRateGlyph::Star);
        QCOMPARE(MdRate::cellSize(), 48.0);
    }

    void valueClamps()
    {
        MdRate rate;
        rate.setMax(5);
        rate.setValue(9.0);
        QCOMPARE(rate.value(), 5.0);
        rate.setValue(-1.0);
        QCOMPARE(rate.value(), 0.0);
    }

    void propertiesNotify()
    {
        MdRate rate;
        QSignalSpy valueSpy(&rate, &MdRate::valueChanged);
        QSignalSpy maxSpy(&rate, &MdRate::maxChanged);
        QSignalSpy readSpy(&rate, &MdRate::readOnlyChanged);
        QSignalSpy halfSpy(&rate, &MdRate::halfStepsChanged);
        QSignalSpy glyphSpy(&rate, &MdRate::glyphChanged);

        rate.setValue(3.0);
        QCOMPARE(valueSpy.count(), 1);

        rate.setMax(10);
        QCOMPARE(maxSpy.count(), 1);

        rate.setReadOnly(true);
        QCOMPARE(readSpy.count(), 1);

        rate.setHalfSteps(true);
        QCOMPARE(halfSpy.count(), 1);

        rate.setGlyph(MdRateGlyph::Heart);
        QCOMPARE(glyphSpy.count(), 1);
    }

    void colours()
    {
        MdRate rate;
        QCOMPARE(rate.resolvedSelectedColor(),
                 MdTheme::instance().color(ColorRole::Primary));
        QCOMPARE(rate.resolvedUnselectedColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));
    }

    void keyboardCommits()
    {
        MdRate rate;
        rate.resize(240, 48);
        rate.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rate));

        QSignalSpy ratedSpy(&rate, &MdRate::rated);
        rate.setFocus();
        QTest::keyClick(&rate, Qt::Key_Right);
        QCOMPARE(rate.value(), 1.0);
        QCOMPARE(ratedSpy.count(), 1);

        QTest::keyClick(&rate, Qt::Key_End);
        QCOMPARE(rate.value(), 5.0);

        QTest::keyClick(&rate, Qt::Key_Home);
        QCOMPARE(rate.value(), 0.0);
    }

    void halfSteps()
    {
        MdRate rate;
        rate.setHalfSteps(true);
        rate.resize(240, 48);
        rate.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rate));

        rate.setFocus();
        QTest::keyClick(&rate, Qt::Key_Right);
        QCOMPARE(rate.value(), 0.5);
    }

    void readOnlyBlocksInput()
    {
        MdRate rate;
        rate.setReadOnly(true);
        rate.resize(240, 48);
        rate.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rate));

        rate.setFocus();
        QTest::keyClick(&rate, Qt::Key_Right);
        QCOMPARE(rate.value(), 0.0);
    }

    void clickCommits()
    {
        MdRate rate;
        rate.setHalfSteps(false);
        rate.resize(240, 48);
        rate.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rate));

        QSignalSpy ratedSpy(&rate, &MdRate::rated);
        QTest::mouseClick(&rate, Qt::LeftButton, {}, QPoint(100, 24));
        // 100 / 48 = 2.08 -> third cell fully filled -> value 3.
        QCOMPARE(rate.value(), 3.0);
        QCOMPARE(ratedSpy.count(), 1);
    }

    void nonBlankRender()
    {
        MdRate rate;
        rate.setValue(3.0);
        rate.resize(240, 48);
        rate.show();
        QVERIFY(QTest::qWaitForWindowExposed(&rate));

        const QImage image = renderWidget(rate);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdRate must paint its star cells");
    }
};

QTEST_MAIN(TestMd3Rate)
#include "TestMd3Rate.moc"
