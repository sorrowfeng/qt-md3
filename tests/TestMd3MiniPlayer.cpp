// TestMd3MiniPlayer — title/artist/thumbnail, play toggle, progress.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMiniPlayer.h"
#include "widgets/MdSeekSlider.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

class TestMd3MiniPlayer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void textPropertiesNotify();
    void playTogglesStateAndEmits();
    void progressForwards();
    void progressVisible();
    void thumbnail();
    void metaProperties();
    void renderSmoke();
};

void TestMd3MiniPlayer::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdFont::registerBundledFonts();
}

void TestMd3MiniPlayer::defaults()
{
    MdMiniPlayer p;
    QVERIFY(p.playButton() != nullptr);
    QVERIFY(p.nextButton() != nullptr);
    QVERIFY(p.progress() != nullptr);
    QVERIFY(!p.isPlaying());
    QCOMPARE(p.title(), QString());
    QVERIFY(p.isProgressVisible());
}

void TestMd3MiniPlayer::textPropertiesNotify()
{
    MdMiniPlayer p;
    QSignalSpy titleSpy(&p, &MdMiniPlayer::titleChanged);
    QSignalSpy artistSpy(&p, &MdMiniPlayer::artistChanged);
    p.setTitle(QStringLiteral("Song"));
    p.setArtist(QStringLiteral("Artist"));
    QCOMPARE(titleSpy.count(), 1);
    QCOMPARE(artistSpy.count(), 1);
    QCOMPARE(p.title(), QStringLiteral("Song"));
    QCOMPARE(p.artist(), QStringLiteral("Artist"));
}

void TestMd3MiniPlayer::playTogglesStateAndEmits()
{
    MdMiniPlayer p;
    QSignalSpy spy(&p, &MdMiniPlayer::playClicked);
    QSignalSpy stateSpy(&p, &MdMiniPlayer::playingChanged);
    p.playButton()->click();
    QVERIFY(p.isPlaying());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(stateSpy.count(), 1);
    p.playButton()->click();
    QVERIFY(!p.isPlaying());
    QCOMPARE(spy.count(), 2);
}

void TestMd3MiniPlayer::progressForwards()
{
    MdMiniPlayer p;
    p.setMax(200.0);
    QCOMPARE(p.max(), 200.0);
    QSignalSpy spy(&p, &MdMiniPlayer::valueChanged);
    p.setValue(80.0);
    QCOMPARE(p.value(), 80.0);
    QCOMPARE(spy.count(), 1);
}

void TestMd3MiniPlayer::progressVisible()
{
    MdMiniPlayer p;
    QSignalSpy spy(&p, &MdMiniPlayer::progressVisibleChanged);
    p.setProgressVisible(false);
    QVERIFY(!p.isProgressVisible());
    QCOMPARE(spy.count(), 1);
}

void TestMd3MiniPlayer::thumbnail()
{
    MdMiniPlayer p;
    QPixmap pix(16, 16);
    pix.fill(Qt::blue);
    QSignalSpy spy(&p, &MdMiniPlayer::thumbnailChanged);
    p.setThumbnail(pix);
    QCOMPARE(spy.count(), 1);
    QVERIFY(!p.thumbnail().isNull());
}

void TestMd3MiniPlayer::metaProperties()
{
    MdMiniPlayer p;
    const QMetaObject *meta = p.metaObject();
    QVERIFY(meta->indexOfProperty("title") >= 0);
    QVERIFY(meta->indexOfProperty("artist") >= 0);
    QVERIFY(meta->indexOfProperty("playing") >= 0);
    QVERIFY(meta->indexOfProperty("value") >= 0);
}

void TestMd3MiniPlayer::renderSmoke()
{
    MdMiniPlayer p;
    p.setTitle(QStringLiteral("Midnight City"));
    p.setArtist(QStringLiteral("M83"));
    p.resize(360, 80);
    p.show();
    QImage image(p.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    p.render(&painter);
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0);
}

QTEST_MAIN(TestMd3MiniPlayer)
#include "TestMd3MiniPlayer.moc"
