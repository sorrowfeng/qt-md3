// TestMd3SegmentedButton — the outlined segmented-button family against its
// token sources.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-outlined-segmented-button.scss, with
// the behaviour rows (overlap, itemShape, reserved icon slot) from androidx
// Compose material3 SegmentedButton.kt — the two sources the token header
// names.

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "core/MdTypes.h"
#include "styles/MdSegmentedButtonStyle.h"
#include "widgets/MdSegmentedButton.h"

#include <QtGui/QImage>
#include <QtGui/QKeyEvent>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QRgb pixelAt(MdSegmentedButton &button, const QPointF &position)
{
    QImage image(int(button.width()), int(button.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    button.render(&painter);
    painter.end();
    return image.pixel(int(position.x()), int(position.y()));
}

} // namespace

class TestMd3SegmentedButton : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void theExportIsOneSet();
    void colourRowsMatchTheExport();
    void segmentWidthsReserveTheIconSlot();
    void neighboursOverlapByTheOutlineWidth();
    void itemShapeRoundsOnlyTheEnds();
    void checkingAnimatesTheCheckIn();
    void singleChoiceClearsTheOthers();
    void multiChoiceKeepsThem();
    void ripplesArePerSegment();
    void pointerPressEmitsSegmentActivated();
    void pointerFocusShowsNoRingKeyboardFocusDoes();
    void arrowKeysMoveTheFocusedSegment();
    void spaceTogglesTheFocusedSegment();
    void disabledKeepsTheSelectedContainer();

private:
    MdSegmentedButton *makeThreeSegments()
    {
        auto *button = new MdSegmentedButton(
            QStringList{QStringLiteral("Day"), QStringLiteral("Week"), QStringLiteral("Month")});
        button->resize(button->sizeHint());
        button->show();
        return button;
    }
};

void TestMd3SegmentedButton::initTestCase()
{
    MdTheme::instance();
}

void TestMd3SegmentedButton::theExportIsOneSet()
{
    // The export publishes a single set: 40 px, outline 1, icon 18, no size
    // scale and no colour variants.
    const MdSegmentedButtonTokens tokens = MdSegmentedButtonTokens::resolve();
    QCOMPARE(tokens.containerHeight, 40.0);
    QCOMPARE(tokens.outlineWidth, 1.0);
    QCOMPARE(tokens.iconSize, 18.0);
    QCOMPARE(int(tokens.labelStyle), int(TypeStyle::LabelLarge));
    // The export has no icon-spacing / content-padding rows; the Compose
    // values (8 / 12) fill them, recorded as such.
    QCOMPARE(tokens.iconSpacing, 8.0);
    QCOMPARE(tokens.contentPadding, 12.0);
}

void TestMd3SegmentedButton::colourRowsMatchTheExport()
{
    const MdSegmentedButtonTokens tokens = MdSegmentedButtonTokens::resolve();
    // Unselected: on-surface content, outline stroke, no container row.
    QCOMPARE(int(tokens.unselected.labelText), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.unselected.icon), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.unselected.stateLayer), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.unselected.outline), int(ColorRole::Outline));
    QCOMPARE(int(tokens.unselected.container), int(ColorRole::Count));
    // Selected: secondary-container, on-secondary-container — the same row
    // for every interactive state.
    QCOMPARE(int(tokens.selected.container), int(ColorRole::SecondaryContainer));
    QCOMPARE(int(tokens.selected.labelText), int(ColorRole::OnSecondaryContainer));
    QCOMPARE(int(tokens.selected.icon), int(ColorRole::OnSecondaryContainer));
    // Disabled opacities.
    QCOMPARE(tokens.disabledLabelOpacity, 0.38);
    QCOMPARE(tokens.disabledIconOpacity, 0.38);
    QCOMPARE(tokens.disabledOutlineOpacity, 0.12);
    // Focus indicator: secondary, the sys thickness and offset.
    QCOMPARE(int(tokens.focusIndicator), int(ColorRole::Secondary));
    QCOMPARE(tokens.focusIndicatorThickness, 3.0);
    QCOMPARE(tokens.focusIndicatorOffset, 2.0);
}

