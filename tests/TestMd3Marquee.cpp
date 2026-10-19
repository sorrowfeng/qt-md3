// TestMd3Marquee — the marquee label: overflow scroll, static fit, theme.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdMarquee.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

class TestMd3Marquee : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void textNotifiesAndUpdatesAccessibleName();
    void typeStyleAndEmphasis();
    void speedClamps();
    void gapProperty();
    void staticWhenItFits();
    void scrollingWhenItOverflows();
    void textColorOverride();
    void metaProperties();
    void renderSmoke();
};

void TestMd3Marquee::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdFont::registerBundledFonts();
}

void TestMd3Marquee::defaults()
{
    MdMarquee m;
    QCOMPARE(m.text(), QString());
    QCOMPARE(m.typeStyle(), TypeStyle::TitleLarge);
    QCOMPARE(m.emphasis(), TypeEmphasis::Baseline);
    QCOMPARE(m.speed(), 40.0);
    QCOMPARE(m.gap(), 24);
    QVERIFY(!m.isScrolling());
    QVERIFY(!m.textColor().isValid());
}

void TestMd3Marquee::textNotifiesAndUpdatesAccessibleName()
{
    MdMarquee m;
    QSignalSpy spy(&m, &MdMarquee::textChanged);
    m.setText(QStringLiteral("A long track title"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m.text(), QStringLiteral("A long track title"));
    QCOMPARE(m.accessibleName(), QStringLiteral("A long track title"));
    m.setText(QStringLiteral("A long track title"));
    QCOMPARE(spy.count(), 1);
}

void TestMd3Marquee::typeStyleAndEmphasis()
{
    MdMarquee m;
    QSignalSpy styleSpy(&m, &MdMarquee::typeStyleChanged);
    QSignalSpy emphSpy(&m, &MdMarquee::emphasisChanged);
    m.setTypeStyle(TypeStyle::HeadlineSmall);
    QCOMPARE(m.typeStyle(), TypeStyle::HeadlineSmall);
    QCOMPARE(styleSpy.count(), 1);
    m.setEmphasis(TypeEmphasis::Emphasized);
    QCOMPARE(m.emphasis(), TypeEmphasis::Emphasized);
    QCOMPARE(emphSpy.count(), 1);
    m.setTypeStyle(TypeStyle::Count);
    QCOMPARE(m.typeStyle(), TypeStyle::HeadlineSmall);
}

void TestMd3Marquee::speedClamps()
{
    MdMarquee m;
    QSignalSpy spy(&m, &MdMarquee::speedChanged);
    m.setSpeed(100.0);
    QCOMPARE(m.speed(), 100.0);
    m.setSpeed(0.0);
    QCOMPARE(m.speed(), 1.0);
    m.setSpeed(10000.0);
    QCOMPARE(m.speed(), 400.0);
    QCOMPARE(spy.count(), 3);
}

void TestMd3Marquee::gapProperty()
{
    MdMarquee m;
    QSignalSpy spy(&m, &MdMarquee::gapChanged);
    m.setGap(40);
    QCOMPARE(m.gap(), 40);
    m.setGap(-5);
    QCOMPARE(m.gap(), 0);
    QCOMPARE(spy.count(), 2);
}

void TestMd3Marquee::staticWhenItFits()
{
    MdMarquee m(QStringLiteral("Hi"));
    m.resize(400, 32);
    m.show();
    QVERIFY(!m.isScrolling());
}

void TestMd3Marquee::scrollingWhenItOverflows()
{
    MdMarquee m(QStringLiteral("A very long track title that will not fit in a narrow bar"));
    m.resize(40, 32);
    m.show();
    QVERIFY(m.isScrolling());
    QSignalSpy spy(&m, &MdMarquee::scrollingChanged);
    m.resize(800, 32);
    QVERIFY(!m.isScrolling());
    QCOMPARE(spy.count(), 1);
}

void TestMd3Marquee::textColorOverride()
{
    MdMarquee m;
    QSignalSpy spy(&m, &MdMarquee::textColorChanged);
    m.setTextColor(QColor(Qt::red));
    QCOMPARE(m.textColor(), QColor(Qt::red));
    QCOMPARE(spy.count(), 1);
}

void TestMd3Marquee::metaProperties()
{
    MdMarquee m;
    const QMetaObject *meta = m.metaObject();
    QVERIFY(meta->indexOfProperty("text") >= 0);
    QVERIFY(meta->indexOfProperty("typeStyle") >= 0);
    QVERIFY(meta->indexOfProperty("emphasis") >= 0);
    QVERIFY(meta->indexOfProperty("speed") >= 0);
    QVERIFY(meta->indexOfProperty("gap") >= 0);
    QVERIFY(meta->indexOfProperty("scrolling") >= 0);
    QVERIFY(meta->indexOfProperty("textColor") >= 0);
}

void TestMd3Marquee::renderSmoke()
{
    MdMarquee m(QStringLiteral("Now playing — a long title"));
    m.resize(200, 40);
    m.show();
    QImage image(m.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    m.render(&painter);
    painter.end();
    QVERIFY(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0);
}

QTEST_MAIN(TestMd3Marquee)
#include "TestMd3Marquee.moc"
