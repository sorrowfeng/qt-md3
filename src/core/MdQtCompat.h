/*
 * Qt 5 / Qt 6 event-API compatibility shims.
 *
 * The library targets Qt 5.15 LTS and Qt 6.x alike, but the pointer-event
 * API changed in Qt 6: `enterEvent` gained the dedicated `QEnterEvent`
 * parameter and `QMouseEvent` / `QHoverEvent` expose the local position
 * through `position()` (Qt 5 spells it `localPos()` / `posF()`). Widgets
 * route every event-coordinate read through this header so a single
 * `#if` block, not each call site, carries the version difference.
 */
#ifndef QT_MD3_MD_QT_COMPAT_H
#define QT_MD3_MD_QT_COMPAT_H

#include <QtGlobal>

#include <QtCore/QEvent>
#include <QtCore/QPointF>
#include <QtGui/QHoverEvent>
#include <QtGui/QMouseEvent>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtGui/QEnterEvent>
#else
#include <QtGui/QCursor>
#include <QtWidgets/QWidget>
#endif

#include "core/QtMd3Export.h"

namespace md {

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)

/// Qt 6 passes `QEnterEvent`; Qt 5 passes a plain `QEvent`.
using MdEnterEvent = QEnterEvent;

/// The local position of a mouse event, both spellings.
inline QPointF mousePosition(const QMouseEvent *event)
{
    return event->position();
}

/// The local position of an enter event, both spellings.
inline QPointF enterPosition(const QWidget * /*widget*/, const QEnterEvent *event)
{
    return event->position();
}

#else // Qt 5

using MdEnterEvent = QEvent;

inline QPointF mousePosition(const QMouseEvent *event)
{
    return event->localPos();
}

/// Qt 5 has no position on the enter event; the cursor's widget mapping is
/// the same value `QEnterEvent::position()` would report.
inline QPointF enterPosition(const QWidget *widget, const QEvent * /*event*/)
{
    return widget->mapFromGlobal(QCursor::pos());
}

#endif

} // namespace md

#endif // QT_MD3_MD_QT_COMPAT_H