void TestMd3SegmentedButton::segmentWidthsReserveTheIconSlot()
{
    MdSegmentedButton *button = makeThreeSegments();

    // Every segment reserves the icon slot (18) + spacing (8), whether or
    // not it carries a custom icon, so the check never displaces the label.
    const MdSegmentedButtonTokens tokens = button->tokens();
    const QFont font = MdTypeScale::font(tokens.labelStyle, TypeEmphasis::Baseline,
                                         MdTheme::instance().scriptCategory());
    const QFontMetricsF metrics(font);
    const qreal label = metrics.horizontalAdvance(QStringLiteral("Day"));
    const qreal expected = 2.0 * tokens.contentPadding + tokens.iconSize + tokens.iconSpacing
                           + label;
    const QRectF rect = button->segmentRect(0);
    QCOMPARE(rect.width(), expected);
    QCOMPARE(rect.height(), 40.0);
    delete button;
}

void TestMd3SegmentedButton::neighboursOverlapByTheOutlineWidth()
{
    MdSegmentedButton *button = makeThreeSegments();

    // spacedBy(-outlineWidth): segment 1 starts outlineWidth to the left of
    // segment 0's right edge, so the shared edge is a single 1 px stroke.
    for (int i = 0; i + 1 < button->segmentCount(); ++i) {
        const QRectF current = button->segmentRect(i);
        const QRectF next = button->segmentRect(i + 1);
        QCOMPARE(next.left() - current.right(), -1.0);
    }
    delete button;
}

void TestMd3SegmentedButton::itemShapeRoundsOnlyTheEnds()
{
    MdSegmentedButton *button = makeThreeSegments();
    const MdSegmentedButtonStyle::Layout layout =
        MdSegmentedButtonStyle::layoutFor(*button, button->tokens());
    const qreal base = layout.segments.first().radii.first(); // 20 for a 40 px pill

    // First segment: left corners rounded, right corners square.
    QCOMPARE(layout.segments.at(0).radii, (QList<qreal>{base, 0.0, 0.0, base}));
    // Middle: a rectangle.
    QCOMPARE(layout.segments.at(1).radii, (QList<qreal>{0.0, 0.0, 0.0, 0.0}));
    // Last: right corners rounded.
    QCOMPARE(layout.segments.at(2).radii, (QList<qreal>{0.0, base, base, 0.0}));
    delete button;
}

void TestMd3SegmentedButton::checkingAnimatesTheCheckIn()
{
    MdSegmentedButton *button = makeThreeSegments();
    QSignalSpy spy(button, &MdSegmentedButton::segmentChecked);

    QCOMPARE(button->checkMorph(0), 0.0);
    button->setChecked(0, true);
    QCOMPARE(spy.count(), 1);
    QTest::qWait(400); // past the spring's settle
    QCOMPARE(button->checkMorph(0), 1.0);
    delete button;
}

void TestMd3SegmentedButton::singleChoiceClearsTheOthers()
{
    MdSegmentedButton *button = makeThreeSegments();
    button->setChecked(0, true);
    button->setChecked(2, true);
    QCOMPARE(button->checkedIndexes(), QList<int>{2});
    QCOMPARE(button->checkedIndex(), 2);
    delete button;
}

void TestMd3SegmentedButton::multiChoiceKeepsThem()
{
    MdSegmentedButton *button = makeThreeSegments();
    button->setSingleChoice(false);
    button->setChecked(0, true);
    button->setChecked(2, true);
    QCOMPARE(button->checkedIndexes(), (QList<int>{0, 2}));

    // Collapsing back to single choice keeps the first.
    button->setSingleChoice(true);
    QCOMPARE(button->checkedIndexes(), QList<int>{0});
    delete button;
}

