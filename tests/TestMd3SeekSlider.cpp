// TestMd3SeekSlider — media seek slider: value model, preview, time format.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "widgets/MdSeekSlider.h"
#include "widgets/MdSlider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

class TestMd3SeekSlider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void valueModelForwardsToSlider();
    void previewToggles();
    void formatTime();
    void secondsPerUnit();
    void signalsForward();
    void metaProperties();
    void renderSmoke();
};

void TestMd3SeekSlider::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdFont::registerBundledFonts();
}

void TestMd3SeekSlider::defaults()
{
    MdSeekSlider s;
    QVERIFY(s.slider() != nullptr);
    QCOMPARE(s.value(), 0.0);
    QCOMPARE(s.min(), 0.0);
    QCOMPARE(s.max(), 100.0);
    QVERIFY(s.isPreviewEnabled());
    QCOMPARE(s.secondsPerUnit(), 1.0);
    QVERIFY(s.previewRect().isEmpty());
}

void TestMd3SeekSlider::valueModelForwardsToSlider()
{
    MdSeekSlider s;
    s.setMin(10.0);
    s.setMax(200.0);
    s.setStep(5.0);
    s.setValue(55.0);
    QCOMPARE(s.min(), 10.0);
    QCOMPARE(s.max(), 200.0);
    QCOMPARE(s.step(), 5.0);
    QCOMPARE(s.value(), 55.0);
    QCOMPARE(s.slider()->value(), 55.0);
}

void TestMd3SeekSlider::previewToggles()
{
    MdSeekSlider s;
    QSignalSpy spy(&s, &MdSeekSlider::previewEnabledChanged);
    s.setPreviewEnabled(false);
    QVERIFY(!s.isPreviewEnabled());
    QCOMPARE(spy.count(), 1);
    s.setPreviewEnabled(false);
    QCOMPARE(spy.count(), 1);
}

void TestMd3SeekSlider::formatTime()
{
    QCOMPARE(MdSeekSlider::formatTime(0), QStringLiteral("0:00"));
    QCOMPARE(MdSeekSlider::formatTime(65), QStringLiteral("1:05"));
    QCOMPARE(MdSeekSlider::formatTime(3599), QStringLiteral("59:59"));
    QCOMPARE(MdSeekSlider::formatTime(3600), QStringLiteral("1:00:00"));
    QCOMPARE(MdSeekSlider::formatTime(-5), QStringLiteral("0:00"));
}

void TestMd3SeekSlider::secondsPerUnit()
{
    MdSeekSlider s;
    QSignalSpy spy(&s, &MdSeekSlider::secondsPerUnitChanged);
    s.setSecondsPerUnit(0.5);
    QCOMPARE(s.secondsPerUnit(), 0.5);
    QCOMPARE(spy.count(), 1);
}

void TestMd3SeekSlider::signalsForward()
{
    MdSeekSlider s;
    QSignalSpy moved(&s, &MdSeekSlider::sliderMoved);
    QSignalSpy changed(&s, &MdSeekSlider::valueChanged);
    s.setValue(30.0);
    QCOMPARE(changed.count(), 1);
    s.slider()->setValue(40.0);
    QCOMPARE(changed.count(), 2);
    QCOMPARE(s.value(), 40.0);
}

void TestMd3SeekSlider::metaProperties()
{
    MdSeekSlider s;
    const QMetaObject *meta = s.metaObject();
    QVERIFY(meta->indexOfProperty("value") >= 0);
    QVERIFY(meta->indexOfProperty("previewEnabled") >= 0);
    QVERIFY(meta->indexOfProperty("secondsPerUnit") >= 0);
}

void TestMd3SeekSlider::renderSmoke()
{
    MdSeekSlider s;
    s.resize(240, 68);
    s.show();
    QImage image(s.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    s.render(&painter);
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0);
}

QTEST_MAIN(TestMd3SeekSlider)
#include "TestMd3SeekSlider.moc"
