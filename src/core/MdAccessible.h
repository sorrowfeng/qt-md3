#ifndef MD_ACCESSIBLE_H
#define MD_ACCESSIBLE_H

// MdAccessible — Qt accessibility wiring for the qt-md3 widget set.
//
// Qt ships default `QAccessibleInterface` implementations for the standard
// controls, and every `Md*` widget that derives from one of those (QPushButton,
// QCheckBox, QRadioButton, QLineEdit, QPlainTextEdit) inherits them for free.
// What the defaults cannot know is the MD3 semantics this port adds on top:
//
//   * a list is a `List` of `ListItem`s, not a plain `Client` box, and a
//     selected row is a *selection* state rather than a checkbox tick;
//   * a slider is a `Slider` with a real `QAccessibleValueInterface`, so a
//     screen reader announces the value instead of a number in a tooltip;
//   * a tab is a `PageTab` whose selection state is what the tab row
//     animates around;
//   * an icon-only button has no text, so its accessible name has to be
//     supplied from the icon or the caller's label.
//
// The factory installed here is consulted by `QAccessible::queryAccessibleInterface`
// and never shadows a role Qt already serves correctly: for an unrecognised
// class it returns nullptr so the platform default still applies.
//
// Registration is idempotent and lazy. `installAccessibleInterfaces()` is safe
// to call any number of times from any thread that may build a widget; it is
// invoked automatically from `MdDesign::initialize()` and from the constructors
// of the widgets that need a custom role, so a host that never calls
// `MdDesign` still gets correct interfaces.

#include "QtMd3Export.h"

namespace md {

/// Register the qt-md3 accessible factory with Qt's accessibility registry.
///
/// Idempotent: calling it again is a no-op. Prefer calling this before the
/// first widget is created (or let `MdDesign::initialize()` do it); widgets
/// that need a custom interface also call it from their constructors.
QT_MD3_EXPORT void installAccessibleInterfaces();

/// True once `installAccessibleInterfaces()` has run. For the test suite.
QT_MD3_EXPORT bool accessibleInterfacesInstalled();

} // namespace md

#endif // MD_ACCESSIBLE_H
