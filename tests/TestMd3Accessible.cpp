// TestMd3Accessible — the Qt accessibility interfaces the port publishes.
//
// The factory in `src/private/MdAccessible.cpp` supplies the roles and states
// Qt's defaults cannot know about: a list is a `List` of `ListItem`s with a
// *selection* state, a slider is a `Slider` with a real value interface, and
// a tab is a `PageTab` whose selected state is what the row animates around.
//
// Everything else (button / switch / text field) rides on Qt's own interfaces;
// what this suite pins for those is that the *names* a screen reader speaks
// are never empty, which is the "icon-only control with no accessible name"
// failure the design system calls out.

#include "core/MdAccessible.h"
#include "core/MdTheme.h"
#include "widgets/MdButton.h"
#include "widgets/MdCheckBox.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTab.h"
#include "widgets/MdTextField.h"

#include <QtGui/QAccessible>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

struct OffscreenDefault
{
    OffscreenDefault()
    {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
};

// NOLINTNEXTLINE(cert-err58-cpp) — qputenv cannot throw.
const OffscreenDefault g_offscreenDefault;

QAccessibleInterface *ifaceFor(QObject *object)
{
    installAccessibleInterfaces();
    return QAccessible::queryAccessibleInterface(object);
}

QString accessibleNameOf(QObject *object)
{
    QAccessibleInterface *iface = ifaceFor(object);
    return iface != nullptr ? iface->text(QAccessible::Name) : QString();
}

QAccessible::State accessibleStateOf(QObject *object)
{
    QAccessibleInterface *iface = ifaceFor(object);
    return iface != nullptr ? iface->state() : QAccessible::State();
}

} // namespace

class TestMd3Accessible : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void theFactoryInstallsIdempotently();
    void buttonsExposeNameRoleAndState();
    void textFieldsExposeEditableTextAndName();
    void selectionControlsExposeCheckedState();
    void switchRoleDistinguishesItFromCheckBox();
    void listItemsExposeActivateAction();
    void sliderExposesRoleAndValue();
    void listAndItemsExposeListSemantics();
    void tabsExposePageTabAndSelection();
    void everyShowcaseWidgetHasANonEmptyAccessibleName();

private:
    MdTheme *m_theme = nullptr;
};

void TestMd3Accessible::initTestCase()
{
    m_theme = &MdTheme::instance();
    installAccessibleInterfaces();
    QVERIFY(accessibleInterfacesInstalled());
}

void TestMd3Accessible::theFactoryInstallsIdempotently()
{
    // Calling it again must not stack factories: a widget still resolves to
    // exactly one interface, and the installed flag stays honest.
    installAccessibleInterfaces();
    installAccessibleInterfaces();
    QVERIFY(accessibleInterfacesInstalled());

    MdList list;
    QAccessibleInterface *iface = ifaceFor(&list);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::List));
}

void TestMd3Accessible::buttonsExposeNameRoleAndState()
{
    // MdButton is a QPushButton, so Qt serves the PushButton role; the port's
    // job is the *name* — an icon-only control with none is unusable.
    MdButton button(QStringLiteral("Save"));
    button.show();
    QAccessibleInterface *iface = ifaceFor(&button);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::Button));
    QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Save"));
    QVERIFY(!accessibleStateOf(&button).disabled);

    button.setEnabled(false);
    QVERIFY2(accessibleStateOf(&button).disabled, "a disabled button must report disabled");
    button.setEnabled(true);

    // Icon-only: the accessible name comes from the icon name, never empty.
    MdIconButton iconButton;
    iconButton.setIconName(QStringLiteral("settings"));
    QCOMPARE(accessibleNameOf(&iconButton), QStringLiteral("settings"));

    MdFab fab;
    fab.setIconName(QStringLiteral("add"));
    QCOMPARE(accessibleNameOf(&fab), QStringLiteral("add"));
}

