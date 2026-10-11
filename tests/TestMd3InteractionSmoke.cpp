// TestMd3InteractionSmoke — pointer and keyboard interaction across the
// gallery's key controls, instantiated directly (not through gallery pages).
//
// The unit suites pin tokens, geometry and paint. They largely do not drive
// real input, so a control can be visually perfect and still refuse a click.
// This suite is the gate for that: every slot is a user action a gallery
// visitor would try, asserted through QTest mouse/key delivery.

#include "core/MdTheme.h"
#include "widgets/MdButton.h"
#include "widgets/MdCheckBox.h"
#include "widgets/MdChip.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdSegmentedButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"
#include "widgets/MdSwitch.h"

#include <QtGui/QMouseEvent>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

void showAndExpose(QWidget &widget)
{
    widget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&widget));
}

/// Send a genuine press / move / release drag in widget-local coordinates.
/// Qt 5's QTest::mouseMove only teleports the cursor (QCursor::setPos) and
/// never synthesises a MouseMove — under QT_QPA_PLATFORM=offscreen that
/// yields no event at all, so the slider never sees the drag. These events
/// are exactly what a real left-button drag delivers through QPA.
void dragSequence(QWidget &widget, const QPoint &from, const QPoint &to)
{
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(from),
                      QPointF(widget.mapToGlobal(from)), Qt::LeftButton,
                      Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &press);
    QMouseEvent move(QEvent::MouseMove, QPointF(to),
                     QPointF(widget.mapToGlobal(to)), Qt::LeftButton,
                     Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &move);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(to),
                        QPointF(widget.mapToGlobal(to)), Qt::LeftButton,
                        Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(&widget, &release);
}

} // namespace

class TestMd3InteractionSmoke : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    // 1. MdButton
    void buttonClickEmits();
    void buttonSpaceEmitsWhenFocused();
    void buttonEnterEmitsWhenFocused();
    void buttonDisabledDoesNotEmit();

    // 2. MdCheckBox
    void checkBoxClickToggles();
    void checkBoxSpaceToggles();
    void checkBoxTristateCycles();

    // 3. MdSwitch
    void switchClickToggles();

    // 4. MdSlider
    void sliderMousePressAndMoveUpdatesValue();
    void sliderGenuineDragSequenceUpdatesValue();
    void sliderArrowKeysStepValue();

    // 5. MdTextField
    void textFieldTypingUpdatesText();
    void textFieldAcceptsFocus();

    // 6. MdTextArea
    void textAreaTypingUpdatesPlainText();

    // 7. MdIconButton
    void iconButtonClickEmitsWhenEnabled();
    void iconButtonDisabledDoesNotEmit();

    // 8. MdMenuItem
    void menuItemClickEmits();

    // 9. MdTab
    void tabClickSelects();

    // 10. MdChip
    void chipClickTogglesWhenCheckable();
    void chipDisabledDoesNotEmit();

    // 11. MdSegmentedButton
    void segmentedButtonClickSelects();
    void segmentedSingleChoiceClearsPrevious();
};

void TestMd3InteractionSmoke::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

void TestMd3InteractionSmoke::cleanup()
{
    // A QCOMPARE failure returns early and leaves later slots mid-flight, so
    // every mutable global is put back unconditionally.
    MdTheme::instance().setThemeMode(ThemeMode::Light);
}

// ---------------------------------------------------------------------------
// 1. MdButton
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::buttonClickEmits()
{
    MdButton button(QStringLiteral("Press"));
    button.resize(button.sizeHint());
    showAndExpose(button);

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QCOMPARE(spy.count(), 1);
}

void TestMd3InteractionSmoke::buttonSpaceEmitsWhenFocused()
{
    MdButton button(QStringLiteral("Press"));
    button.resize(button.sizeHint());
    showAndExpose(button);

    button.setFocus(Qt::TabFocusReason);
    QVERIFY(button.hasFocus());

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::keyClick(&button, Qt::Key_Space);
    QCOMPARE(spy.count(), 1);
}