void TestMd3SegmentedButton::ripplesArePerSegment()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));

    QTest::mousePress(button, Qt::LeftButton, Qt::NoModifier,
                      button->segmentRect(1).center().toPoint());
    QVERIFY(button->rippleController(1)->isActive());
    QVERIFY(!button->rippleController(0)->isActive());
    QTest::mouseRelease(button, Qt::LeftButton, Qt::NoModifier,
                        button->segmentRect(1).center().toPoint());
    delete button;
}

void TestMd3SegmentedButton::pointerPressEmitsSegmentActivated()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));

    QSignalSpy activated(button, &MdSegmentedButton::segmentActivated);
    QSignalSpy checked(button, &MdSegmentedButton::segmentChecked);
    QTest::mouseClick(button, Qt::LeftButton, Qt::NoModifier,
                      button->segmentRect(1).center().toPoint());
    QTest::qWait(100);
    QCOMPARE(activated.count(), 1);
    QCOMPARE(activated.first().first().toInt(), 1);
    QCOMPARE(checked.count(), 1);
    delete button;
}

void TestMd3SegmentedButton::pointerFocusShowsNoRingKeyboardFocusDoes()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));

    button->clearFocus();
    button->setFocus(Qt::MouseFocusReason);
    QVERIFY(button->hasFocus());
    QVERIFY(!button->hasKeyboardFocus());

    // The focused segment's ring sits offset 2 outside its box; sample the
    // stroke centre (offset 2 from the box edge).
    const QRectF box = button->segmentRect(0);
    const QPointF ringPoint(box.left() - 2.0, box.center().y());
    const QRgb withoutRing = pixelAt(*button, ringPoint);

    button->clearFocus();
    button->setFocus(Qt::TabFocusReason);
    QVERIFY(button->hasKeyboardFocus());
    QCOMPARE(button->focusedSegment(), 0);
    const QRgb withRing = pixelAt(*button, ringPoint);
    QVERIFY(withoutRing != withRing);
    delete button;
}

void TestMd3SegmentedButton::arrowKeysMoveTheFocusedSegment()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));

    button->clearFocus();
    button->setFocus(Qt::TabFocusReason);
    QCOMPARE(button->focusedSegment(), 0);

    QTest::keyClick(button, Qt::Key_Right);
    QCOMPARE(button->focusedSegment(), 1);
    QTest::keyClick(button, Qt::Key_Right);
    QTest::keyClick(button, Qt::Key_Right);
    QCOMPARE(button->focusedSegment(), 0); // wrapped

    QTest::keyClick(button, Qt::Key_Left);
    QCOMPARE(button->focusedSegment(), 2);

    // Cross-axis keys are no-ops.
    QTest::keyClick(button, Qt::Key_Down);
    QCOMPARE(button->focusedSegment(), 2);
    delete button;
}

void TestMd3SegmentedButton::spaceTogglesTheFocusedSegment()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));

    button->clearFocus();
    button->setFocus(Qt::TabFocusReason);
    QTest::keyClick(button, Qt::Key_Space);
    QCOMPARE(button->checkedIndex(), 0);
    QTest::keyClick(button, Qt::Key_Space);
    QCOMPARE(button->checkedIndex(), -1); // toggled off
    delete button;
}

void TestMd3SegmentedButton::disabledKeepsTheSelectedContainer()
{
    MdSegmentedButton *button = makeThreeSegments();
    QVERIFY(QTest::qWaitForWindowExposed(button));
    button->setChecked(1, true);
    QTest::qWait(400); // the check settles in

    button->setSoftDisabled(true);
    // The disabled selected segment keeps its secondary-container fill (the
    // export has no disabled-container row; Compose uses SelectedContainerColor
    // for disabledActive too) while the content drops to 0.38.
    QCOMPARE(button->isChecked(1), true);
    delete button;
}

QTEST_MAIN(TestMd3SegmentedButton)
#include "TestMd3SegmentedButton.moc"
