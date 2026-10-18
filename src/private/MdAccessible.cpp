// MdAccessible — the accessible factory and the MD3-specific interfaces.
//
// Lives under `src/private/` because it is the one place in the library that
// has to know every widget type at once; `src/core/MdAccessible.h` publishes
// only the two entry points a host needs. See that header for why the factory
// exists and what it deliberately does not shadow.

#include "core/MdAccessible.h"

#include "widgets/MdButton.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTab.h"
#include "widgets/MdTextField.h"

#include <QtGui/QAccessible>
#include <QtWidgets/qaccessiblewidget.h>

#include <atomic>

namespace md {

namespace {

/// Shared bookkeeping for "the widget's name is its label, and an icon-only
/// control must still have one".
QString nameOrFallback(const QWidget *widget, const QString &fallback)
{
    if (widget == nullptr) {
        return fallback;
    }
    const QString explicitName = widget->accessibleName();
    return explicitName.isEmpty() ? fallback : explicitName;
}

// ---------------------------------------------------------------------------
// MdSwitch — role Switch when Qt has it, CheckBox + "switch" description otherwise
// ---------------------------------------------------------------------------

/// Qt has no `QAccessible::Switch` before 6.11 (and never on Qt 5). ARIA's
/// `switch` is a checkbox-like toggle, so the portable fallback is
/// `QAccessible::CheckBox` — which keeps the checked / checkable state Qt
/// already reports for QAbstractButton — plus a description that says
/// "switch" so a screen reader can still tell the control apart from
/// MdCheckBox. When the role exists (Qt >= 6.11) it is used directly and the
/// description stays the caller's.
class MdSwitchAccessible : public QAccessibleWidget
{
public:
    explicit MdSwitchAccessible(MdSwitch *sw)
        : QAccessibleWidget(sw, switchRole())
    {
    }

    static QAccessible::Role switchRole()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
        return QAccessible::Switch;
#else
        return QAccessible::CheckBox;
#endif
    }

    QString text(QAccessible::Text t) const override
    {
        auto *sw = static_cast<MdSwitch *>(widget());
        if (sw == nullptr) {
            return QString();
        }
        switch (t) {
        case QAccessible::Name:
            return nameOrFallback(sw, sw->text());
        case QAccessible::Description: {
#if QT_VERSION >= QT_VERSION_CHECK(6, 11, 0)
            // The Switch role already distinguishes the control.
            return QAccessibleWidget::text(t);
#else
            // Role is CheckBox; the description is what distinguishes it
            // from MdCheckBox. Always say "switch" so the two never collide,
            // even when the caller wrote their own description.
            const QString explicitDescription = sw->accessibleDescription();
            return explicitDescription.isEmpty() ? QStringLiteral("switch")
                                                 : QStringLiteral("switch, ") + explicitDescription;
#endif
        }
        default:
            break;
        }
        return QAccessibleWidget::text(t);
    }

    QAccessible::State state() const override
    {
        QAccessible::State s = QAccessibleWidget::state();
        auto *sw = static_cast<MdSwitch *>(widget());
        if (sw != nullptr) {
            s.checkable = true;
            s.checked = sw->isChecked();
            s.focusable = sw->focusPolicy() != Qt::NoFocus;
            s.disabled = !sw->isEnabled();
        }
        return s;
    }
};

// ---------------------------------------------------------------------------
// MdList / MdListItem
// ---------------------------------------------------------------------------

class MdListItemAccessible : public QAccessibleWidget
{
public:
    explicit MdListItemAccessible(MdListItem *item)
        : QAccessibleWidget(item, QAccessible::ListItem)
    {
    }

    void *interface_cast(QAccessible::InterfaceType t) override
    {
        // QAccessibleWidget already inherits QAccessibleActionInterface, so
        // the default cast works; just confirm the interface is offered.
        return QAccessibleWidget::interface_cast(t);
    }

    // QAccessibleActionInterface -------------------------------------------

