// TestMd3DatePickerDocked — the docked date picker face:
// `md.comp.date-picker-docked.*` at export 34.0.21.
//
// Pins the geometry rows the export publishes (the 360×456 container, the
// 64 px header, the 48 px date cells with the 40 px state layer, the 40 px
// menu buttons with 18 px icons, the 48 px dropdown list items), the date /
// menu-button colour tables and the variant switch on `MdDatePicker`.
//
// The export publishes **no focus-indicator rows** for this family. That gap
// is recorded in porting-todo; this suite does not invent them.

#include "core/MdDatePickerDockedTokens.h"
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

class TestMd3DatePickerDocked : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void tokenTableMatchesTheExport();
    void dateColoursFollowTheExport();
    void menuButtonRowsFollowTheExport();
    void geometryFollowsTheTokens();
    void variantSwitchesTheSurface();
    void gridMapsDaysToSlots();
    void keyboardStepsTheDate();
    void renderSmoke();
    void metaProperties();
};

void TestMd3DatePickerDocked::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    QVERIFY(MdDatePickerStyle::shared() != nullptr);
}

void TestMd3DatePickerDocked::cleanup()
{
    // Reset every mutable global a failing QCOMPARE could have left behind.
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3DatePickerDocked::tokenTableMatchesTheExport()
{
    const MdDatePickerDockedTokens tokens = MdDatePickerDockedTokens::resolve();

    // md.comp.date-picker-docked.* at 34.0.21.
    QCOMPARE(tokens.containerWidth, 360.0);
    QCOMPARE(tokens.containerHeight, 456.0);
    QCOMPARE(tokens.headerHeight, 64.0);
    QCOMPARE(tokens.dateCellWidth, 48.0);
    QCOMPARE(tokens.dateCellHeight, 48.0);
    QCOMPARE(tokens.dateStateLayerWidth, 40.0);
    QCOMPARE(tokens.dateStateLayerHeight, 40.0);
    QCOMPARE(tokens.dateTodayOutlineWidth, 1.0);
    QCOMPARE(tokens.dateOutsideMonthLabelOpacity, 0.38);
    QCOMPARE(tokens.menuButtonHeight, 40.0);
    QCOMPARE(tokens.menuButtonIconSize, 18.0);
    QCOMPARE(tokens.menuListItemHeight, 48.0);
    QCOMPARE(tokens.menuListSelectedIconSize, 24.0);
    QCOMPARE(tokens.containerColor, ColorRole::SurfaceContainerHigh);
    QCOMPARE(tokens.containerElevation, ElevationLevel::Level3);
    QCOMPARE(tokens.containerShape, ShapeCorner::Large);
    QCOMPARE(tokens.dateCellShape, ShapeCorner::Full);
    QCOMPARE(tokens.dateStateLayerShape, ShapeCorner::Full);
    QCOMPARE(tokens.menuButtonShape, ShapeCorner::Full);
    QCOMPARE(tokens.dateTodayColor, ColorRole::Primary);
    QCOMPARE(tokens.weekdayColor, ColorRole::OnSurface);
    QCOMPARE(tokens.menuListSelectedContainerColor, ColorRole::SurfaceVariant);
}

void TestMd3DatePickerDocked::dateColoursFollowTheExport()
{
    const MdDatePickerDockedTokens tokens = MdDatePickerDockedTokens::resolve();
    const int e = int(MdDatePickerDockedState::Enabled);
    const int d = int(MdDatePickerDockedState::Disabled);

    QCOMPARE(tokens.dateSelected[e].role, ColorRole::OnPrimary);
    QCOMPARE(tokens.dateToday[e].role, ColorRole::Primary);
    QCOMPARE(tokens.dateUnselected[e].role, ColorRole::OnSurface);

    // The date-cell tables publish no disabled rows; the fallback is
    // on-surface / on-primary / primary at 0.38, the modal set's rule.
    QCOMPARE(tokens.dateUnselected[d].opacity, 0.38);
    QCOMPARE(tokens.dateSelected[d].opacity, 0.38);
}

void TestMd3DatePickerDocked::menuButtonRowsFollowTheExport()
{
    const MdDatePickerDockedTokens tokens = MdDatePickerDockedTokens::resolve();
    const int e = int(MdDatePickerDockedState::Enabled);
    const int d = int(MdDatePickerDockedState::Disabled);

    // Enabled / hover / focus / pressed all read on-surface-variant.
    QCOMPARE(tokens.menuButtonLabel[e].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.menuButtonIcon[e].role, ColorRole::OnSurfaceVariant);

    // Disabled: on-surface at the published 0.38 opacities.
    QCOMPARE(tokens.menuButtonLabel[d].role, ColorRole::OnSurfaceVariant);
    QCOMPARE(tokens.menuButtonLabel[d].opacity, 0.38);
    QCOMPARE(tokens.menuButtonIcon[d].opacity, 0.38);

    // The dropdown's list rows.
    QCOMPARE(tokens.menuListLabel[e].role, ColorRole::OnSurface);
    QCOMPARE(tokens.menuListSelectedIcon[e].role, ColorRole::OnSurface);
    QCOMPARE(tokens.menuListSelectedIcon[int(MdDatePickerDockedState::Hovered)].role,
             ColorRole::OnSurfaceVariant);
}

void TestMd3DatePickerDocked::geometryFollowsTheTokens()
{
    MdDatePicker picker;
    picker.setVariant(MdDatePickerVariant::Docked);
    QCOMPARE(picker.sizeHint().width(), 360);
    QCOMPARE(picker.sizeHint().height(), 456);

    QCOMPARE(picker.headerRect().height(), 64.0);

    const QRectF cell = picker.dateCellRect(0, 0);
    QCOMPARE(cell.width(), 48.0);
    QCOMPARE(cell.height(), 48.0);

    const QRectF pill = picker.dateStateLayerRect(0, 0);
    QCOMPARE(pill.width(), 40.0);
    QCOMPARE(pill.height(), 40.0);
    // The pill is centred in the cell (4 px inset every side).
    QCOMPARE(pill.left() - cell.left(), 4.0);
    QCOMPARE(pill.top() - cell.top(), 4.0);

    const QRectF month = picker.monthMenuButtonRect();
    const QRectF year = picker.yearMenuButtonRect();
    QCOMPARE(month.height(), 40.0);
    QCOMPARE(year.height(), 40.0);
    QVERIFY(month.left() < year.left());
}

void TestMd3DatePickerDocked::variantSwitchesTheSurface()
{
    MdDatePicker picker;
    QCOMPARE(picker.variant(), MdDatePickerVariant::Modal);
    QCOMPARE(picker.sizeHint().height(), 524);

    QSignalSpy spy(&picker, &MdDatePicker::variantChanged);
    picker.setVariant(MdDatePickerVariant::Docked);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(picker.variant(), MdDatePickerVariant::Docked);
    QCOMPARE(picker.sizeHint().height(), 456);
    QCOMPARE(picker.headerRect().height(), 64.0);
}

void TestMd3DatePickerDocked::gridMapsDaysToSlots()
{
    // June 2026 starts on a Monday — day 1 sits in slot (0, 0).
    MdDatePicker picker;
    picker.setVariant(MdDatePickerVariant::Docked);
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    QCOMPARE(picker.dateAtCell(0, 0), QDate(2026, 6, 1));
    QCOMPARE(picker.dateAtCell(0, 6), QDate(2026, 6, 7));
    QCOMPARE(picker.dateAtCell(3, 2), QDate(2026, 6, 24));
    QVERIFY(!picker.dateAtCell(5, 6).isValid());
}

void TestMd3DatePickerDocked::keyboardStepsTheDate()
{
    MdDatePicker picker;
    picker.setVariant(MdDatePickerVariant::Docked);
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    picker.setSelectedDate(QDate(2026, 6, 10));
    picker.show();
    QVERIFY(QTest::qWaitForWindowExposed(&picker));
    picker.setFocus();

    QTest::keyClick(&picker, Qt::Key_Right);
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 11));
    QTest::keyClick(&picker, Qt::Key_Left);
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 10));
    QTest::keyClick(&picker, Qt::Key_PageDown);
    QCOMPARE(picker.displayedMonth().month(), 7);
}

void TestMd3DatePickerDocked::renderSmoke()
{
    MdDatePicker picker;
    picker.setVariant(MdDatePickerVariant::Docked);
    picker.setSelectedDate(QDate(2026, 6, 15));
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    const QImage image = render(picker);
    QVERIFY(hasInk(image));
}

void TestMd3DatePickerDocked::metaProperties()
{
    MdDatePicker picker;
    QVERIFY(picker.metaObject()->indexOfProperty("variant") >= 0);

    QSignalSpy spy(&picker, &MdDatePicker::variantChanged);
    picker.setVariant(MdDatePickerVariant::Docked);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestMd3DatePickerDocked)
#include "TestMd3DatePickerDocked.moc"
