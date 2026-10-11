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
#include "widgets/MdDatePicker.h"
#include "widgets/MdDialog.h"
#include "widgets/MdDialogHost.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdMenu.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdSegmentedButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"
#include "widgets/MdTimePicker.h"
#include "widgets/MdSwitch.h"

#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
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
    void segmentedStretchedClickStillSelects();

    // 12. MdList / MdListItem
    void listClickSelects();
    void listItemsStackWithoutOverlap();

    // 13. MdMenuItem selection on click
    void menuClickSetsSelected();
    void menuSurfacePaintsFullContainerRect();
    void sliderHandleDragUpdatesValue();

    // 14. Dialog action / body clicks
    void dialogActionButtonsReceiveClicks();
    void dialogBodyClickDoesNotDismiss();

    // 15. TimePicker face switching
    void timePickerMinutesFaceViaSelectorClick();
    void timePicker24hMinutesFaceViaSelectorClick();
    void timePickerDialSetsMinutes();

    // 16. DatePicker click selection + press feedback
    void datePickerClickSelectsDate();
    void datePickerYearClickMovesSelection();
    void datePickerPressShowsStateLayer();

    // 17. SearchBar typing + visible text + full pill
    void searchBarTypingUpdatesText();
    void searchBarTextIsVisible();
    void searchBarPaintsFullPill();
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

void TestMd3InteractionSmoke::segmentedStretchedClickStillSelects()
{
    // A layout may stretch the control well past its natural width. The
    // segment row must fill that width — otherwise a click at the centre (or
    // at 3/4, the gallery's probe point) lands on empty space and the button
    // looks dead.
    MdSegmentedButton button(QStringList{QStringLiteral("A"), QStringLiteral("B"),
                                         QStringLiteral("C")});
    const QSize hint = button.sizeHint();
    button.resize(hint.width() * 2, hint.height());
    showAndExpose(button);

    QSignalSpy checked(&button, &MdSegmentedButton::segmentChecked);
    QTest::mouseClick(&button, Qt::LeftButton, Qt::NoModifier,
                      QPoint(button.width() * 3 / 4, button.height() / 2));
    QVERIFY2(checked.count() >= 1,
             "Click at 3/4 of a stretched segmented button produced no segmentChecked");
    QCOMPARE(button.checkedIndex(), 2);
}

void TestMd3InteractionSmoke::listClickSelects()
{
    MdList list;
    list.resize(360, 300);
    showAndExpose(list);

    auto *one = new MdListItem(QStringLiteral("One"));
    one->setInteractive(true);
    auto *two = new MdListItem(QStringLiteral("Two"));
    two->setInteractive(true);
    list.addItem(one);
    list.addItem(two);
    QApplication::processEvents();

    QSignalSpy activated(&list, &MdList::itemActivated);
    QSignalSpy selected(&list, &MdList::selectionChanged);
    QTest::mouseClick(two, Qt::LeftButton, Qt::NoModifier, two->rect().center());
    QVERIFY2(activated.count() >= 1, "List item click produced no itemActivated");
    QVERIFY2(two->isSelected(), "List item click did not select the item");
    QVERIFY(!one->isSelected());
    QVERIFY(selected.count() >= 1);
}