void TestMd3InteractionSmoke::buttonEnterEmitsWhenFocused()
{
    MdButton button(QStringLiteral("Press"));
    button.resize(button.sizeHint());
    showAndExpose(button);

    button.setFocus(Qt::TabFocusReason);
    QVERIFY(button.hasFocus());

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::keyClick(&button, Qt::Key_Return);
    QCOMPARE(spy.count(), 1);

    QTest::keyClick(&button, Qt::Key_Enter);
    QCOMPARE(spy.count(), 2);
}

void TestMd3InteractionSmoke::buttonDisabledDoesNotEmit()
{
    MdButton button(QStringLiteral("Press"));
    button.setEnabled(false);
    button.resize(button.sizeHint());
    showAndExpose(button);

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QTest::keyClick(&button, Qt::Key_Space);
    QTest::keyClick(&button, Qt::Key_Return);
    QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// 2. MdCheckBox
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::checkBoxClickToggles()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    showAndExpose(box);

    QCOMPARE(box.checkState(), Qt::Unchecked);
    QTest::mouseClick(&box, Qt::LeftButton, Qt::NoModifier, box.rect().center());
    QCOMPARE(box.checkState(), Qt::Checked);
    QTest::mouseClick(&box, Qt::LeftButton, Qt::NoModifier, box.rect().center());
    QCOMPARE(box.checkState(), Qt::Unchecked);
}

void TestMd3InteractionSmoke::checkBoxSpaceToggles()
{
    MdCheckBox box;
    box.resize(box.sizeHint());
    showAndExpose(box);

    box.setFocus(Qt::TabFocusReason);
    QVERIFY(box.hasFocus());

    QTest::keyClick(&box, Qt::Key_Space);
    QCOMPARE(box.checkState(), Qt::Checked);
    QTest::keyClick(&box, Qt::Key_Space);
    QCOMPARE(box.checkState(), Qt::Unchecked);
}

void TestMd3InteractionSmoke::checkBoxTristateCycles()
{
    MdCheckBox box;
    box.setTristate(true);
    box.resize(box.sizeHint());
    showAndExpose(box);

    // Off → On → Indeterminate → Off (Compose's toggleValue order).
    QTest::mouseClick(&box, Qt::LeftButton, Qt::NoModifier, box.rect().center());
    QCOMPARE(box.checkState(), Qt::Checked);
    QTest::mouseClick(&box, Qt::LeftButton, Qt::NoModifier, box.rect().center());
    QCOMPARE(box.checkState(), Qt::PartiallyChecked);
    QTest::mouseClick(&box, Qt::LeftButton, Qt::NoModifier, box.rect().center());
    QCOMPARE(box.checkState(), Qt::Unchecked);
}

// ---------------------------------------------------------------------------
// 3. MdSwitch
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::switchClickToggles()
{
    MdSwitch sw;
    sw.resize(sw.sizeHint());
    showAndExpose(sw);

    QCOMPARE(sw.isChecked(), false);
    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, sw.rect().center());
    QCOMPARE(sw.isChecked(), true);
    QTest::mouseClick(&sw, Qt::LeftButton, Qt::NoModifier, sw.rect().center());
    QCOMPARE(sw.isChecked(), false);
}

