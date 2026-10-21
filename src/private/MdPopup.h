// MdPopup.h — internal Qt::Popup show helpers.
//
// A Qt::Popup maps its native window in show() *before* any paint can run.
// On Windows that window is composited for a frame with the class background
// (white/black) or an uninitialised buffer — the flash users see when a menu
// or combo box opens. Three things close the gap:
//   * WA_NoSystemBackground stops the system from filling the window,
//   * WA_TranslucentBackground lets the rounded surface and shadow show through,
//   * the native window is held at opacity 0 until the synchronous first
//     paint has landed, so the compositor never samples an empty layer.
//
// Geometry must be final (resized and moved) before this is called: the
// window is mapped once and must not jump afterwards.

#ifndef MD_PRIVATE_POPUP_H
#define MD_PRIVATE_POPUP_H

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QPointer>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QPixmap>
#include <QtGui/QScreen>
#include <QtGui/QWindow>
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
    // Keep the popup's native window from forcing native ancestors: a
    // full-window native child HWND becomes the Win32 hit target and can
    // swallow real mouse input after the popup closes.
    popup->setAttribute(Qt::WA_DontCreateNativeAncestors, true);
    // Windows 11 draws a DWM drop shadow around every WS_POPUP. On a
    // frameless translucent surface that shadow reads as a black edge /
    // residue around the rounded corners (especially while the open scale is
    // under 1 and the window rect is larger than the surface). NoDropShadow
    // turns the compositor shadow off; Frameless keeps the popup borderless.
    popup->setWindowFlags(popup->windowFlags() | Qt::FramelessWindowHint
                          | Qt::NoDropShadowWindowHint);
    // Create the platform window now so show() maps an already-configured
    // window rather than creating + mapping + painting in one step.
    popup->winId();
    // Hold the native window invisible until the first paint is in the
    // backing store. show() maps immediately; the compositor can sample the
    // empty layer for a frame before repaint() flushes, which is the flash
    // WA_TranslucentBackground alone does not remove on Windows.
    QWindow *handle = popup->windowHandle();
    const qreal restoreOpacity = handle ? handle->opacity() : 1.0;
    if (handle) {
        handle->setOpacity(0.0);
    }
    popup->show();
    popup->raise();
    // Synchronous first paint: without this the native window is visible
    // while the backing store is still empty (the blank/default frame).
    // Widgets that animate from a closed state must have seeded their
    // openProgress/openAlpha *before* this call so the frame that lands is
    // the flight's first real image, not a zero-opacity rectangle.
    popup->repaint();
    if (handle) {
        handle->setOpacity(restoreOpacity);
    }
}

/// Optional frame dump for popup open flights. Set QT_MD3_POPUP_FRAMES to a
/// directory and the helper writes `<tag>-t<ms>.png` (widget grab) plus
/// `<tag>-t<ms>-screen.png` (compositor grab) every 8 ms from before the map
/// to +100 ms, so a blank / unstyled / double-window frame can be pinned to
/// a timestamp. No-op when the variable is unset — zero cost in normal runs.
class PopupFrameDumper : public QObject
{
public:
    explicit PopupFrameDumper(QWidget *popup, const QString &tag, QObject *parent = nullptr)
        : QObject(parent)
        , m_popup(popup)
        , m_tag(tag)
    {
        const QString dir = qEnvironmentVariable("QT_MD3_POPUP_FRAMES");
        if (dir.isEmpty() || popup == nullptr) {
            return;
        }
        m_dir = dir;
        QDir().mkpath(m_dir);
        m_clock.start();
        // t=-1: the widget as it stands before the map.
        grab(QStringLiteral("pre"));
        auto *timer = new QTimer(this);
        timer->setInterval(8);
        timer->setTimerType(Qt::PreciseTimer);
        connect(timer, &QTimer::timeout, this, [this, timer] {
            const qint64 ms = m_clock.elapsed();
            if (ms > 100) {
                timer->stop();
                // Self-delete when parented to the popup so a re-open does
                // not stack dumpers.
                deleteLater();
                return;
            }
            grab(QStringLiteral("t%1").arg(ms));
        });
        timer->start();
    }

private:
    void grab(const QString &label)
    {
        if (m_popup.isNull()) {
            return;
        }
        const QString stem = QDir(m_dir).filePath(m_tag + QLatin1Char('-') + label);
        const QPixmap shot = m_popup->grab();
        if (!shot.isNull()) {
            shot.save(stem + QStringLiteral(".png"));
        }
        if (QScreen *screen = QGuiApplication::primaryScreen()) {
            const QPixmap screenShot = screen->grabWindow(0);
            if (!screenShot.isNull()) {
                screenShot.save(stem + QStringLiteral("-screen.png"));
            }
        }
    }

    QPointer<QWidget> m_popup;
    QString m_tag;
    QString m_dir;
    QElapsedTimer m_clock;
};

} // namespace md

#endif // MD_PRIVATE_POPUP_H