void TestMd3Accessible::textFieldsExposeEditableTextAndName()
{
    // MdTextField is a QLineEdit: role EditableText, value = the text. The
    // port's addition is that the *label* is what the reader speaks.
    MdTextField field;
    field.setLabelText(QStringLiteral("Email"));
    field.setText(QStringLiteral("a@b.c"));
    field.show();

    QAccessibleInterface *iface = ifaceFor(&field);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::EditableText));

    // The spoken name is the label, either via accessibleName (the widget
    // mirrors it) or via the field's own fallback.
    const QString name = accessibleNameOf(&field);
    QVERIFY2(!name.isEmpty(), "a labelled text field must have an accessible name");

    QAccessibleValueInterface *value = iface->valueInterface();
    Q_UNUSED(value);
    // QLineEdit's editable-text interface carries the value; check the text
    // is reachable through the text interface.
    QAccessibleTextInterface *text = iface->textInterface();
    if (text != nullptr) {
        const int n = text->characterCount();
        QCOMPARE(text->text(0, n), QStringLiteral("a@b.c"));
    }
}

void TestMd3Accessible::selectionControlsExposeCheckedState()
{
    // MdCheckBox / MdRadioButton / MdSwitch all inherit Qt's toggle
    // semantics: role CheckBox / RadioButton and a checked state that tracks
    // the control. What must not happen is the checked state being lost
    // because the paint is custom.
    {
        MdCheckBox box(QStringLiteral("Accept"));
        box.setChecked(true);
        QAccessibleInterface *iface = ifaceFor(&box);
        QVERIFY(iface != nullptr);
        QCOMPARE(int(iface->role()), int(QAccessible::CheckBox));
        QVERIFY2(accessibleStateOf(&box).checked, "a checked checkbox must report checked");
        QVERIFY(accessibleStateOf(&box).checkable);
        QCOMPARE(accessibleNameOf(&box), QStringLiteral("Accept"));

        box.setChecked(false);
        QVERIFY2(!accessibleStateOf(&box).checked, "an unchecked checkbox must report unchecked");
    }
    {
        MdRadioButton radio(QStringLiteral("Option"));
        radio.setChecked(true);
        QAccessibleInterface *iface = ifaceFor(&radio);
        QVERIFY(iface != nullptr);
        QCOMPARE(int(iface->role()), int(QAccessible::RadioButton));
        QVERIFY(accessibleStateOf(&radio).checked);
    }
    {
        // MdSwitch derives from QCheckBox, so it reports as a CheckBox with a
        // checked state — the visual is a switch, the semantics are a toggle.
        MdSwitch sw(QStringLiteral("Wi-Fi"));
        sw.setChecked(true);
        QVERIFY(accessibleStateOf(&sw).checked);
        QVERIFY(accessibleStateOf(&sw).checkable);
        QVERIFY(!accessibleNameOf(&sw).isEmpty());
    }
}

void TestMd3Accessible::switchRoleDistinguishesItFromCheckBox()
{
    // Qt has no QAccessible::Switch before 6.11 (and never on Qt 5). The
    // port uses Switch when it exists and falls back to CheckBox with a
    // description that says "switch" so the two never collide.
    {
        MdSwitch sw(QStringLiteral("Wi-Fi"));
        sw.setChecked(true);
        QAccessibleInterface *iface = ifaceFor(&sw);
        QVERIFY(iface != nullptr);

#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
        QCOMPARE(int(iface->role()), int(QAccessible::Switch));
#else
        QCOMPARE(int(iface->role()), int(QAccessible::CheckBox));
#endif
        QVERIFY2(accessibleStateOf(&sw).checked, "a checked switch must report checked");
        QVERIFY(accessibleStateOf(&sw).checkable);
        QCOMPARE(accessibleNameOf(&sw), QStringLiteral("Wi-Fi"));

        // The description is what tells a screen reader "switch" rather than
        // "checkbox" when the role itself cannot.
        const QString description = iface->text(QAccessible::Description);
        QVERIFY2(description.contains(QStringLiteral("switch")),
                 qPrintable(QStringLiteral("switch description must say 'switch', got '%1'")
                                .arg(description)));
    }
    {
        // MdCheckBox must NOT carry the switch marker.
        MdCheckBox box(QStringLiteral("Accept"));
        box.setChecked(true);
        QAccessibleInterface *iface = ifaceFor(&box);
        QVERIFY(iface != nullptr);
        QCOMPARE(int(iface->role()), int(QAccessible::CheckBox));
        const QString description = iface->text(QAccessible::Description);
        QVERIFY2(!description.contains(QStringLiteral("switch")),
                 qPrintable(QStringLiteral("checkbox description must not say 'switch', got '%1'")
                                .arg(description)));
    }
}

