// TestMd3Pagination — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the pagination semantics the header documents:
//   * page count / current page (1-based) with NOTIFY;
//   * previous / next icon buttons and Text/Filled page buttons;
//   * Left/Right/Home/End keys and wrap;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdPagination.h"

#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>

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

class TestMd3Pagination : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdPagination pager;
        QCOMPARE(pager.pageCount(), 1);
        QCOMPARE(pager.currentPage(), 1);
        QCOMPARE(pager.visiblePages(), 5);
        QVERIFY(!pager.isShowFirstLast());
        QVERIFY(!pager.isWrap());
    }

    void pageProperties()
    {
        MdPagination pager;
        QSignalSpy countSpy(&pager, &MdPagination::pageCountChanged);
        QSignalSpy pageSpy(&pager, &MdPagination::currentPageChanged);
        QSignalSpy selectedSpy(&pager, &MdPagination::pageSelected);

        pager.setPageCount(10);
        QCOMPARE(pager.pageCount(), 10);
        QCOMPARE(countSpy.count(), 1);

        pager.setCurrentPage(3);
        QCOMPARE(pager.currentPage(), 3);
        QCOMPARE(pageSpy.count(), 1);
        QCOMPARE(selectedSpy.count(), 1);

        // Clamped into [1, pageCount].
        pager.setCurrentPage(99);
        QCOMPARE(pager.currentPage(), 10);
        pager.setCurrentPage(0);
        QCOMPARE(pager.currentPage(), 1);
    }

    void navigation()
    {
        MdPagination pager;
        pager.setPageCount(5);
        pager.setCurrentPage(3);

        pager.previousPage();
        QCOMPARE(pager.currentPage(), 2);
        pager.nextPage();
        QCOMPARE(pager.currentPage(), 3);
        pager.firstPage();
        QCOMPARE(pager.currentPage(), 1);
        pager.lastPage();
        QCOMPARE(pager.currentPage(), 5);

        // No wrap: previous on page 1 stays put.
        pager.setCurrentPage(1);
        pager.previousPage();
        QCOMPARE(pager.currentPage(), 1);

        pager.setWrap(true);
        pager.previousPage();
        QCOMPARE(pager.currentPage(), 5);
    }

    void keyboard()
    {
        MdPagination pager;
        pager.setPageCount(8);
        pager.setCurrentPage(4);
        pager.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pager));

        QKeyEvent left(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
        QApplication::sendEvent(&pager, &left);
        QCOMPARE(pager.currentPage(), 3);

        QKeyEvent right(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
        QApplication::sendEvent(&pager, &right);
        QCOMPARE(pager.currentPage(), 4);

        QKeyEvent home(QEvent::KeyPress, Qt::Key_Home, Qt::NoModifier);
        QApplication::sendEvent(&pager, &home);
        QCOMPARE(pager.currentPage(), 1);

        QKeyEvent end(QEvent::KeyPress, Qt::Key_End, Qt::NoModifier);
        QApplication::sendEvent(&pager, &end);
        QCOMPARE(pager.currentPage(), 8);
    }

    void pageButtonsUpdateCurrent()
    {
        MdPagination pager;
        pager.setPageCount(4);
        pager.setCurrentPage(1);
        pager.resize(360, 48);
        pager.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pager));

        const QList<QPushButton *> buttons =
            pager.findChildren<QPushButton *>(QStringLiteral("md-pagination-page"));
        QVERIFY(buttons.size() >= 2);
        QSignalSpy spy(&pager, &MdPagination::pageSelected);
        buttons.last()->click();
        QCOMPARE(pager.currentPage(), 4);
        QVERIFY(spy.count() >= 1);
    }

    void showFirstLast()
    {
        MdPagination pager;
        auto *first = pager.findChild<QPushButton *>(QStringLiteral("md-pagination-first"));
        auto *last = pager.findChild<QPushButton *>(QStringLiteral("md-pagination-last"));
        QVERIFY(first != nullptr);
        QVERIFY(last != nullptr);
        QVERIFY(first->isHidden());
        QVERIFY(last->isHidden());

        pager.setShowFirstLast(true);
        QVERIFY(!first->isHidden());
        QVERIFY(!last->isHidden());

        pager.setPageCount(3);
        pager.setCurrentPage(3);
        QVERIFY(first->isEnabled());
        QVERIFY(!last->isEnabled());
    }

    void nonBlankRender()
    {
        MdPagination pager;
        pager.setPageCount(7);
        pager.setCurrentPage(3);
        pager.setShowFirstLast(true);
        pager.resize(420, 48);
        pager.show();
        QVERIFY(QTest::qWaitForWindowExposed(&pager));

        const QImage image = renderWidget(pager);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdPagination must paint its composed buttons");
    }
};

QTEST_MAIN(TestMd3Pagination)
#include "TestMd3Pagination.moc"
