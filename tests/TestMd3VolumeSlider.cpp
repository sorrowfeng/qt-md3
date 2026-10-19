// TestMd3VolumeSlider — mute icon levels, value model, mute toggle.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdVolumeSlider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

class TestMd3VolumeSlider : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void valueClampsAndNotifies();
    void muteTogglesAndIconChanges();
    void raisingVolumeUnmutes();
    void iconVisible();
    void metaProperties();
    void renderSmoke();
};

void TestMd3VolumeSlider::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdFont::registerBundledFonts();
}

void TestMd3VolumeSlider::defaults()
{
    MdVolumeSlider v;
    QVERIFY(v.slider() != nullptr);
    QVERIFY(v.button() != nullptr);
    QCOMPARE(v.value(), 80.0);
    QVERIFY(!v.isMuted());
    QCOMPARE(v.iconName(), QStringLiteral("volume_up"));
    QVERIFY(v.isIconVisible());
}

void TestMd3VolumeSlider::valueClampsAndNotifies()
{
    MdVolumeSlider v;
    QSignalSpy spy(&v, &MdVolumeSlider::valueChanged);
    v.setValue(50.0);
    QCOMPARE(v.value(), 50.0);
    v.setValue(200.0);
    QCOMPARE(v.value(), 100.0);
    v.setValue(-10.0);
    QCOMPARE(v.value(), 0.0);
    QCOMPARE(spy.count(), 3);
    QCOMPARE(v.slider()->value(), 0.0);
}

void TestMd3VolumeSlider::muteTogglesAndIconChanges()
{
    MdVolumeSlider v;
    QSignalSpy mutedSpy(&v, &MdVolumeSlider::mutedChanged);
    QSignalSpy iconSpy(&v, &MdVolumeSlider::iconNameChanged);
    v.setMuted(true);
    QVERIFY(v.isMuted());
    QCOMPARE(v.iconName(), QStringLiteral("volume_off"));
    QCOMPARE(mutedSpy.count(), 1);
    QVERIFY(iconSpy.count() >= 1);

    v.setValue(20.0);
    v.setMuted(false);
    QCOMPARE(v.iconName(), QStringLiteral("volume_down"));
    v.setValue(90.0);
    QCOMPARE(v.iconName(), QStringLiteral("volume_up"));
}

void TestMd3VolumeSlider::raisingVolumeUnmutes()
{
    MdVolumeSlider v;
    v.setValue(40.0);
    v.setMuted(true);
    QVERIFY(v.isMuted());
    QSignalSpy spy(&v, &MdVolumeSlider::mutedChanged);
    v.setValue(60.0);
    QVERIFY(!v.isMuted());
    QCOMPARE(spy.count(), 1);
}

void TestMd3VolumeSlider::iconVisible()
{
    MdVolumeSlider v;
    QSignalSpy spy(&v, &MdVolumeSlider::iconVisibleChanged);
    v.setIconVisible(false);
    QVERIFY(!v.isIconVisible());
    QVERIFY(!v.button()->isVisible());
    QCOMPARE(spy.count(), 1);
}

void TestMd3VolumeSlider::metaProperties()
{
    MdVolumeSlider v;
    const QMetaObject *meta = v.metaObject();
    QVERIFY(meta->indexOfProperty("value") >= 0);
    QVERIFY(meta->indexOfProperty("muted") >= 0);
    QVERIFY(meta->indexOfProperty("iconName") >= 0);
}

void TestMd3VolumeSlider::renderSmoke()
{
    MdVolumeSlider v;
    v.resize(240, 48);
    v.show();
    QImage image(v.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    v.render(&painter);
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0);
}

QTEST_MAIN(TestMd3VolumeSlider)
#include "TestMd3VolumeSlider.moc"
