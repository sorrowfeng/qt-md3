// TestMd3BackTop — Stage 2 Qt extension (Appendix D, P3).
//
// Small FAB back-to-top: property defaults, NOTIFY, scroll visibility rule,
// scrollToTop, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTypes.h"
#include "widgets/MdBackTop.h"
#include "widgets/MdFab.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QWidget>

using namespace md;

class TestMd3BackTop : public QObject
{
    Q_OBJECT

private slots:
    void defaultState()
    {
        MdBackTop b;
        QCOMPARE(b.visibilityThreshold(), 320);
        QCOMPARE(b.iconName(), QStringLiteral("arrow_upward"));
        QCOMPARE(b.isButtonVisible(), false);
        QVERIFY(b.fab() != nullptr);
    }

    void propertiesNotify()
    {
        MdBackTop b;
        QSignalSpy thresholdSpy(&b, &MdBackTop::visibilityThresholdChanged);
        QSignalSpy iconSpy(&b, &MdBackTop::iconNameChanged);
        QSignalSpy visibleSpy(&b, &MdBackTop::buttonVisibleChanged);
        QSignalSpy areaSpy(&b, &MdBackTop::scrollAreaChanged);

        b.setVisibilityThreshold(100);
        QCOMPARE(thresholdSpy.count(), 1);
        b.setIconName(QStringLiteral("keyboard_arrow_up"));
        QCOMPARE(iconSpy.count(), 1);
        QVERIFY(iconSpy.count() >= 1);

        QScrollArea area;
        auto *content = new QWidget;
        content->setMinimumSize(100, 2000);
        area.setWidget(content);
        area.resize(200, 150);
        b.setScrollArea(&area);
        QCOMPARE(areaSpy.count(), 1);
    }

    void scrollVisibility()
    {
        MdBackTop b;
        b.setVisibilityThreshold(100);
        QScrollArea area;
        auto *content = new QWidget;
        content->setMinimumSize(100, 2000);
        area.setWidget(content);
        area.resize(200, 150);
        area.show();
        b.setScrollArea(&area);
        QSignalSpy visibleSpy(&b, &MdBackTop::buttonVisibleChanged);

        area.verticalScrollBar()->setValue(0);
        b.refresh();
        QCOMPARE(b.isButtonVisible(), false);

        area.verticalScrollBar()->setValue(150);
        b.refresh();
        QCOMPARE(b.isButtonVisible(), true);
        QVERIFY(visibleSpy.count() >= 1);

        area.verticalScrollBar()->setValue(0);
        b.refresh();
        QCOMPARE(b.isButtonVisible(), false);
    }

    void scrollToTopEmitsClicked()
    {
        MdBackTop b;
        b.setVisibilityThreshold(50);
        QScrollArea area;
        auto *content = new QWidget;
        content->setMinimumSize(100, 2000);
        area.setWidget(content);
        area.resize(200, 150);
        area.show();
        b.setScrollArea(&area);
        area.verticalScrollBar()->setValue(200);

        QSignalSpy clickedSpy(&b, &MdBackTop::clicked);
        b.scrollToTop();
        QCOMPARE(clickedSpy.count(), 1);
        QCOMPARE(area.verticalScrollBar()->value(), 0);
    }

    void fabIsSmallSurface()
    {
        MdBackTop b;
        QCOMPARE(b.fab()->fabSize(), FabSize::Small);
    }

    void nonBlankRender()
    {
        MdBackTop b;
        b.resize(40, 40);
        // The FAB child paints; force it visible for the smoke check.
        b.fab()->show();
        QImage image(b.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            b.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdBackTop painted nothing");
    }
};

QTEST_MAIN(TestMd3BackTop)
#include "TestMd3BackTop.moc"