void TestMd3InteractionSmoke::listItemsStackWithoutOverlap()
{
    // Height-for-width: a wrapping supporting row grows when the list narrows.
    // The list must report that height so a band places it tall enough —
    // otherwise neighbouring items share the same y and paint on top of each
    // other.
    MdList list;
    showAndExpose(list);

    auto *one = new MdListItem(QStringLiteral("One"));
    one->setInteractive(true);
    auto *two = new MdListItem(QStringLiteral("Two"));
    two->setInteractive(true);
    two->setSupporting(QStringLiteral("Supporting text that wraps when the list is narrow"));
    auto *three = new MdListItem(QStringLiteral("Three"));
    three->setInteractive(true);
    three->setOverline(QStringLiteral("OVERLINE"));
    three->setSupporting(QStringLiteral("Supporting"));
    for (MdListItem *item : {one, two, three}) {
        list.addItem(item);
    }

    for (int width : {220, 360, 640}) {
        list.resize(width, list.heightForWidth(width));
        QApplication::processEvents();

        const QList<MdListItem *> items = list.items();
        for (int i = 0; i + 1 < items.size(); ++i) {
            const QRect a = items.at(i)->geometry();
            const QRect b = items.at(i + 1)->geometry();
            const int overlap = qMin(a.bottom(), b.bottom()) - qMax(a.top(), b.top());
            QVERIFY2(overlap < 0,
                     qPrintable(QStringLiteral("items %1 and %2 overlap by %3 at width %4")
                                    .arg(i)
                                    .arg(i + 1)
                                    .arg(overlap)
                                    .arg(width)));
        }
        QVERIFY2(list.height() >= list.heightForWidth(width) - 1,
                 qPrintable(QStringLiteral("list shorter than heightForWidth at width %1")
                                .arg(width)));
    }
}

void TestMd3InteractionSmoke::menuClickSetsSelected()
{
    MdMenu menu;
    menu.setWindowFlags(Qt::Widget);
    menu.resize(220, 200);
    menu.addItem(QStringLiteral("First"));
    menu.addItem(QStringLiteral("Second"));
    menu.addItem(QStringLiteral("Third"));
    showAndExpose(menu);
    QApplication::processEvents();

    MdMenuItem *target = menu.items().at(1);
    QSignalSpy clicked(target, &QAbstractButton::clicked);
    QSignalSpy selectedSpy(target, &MdMenuItem::selectedChanged);
    QTest::mouseClick(target, Qt::LeftButton, Qt::NoModifier, target->rect().center());
    QVERIFY2(clicked.count() >= 1, "Menu item click produced no clicked()");
    QVERIFY2(target->isSelected(), "Menu item click did not set selected");
    QVERIFY(selectedSpy.count() >= 1);
}

void TestMd3InteractionSmoke::menuSurfacePaintsFullContainerRect()
{
    MdMenu menu;
    menu.setWindowFlags(Qt::Widget);
    menu.resize(200, 160);
    menu.addItem(QStringLiteral("First"));
    menu.addItem(QStringLiteral("Second"));
    showAndExpose(menu);
    QApplication::processEvents();

    const QImage image = menu.grab().toImage();
    const QColor surfaceContainer =
        MdTheme::instance().color(ColorRole::SurfaceContainer);
    auto near = [](const QColor &a, const QColor &b) {
        return a.isValid() && b.isValid() && qAbs(a.red() - b.red()) < 28
               && qAbs(a.green() - b.green()) < 28 && qAbs(a.blue() - b.blue()) < 28
               && a.alpha() > 200;
    };
    const int w = image.width();
    const int h = image.height();
    // Mid-edges of the container rect must carry the surface (and its
    // outline) — a missing band here is the "incomplete border" defect.
    const QColor top = image.pixelColor(w / 2, 1);
    const QColor bottom = image.pixelColor(w / 2, h - 2);
    const QColor left = image.pixelColor(1, h / 2);
    const QColor right = image.pixelColor(w - 2, h / 2);
    QVERIFY2(near(top, surfaceContainer), "menu top edge not painted");
    QVERIFY2(near(bottom, surfaceContainer), "menu bottom edge not painted");
    QVERIFY2(near(left, surfaceContainer), "menu left edge not painted");
    QVERIFY2(near(right, surfaceContainer), "menu right edge not painted");
}

void TestMd3InteractionSmoke::sliderHandleDragUpdatesValue()
{
    // Press on the handle itself (not the track) and drag — the overhang used
    // to sit outside the widget's sizeHint height, so a real grab on the pill
    // never reached the slider.
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(0.0);
    slider.resize(240, slider.sizeHint().height());
    showAndExpose(slider);

    QVERIFY2(slider.sizeHint().height() >= slider.handleRect().height(),
             "slider sizeHint shorter than its handle");

    slider.setValue(20.0);
    const QPoint handle = slider.handleRect().center().toPoint();
    const QPoint target(slider.width() - 30, handle.y());
    dragSequence(slider, handle, target);
    QVERIFY2(slider.value() > 60.0,
             qPrintable(QStringLiteral("handle drag left value at %1").arg(slider.value())));
}