void TestMd3Accessible::listItemsExposeActivateAction()
{
    // Screen readers invoke actions; Space/Enter is not the only path. The
    // item must expose QAccessibleActionInterface with an "activate" action
    // that fires the same signal a click does.
    MdListItem item(QStringLiteral("Inbox"));
    item.setInteractive(true);
    item.show();
    QApplication::processEvents();

    QAccessibleInterface *iface = ifaceFor(&item);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::ListItem));

    QAccessibleActionInterface *actions = iface->actionInterface();
    QVERIFY2(actions != nullptr,
             "a list item must expose QAccessibleActionInterface");

    const QStringList names = actions->actionNames();
    QVERIFY2(names.contains(QStringLiteral("activate")),
             qPrintable(QStringLiteral("action names must include 'activate', got %1")
                            .arg(names.join(QLatin1String(", ")))));
    QVERIFY2(names.size() >= 1, "at least one action is required");

    // doAction triggers the same path as click.
    QSignalSpy activated(&item, &MdListItem::activated);
    actions->doAction(QStringLiteral("activate"));
    QCOMPARE(activated.count(), 1);

    // Non-interactive items do not activate.
    item.setInteractive(false);
    actions->doAction(QStringLiteral("activate"));
    QCOMPARE(activated.count(), 1);

    // And the key bindings advertise Space / Return.
    const QStringList keys = actions->keyBindingsForAction(QStringLiteral("activate"));
    QVERIFY2(keys.contains(QStringLiteral("Space")) || keys.contains(QStringLiteral("Return")),
             qPrintable(QStringLiteral("key bindings must include Space or Return, got %1")
                            .arg(keys.join(QLatin1String(", ")))));
}

void TestMd3Accessible::sliderExposesRoleAndValue()
{
    // The whole point of the custom interface: a slider is a Slider with a
    // value, not a Client box whose value only exists as a painted number.
    MdSlider slider;
    slider.setMin(0.0);
    slider.setMax(100.0);
    slider.setStep(5.0);
    slider.setValue(30.0);
    slider.show();

    QAccessibleInterface *iface = ifaceFor(&slider);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::Slider));

    QAccessibleValueInterface *value = iface->valueInterface();
    QVERIFY2(value != nullptr, "a slider must expose QAccessibleValueInterface");
    QCOMPARE(value->currentValue().toReal(), 30.0);
    QCOMPARE(value->minimumValue().toReal(), 0.0);
    QCOMPARE(value->maximumValue().toReal(), 100.0);
    QCOMPARE(value->minimumStepSize().toReal(), 5.0);

    // Writing through the interface drives the widget.
    value->setCurrentValue(80.0);
    QCOMPARE(slider.value(), 80.0);

    slider.setEnabled(false);
    QVERIFY(accessibleStateOf(&slider).disabled);
    slider.setEnabled(true);
}

