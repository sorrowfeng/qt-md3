// TestMd3Slider — the Sliders family: MdSlider + MdSliderTokens + MdSliderStyle.
//
// Pins the export's metric rows (the 4 px pill handle with its per-state
// widths, the five Expressive size rows), the colour tables' disabled
// fallbacks, the value indicator's reveal, the tick marks and stop indicators,
// the keyboard walk and the range form's two handles.

#include "core/MdSliderTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdSliderStyle.h"
#include "widgets/MdSlider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdSlider &slider)
{
    QImage image(slider.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdSlider &>(slider).render(&painter);
    return image;
}

bool hasInk(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3Slider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void sizeRowsMatchTheExport();
    void disabledRowsCarryTheirOpacities();
    void handleWidthNarrowsUnderFocusAndPress();
    void geometryFollowsTheTokens();
    void valueClampsAndSnaps();
    void keyboardWalksTheValue();
    void rangeFormKeepsTwoHandles();
    void valueIndicatorReveals();
    void tickMarksPaint();
    void renderSmoke();
    void metaProperties();
};

void TestMd3Slider::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdSliderStyle::shared() != nullptr);
}

void TestMd3Slider::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3Slider::tokenTableMatchesTheExport()
{
    const MdSliderTokens tokens = MdSliderTokens::resolve(MdSliderSize::Medium);

    // The base metric rows — md.comp.slider.* at 34.0.21.
    QCOMPARE(tokens.handleWidth, 4.0);
    QCOMPARE(tokens.hoverHandleWidth, 4.0);
    QCOMPARE(tokens.focusHandleWidth, 2.0);
    QCOMPARE(tokens.pressedHandleWidth, 2.0);
    QCOMPARE(tokens.disabledHandleWidth, 4.0);
    QCOMPARE(tokens.handleLeadingSpace, 6.0);
    QCOMPARE(tokens.handleTrailingSpace, 6.0);
    QCOMPARE(tokens.handlePadding, 6.0);
    QCOMPARE(tokens.stopIndicatorSize, 4.0);
    QCOMPARE(tokens.stopIndicatorTrailingSpace, 4.0);
    QCOMPARE(tokens.tickMarkSize, 2.0);
    QCOMPARE(tokens.stateLayerSize, 40.0);
    QCOMPARE(tokens.valueIndicatorBottomSpace, 12.0);
    QCOMPARE(tokens.valueIndicatorMinSize, 28.0);

    // The disabled opacities.
    QCOMPARE(tokens.disabledActiveTrackOpacity, 0.38);
    QCOMPARE(tokens.disabledInactiveTrackOpacity, 0.12);
    QCOMPARE(tokens.disabledHandleOpacity, 0.38);
    QCOMPARE(tokens.disabledStopIndicatorOpacity, 0.38);

    // The value indicator's two rows.
    QCOMPARE(tokens.valueIndicatorContainer, ColorRole::InverseSurface);
    QCOMPARE(tokens.valueIndicatorLabel, ColorRole::InverseOnSurface);
}

void TestMd3Slider::sizeRowsMatchTheExport()
{
    // The five Expressive size sets: md.comp.slider.<size>.*.
    struct Row {
        MdSliderSize size;
        qreal track;
        qreal handle;
        qreal endRadius;
        qreal icon;
        qreal iconPadding;
    };
    const Row rows[] = {
        {MdSliderSize::XSmall, 16.0, 44.0, 8.0, 0.0, 0.0},
        {MdSliderSize::Small, 24.0, 44.0, 8.0, 0.0, 0.0},
        {MdSliderSize::Medium, 40.0, 44.0, 12.0, 24.0, 6.0},
        {MdSliderSize::Large, 56.0, 68.0, 16.0, 24.0, 6.0},
        {MdSliderSize::XLarge, 96.0, 108.0, 28.0, 32.0, 8.0},
    };
    for (const Row &row : rows) {
        const MdSliderTokens tokens = MdSliderTokens::resolve(row.size);
        QCOMPARE(tokens.trackHeight, row.track);
        QCOMPARE(tokens.handleHeight, row.handle);
        QCOMPARE(tokens.trackEndRadius, row.endRadius);
        QCOMPARE(tokens.iconSize, row.icon);
        QCOMPARE(tokens.iconPadding, row.iconPadding);
    }
}

void TestMd3Slider::disabledRowsCarryTheirOpacities()
{
    const MdSliderTokens tokens = MdSliderTokens::resolve(MdSliderSize::Medium);
    const int d = int(MdSliderInteraction::Disabled);

    QCOMPARE(tokens.activeTrack[d].role, ColorRole::OnSurface);
    QCOMPARE(tokens.activeTrack[d].opacity, 0.38);
    QCOMPARE(tokens.inactiveTrack[d].role, ColorRole::OnSurface);
    QCOMPARE(tokens.inactiveTrack[d].opacity, 0.12);
    QCOMPARE(tokens.handle[d].role, ColorRole::OnSurface);
    QCOMPARE(tokens.handle[d].opacity, 0.38);

    // The enabled rows all resolve the export's single family.
    for (int i = 0; i < d; ++i) {
        QCOMPARE(tokens.activeTrack[i].role, ColorRole::Primary);
        QCOMPARE(tokens.inactiveTrack[i].role, ColorRole::SecondaryContainer);
        QCOMPARE(tokens.handle[i].role, ColorRole::Primary);
    }
}