// ---------------------------------------------------------------------------
// 14. Dialog action / body clicks
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::dialogActionButtonsReceiveClicks()
{
    QWidget parent;
    parent.resize(800, 600);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("Reset settings?"));
    host.dialog()->setText(QStringLiteral("This resets every setting."));
    auto *confirm = new MdButton(QStringLiteral("Accept"));
    confirm->setVariant(ButtonVariant::Text);
    host.dialog()->setConfirmButton(confirm);
    auto *dismiss = new MdButton(QStringLiteral("Cancel"));
    dismiss->setVariant(ButtonVariant::Text);
    host.dialog()->setDismissButton(dismiss);

    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(40);

    // Hit-test first: a covered button is a stacking fault, not a spy fault.
    const QPoint global = confirm->mapToGlobal(confirm->rect().center());
    QWidget *hit = QApplication::widgetAt(global);
    QVERIFY2(hit == confirm || (hit && confirm->isAncestorOf(hit)),
             "dialog confirm button is covered — click cannot reach it");

    QSignalSpy confirmSpy(confirm, &QAbstractButton::clicked);
    QTest::mouseClick(confirm, Qt::LeftButton, Qt::NoModifier, confirm->rect().center());
    QCOMPARE(confirmSpy.count(), 1);

    QSignalSpy dismissSpy(dismiss, &QAbstractButton::clicked);
    QTest::mouseClick(dismiss, Qt::LeftButton, Qt::NoModifier, dismiss->rect().center());
    QCOMPARE(dismissSpy.count(), 1);
}

void TestMd3InteractionSmoke::dialogBodyClickDoesNotDismiss()
{
    QWidget parent;
    parent.resize(800, 600);
    MdDialogHost host(&parent);
    host.dialog()->setTitle(QStringLiteral("T"));
    host.dialog()->setText(QStringLiteral("B"));

    int dismissals = 0;
    QObject::connect(&host, &MdDialogHost::dismissed, [&dismissals] { ++dismissals; });

    parent.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parent));
    host.show();
    QTest::qWait(40);
    QVERIFY(host.isShowing());

    // A press on the dialog body must be absorbed by the dialog — it is not
    // a scrim click and must not run onDismissRequest.
    QMouseEvent bodyPress(QEvent::MouseButtonPress,
                          QPointF(host.dialog()->rect().center()),
                          QPointF(host.dialog()->mapToGlobal(host.dialog()->rect().center())),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(host.dialog(), &bodyPress);
    QCOMPARE(dismissals, 0);
    QVERIFY(host.isShowing());

    // A press on the scrim still dismisses.
    QTest::mousePress(&host, Qt::LeftButton, Qt::NoModifier, QPoint(4, host.height() / 2));
    QCOMPARE(dismissals, 1);
}

// ---------------------------------------------------------------------------
// 15. TimePicker face switching
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::timePickerMinutesFaceViaSelectorClick()
{
    MdTimePicker picker;
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    QCOMPARE(picker.face(), MdTimePickerFace::Hours);
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier,
                      picker.timeSelectorRect(false).center().toPoint());
    QCOMPARE(picker.face(), MdTimePickerFace::Minutes);

    // And back to hours.
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier,
                      picker.timeSelectorRect(true).center().toPoint());
    QCOMPARE(picker.face(), MdTimePickerFace::Hours);
}

