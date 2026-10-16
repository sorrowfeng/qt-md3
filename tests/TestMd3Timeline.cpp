// TestMd3Timeline — Stage 2 Qt extension (Appendix D, P2).
//
// Event list + connector: property defaults, NOTIFY, status colours,
// connector visibility, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdTimeline.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Timeline : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdTimelineStatus>("md::MdTimelineStatus");
        qRegisterMetaType<MdTimelineItem>("md::MdTimelineItem");
        qRegisterMetaType<QList<md::MdTimelineItem>>("QList<md::MdTimelineItem>");
    }

    void defaultState()
    {
        MdTimeline t;
        QVERIFY(t.items().isEmpty());
        QCOMPARE(t.isConnectorVisible(), true);
        QCOMPARE(t.isTimestampsVisible(), true);
    }

    void itemsAndNotify()
    {
        MdTimeline t;
        QSignalSpy itemsSpy(&t, &MdTimeline::itemsChanged);
        QSignalSpy connSpy(&t, &MdTimeline::connectorVisibleChanged);
        QSignalSpy stampSpy(&t, &MdTimeline::timestampsVisibleChanged);

        t.addItem(MdTimelineItem(QStringLiteral("Started"), QStringLiteral("Boot"),
                                 QStringLiteral("09:00"), MdTimelineStatus::Complete));
        QCOMPARE(itemsSpy.count(), 1);
        QCOMPARE(t.items().size(), 1);

        t.setConnectorVisible(false);
        QCOMPARE(connSpy.count(), 1);
        t.setTimestampsVisible(false);
        QCOMPARE(stampSpy.count(), 1);

        t.clearItems();
        QVERIFY(t.items().isEmpty());
    }

    void statusColours()
    {
        MdTimeline t;
        QCOMPARE(t.resolvedDotColor(MdTimelineStatus::Default),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
        QCOMPARE(t.resolvedDotColor(MdTimelineStatus::Active),
                 MdTheme::instance().color(ColorRole::Primary));
        QCOMPARE(t.resolvedDotColor(MdTimelineStatus::Complete),
                 MdTheme::instance().color(ColorRole::Primary));
        QCOMPARE(t.resolvedDotColor(MdTimelineStatus::Error),
                 MdTheme::instance().color(ColorRole::Error));
        QCOMPARE(t.resolvedTitleColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
        QCOMPARE(t.resolvedSupportingColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        QCOMPARE(t.resolvedConnectorColor(),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
    }

    void heightForWidth()
    {
        MdTimeline t;
        t.addItem(MdTimelineItem(QStringLiteral("One"), QStringLiteral("First")));
        t.addItem(MdTimelineItem(QStringLiteral("Two"), QStringLiteral("Second")));
        QVERIFY(t.heightForWidth(320) > 0);
    }

    void nonBlankRender()
    {
        MdTimeline t;
        t.addItem(MdTimelineItem(QStringLiteral("Created"), QStringLiteral("Draft"),
                                 QStringLiteral("Mon"), MdTimelineStatus::Complete));
        t.addItem(MdTimelineItem(QStringLiteral("Review"), QStringLiteral("In review"),
                                 QStringLiteral("Tue"), MdTimelineStatus::Active));
        t.addItem(MdTimelineItem(QStringLiteral("Merged"), QString(),
                                 QStringLiteral("Wed"), MdTimelineStatus::Error));
        t.resize(320, 220);
        QImage image(t.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            t.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdTimeline painted nothing");
    }
};

QTEST_MAIN(TestMd3Timeline)
#include "TestMd3Timeline.moc"