// ---------------------------------------------------------------------------
// 4. MdSlider
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::sliderMousePressAndMoveUpdatesValue()
{
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(0.0); // continuous: valueAt maps the track linearly
    slider.resize(240, 48);
    showAndExpose(slider);

    QCOMPARE(slider.value(), 0.0);

    // Press near the leading end of the track (track inset is 20 px).
    QTest::mousePress(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(30, 24));
    const qreal pressed = slider.value();
    QVERIFY(pressed > 0.0);
    QVERIFY(pressed < 25.0);

    // Qt 5 QTest::mouseMove only teleports the cursor (QCursor::setPos) and
    // synthesises no MouseMove — under offscreen the slider never sees the
    // drag. Deliver the move a real left-button drag produces instead.
    QMouseEvent dragMove(QEvent::MouseMove, QPointF(200, 24),
                         QPointF(slider.mapToGlobal(QPoint(200, 24))),
                         Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(&slider, &dragMove);
    const qreal moved = slider.value();
    QVERIFY(moved > pressed);
    QVERIFY(moved > 75.0);

    QTest::mouseRelease(&slider, Qt::LeftButton, Qt::NoModifier, QPoint(200, 24));
}

void TestMd3InteractionSmoke::sliderGenuineDragSequenceUpdatesValue()
{
    // Full press / move / release as the windowing system delivers them
    // during a real left-button drag — independent of QTest::mouseMove.
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(0.0);
    slider.resize(240, 48);
    showAndExpose(slider);

    QCOMPARE(slider.value(), 0.0);
    dragSequence(slider, QPoint(30, 24), QPoint(200, 24));
    QVERIFY(slider.value() > 75.0);
}

void TestMd3InteractionSmoke::sliderArrowKeysStepValue()
{
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(5.0);
    slider.resize(240, 48);
    showAndExpose(slider);

    slider.setFocus(Qt::TabFocusReason);
    QVERIFY(slider.hasFocus());

    QSignalSpy spy(&slider, &MdSlider::valueChanged);
    QTest::keyClick(&slider, Qt::Key_Right);
    QCOMPARE(slider.value(), 5.0);
    QTest::keyClick(&slider, Qt::Key_Right);
    QCOMPARE(slider.value(), 10.0);
    QTest::keyClick(&slider, Qt::Key_Left);
    QCOMPARE(slider.value(), 5.0);
    QCOMPARE(spy.count(), 3);
}

// ---------------------------------------------------------------------------
// 5. MdTextField
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::textFieldTypingUpdatesText()
{
    MdTextField field;
    field.resize(240, 56);
    showAndExpose(field);

    field.setFocus(Qt::TabFocusReason);
    QVERIFY(field.hasFocus());

    QTest::keyClicks(&field, QStringLiteral("hello"));
    QCOMPARE(field.text(), QStringLiteral("hello"));
}

void TestMd3InteractionSmoke::textFieldAcceptsFocus()
{
    // A container with a second focusable child, so "is focused" is a real
    // claim: a sole top-level widget is the window's focus widget on show.
    QWidget host;
    MdTextField field(&host);
    MdTextField sibling(&host);
    field.setGeometry(0, 0, 240, 56);
    sibling.setGeometry(0, 60, 240, 56);
    host.resize(240, 120);
    showAndExpose(host);

    sibling.setFocus(Qt::TabFocusReason);
    QVERIFY(sibling.hasFocus());
    QVERIFY(!field.hasFocus());

    QTest::mouseClick(&field, Qt::LeftButton, Qt::NoModifier, field.rect().center());
    QVERIFY(field.hasFocus());
    QVERIFY(!sibling.hasFocus());
    QVERIFY(field.isPopulated() || field.text().isEmpty());
}

// ---------------------------------------------------------------------------
// 6. MdTextArea
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::textAreaTypingUpdatesPlainText()
{
    MdTextArea area;
    area.resize(240, 120);
    showAndExpose(area);

    area.setFocus(Qt::TabFocusReason);
    QVERIFY(area.hasFocus());

    QTest::keyClicks(&area, QStringLiteral("hello"));
    QCOMPARE(area.toPlainText(), QStringLiteral("hello"));
}

// ---------------------------------------------------------------------------
// 7. MdIconButton
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::iconButtonClickEmitsWhenEnabled()
{
    MdIconButton button(QStringLiteral("add"));
    button.resize(button.sizeHint());
    showAndExpose(button);

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QCOMPARE(spy.count(), 1);
}

void TestMd3InteractionSmoke::iconButtonDisabledDoesNotEmit()
{
    MdIconButton button(QStringLiteral("add"));
    button.setEnabled(false);
    button.resize(button.sizeHint());
    showAndExpose(button);

    QSignalSpy spy(&button, &QAbstractButton::clicked);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, button.rect().center());
    QTest::keyClick(&button, Qt::Key_Space);
    QCOMPARE(spy.count(), 0);
}

// ---------------------------------------------------------------------------
// 8. MdMenuItem
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::menuItemClickEmits()
{
    MdMenuItem item(QStringLiteral("Item"));
    item.resize(item.sizeHint());
    showAndExpose(item);

    QSignalSpy spy(&item, &QAbstractButton::clicked);
    QTest::mouseClick(&item, Qt::LeftButton, Qt::NoModifier, item.rect().center());
    QCOMPARE(spy.count(), 1);
}