    QStringList actionNames() const override
    {
        return {QStringLiteral("activate")};
    }

    QString localizedActionName(const QString &name) const override
    {
        if (name == QLatin1String("activate")) {
            return QAccessibleActionInterface::tr("Activate");
        }
        return name;
    }

    QString localizedActionDescription(const QString &name) const override
    {
        if (name == QLatin1String("activate")) {
            return QAccessibleActionInterface::tr("Activates the list item");
        }
        return name;
    }

    void doAction(const QString &actionName) override
    {
        if (actionName != QLatin1String("activate")) {
            return;
        }
        auto *item = static_cast<MdListItem *>(widget());
        if (item != nullptr) {
            // Same path as a click / Space / Enter.
            item->activate();
        }
    }

    QStringList keyBindingsForAction(const QString &actionName) const override
    {
        if (actionName == QLatin1String("activate")) {
            return {QStringLiteral("Space"), QStringLiteral("Return")};
        }
        return {};
    }

    QString text(QAccessible::Text t) const override
    {
        auto *item = static_cast<MdListItem *>(widget());
        if (item == nullptr) {
            return QString();
        }
        switch (t) {
        case QAccessible::Name: {
            // The headline is the row's speech label; supporting text is the
            // description a reader offers after the name.
            const QString explicitName = item->accessibleName();
            return explicitName.isEmpty() ? item->headline() : explicitName;
        }
        case QAccessible::Description:
            return item->supporting();
        default:
            break;
        }
        return QAccessibleWidget::text(t);
    }

    QAccessible::State state() const override
    {
        QAccessible::State s = QAccessibleWidget::state();
        auto *item = static_cast<MdListItem *>(widget());
        if (item != nullptr) {
            s.selectable = true;
            s.selected = item->isSelected();
            s.focusable = item->isInteractive();
            s.disabled = !item->isEnabled();
        }
        return s;
    }
};

class MdListAccessible : public QAccessibleWidget
{
public:
    explicit MdListAccessible(MdList *list)
        : QAccessibleWidget(list, QAccessible::List)
    {
    }

    QString text(QAccessible::Text t) const override
    {
        if (t == QAccessible::Name) {
            return nameOrFallback(static_cast<QWidget *>(widget()), QStringLiteral("list"));
        }
        return QAccessibleWidget::text(t);
    }

    int childCount() const override
    {
        auto *list = static_cast<MdList *>(widget());
        return list != nullptr ? list->itemCount() : 0;
    }

    QAccessibleInterface *child(int index) const override
    {
        auto *list = static_cast<MdList *>(widget());
        if (list == nullptr) {
            return nullptr;
        }
        const QList<MdListItem *> rows = list->items();
        if (index < 0 || index >= rows.size()) {
            return nullptr;
        }
        return QAccessible::queryAccessibleInterface(rows.at(index));
    }

    int indexOfChild(const QAccessibleInterface *childInterface) const override
    {
        auto *list = static_cast<MdList *>(widget());
        if (list == nullptr || childInterface == nullptr) {
            return -1;
        }
        const QList<MdListItem *> rows = list->items();
        for (int i = 0; i < rows.size(); ++i) {
            if (childInterface->object() == rows.at(i)) {
                return i;
            }
        }
        return -1;
    }
};

// ---------------------------------------------------------------------------
// MdSlider — role Slider with a real value interface
// ---------------------------------------------------------------------------

class MdSliderAccessible : public QAccessibleWidget, public QAccessibleValueInterface
{
public:
    explicit MdSliderAccessible(MdSlider *slider)
        : QAccessibleWidget(slider, QAccessible::Slider)
    {
    }

    void *interface_cast(QAccessible::InterfaceType t) override
    {
        if (t == QAccessible::ValueInterface) {
            return static_cast<QAccessibleValueInterface *>(this);
        }
        return QAccessibleWidget::interface_cast(t);
    }

