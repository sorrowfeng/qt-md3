// MdPopup.h — internal Qt::Popup show helpers.
//
// A Qt::Popup maps its native window in show() *before* any paint can run.
// On Windows that window is composited for a frame with the class background
// (white/black) or an uninitialised buffer — the flash users see when a menu
// or combo box opens. Two attributes and a synchronous first paint close the
// gap: WA_NoSystemBackground stops the system from filling the window,
// WA_TranslucentBackground lets the rounded surface and shadow show through,
// and repaint() fills the backing store before the call returns to the caller.
//
// Geometry must be final (resized and moved) before this is called: the
// window is mapped once and must not jump afterwards.

#ifndef MD_PRIVATE_POPUP_H
#define MD_PRIVATE_POPUP_H

#include <QtWidgets/QWidget>

namespace md {

/// Maps `popup` without the first-frame flash. The widget must already carry
/// `Qt::Popup` (or `Qt::ToolTip`) window flags and its final geometry.
inline void showPopupWithoutFlash(QWidget *popup)
{
    // Both attributes must be set before the platform window is created;
    // setting them after the first show leaves the system background in the
    // first composite.
    popup->setAttribute(Qt::WA_TranslucentBackground, true);
    popup->setAttribute(Qt::WA_NoSystemBackground, true);
    // Create the platform window now so show() maps an already-configured
    // window rather than creating + mapping + painting in one step.
    popup->winId();
    popup->show();
    popup->raise();
    // Synchronous first paint: without this the native window is visible
    // while the backing store is still empty (the blank/default frame).
    // Widgets that animate from a closed state must have seeded their
    // openProgress/openAlpha *before* this call so the frame that lands is
    // the flight's first real image, not a zero-opacity rectangle.
    popup->repaint();
}

} // namespace md

#endif // MD_PRIVATE_POPUP_H