void TestMd3InteractionSmoke::timePicker24hMinutesFaceViaSelectorClick()
{
    MdTimePicker picker;
    picker.set24h(true);
    picker.setTime(QTime(18, 45));
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    // The 24h field must split into distinct hours / minutes hit targets —
    // one combined rect made the minutes face unreachable.
    const QRectF hours = picker.timeSelectorRect(true);
    const QRectF minutes = picker.timeSelectorRect(false);
    QVERIFY2(!hours.intersects(minutes), "24h hours and minutes hit rects overlap");
    QVERIFY2(hours != minutes, "24h hours and minutes hit rects are identical");

    QCOMPARE(picker.face(), MdTimePickerFace::Hours);
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier, minutes.center().toPoint());
    QCOMPARE(picker.face(), MdTimePickerFace::Minutes);

    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier, hours.center().toPoint());
    QCOMPARE(picker.face(), MdTimePickerFace::Hours);
}

void TestMd3InteractionSmoke::timePickerDialSetsMinutes()
{
    MdTimePicker picker;
    picker.setFace(MdTimePickerFace::Minutes);
    picker.setTime(QTime(10, 30));
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    // Click near 3 o'clock on the dial — the minutes face snaps to 5-minute
    // slots, so the minute must move off 30.
    const QRectF dial = picker.dialRect();
    const QPoint pos = (dial.center() + QPointF(dial.width() / 2.0 - 24.0, 0)).toPoint();
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier, pos);
    QVERIFY2(picker.time().minute() != 30,
             qPrintable(QStringLiteral("dial click left minute at %1").arg(picker.time().minute())));
}

// ---------------------------------------------------------------------------
// 16. DatePicker click selection + press feedback
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::datePickerClickSelectsDate()
{
    MdDatePicker picker;
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    picker.setSelectedDate(QDate(2026, 6, 1));
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    // June 2026: day 15 sits in cell (2, 0) on the Monday-first grid.
    QCOMPARE(picker.dateAtCell(2, 0), QDate(2026, 6, 15));
    QSignalSpy spy(&picker, &MdDatePicker::selectedDateChanged);
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier,
                      picker.dateCellRect(2, 0).center().toPoint());
    QCOMPARE(picker.selectedDate(), QDate(2026, 6, 15));
    QVERIFY(spy.count() >= 1);
}

void TestMd3InteractionSmoke::datePickerYearClickMovesSelection()
{
    MdDatePicker picker;
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    picker.setSelectedDate(QDate(2026, 6, 15));
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    // Open the year face from the header's supporting line.
    const QRectF header = picker.headerRect();
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier,
                      QPoint(int(header.left() + 40), int(header.bottom() - 24)));
    QCOMPARE(picker.face(), MdDatePickerFace::Years);

    // Year cells are laid out around the displayed year; index 8 is +2.
    QSignalSpy spy(&picker, &MdDatePicker::selectedDateChanged);
    QTest::mouseClick(&picker, Qt::LeftButton, Qt::NoModifier,
                      picker.yearCellRect(8).center().toPoint());
    QCOMPARE(picker.face(), MdDatePickerFace::Calendar);
    QCOMPARE(picker.selectedDate().year(), 2028);
    QVERIFY(spy.count() >= 1);
}

void TestMd3InteractionSmoke::datePickerPressShowsStateLayer()
{
    MdDatePicker picker;
    picker.setDisplayedMonth(QDate(2026, 6, 1));
    picker.setSelectedDate(QDate(2026, 6, 1));
    picker.resize(picker.sizeHint());
    showAndExpose(picker);

    QCOMPARE(picker.pressedCell(), -1);
    QTest::mousePress(&picker, Qt::LeftButton, Qt::NoModifier,
                      picker.dateCellRect(2, 0).center().toPoint());
    QCOMPARE(picker.pressedCell(), 2 * 7 + 0);
    QTest::mouseRelease(&picker, Qt::LeftButton, Qt::NoModifier,
                        picker.dateCellRect(2, 0).center().toPoint());
    QCOMPARE(picker.pressedCell(), -1);
}

// ---------------------------------------------------------------------------
// 17. SearchBar typing + visible text + full pill
// ---------------------------------------------------------------------------

void TestMd3InteractionSmoke::searchBarTypingUpdatesText()
{
    MdSearchBar bar;
    bar.resize(360, 56);
    showAndExpose(bar);

    QTest::mouseClick(&bar, Qt::LeftButton, Qt::NoModifier, bar.rect().center());
    QVERIFY(bar.hasFocus());
    QTest::keyClicks(&bar, QStringLiteral("hello"));
    QCOMPARE(bar.text(), QStringLiteral("hello"));
}