void TestMd3Slider::handleWidthNarrowsUnderFocusAndPress()
{
    MdSlider slider;
    slider.resize(240, 48);
    QCOMPARE(slider.animatedHandleWidth(), 4.0);

    // Pressed: the pill narrows to 2 px immediately (the SnapSpec contract).
    QTest::mousePress(&slider, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(120, 24));
    QCOMPARE(slider.animatedHandleWidth(), 2.0);
    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::KeyboardModifiers(), QPoint(120, 24));
}

void TestMd3Slider::geometryFollowsTheTokens()
{
    MdSlider slider;
    slider.setSliderSize(MdSliderSize::Medium);
    slider.resize(240, 48);

    const MdSliderTokens tokens = slider.sliderTokens();
    QCOMPARE(tokens.trackHeight, 40.0);

    const QRectF track = slider.trackRect();
    QCOMPARE(track.height(), 40.0);
    // Inset by half the state layer so the pill can sit flush at the ends.
    QCOMPARE(track.left(), 20.0);

    const QRectF handle = slider.handleRect();
    QCOMPARE(handle.height(), 44.0);
}

void TestMd3Slider::valueClampsAndSnaps()
{
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);

    slider.setValue(150.0);
    QCOMPARE(slider.value(), 100.0);
    slider.setValue(-20.0);
    QCOMPARE(slider.value(), 0.0);

    slider.setStep(5.0);
    slider.setValue(12.0);
    QCOMPARE(slider.value(), 10.0);
    slider.setValue(13.0);
    QCOMPARE(slider.value(), 15.0);
}

void TestMd3Slider::keyboardWalksTheValue()
{
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(5.0);
    slider.resize(240, 48);
    slider.show();
    QVERIFY(QTest::qWaitForWindowExposed(&slider));
    slider.setFocus();

    QTest::keyClick(&slider, Qt::Key_Right);
    QCOMPARE(slider.value(), 5.0);
    QTest::keyClick(&slider, Qt::Key_Left);
    QCOMPARE(slider.value(), 0.0);
    QTest::keyClick(&slider, Qt::Key_End);
    QCOMPARE(slider.value(), 100.0);
    QTest::keyClick(&slider, Qt::Key_Home);
    QCOMPARE(slider.value(), 0.0);

    // PageUp walks a tenth of the range.
    slider.setStep(0.0);
    QTest::keyClick(&slider, Qt::Key_PageUp);
    QCOMPARE(slider.value(), 10.0);
}

void TestMd3Slider::rangeFormKeepsTwoHandles()
{
    MdSlider slider;
    slider.setRange(true);
    slider.setValueStart(25.0);
    slider.setValueEnd(75.0);

    QCOMPARE(slider.isRange(), true);
    QCOMPARE(slider.valueStart(), 25.0);
    QCOMPARE(slider.valueEnd(), 75.0);

    // The two handles sit at their own fractions of the track.
    slider.resize(240, 48);
    const QRectF start = slider.handleRect(true);
    const QRectF end = slider.handleRect(false);
    QVERIFY(start.center().x() < end.center().x());
}

void TestMd3Slider::valueIndicatorReveals()
{
    MdSlider slider;
    slider.setLabeled(true);
    slider.resize(240, 48);
    QCOMPARE(slider.labelReveal(), 0.0);

    // Hovering drives the reveal target; the tick loop settles it.
    QEnterEvent enter(QPointF(120, 24), QPointF(120, 24), QPointF(120, 24));
    QApplication::sendEvent(&slider, &enter);
    QTRY_VERIFY_WITH_TIMEOUT(slider.labelReveal() > 0.5, 500);
}

void TestMd3Slider::tickMarksPaint()
{
    MdSlider slider;
    slider.setStep(10.0);
    slider.setTicks(true);
    slider.resize(240, 48);
    slider.setValue(50.0);

    const QImage image = render(slider);
    QVERIFY(hasInk(image));
}

void TestMd3Slider::renderSmoke()
{
    MdSlider slider;
    slider.resize(240, 48);
    slider.setValue(30.0);
    const QImage image = render(slider);
    QVERIFY(hasInk(image));

    slider.setSliderSize(MdSliderSize::XLarge);
    slider.resize(320, 120);
    QVERIFY(hasInk(render(slider)));
}

void TestMd3Slider::metaProperties()
{
    MdSlider slider;
    const QMetaObject *meta = slider.metaObject();

    QVERIFY(meta->indexOfProperty("value") >= 0);
    QVERIFY(meta->indexOfProperty("valueStart") >= 0);
    QVERIFY(meta->indexOfProperty("valueEnd") >= 0);
    QVERIFY(meta->indexOfProperty("min") >= 0);
    QVERIFY(meta->indexOfProperty("max") >= 0);
    QVERIFY(meta->indexOfProperty("step") >= 0);
    QVERIFY(meta->indexOfProperty("ticks") >= 0);
    QVERIFY(meta->indexOfProperty("labeled") >= 0);
    QVERIFY(meta->indexOfProperty("range") >= 0);
    QVERIFY(meta->indexOfProperty("sliderSize") >= 0);
    QVERIFY(meta->indexOfProperty("valueLabel") >= 0);

    QSignalSpy spy(&slider, &MdSlider::valueChanged);
    slider.setProperty("value", 42.0);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3Slider)
#include "TestMd3Slider.moc"
