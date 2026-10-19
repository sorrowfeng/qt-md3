// TestMd3ColorPicker — Stage 2 Qt extension (Appendix D, P2).
//
// Dialog + Slider(HSV) + Text field(Hex): property defaults, HSV/hex sync,
// NOTIFY, swatch shape, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdColorPicker.h"
#include "widgets/MdSlider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3ColorPicker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<ShapeCorner>("md::ShapeCorner");
    }

    void defaultState()
    {
        MdColorPicker picker;
        QVERIFY(picker.color().isValid());
        QCOMPARE(picker.hue(), 0.0);
        QCOMPARE(picker.saturation(), 1.0);
        QCOMPARE(picker.value(), 1.0);
        QVERIFY(picker.isHexVisible());
        QCOMPARE(picker.swatchShape(), ShapeCorner::Full);
    }

    void hexRoundTrip()
    {
        MdColorPicker picker;
        picker.setHexCode(QStringLiteral("#FF8800"));
        QCOMPARE(picker.color().name(QColor::HexRgb).toUpper(), QStringLiteral("#FF8800"));
        QCOMPARE(picker.hexCode(), QStringLiteral("#FF8800"));
    }

    void hsvSync()
    {
        MdColorPicker picker;
        QSignalSpy colorSpy(&picker, &MdColorPicker::colorChanged);
        QSignalSpy hueSpy(&picker, &MdColorPicker::hueChanged);

        picker.setHue(120.0);
        QCOMPARE(picker.hue(), 120.0);
        QVERIFY(colorSpy.count() > 0);
        QVERIFY(hueSpy.count() > 0);

        picker.setSaturation(0.5);
        QCOMPARE(picker.saturation(), 0.5);
        picker.setValue(0.25);
        QCOMPARE(picker.value(), 0.25);
    }

    void colorSetsHsv()
    {
        MdColorPicker picker;
        const QColor red(255, 0, 0);
        picker.setColor(red);
        QCOMPARE(picker.color().red(), 255);
        QCOMPARE(picker.color().green(), 0);
        QCOMPARE(picker.color().blue(), 0);
    }

    void propertiesNotify()
    {
        MdColorPicker picker;
        QSignalSpy hexSpy(&picker, &MdColorPicker::hexVisibleChanged);
        QSignalSpy shapeSpy(&picker, &MdColorPicker::swatchShapeChanged);

        picker.setHexVisible(false);
        QCOMPARE(hexSpy.count(), 1);
        QVERIFY(!picker.isHexVisible());
        picker.setHexVisible(false);
        QCOMPARE(hexSpy.count(), 1);

        picker.setSwatchShape(ShapeCorner::ExtraLarge);
        QCOMPARE(shapeSpy.count(), 1);
        QCOMPARE(picker.swatchShape(), ShapeCorner::ExtraLarge);
    }

    void ringColour()
    {
        MdColorPicker picker;
        QCOMPARE(picker.resolvedRingColor(),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
    }

    void nonBlankRender()
    {
        MdColorPicker picker;
        picker.setColor(QColor(30, 144, 255));
        picker.resize(picker.sizeHint());
        QImage image(picker.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            picker.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdColorPicker painted nothing");
    }

    void narrowWidthKeepsSlidersInside()
    {
        // Phone widths used to hard-code a 240 px slider column, so the swatch
        // disc painted over the tracks. The sliders must stay left of the
        // trailing edge at every width we ship.
        const int widths[] = {240, 280, 320, 360, 420};
        for (int w : widths) {
            MdColorPicker picker;
            picker.resize(w, picker.minimumSizeHint().height());
            picker.show();
            QApplication::processEvents();

            const QList<qreal> unused;
            Q_UNUSED(unused);
            const QRect hue = picker.hueSlider()->geometry();
            const QRect sat = picker.saturationSlider()->geometry();
            const QRect val = picker.valueSlider()->geometry();
            QVERIFY(hue.isValid() && sat.isValid() && val.isValid());
            QVERIFY2(hue.right() < w, qPrintable(QStringLiteral("hue clipped at %1").arg(w)));
            QCOMPARE(hue.left(), val.left());
            QCOMPARE(hue.width(), val.width());
            // Sliders occupy a single column; they must not run under the
            // swatch, which is painted on the trailing side.
            QVERIFY2(hue.width() > 80, qPrintable(QStringLiteral("slider column too narrow at %1").arg(w)));
        }
    }
};

QTEST_MAIN(TestMd3ColorPicker)
#include "TestMd3ColorPicker.moc"