// ---------------------------------------------------------------------------
// 9. MdTab
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::tabClickSelects()
{
    MdTabs tabs;
    tabs.addTab(new MdTab(QStringLiteral("A")));
    tabs.addTab(new MdTab(QStringLiteral("B")));
    tabs.addTab(new MdTab(QStringLiteral("C")));
    tabs.resize(300, 48);
    showAndExpose(tabs);

    // The first tab arrives selected.
    QCOMPARE(tabs.currentIndex(), 0);
    QVERIFY(tabs.tabAt(0)->isSelected());
    QVERIFY(!tabs.tabAt(2)->isSelected());

    // A click on an inactive tab selects it and updates the row.
    QSignalSpy spy(&tabs, &MdTabs::currentChanged);
    QTest::mouseClick(tabs.tabAt(2), Qt::LeftButton, Qt::NoModifier,
                      tabs.tabAt(2)->rect().center());
    QCOMPARE(tabs.currentIndex(), 2);
    QVERIFY(tabs.tabAt(2)->isSelected());
    QVERIFY(!tabs.tabAt(0)->isSelected());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toInt(), 2);
}

// ---------------------------------------------------------------------------
// 10. MdChip
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::chipClickTogglesWhenCheckable()
{
    MdChip chip(QStringLiteral("Filter"));
    chip.setCheckable(true);
    chip.resize(chip.sizeHint());
    showAndExpose(chip);

    QVERIFY(!chip.isChecked());
    QSignalSpy spy(&chip, &QAbstractButton::clicked);
    QTest::mouseClick(&chip, Qt::LeftButton, Qt::NoModifier, chip.rect().center());
    QCOMPARE(spy.count(), 1);
    QVERIFY(chip.isChecked());

    QTest::mouseClick(&chip, Qt::LeftButton, Qt::NoModifier, chip.rect().center());
    QCOMPARE(spy.count(), 2);
    QVERIFY(!chip.isChecked());
}

void TestMd3InteractionSmoke::chipDisabledDoesNotEmit()
{
    MdChip chip(QStringLiteral("Off"));
    chip.setCheckable(true);
    chip.setEnabled(false);
    chip.resize(chip.sizeHint());
    showAndExpose(chip);

    QSignalSpy spy(&chip, &QAbstractButton::clicked);
    QTest::mouseClick(&chip, Qt::LeftButton, Qt::NoModifier, chip.rect().center());
    QCOMPARE(spy.count(), 0);
    QVERIFY(!chip.isChecked());
}

// ---------------------------------------------------------------------------
// 11. MdSegmentedButton
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::segmentedButtonClickSelects()
{
    MdSegmentedButton button(QStringList{QStringLiteral("A"), QStringLiteral("B"),
                                         QStringLiteral("C")});
    button.setSingleChoice(true);
    button.resize(button.sizeHint());
    showAndExpose(button);

    QCOMPARE(button.segmentCount(), 3);
    QVERIFY(!button.isChecked(1));

    QSignalSpy activated(&button, &MdSegmentedButton::segmentActivated);
    QSignalSpy checked(&button, &MdSegmentedButton::segmentChecked);
    // Click the middle third of the row.
    const QPoint mid(button.width() / 2, button.height() / 2);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, mid);
    QVERIFY2(activated.count() >= 1 || checked.count() >= 1,
             "Segment click produced neither segmentActivated nor segmentChecked");
    QVERIFY(button.isChecked(1));
    QCOMPARE(button.checkedIndex(), 1);
}

void TestMd3InteractionSmoke::segmentedSingleChoiceClearsPrevious()
{
    MdSegmentedButton button(QStringList{QStringLiteral("A"), QStringLiteral("B"),
                                         QStringLiteral("C")});
    button.setSingleChoice(true);
    button.resize(button.sizeHint());
    showAndExpose(button);

    button.setChecked(0, true);
    QVERIFY(button.isChecked(0));

    // Click the last segment's third of the row.
    const QPoint last(button.width() * 5 / 6, button.height() / 2);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier, last);
    QVERIFY(button.isChecked(2));
    QVERIFY(!button.isChecked(0));
    QCOMPARE(button.checkedIndex(), 2);
}

QTEST_MAIN(TestMd3InteractionSmoke)

#include "TestMd3InteractionSmoke.moc"
