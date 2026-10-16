// TestMd3Dock — Stage 2 Qt extension (Appendix D, P1).
//
// MdDockWidget + MdDockManager: Side sheet + surface container with
// shape-highlight dock guides. Property defaults, NOTIFY, docking,
// guide visibility, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdDockManager.h"
#include "widgets/MdDockWidget.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace md;

class TestMd3Dock : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdDockArea>("md::MdDockArea");
    }

    void widgetDefaults()
    {
        MdDockWidget dock;
        QCOMPARE(dock.title(), QString());
        QCOMPARE(dock.area(), MdDockArea::None);
        QVERIFY(dock.isClosable());
        QVERIFY(dock.isOpen());
        QCOMPARE(MdDockWidget::headerHeight(), 48);
    }

    void widgetPropertiesNotify()
    {
        MdDockWidget dock;
        QSignalSpy titleSpy(&dock, &MdDockWidget::titleChanged);
        QSignalSpy areaSpy(&dock, &MdDockWidget::areaChanged);
        QSignalSpy closableSpy(&dock, &MdDockWidget::closableChanged);
        QSignalSpy openSpy(&dock, &MdDockWidget::openChanged);

        dock.setTitle(QStringLiteral("Explorer"));
        QCOMPARE(titleSpy.count(), 1);
        dock.setArea(MdDockArea::Left);
        QCOMPARE(areaSpy.count(), 1);
        dock.setClosable(false);
        QCOMPARE(closableSpy.count(), 1);
        dock.setOpen(false);
        QCOMPARE(openSpy.count(), 1);
        QVERIFY(!dock.isOpen());

        // Re-setting the same value must not re-notify.
        dock.setTitle(QStringLiteral("Explorer"));
        QCOMPARE(titleSpy.count(), 1);
    }

    void widgetColours()
    {
        MdDockWidget dock;
        QCOMPARE(dock.resolvedSurfaceColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainer));
        QCOMPARE(dock.resolvedTitleColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
        QCOMPARE(dock.resolvedDividerColor(),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
    }

    void widgetContent()
    {
        MdDockWidget dock;
        auto *inner = new QWidget;
        dock.setContentWidget(inner);
        QCOMPARE(dock.contentWidget(), inner);
        QCOMPARE(inner->parent(), &dock);
    }

    void managerDefaults()
    {
        MdDockManager manager;
        QCOMPARE(manager.centerWidget(), nullptr);
        QCOMPARE(manager.leftWidth(), 0);
        QCOMPARE(manager.rightWidth(), 0);
        QCOMPARE(manager.bottomHeight(), 0);
        QVERIFY(!manager.isGuideVisible());
    }

    void managerDocking()
    {
        MdDockManager manager;
        auto *panel = new MdDockWidget(QStringLiteral("Left panel"));
        auto *addedSpy = new QSignalSpy(&manager, &MdDockManager::dockWidgetAdded);
        manager.addDockWidget(panel, MdDockArea::Left);
        QCOMPARE(addedSpy->count(), 1);
        QVERIFY(manager.dockWidgets(MdDockArea::Left).contains(panel));
        QCOMPARE(panel->area(), MdDockArea::Left);
        QVERIFY(manager.leftWidth() > 0);
        delete addedSpy;
    }

    void managerRemove()
    {
        MdDockManager manager;
        auto *panel = new MdDockWidget(QStringLiteral("Right panel"));
        manager.addDockWidget(panel, MdDockArea::Right);
        QSignalSpy removedSpy(&manager, &MdDockManager::dockWidgetRemoved);
        manager.removeDockWidget(panel);
        QCOMPARE(removedSpy.count(), 1);
        QVERIFY(manager.dockWidgets(MdDockArea::Right).isEmpty());
        QCOMPARE(panel->area(), MdDockArea::Floating);
    }

    void managerGuide()
    {
        MdDockManager manager;
        QSignalSpy guideSpy(&manager, &MdDockManager::guideVisibleChanged);
        manager.showGuide(MdDockArea::Bottom);
        QVERIFY(manager.isGuideVisible());
        QCOMPARE(manager.guideArea(), MdDockArea::Bottom);
        QCOMPARE(guideSpy.count(), 1);
        manager.hideGuide();
        QVERIFY(!manager.isGuideVisible());
        QCOMPARE(guideSpy.count(), 2);
    }

    void managerGuideColour()
    {
        MdDockManager manager;
        QCOMPARE(manager.resolvedGuideColor(),
                 MdTheme::instance().color(ColorRole::SecondaryContainer));
    }

    void nonBlankRender()
    {
        MdDockManager manager;
        manager.resize(640, 480);
        auto *center = new QWidget;
        center->setMinimumSize(100, 100);
        manager.setCenterWidget(center);
        auto *panel = new MdDockWidget(QStringLiteral("Dock"));
        manager.addDockWidget(panel, MdDockArea::Left);
        manager.showGuide(MdDockArea::Right);

        QImage image(manager.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            manager.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdDockManager painted nothing");
    }
};

QTEST_MAIN(TestMd3Dock)
#include "TestMd3Dock.moc"
