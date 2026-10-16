// TestMd3MenuBar — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the menu-bar semantics the header documents:
//   * horizontal items on a surface band with label-large labels;
//   * items / currentIndex / openIndex properties with NOTIFY;
//   * MdMenu popup open/close and itemTriggered for menu-less items;
//   * Left/Right keyboard walk;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdMenu.h"
#include "widgets/MdMenuBar.h"

#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QImage renderWidget(QWidget &widget)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32);
}

} // namespace

class TestMd3MenuBar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdMenuBar bar;
        QVERIFY(bar.items().isEmpty());
        QCOMPARE(bar.currentIndex(), -1);
        QCOMPARE(bar.openIndex(), -1);
        QCOMPARE(bar.barHeight(), 0);
        QVERIFY(bar.sizeHint().height() > 0);
    }

    void itemsAndSignals()
    {
        MdMenuBar bar;
        QSignalSpy itemsSpy(&bar, &MdMenuBar::itemsChanged);
        QSignalSpy currentSpy(&bar, &MdMenuBar::currentIndexChanged);

        bar.setItems(QStringList() << QStringLiteral("File") << QStringLiteral("Edit"));
        QCOMPARE(bar.itemCount(), 2);
        QCOMPARE(itemsSpy.count(), 1);

        bar.setCurrentIndex(1);
        QCOMPARE(bar.currentIndex(), 1);
        QCOMPARE(currentSpy.count(), 1);

        bar.setItemText(0, QStringLiteral("File2"));
        QCOMPARE(bar.itemText(0), QStringLiteral("File2"));

        bar.setItemEnabled(1, false);
        QVERIFY(!bar.isItemEnabled(1));
    }

    void itemRects()
    {
        MdMenuBar bar;
        bar.setItems(QStringList() << QStringLiteral("File") << QStringLiteral("Edit"));
        bar.resize(400, 48);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        const QRect first = bar.itemRect(0);
        const QRect second = bar.itemRect(1);
        QVERIFY(!first.isEmpty());
        QVERIFY(!second.isEmpty());
        QCOMPARE(first.left(), 0);
        QCOMPARE(second.left(), first.right() + 1);
        QCOMPARE(bar.itemRect(5), QRect());
    }

    void tokenColors()
    {
        MdMenuBar bar;
        QCOMPARE(bar.resolvedBackgroundColor(), MdTheme::instance().color(ColorRole::Surface));
        QCOMPARE(bar.resolvedContentColor(), MdTheme::instance().color(ColorRole::OnSurface));

        bar.setBackgroundColor(QColor(1, 2, 3));
        QCOMPARE(bar.resolvedBackgroundColor(), QColor(1, 2, 3));
    }

    void itemWithoutMenuTriggers()
    {
        MdMenuBar bar;
        bar.addItem(QStringLiteral("Help"));
        bar.resize(200, 48);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        QSignalSpy spy(&bar, &MdMenuBar::itemTriggered);
        bar.openItem(0);
        QCOMPARE(spy.count(), 1);
        QCOMPARE(spy.at(0).at(0).toInt(), 0);
    }

    void menuOpensAndCloses()
    {
        MdMenuBar bar;
        const int index = bar.addItem(QStringLiteral("File"));
        auto *menu = new MdMenu(&bar);
        menu->addItem(QStringLiteral("Open"));
        bar.setMenu(index, menu);

        QSignalSpy openedSpy(&bar, &MdMenuBar::menuOpened);
        QSignalSpy openIndexSpy(&bar, &MdMenuBar::openIndexChanged);
        bar.openItem(index);
        QCOMPARE(bar.openIndex(), index);
        QCOMPARE(openedSpy.count(), 1);
        QVERIFY(openIndexSpy.count() >= 1);

        QSignalSpy closedSpy(&bar, &MdMenuBar::menuClosed);
        bar.closeMenu();
        QCOMPARE(bar.openIndex(), -1);
        QCOMPARE(closedSpy.count(), 1);
    }

    void keyboardWalk()
    {
        MdMenuBar bar;
        bar.setItems(QStringList() << QStringLiteral("File") << QStringLiteral("Edit")
                                   << QStringLiteral("View"));
        bar.resize(300, 48);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));
        bar.setFocus();

        QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        QApplication::sendEvent(&bar, &right);
        QVERIFY(bar.currentIndex() == 0 || bar.currentIndex() == 1);

        QKeyEvent left(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
        QApplication::sendEvent(&bar, &left);
        QKeyEvent right2(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        QApplication::sendEvent(&bar, &right2);
        QVERIFY(bar.currentIndex() >= 0 && bar.currentIndex() < 3);
    }

    void nonBlankRender()
    {
        MdMenuBar bar;
        bar.setItems(QStringList() << QStringLiteral("File") << QStringLiteral("Edit")
                                   << QStringLiteral("View") << QStringLiteral("Help"));
        bar.resize(480, 48);
        bar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&bar));

        const QImage image = renderWidget(bar);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdMenuBar must paint its surface and labels");
    }
};

QTEST_MAIN(TestMd3MenuBar)
#include "TestMd3MenuBar.moc"