void TestMd3InteractionSmoke::searchBarTextIsVisible()
{
    // Qt 5's Windows offscreen platform has no font database: drawText is a
    // silent no-op, so a glyph-ink probe cannot tell "text hidden under the
    // chrome" from "text drawn but the engine paints nothing". Detect that
    // first — otherwise the assertion fails for an environment reason.
    {
        QImage fontProbe(120, 40, QImage::Format_ARGB32);
        fontProbe.fill(Qt::white);
        {
            QPainter painter(&fontProbe);
            painter.setPen(Qt::black);
            painter.drawText(fontProbe.rect(), Qt::AlignCenter, QStringLiteral("hello"));
        }
        int probeInk = 0;
        for (int y = 0; y < fontProbe.height(); ++y) {
            for (int x = 0; x < fontProbe.width(); ++x) {
                if (qGray(fontProbe.pixel(x, y)) < 100) {
                    ++probeInk;
                }
            }
        }
        if (probeInk == 0) {
            QSKIP("offscreen font engine paints no glyph ink; cannot verify text visibility");
        }
    }

    // The real claim is paint order: the typed line must sit *on top of* the
    // pill, not under it. Compare an empty bar with a populated one — if the
    // chrome covered the text the two grabs would be identical.
    auto inkCount = [](const QImage &image) {
        int ink = 0;
        const int y0 = image.height() / 4;
        const int y1 = image.height() * 3 / 4;
        const int x0 = 48;
        const int x1 = qMin(300, image.width() - 8);
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                if (qGray(image.pixel(x, y)) < 120) {
                    ++ink;
                }
            }
        }
        return ink;
    };

    MdSearchBar emptyBar;
    emptyBar.resize(360, 56);
    showAndExpose(emptyBar);
    const int emptyInk = inkCount(emptyBar.grab().toImage());

    MdSearchBar bar;
    bar.resize(360, 56);
    bar.setText(QStringLiteral("hello"));
    showAndExpose(bar);
    const int textInk = inkCount(bar.grab().toImage());

    QVERIFY2(textInk > emptyInk,
             qPrintable(QStringLiteral("typed text adds no ink (empty=%1, text=%2) "
                                       "— either not painted or hidden under the chrome")
                            .arg(emptyInk)
                            .arg(textInk)));
    QVERIFY2(textInk - emptyInk > 20,
             qPrintable(QStringLiteral("typed text ink too faint (empty=%1, text=%2)")
                            .arg(emptyInk)
                            .arg(textInk)));
}

void TestMd3InteractionSmoke::searchBarPaintsFullPill()
{
    MdSearchBar bar;
    bar.resize(360, 56);
    showAndExpose(bar);

    const QImage image = bar.grab().toImage();
    const QColor container = MdTheme::instance().color(ColorRole::SurfaceContainerHigh);
    auto near = [](const QColor &a, const QColor &b) {
        return a.isValid() && b.isValid() && qAbs(a.red() - b.red()) < 28
               && qAbs(a.green() - b.green()) < 28 && qAbs(a.blue() - b.blue()) < 28
               && a.alpha() > 200;
    };
    // The pill's flat top / bottom spans the middle of the bar; the corners
    // of the widget rect are outside the stadium and may be transparent.
    QVERIFY2(near(image.pixelColor(image.width() / 2, 3), container),
             "search bar top edge not painted");
    QVERIFY2(near(image.pixelColor(image.width() / 2, image.height() - 4), container),
             "search bar bottom edge not painted");
    QVERIFY2(near(image.pixelColor(2, image.height() / 2), container),
             "search bar left tip not painted");
    QVERIFY2(near(image.pixelColor(image.width() - 3, image.height() / 2), container),
             "search bar right tip not painted");
}

QTEST_MAIN(TestMd3InteractionSmoke)

#include "TestMd3InteractionSmoke.moc"
