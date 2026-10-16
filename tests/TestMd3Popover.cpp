// TestMd3Popover — Stage 2 Qt extension (Appendix D, P2).
//
// The Menu-semantics anchored surface, plus the MdPopconfirm action variant:
// property defaults, NOTIFY, open/close, confirm/cancel, non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdButton.h"
#include "widgets/MdPopconfirm.h"
#include "widgets/MdPopover.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace md;

class TestMd3Popover : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<ShapeCorner>("md::ShapeCorner");
    }

    void defaultState()
    {
        MdPopover pop;
        QCOMPARE(pop.title(), QString());
        QVERIFY(!pop.isOpen());
        QCOMPARE(pop.shape(), ShapeCorner::ExtraSmall);
        QCOMPARE(pop.openProgress(), 1.0);
    }

    void propertiesNotify()
    {
        MdPopover pop;
        QSignalSpy titleSpy(&pop, &MdPopover::titleChanged);
        QSignalSpy shapeSpy(&pop, &MdPopover::shapeChanged);
        QSignalSpy openSpy(&pop, &MdPopover::openChanged);

        pop.setTitle(QStringLiteral("Details"));
        QCOMPARE(titleSpy.count(), 1);
        pop.setShape(ShapeCorner::Medium);
        QCOMPARE(shapeSpy.count(), 1);

        pop.setOpen(true);
        QCOMPARE(openSpy.count(), 1);
        QVERIFY(pop.isOpen());
        pop.setOpen(true);
        QCOMPARE(openSpy.count(), 1);
    }

    void surfaceColours()
    {
        MdPopover pop;
        QCOMPARE(pop.resolvedSurfaceColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainer));
        QCOMPARE(pop.resolvedTitleColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
    }

    void contentWidgetIsHosted()
    {
        MdPopover pop;
        auto *inner = new QWidget;
        pop.setContentWidget(inner);
        QCOMPARE(pop.contentWidget(), inner);
        QCOMPARE(inner->parent(), &pop);
    }

    void popconfirmDefaults()
    {
        MdPopconfirm confirm;
        QCOMPARE(confirm.text(), QString());
        QVERIFY(confirm.confirmButton() != nullptr);
        QVERIFY(confirm.cancelButton() != nullptr);
    }

    void popconfirmConfirmCancel()
    {
        MdPopconfirm confirm;
        QSignalSpy confirmedSpy(&confirm, &MdPopconfirm::confirmed);
        QSignalSpy cancelledSpy(&confirm, &MdPopconfirm::cancelled);
        QSignalSpy textSpy(&confirm, &MdPopconfirm::textChanged);

        confirm.setText(QStringLiteral("Delete this?"));
        QCOMPARE(textSpy.count(), 1);
        QCOMPARE(confirm.text(), QStringLiteral("Delete this?"));

        confirm.confirmButton()->click();
        QCOMPARE(confirmedSpy.count(), 1);
        QCOMPARE(cancelledSpy.count(), 0);
    }

    void nonBlankRender()
    {
        MdPopover pop;
        pop.setTitle(QStringLiteral("Preview"));
        auto *inner = new QWidget;
        inner->setMinimumSize(160, 48);
        pop.setContentWidget(inner);
        pop.resize(220, 120);
        QImage image(pop.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            pop.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdPopover painted nothing");
    }
};

QTEST_MAIN(TestMd3Popover)
#include "TestMd3Popover.moc"
