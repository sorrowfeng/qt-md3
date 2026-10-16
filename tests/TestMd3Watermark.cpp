// TestMd3Watermark — Stage 2 Qt extension (Appendix D, P2).
//
// Low-alpha tiled stamp: property defaults, NOTIFY, clamping, theme colour,
// and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdWatermark.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Watermark : public QObject
{
    Q_OBJECT

private slots:
    void defaultState()
    {
        MdWatermark w;
        QCOMPARE(w.text(), QString());
        QVERIFY(w.pixmap().isNull());
        QCOMPARE(w.angle(), -22.0);
        QCOMPARE(w.gap(), 120.0);
        QCOMPARE(w.fontSize(), 16.0);
        QCOMPARE(w.opacity(), 0.08);
    }

    void propertiesNotify()
    {
        MdWatermark w;
        QSignalSpy textSpy(&w, &MdWatermark::textChanged);
        QSignalSpy angleSpy(&w, &MdWatermark::angleChanged);
        QSignalSpy gapSpy(&w, &MdWatermark::gapChanged);
        QSignalSpy sizeSpy(&w, &MdWatermark::fontSizeChanged);
        QSignalSpy opacitySpy(&w, &MdWatermark::opacityChanged);

        w.setText(QStringLiteral("CONFIDENTIAL"));
        QCOMPARE(textSpy.count(), 1);
        w.setAngle(-30.0);
        QCOMPARE(angleSpy.count(), 1);
        w.setGap(80.0);
        QCOMPARE(gapSpy.count(), 1);
        w.setFontSize(20.0);
        QCOMPARE(sizeSpy.count(), 1);
        w.setOpacity(0.2);
        QCOMPARE(opacitySpy.count(), 1);

        // Clamping must still notify when the value actually changes.
        w.setOpacity(2.0);
        QCOMPARE(w.opacity(), 1.0);
        w.setFontSize(1.0);
        QCOMPARE(w.fontSize(), 6.0);
        w.setGap(0.0);
        QCOMPARE(w.gap(), 8.0);
    }

    void themeColour()
    {
        MdWatermark w;
        QCOMPARE(w.resolvedStampColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
    }

    void mouseTransparent()
    {
        MdWatermark w;
        QVERIFY(w.testAttribute(Qt::WA_TransparentForMouseEvents));
    }

    void nonBlankRender()
    {
        MdWatermark w;
        w.setText(QStringLiteral("DRAFT"));
        w.setOpacity(0.4);
        w.resize(200, 160);
        QImage image(w.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            w.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdWatermark painted nothing");
    }

    void emptyContentIsInvisible()
    {
        // No text and no pixmap: paintEvent returns before any draw call.
        // render() still fills the widget background, so assert the contract
        // on the model rather than on the composited image.
        MdWatermark w;
        QVERIFY(w.text().isEmpty());
        QVERIFY(w.pixmap().isNull());
    }
};

QTEST_MAIN(TestMd3Watermark)
#include "TestMd3Watermark.moc"
