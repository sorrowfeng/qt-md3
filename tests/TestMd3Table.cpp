// TestMd3Table — Stage 2 Qt extension (Appendix D, P0).
//
// MD3 has no data-table spec; this suite pins the List-row + surface-container
// header design the header documents:
//
//   * setRowCount / setColumnCount / setCellText form the string grid;
//   * the header paints (surface-container, label-large) and toggles;
//   * selection is a row index with NOTIFY, keyboard Up/Down/Home/End;
//   * hover / selected state layers and outline-variant dividers;
//   * alternatingRowColors switches the surface-container-lowest / low
//     ladder;
//   * cell widgets are parented and positioned;
//   * non-blank render smoke.

#include "core/MdListTokens.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdTable.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

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

bool anyPixelPainted(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 8) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3Table : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdTable table;
        QCOMPARE(table.rowCount(), 0);
        QCOMPARE(table.columnCount(), 0);
        QCOMPARE(table.currentRow(), -1);
        QVERIFY(table.showHeader());
        QVERIFY(table.showDividers());
        QVERIFY(!table.alternatingRowColors());
    }

    void gridApi()
    {
        MdTable table;
        QSignalSpy rowSpy(&table, &MdTable::rowCountChanged);
        QSignalSpy colSpy(&table, &MdTable::columnCountChanged);

        table.setRowCount(3);
        table.setColumnCount(2);
        QCOMPARE(table.rowCount(), 3);
        QCOMPARE(table.columnCount(), 2);
        QCOMPARE(rowSpy.count(), 1);
        QCOMPARE(colSpy.count(), 1);

        table.setCellText(0, 0, QStringLiteral("A"));
        table.setCellText(0, 1, QStringLiteral("B"));
        table.setCellText(2, 1, QStringLiteral("Z"));
        QCOMPARE(table.cellText(0, 0), QStringLiteral("A"));
        QCOMPARE(table.cellText(2, 1), QStringLiteral("Z"));
        QCOMPARE(table.cellText(1, 0), QString());

        // Out-of-range writes are ignored.
        table.setCellText(99, 0, QStringLiteral("nope"));
        QCOMPARE(table.cellText(99, 0), QString());
    }

    void headerLabels()
    {
        MdTable table;
        table.setColumnCount(3);
        table.setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Qty"),
                                         QStringLiteral("Price")});
        QCOMPARE(table.horizontalHeaderLabels().size(), 3);
    }

    void selectionAndKeyboard()
    {
        MdTable table;
        table.setRowCount(4);
        table.setColumnCount(1);
        QSignalSpy spy(&table, &MdTable::currentRowChanged);

        table.setCurrentRow(1);
        QCOMPARE(table.currentRow(), 1);
        QCOMPARE(spy.count(), 1);

        table.setFocus(Qt::OtherFocusReason);
        QTest::keyClick(&table, Qt::Key_Down);
        QCOMPARE(table.currentRow(), 2);
        QTest::keyClick(&table, Qt::Key_Up);
        QCOMPARE(table.currentRow(), 1);
        QTest::keyClick(&table, Qt::Key_Home);
        QCOMPARE(table.currentRow(), 0);
        QTest::keyClick(&table, Qt::Key_End);
        QCOMPARE(table.currentRow(), 3);

        QSignalSpy activated(&table, &MdTable::cellActivated);
        QTest::keyClick(&table, Qt::Key_Return);
        QCOMPARE(activated.count(), 1);
        QCOMPARE(activated.takeFirst().at(0).toInt(), 3);
    }

    void appearanceProperties()
    {
        MdTable table;
        QSignalSpy headerSpy(&table, &MdTable::showHeaderChanged);
        QSignalSpy dividerSpy(&table, &MdTable::showDividersChanged);
        QSignalSpy altSpy(&table, &MdTable::alternatingRowColorsChanged);
        QSignalSpy rowHeightSpy(&table, &MdTable::rowHeightChanged);

        table.setShowHeader(false);
        QVERIFY(!table.showHeader());
        QCOMPARE(headerSpy.count(), 1);

        table.setShowDividers(false);
        QVERIFY(!table.showDividers());
        QCOMPARE(dividerSpy.count(), 1);

        table.setAlternatingRowColors(true);
        QVERIFY(table.alternatingRowColors());
        QCOMPARE(altSpy.count(), 1);

        table.setRowHeight(40.0);
        QCOMPARE(table.rowHeight(), 40.0);
        QCOMPARE(rowHeightSpy.count(), 1);

        // 0 falls back to the list one-line height.
        table.setRowHeight(0.0);
        QCOMPARE(table.rowHeight(), MdListTokens::resolve(MdListVariant::Standard).oneLineHeight);
    }

    void cellWidgets()
    {
        MdTable table;
        table.setRowCount(2);
        table.setColumnCount(2);
        table.resize(400, 200);

        auto *label = new QLabel(QStringLiteral("hi"));
        table.setCellWidget(1, 1, label);
        QCOMPARE(table.cellWidget(1, 1), label);
        QCOMPARE(label->parent(), &table);
    }

    void geometry()
    {
        MdTable table;
        table.setRowCount(2);
        table.setColumnCount(2);
        table.resize(400, 200);

        QVERIFY(table.headerRect().height() > 0);
        QCOMPARE(table.bodyRect().top(), table.headerRect().height());
        QCOMPARE(table.rowRect(0).height(), table.rowHeight());
        QVERIFY(table.cellRect(0, 0).width() > 0);
        QVERIFY(table.cellRect(0, 1).left() > table.cellRect(0, 0).left());
    }

    void nonBlankRender()
    {
        MdTable table;
        table.setColumnCount(3);
        table.setHorizontalHeaderLabels({QStringLiteral("Name"), QStringLiteral("Role"),
                                         QStringLiteral("Team")});
        table.setRowCount(3);
        table.setCellText(0, 0, QStringLiteral("Ada"));
        table.setCellText(0, 1, QStringLiteral("Engineer"));
        table.setCellText(0, 2, QStringLiteral("Core"));
        table.setCellText(1, 0, QStringLiteral("Grace"));
        table.setCellText(1, 1, QStringLiteral("Lead"));
        table.setCellText(1, 2, QStringLiteral("Compilers"));
        table.setCellText(2, 0, QStringLiteral("Alan"));
        table.setCellText(2, 1, QStringLiteral("Research"));
        table.setCellText(2, 2, QStringLiteral("Math"));
        table.setAlternatingRowColors(true);
        table.resize(480, 240);
        table.show();
        QVERIFY(QTest::qWaitForWindowExposed(&table));

        const QImage image = renderWidget(table);
        QVERIFY2(anyPixelPainted(image), "MdTable must paint its header and rows");
    }
};

QTEST_MAIN(TestMd3Table)
#include "TestMd3Table.moc"
