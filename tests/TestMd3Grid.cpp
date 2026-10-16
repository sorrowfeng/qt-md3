// TestMd3Grid — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the responsive grid:
//   * the published window-size-class breakpoints;
//   * per-class column / gutter / margin defaults;
//   * column, gutter and margin properties with NOTIFY;
//   * row-major child placement with spans, and RTL mirroring.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdGrid.h"

#include <QtGui/QImage>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>

using namespace md;

namespace {

QWidget *makeBox(const QSize &size, QWidget *parent)
{
    auto *box = new QWidget(parent);
    // Minimum, not fixed: the grid grows children to their track width.
    box->setMinimumSize(size);
    return box;
}

} // namespace

class TestMd3Grid : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void sizeClassBreakpoints()
    {
        QCOMPARE(MdGrid::sizeClassForWidth(0), MdWindowSizeClass::Compact);
        QCOMPARE(MdGrid::sizeClassForWidth(599), MdWindowSizeClass::Compact);
        QCOMPARE(MdGrid::sizeClassForWidth(600), MdWindowSizeClass::Medium);
        QCOMPARE(MdGrid::sizeClassForWidth(839), MdWindowSizeClass::Medium);
        QCOMPARE(MdGrid::sizeClassForWidth(840), MdWindowSizeClass::Expanded);
        QCOMPARE(MdGrid::sizeClassForWidth(1199), MdWindowSizeClass::Expanded);
        QCOMPARE(MdGrid::sizeClassForWidth(1200), MdWindowSizeClass::Large);
        QCOMPARE(MdGrid::sizeClassForWidth(1599), MdWindowSizeClass::Large);
        QCOMPARE(MdGrid::sizeClassForWidth(1600), MdWindowSizeClass::ExtraLarge);
        QCOMPARE(MdGrid::sizeClassForWidth(2000), MdWindowSizeClass::ExtraLarge);
    }

    void classSpecDefaults()
    {
        const MdGridClassSpec compact = MdGrid::specFor(MdWindowSizeClass::Compact);
        QCOMPARE(compact.columns, 4);
        QCOMPARE(compact.gutter, 16.0);
        QCOMPARE(compact.margin, 16.0);

        const MdGridClassSpec medium = MdGrid::specFor(MdWindowSizeClass::Medium);
        QCOMPARE(medium.columns, 8);
        QCOMPARE(medium.gutter, 24.0);
        QCOMPARE(medium.margin, 24.0);

        const MdGridClassSpec expanded = MdGrid::specFor(MdWindowSizeClass::Expanded);
        QCOMPARE(expanded.columns, 12);
        QCOMPARE(expanded.gutter, 24.0);
        QCOMPARE(expanded.margin, 24.0);
    }

    void defaultState()
    {
        MdGrid grid;
        grid.resize(400, 200);
        grid.show();
        QVERIFY(QTest::qWaitForWindowExposed(&grid));
        QCOMPARE(grid.sizeClass(), MdWindowSizeClass::Count); // auto
        QCOMPARE(grid.effectiveSizeClass(), MdWindowSizeClass::Compact);
        QCOMPARE(grid.columns(), 4);
        QCOMPARE(grid.gutter(), 16.0);
        QCOMPARE(grid.margin(), 16.0);
    }

    void propertiesNotify()
    {
        MdGrid grid;
        QSignalSpy classSpy(&grid, &MdGrid::sizeClassChanged);
        QSignalSpy effectiveSpy(&grid, &MdGrid::effectiveSizeClassChanged);
        QSignalSpy colSpy(&grid, &MdGrid::columnsChanged);
        QSignalSpy gutterSpy(&grid, &MdGrid::gutterChanged);
        QSignalSpy marginSpy(&grid, &MdGrid::marginChanged);

        grid.setSizeClass(MdWindowSizeClass::Expanded);
        QCOMPARE(classSpy.count(), 1);
        QCOMPARE(effectiveSpy.count(), 1);
        QCOMPARE(grid.columns(), 12);

        grid.setColumns(6);
        QCOMPARE(colSpy.count(), 1);
        QCOMPARE(grid.columns(), 6);

        grid.setGutter(12.0);
        QCOMPARE(gutterSpy.count(), 1);
        QCOMPARE(grid.gutter(), 12.0);

        grid.setMargin(8.0);
        QCOMPARE(marginSpy.count(), 1);
        QCOMPARE(grid.margin(), 8.0);
    }

    void tracksChildrenWithSpans()
    {
        MdGrid grid;
        grid.setSizeClass(MdWindowSizeClass::Compact);
        grid.setColumns(4);
        grid.setMargin(0.0);
        grid.setGutter(0.0);
        grid.resize(400, 200);

        auto *a = makeBox(QSize(10, 40), &grid);
        auto *b = makeBox(QSize(10, 40), &grid);
        auto *c = makeBox(QSize(10, 40), &grid);
        grid.setItemSpan(c, 2);

        grid.show();
        QVERIFY(QTest::qWaitForWindowExposed(&grid));
        grid.resize(400, 201);
        grid.resize(400, 200);
        QCoreApplication::processEvents();

        // 4 tracks over 400 px = 100 px each. a and b take tracks 0 and 1;
        // c spans tracks 2-3 on the first row.
        QCOMPARE(a->x(), 0);
        QCOMPARE(a->width(), 100);
        QCOMPARE(b->x(), 100);
        QCOMPARE(c->x(), 200);
        QCOMPARE(c->width(), 200);
        QCOMPARE(a->y(), 0);
        QCOMPARE(b->y(), 0);
        QCOMPARE(c->y(), 0);
    }

    void wrapsToNextRow()
    {
        MdGrid grid;
        grid.setSizeClass(MdWindowSizeClass::Compact);
        grid.setColumns(2);
        grid.setMargin(0.0);
        grid.setGutter(0.0);
        grid.resize(200, 200);

        auto *a = makeBox(QSize(10, 40), &grid);
        auto *b = makeBox(QSize(10, 40), &grid);
        auto *c = makeBox(QSize(10, 40), &grid);
        grid.show();
        QVERIFY(QTest::qWaitForWindowExposed(&grid));
        grid.resize(200, 201);
        grid.resize(200, 200);
        QCoreApplication::processEvents();

        QCOMPARE(a->x(), 0);
        QCOMPARE(b->x(), 100);
        QCOMPARE(c->x(), 0);
        QVERIFY(c->y() > 0);
    }

    void rtlMirrorsTracks()
    {
        MdGrid grid;
        grid.setLayoutDirection(Qt::RightToLeft);
        grid.setSizeClass(MdWindowSizeClass::Compact);
        grid.setColumns(2);
        grid.setMargin(0.0);
        grid.setGutter(0.0);
        grid.resize(200, 100);

        auto *a = makeBox(QSize(10, 40), &grid);
        auto *b = makeBox(QSize(10, 40), &grid);
        grid.show();
        QVERIFY(QTest::qWaitForWindowExposed(&grid));
        grid.resize(200, 101);
        grid.resize(200, 100);
        QCoreApplication::processEvents();

        // In RTL the first track sits at the right edge.
        QVERIFY(a->x() > b->x());
    }

    void columnWidthTracksMetrics()
    {
        MdGrid grid;
        grid.setSizeClass(MdWindowSizeClass::Compact);
        grid.setColumns(4);
        grid.setMargin(16.0);
        grid.setGutter(16.0);
        grid.resize(400, 100);
        // inner = 400 - 32 = 368; gutters = 3*16 = 48; track = 320/4 = 80.
        QCOMPARE(grid.columnWidth(), 80.0);
    }
};

QTEST_MAIN(TestMd3Grid)
#include "TestMd3Grid.moc"