void TestMd3Accessible::listAndItemsExposeListSemantics()
{
    MdList list;
    list.setSelectionMode(MdList::SelectionMode::Single);

    auto *first = new MdListItem(QStringLiteral("Inbox"));
    first->setInteractive(true);
    auto *second = new MdListItem(QStringLiteral("Starred"));
    second->setInteractive(true);
    list.addItem(first);
    list.addItem(second);
    list.show();

    QAccessibleInterface *listIface = ifaceFor(&list);
    QVERIFY(listIface != nullptr);
    QCOMPARE(int(listIface->role()), int(QAccessible::List));
    QCOMPARE(listIface->childCount(), 2);

    QAccessibleInterface *firstIface = ifaceFor(first);
    QVERIFY(firstIface != nullptr);
    QCOMPARE(int(firstIface->role()), int(QAccessible::ListItem));
    QCOMPARE(firstIface->text(QAccessible::Name), QStringLiteral("Inbox"));
    QCOMPARE(listIface->child(0), firstIface);

    // The selection state is what a reader announces as "selected".
    QVERIFY2(!accessibleStateOf(first).selected, "an unselected row must not report selected");
    list.activateItem(first);
    QVERIFY2(accessibleStateOf(first).selected, "an activated row must report selected");
    QVERIFY(!accessibleStateOf(second).selected);

    // Supporting text is the description, not the name.
    second->setSupporting(QStringLiteral("12 unread"));
    QAccessibleInterface *secondIface = ifaceFor(second);
    QVERIFY(secondIface != nullptr);
    QCOMPARE(secondIface->text(QAccessible::Description), QStringLiteral("12 unread"));
}

void TestMd3Accessible::tabsExposePageTabAndSelection()
{
    MdTab tab(QStringLiteral("Overview"));
    tab.show();

    QAccessibleInterface *iface = ifaceFor(&tab);
    QVERIFY(iface != nullptr);
    QCOMPARE(int(iface->role()), int(QAccessible::PageTab));
    QCOMPARE(iface->text(QAccessible::Name), QStringLiteral("Overview"));

    QVERIFY2(!accessibleStateOf(&tab).selected, "an unselected tab must not report selected");
    QVERIFY(accessibleStateOf(&tab).selectable);

    tab.setSelected(true);
    QVERIFY2(accessibleStateOf(&tab).selected, "a selected tab must report selected");
}

void TestMd3Accessible::everyShowcaseWidgetHasANonEmptyAccessibleName()
{
    // The floor the design system asks for: no interactive control in a
    // showcase may present an empty name. These are the same constructions the
    // gallery pages use.
    MdButton textButton(QStringLiteral("Continue"));
    QVERIFY2(!accessibleNameOf(&textButton).isEmpty(), "text button");

    MdButton iconOnly;
    iconOnly.setLeadingIcon(QStringLiteral("arrow_back"));
    iconOnly.setAccessibleName(QStringLiteral("Back"));
    QVERIFY2(!accessibleNameOf(&iconOnly).isEmpty(), "icon-only button");

    MdIconButton iconButton;
    iconButton.setIconName(QStringLiteral("delete"));
    QVERIFY2(!accessibleNameOf(&iconButton).isEmpty(), "icon button");

    MdFab fab;
    fab.setIconName(QStringLiteral("edit"));
    QVERIFY2(!accessibleNameOf(&fab).isEmpty(), "fab");

    MdTextField field;
    field.setLabelText(QStringLiteral("Password"));
    QVERIFY2(!accessibleNameOf(&field).isEmpty(), "text field");

    MdCheckBox check(QStringLiteral("Remember me"));
    QVERIFY2(!accessibleNameOf(&check).isEmpty(), "checkbox");

    MdSwitch sw(QStringLiteral("Notifications"));
    QVERIFY2(!accessibleNameOf(&sw).isEmpty(), "switch");

    MdRadioButton radio(QStringLiteral("Monthly"));
    QVERIFY2(!accessibleNameOf(&radio).isEmpty(), "radio");

    MdSlider slider;
    slider.setAccessibleName(QStringLiteral("Volume"));
    QVERIFY2(!accessibleNameOf(&slider).isEmpty(), "slider");

    MdList list;
    auto *row = new MdListItem(QStringLiteral("Row"));
    list.addItem(row);
    QVERIFY2(!accessibleNameOf(&list).isEmpty(), "list");
    QVERIFY2(!accessibleNameOf(row).isEmpty(), "list item");

    MdTab tab(QStringLiteral("Details"));
    QVERIFY2(!accessibleNameOf(&tab).isEmpty(), "tab");
}

QTEST_MAIN(TestMd3Accessible)

#include "TestMd3Accessible.moc"
