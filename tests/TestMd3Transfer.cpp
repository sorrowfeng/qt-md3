// TestMd3Transfer — Stage 2 Qt extension (Appendix D, P2).
//
// Dual-list shuttle: property defaults, NOTIFY, selection, move operations,
// theme colours, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdTransfer.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Transfer : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdTransferItem>("md::MdTransferItem");
        qRegisterMetaType<QList<md::MdTransferItem>>("QList<md::MdTransferItem>");
    }

    void defaultState()
    {
        MdTransfer t;
        QCOMPARE(t.sourceTitle(), QStringLiteral("Source"));
        QCOMPARE(t.targetTitle(), QStringLiteral("Target"));
        QVERIFY(t.sourceItems().isEmpty());
        QVERIFY(t.targetItems().isEmpty());
        QCOMPARE(t.multiSelect(), true);
    }

    void itemsAndNotify()
    {
        MdTransfer t;
        QSignalSpy sourceSpy(&t, &MdTransfer::sourceItemsChanged);
        QSignalSpy targetSpy(&t, &MdTransfer::targetItemsChanged);
        QSignalSpy selSpy(&t, &MdTransfer::selectionChanged);

        t.setSourceItems(QList<MdTransferItem>()
                         << MdTransferItem(QStringLiteral("a"), QStringLiteral("Alpha"))
                         << MdTransferItem(QStringLiteral("b"), QStringLiteral("Beta")));
        QCOMPARE(sourceSpy.count(), 1);
        QCOMPARE(t.sourceItems().size(), 2);

        t.selectSourceRow(0);
        QCOMPARE(selSpy.count(), 1);
        QCOMPARE(t.selectedSourceRows(), QList<int>() << 0);

        QSignalSpy movedSpy(&t, &MdTransfer::movedToTarget);
        t.moveSelectedToTarget();
        QCOMPARE(movedSpy.count(), 1);
        QCOMPARE(t.sourceItems().size(), 1);
        QCOMPARE(t.targetItems().size(), 1);
        QCOMPARE(t.targetItems().first().id, QStringLiteral("a"));

        t.selectTargetRow(0);
        QSignalSpy backSpy(&t, &MdTransfer::movedToSource);
        t.moveSelectedToSource();
        QCOMPARE(backSpy.count(), 1);
        QCOMPARE(t.sourceItems().size(), 2);
        QVERIFY(t.targetItems().isEmpty());
    }

    void multiSelectToggle()
    {
        MdTransfer t;
        QSignalSpy multiSpy(&t, &MdTransfer::multiSelectChanged);
        t.setSourceItems(QList<MdTransferItem>()
                         << MdTransferItem(QStringLiteral("a"), QStringLiteral("A"))
                         << MdTransferItem(QStringLiteral("b"), QStringLiteral("B")));
        t.selectSourceRow(0);
        t.selectSourceRow(1);
        QCOMPARE(t.selectedSourceRows().size(), 2);

        t.setMultiSelect(false);
        QCOMPARE(multiSpy.count(), 1);
        QCOMPARE(t.selectedSourceRows().size(), 1);
    }

    void themeColours()
    {
        MdTransfer t;
        QCOMPARE(t.resolvedPanelColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainer));
        QCOMPARE(t.resolvedSelectedBackground(),
                 MdTheme::instance().color(ColorRole::SecondaryContainer));
        QCOMPARE(t.resolvedSelectedContent(),
                 MdTheme::instance().color(ColorRole::OnSecondaryContainer));
    }

    void nonBlankRender()
    {
        MdTransfer t;
        t.setSourceItems(QList<MdTransferItem>()
                         << MdTransferItem(QStringLiteral("a"), QStringLiteral("Alpha")));
        t.setTargetItems(QList<MdTransferItem>()
                         << MdTransferItem(QStringLiteral("b"), QStringLiteral("Beta")));
        t.resize(520, 240);
        QImage image(t.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            t.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdTransfer painted nothing");
    }
};

QTEST_MAIN(TestMd3Transfer)
#include "TestMd3Transfer.moc"
