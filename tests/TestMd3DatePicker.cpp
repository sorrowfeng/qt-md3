// TestMd3DatePicker — the Date pickers family: MdDatePicker + tokens + style.
//
// Pins the export's metric rows (the 360×524 container, the 120 px header,
// the 40 px date cells, the 72×36 year cells), the date / year colour tables,
// the grid maths (Monday-first, the month's day→slot mapping) and the range
// form's two endpoints.

#include "core/MdDatePickerTokens.h"
#include "core/MdTheme.h"
#include "styles/MdDatePickerStyle.h"
#include "widgets/MdDatePicker.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

QImage render(const MdDatePicker &picker)
{
    QImage image(picker.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    const_cast<MdDatePicker &>(picker).render(&painter);
    return image;
}

bool hasInk(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3DatePicker : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void dateColoursFollowTheExport();
    void geometryFollowsTheTokens();
    void gridMapsDaysToSlots();
    void keyboardStepsTheDate();
    void rangeFormKeepsTwoEnds();
    void faceSwitchesToYears();
    void renderSmoke();
    void metaProperties();
};

void TestMd3DatePicker::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdDatePickerStyle::shared() != nullptr);
}

void TestMd3DatePicker::cleanup()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3DatePicker::tokenTableMatchesTheExport()
{
    const MdDatePickerTokens tokens = MdDatePickerTokens::resolve();

    // md.comp.date-picker-modal.* at 34.0.21.
    QCOMPARE(tokens.containerWidth, 360.0);
    QCOMPARE(tokens.containerHeight, 524.0);
    QCOMPARE(tokens.headerHeight, 120.0);
    QCOMPARE(tokens.dateCellSize, 40.0);
    QCOMPARE(tokens.dateTodayOutlineWidth, 1.0);
    QCOMPARE(tokens.rangeIndicatorSize, 40.0);
    QCOMPARE(tokens.yearWidth, 72.0);
    QCOMPARE(tokens.yearHeight, 36.0);
    QCOMPARE(tokens.containerRadius, 28.0);
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
    QCOMPARE(tokens.headerHeadlineColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.weekdayColor, ColorRole::OnSurface);
    QCOMPARE(tokens.monthSubheadColor, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.rangeIndicatorColor, ColorRole::SecondaryContainer);
}

void TestMd3DatePicker::dateColoursFollowTheExport()
{
    const MdDatePickerTokens tokens = MdDatePickerTokens::resolve();
    const int e = int(MdDatePickerState::Enabled);

    QCOMPARE(tokens.dateSelected[e].role, ColorRole::Primary);
    QCOMPARE(tokens.dateUnselected[e].role, ColorRole::OnSurface);
    QCOMPARE(tokens.yearSelected[e].role, ColorRole::Primary);
    QCOMPARE(tokens.yearUnselected[e].role, ColorRole::OnSurfaceVariant);

    // The export publishes no disabled rows; the accessor falls back to
    // on-surface at 0.38.
    const int d = int(MdDatePickerState::Disabled);
    QCOMPARE(tokens.dateSelected[d].role, ColorRole::OnSurface);
    QCOMPARE(tokens.dateSelected[d].opacity, 0.38);
}

void TestMd3DatePicker::geometryFollowsTheTokens()
{
    MdDatePicker picker;
    QCOMPARE(picker.sizeHint().width(), 360);
    QCOMPARE(picker.sizeHint().height(), 524);

    const QRectF cell = picker.dateCellRect(0, 0);
    QCOMPARE(cell.width(), 40.0);

    const QRectF year = picker.yearCellRect(0);
    QCOMPARE(year.width(), 72.0);
    QCOMPARE(year.height(), 36.0);
}

void TestMd3DatePicker::gridMapsDaysToSlots()
{
    // June 2026 starts on a Monday — day 1 sits in slot (0, 0).
    MdDatePicker picker;
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    QCOMPARE(picker.dateAtCell(0, 0), QDate(2026, 6, 1));
    QCOMPARE(picker.dateAtCell(0, 6), QDate(2026, 6, 7));
    QCOMPARE(picker.dateAtCell(3, 2), QDate(2026, 6, 24));
    // Outside the month: invalid.
    QVERIFY(!picker.dateAtCell(5, 6).isValid());
}

void TestMd3DatePicker::keyboardStepsTheDate()
{
    MdDatePicker picker;
    picker.setSelectedDate(QDate(2026, 6, 10));
    picker.show();
    QVERIFY(QTest::qWaitForWindowExposed(&picker));
    picker.setFocus();

    QTest::keyClick(&picker, Qt::Key_Right);
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 11));
    QTest::keyClick(&picker, Qt::Key_Left);
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 10));
    QTest::keyClick(&picker, Qt::Key_Down);
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 17));
}

void TestMd3DatePicker::rangeFormKeepsTwoEnds()
{
    MdDatePicker picker;
    picker.setRange(true);
    picker.setRangeStart(QDate(2026, 6, 10));
    picker.setRangeEnd(QDate(2026, 6, 20));

    QCOMPARE(picker.isRange(), true);
    QCOMPARE(picker.rangeStart(), QDate(2026, 6, 10));
    QCOMPARE(picker.rangeEnd(), QDate(2026, 6, 20));
}

void TestMd3DatePicker::faceSwitchesToYears()
{
    MdDatePicker picker;
    QCOMPARE(picker.face(), MdDatePickerFace::Calendar);
    picker.setFace(MdDatePickerFace::Years);
    QCOMPARE(picker.face(), MdDatePickerFace::Years);
}

void TestMd3DatePicker::renderSmoke()
{
    MdDatePicker picker;
    picker.setSelectedDate(QDate(2026, 6, 15));
    const QImage image = render(picker);
    QVERIFY(hasInk(image));
}

void TestMd3DatePicker::metaProperties()
{
    MdDatePicker picker;
    const QMetaObject *meta = picker.metaObject();
    QVERIFY(meta->indexOfProperty("selectedDate") >= 0);
    QVERIFY(meta->indexOfProperty("displayedMonth") >= 0);
    QVERIFY(meta->indexOfProperty("range") >= 0);
    QVERIFY(meta->indexOfProperty("rangeStart") >= 0);
    QVERIFY(meta->indexOfProperty("rangeEnd") >= 0);
    QVERIFY(meta->indexOfProperty("face") >= 0);

    QSignalSpy spy(&picker, &MdDatePicker::selectedDateChanged);
    picker.setProperty("selectedDate", QDate(2026, 7, 1));
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3DatePicker)
#include "TestMd3DatePicker.moc"
