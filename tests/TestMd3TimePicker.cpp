// TestMd3TimePicker — the Time pickers family: MdTimePicker + tokens + style.
//
// Pins the export's metric rows (the 256 px dial, the 48 px selector handle on
// an 8 px centre and 2 px track, the 96×80 / 114×80 time selectors, the
// 216×38 period selector), the two selector colour tables, the dial's snapping
// (hours to 1, minutes to 5-minute slots) and the face / period switching.

#include "core/MdTimePickerTokens.h"
#include "core/MdTheme.h"
#include "styles/MdTimePickerStyle.h"
#include "widgets/MdTimePicker.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdTimePicker &picker)
{
    QImage image(picker.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdTimePicker &>(picker).render(&painter);
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

class TestMd3TimePicker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void selectorColoursFollowTheExport();
    void geometryFollowsTheTokens();
    void dialSnapsToHours();
    void dialSnapsToFiveMinutes();
    void faceAndPeriodSwitch();
    void keyboardStepsTheTime();
    void renderSmoke();
    void metaProperties();
};

void TestMd3TimePicker::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdTimePickerStyle::shared() != nullptr);
}

void TestMd3TimePicker::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3TimePicker::tokenTableMatchesTheExport()
{
    const MdTimePickerTokens tokens = MdTimePickerTokens::resolve();

    // md.comp.time-picker.* at 34.0.21.
    QCOMPARE(tokens.dialSize, 256.0);
    QCOMPARE(tokens.selectorHandleSize, 48.0);
    QCOMPARE(tokens.selectorCenterSize, 8.0);
    QCOMPARE(tokens.selectorTrackWidth, 2.0);
    QCOMPARE(tokens.timeSelectorWidth, 96.0);
    QCOMPARE(tokens.timeSelectorHeight, 80.0);
    QCOMPARE(tokens.timeSelector24hWidth, 114.0);
    QCOMPARE(tokens.periodWidth, 216.0);
    QCOMPARE(tokens.periodHeight, 38.0);
    QCOMPARE(tokens.periodVerticalWidth, 52.0);
    QCOMPARE(tokens.periodVerticalHeight, 80.0);
    QCOMPARE(tokens.periodOutlineWidth, 1.0);
    QCOMPARE(tokens.containerRadius, 28.0);
    QCOMPARE(tokens.timeSelectorRadius, 8.0);
    QCOMPARE(tokens.periodRadius, 8.0);

    // The container and headline rows.
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
    QCOMPARE(tokens.headlineColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.dialColor, ColorRole::SurfaceContainerHighest);
}

void TestMd3TimePicker::selectorColoursFollowTheExport()
{
    const MdTimePickerTokens tokens = MdTimePickerTokens::resolve();
    const int e = int(MdTimeSelectorState::Enabled);

    // Time selectors: primary-container selected, surface-container-highest
    // unselected.
    QCOMPARE(tokens.timeSelected[e].role, ColorRole::PrimaryContainer);
    QCOMPARE(tokens.timeUnselected[e].role, ColorRole::SurfaceContainerHighest);
    // Period selectors: tertiary-container selected, on-surface-variant
    // unselected.
    QCOMPARE(tokens.periodSelected[e].role, ColorRole::TertiaryContainer);
    QCOMPARE(tokens.periodUnselected[e].role, ColorRole::OnSurfaceVariant);

    // The export publishes no disabled rows; the accessor falls back to
    // on-surface at 0.38.
    const int d = int(MdTimeSelectorState::Disabled);
    QCOMPARE(tokens.timeSelected[d].role, ColorRole::OnSurface);
    QCOMPARE(tokens.timeSelected[d].opacity, 0.38);
}

void TestMd3TimePicker::geometryFollowsTheTokens()
{
    MdTimePicker picker;
    const QSize hint = picker.sizeHint();
    QVERIFY(hint.width() >= 256);
    QVERIFY(hint.height() >= 256);

    const QRectF dial = picker.dialRect();
    QCOMPARE(dial.width(), 256.0);

    const QRectF handle = picker.selectorHandleRect();
    QCOMPARE(handle.width(), 48.0);
}

void TestMd3TimePicker::dialSnapsToHours()
{
    MdTimePicker picker;
    picker.setTime(QTime(12, 0));
    picker.setFace(MdTimePickerFace::Hours);

    // The dial's right edge is 3 o'clock.
    picker.setTime(QTime(3, 0));
    QCOMPARE(picker.time().hour(), 3);

    // 12h mode: hour 0 renders as 12.
    picker.setTime(QTime(0, 30));
    QCOMPARE(picker.time().hour(), 0);
}

void TestMd3TimePicker::dialSnapsToFiveMinutes()
{
    MdTimePicker picker;
    picker.setFace(MdTimePickerFace::Minutes);
    picker.setTime(QTime(12, 0));

    // Keyboard steps the minutes face by 5.
    picker.show();
    QVERIFY(QTest::qWaitForWindowExposed(&picker));
    picker.setFocus();
    QTest::keyClick(&picker, Qt::Key_Right);
    QCOMPARE(picker.time().minute(), 5);
    QTest::keyClick(&picker, Qt::Key_Left);
    QCOMPARE(picker.time().minute(), 0);
}

void TestMd3TimePicker::faceAndPeriodSwitch()
{
    MdTimePicker picker;
    QCOMPARE(picker.face(), MdTimePickerFace::Hours);
    picker.setFace(MdTimePickerFace::Minutes);
    QCOMPARE(picker.face(), MdTimePickerFace::Minutes);

    picker.setTime(QTime(9, 0));
    QCOMPARE(picker.period(), MdTimePeriod::Am);
    picker.setPeriod(MdTimePeriod::Pm);
    QCOMPARE(picker.time().hour(), 21);
    picker.setPeriod(MdTimePeriod::Am);
    QCOMPARE(picker.time().hour(), 9);
}

void TestMd3TimePicker::keyboardStepsTheTime()
{
    MdTimePicker picker;
    picker.setTime(QTime(3, 0));
    picker.show();
    QVERIFY(QTest::qWaitForWindowExposed(&picker));
    picker.setFocus();

    QTest::keyClick(&picker, Qt::Key_Right);
    QCOMPARE(picker.time().hour(), 4);
    QTest::keyClick(&picker, Qt::Key_Left);
    QCOMPARE(picker.time().hour(), 3);
    // Tab flips the face.
    QTest::keyClick(&picker, Qt::Key_Tab);
    QCOMPARE(picker.face(), MdTimePickerFace::Minutes);
}

void TestMd3TimePicker::renderSmoke()
{
    MdTimePicker picker;
    picker.setTime(QTime(10, 30));
    const QImage image = render(picker);
    QVERIFY(hasInk(image));
}

void TestMd3TimePicker::metaProperties()
{
    MdTimePicker picker;
    const QMetaObject *meta = picker.metaObject();
    QVERIFY(meta->indexOfProperty("time") >= 0);
    QVERIFY(meta->indexOfProperty("is24h") >= 0);
    QVERIFY(meta->indexOfProperty("face") >= 0);
    QVERIFY(meta->indexOfProperty("period") >= 0);

    QSignalSpy spy(&picker, &MdTimePicker::timeChanged);
    picker.setProperty("time", QTime(8, 15));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3TimePicker)
#include "TestMd3TimePicker.moc"