    QString text(QAccessible::Text t) const override
    {
        if (t == QAccessible::Name) {
            return nameOrFallback(slider(), QStringLiteral("slider"));
        }
        return QAccessibleWidget::text(t);
    }

    QAccessible::State state() const override
    {
        QAccessible::State s = QAccessibleWidget::state();
        MdSlider *const w = slider();
        if (w != nullptr) {
            s.focusable = true;
            s.readOnly = false;
            s.disabled = !w->isEnabled();
        }
        return s;
    }

    // QAccessibleValueInterface -------------------------------------------
    QVariant currentValue() const override
    {
        MdSlider *const w = slider();
        return w != nullptr ? QVariant(w->value()) : QVariant();
    }

    void setCurrentValue(const QVariant &value) override
    {
        MdSlider *const w = slider();
        if (w != nullptr) {
            w->setValue(value.toReal());
        }
    }

    QVariant maximumValue() const override
    {
        MdSlider *const w = slider();
        return w != nullptr ? QVariant(w->max()) : QVariant();
    }

    QVariant minimumValue() const override
    {
        MdSlider *const w = slider();
        return w != nullptr ? QVariant(w->min()) : QVariant();
    }

    QVariant minimumStepSize() const override
    {
        MdSlider *const w = slider();
        return w != nullptr ? QVariant(w->step()) : QVariant();
    }

private:
    MdSlider *slider() const { return static_cast<MdSlider *>(widget()); }
};

// ---------------------------------------------------------------------------
// MdTab — role PageTab with the tab row's selection state
// ---------------------------------------------------------------------------

class MdTabAccessible : public QAccessibleWidget
{
public:
    explicit MdTabAccessible(MdTab *tab)
        : QAccessibleWidget(tab, QAccessible::PageTab)
    {
    }

    QString text(QAccessible::Text t) const override
    {
        auto *tab = static_cast<MdTab *>(widget());
        if (tab != nullptr && t == QAccessible::Name) {
            return nameOrFallback(tab, tab->label());
        }
        return QAccessibleWidget::text(t);
    }

    QAccessible::State state() const override
    {
        QAccessible::State s = QAccessibleWidget::state();
        auto *tab = static_cast<MdTab *>(widget());
        if (tab != nullptr) {
            s.selectable = true;
            s.selected = tab->isSelected();
            s.focusable = true;
            s.disabled = !tab->isEnabled();
        }
        return s;
    }
};

// ---------------------------------------------------------------------------
// The factory
// ---------------------------------------------------------------------------

QAccessibleInterface *mdAccessibleFactory(const QString &classname, QObject *object)
{
    Q_UNUSED(classname);
    if (object == nullptr) {
        return nullptr;
    }
    // qobject_cast rather than string-matching `classname`: a host that
    // subclasses an Md* widget must still get the right role.
    if (auto *list = qobject_cast<MdList *>(object)) {
        return new MdListAccessible(list);
    }
    if (auto *item = qobject_cast<MdListItem *>(object)) {
        return new MdListItemAccessible(item);
    }
    if (auto *slider = qobject_cast<MdSlider *>(object)) {
        return new MdSliderAccessible(slider);
    }
    if (auto *sw = qobject_cast<MdSwitch *>(object)) {
        return new MdSwitchAccessible(sw);
    }
    if (auto *tab = qobject_cast<MdTab *>(object)) {
        return new MdTabAccessible(tab);
    }
    // Everything else (MdButton / MdIconButton / MdFab as QPushButton,
    // MdCheckBox as QCheckBox, MdTextField as QLineEdit) already has a
    // correct default interface — return nullptr so it is used.
    return nullptr;
}

std::atomic<bool> g_installed(false);

} // namespace

void installAccessibleInterfaces()
{
    if (g_installed.exchange(true)) {
        return;
    }
    QAccessible::installFactory(mdAccessibleFactory);
}

bool accessibleInterfacesInstalled()
{
    return g_installed.load();
}

} // namespace md
