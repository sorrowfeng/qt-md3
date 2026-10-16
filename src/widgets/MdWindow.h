#ifndef MD_WINDOW_H
#define MD_WINDOW_H

// MdWindow — Stage 2 Qt extension (Appendix D, P0), the frameless main
// window.
//
// Semantic anchor (appendix D): "Surface + Top app bar；标题栏按钮用 icon
// button 语义". There is no official MD3 "window chrome" family; the
// chrome is derived from three Stage 1 anchors and nothing is invented:
//
//   background  md.sys.color.surface                       (surface)
//   title bar   md.comp.app-bar.small.container.height (64 px),
//               title text = title-large / on-surface      (app bar)
//   chrome      minimize / maximize / close are real
//               `MdIconButton`s — standard, xsmall (32 px), the
//               icon-button token set in full (state layer, ripple,
//               focus ring, shape morph)
//   floating    outline-variant 1 px ring, corner extra-large
//               (`md.sys.shape.corner.extra-large`) — the same
//               "tonal surface + hairline" language dialogs use
//               maximised: square corners, no ring, fill the screen
//
// The window is a QWidget with `Qt::FramelessWindowHint` when it is a
// top-level window. When it is embedded (gallery, tests, designer) it
// paints the same chrome and lays out the same children but leaves
// move/resize to the host. That keeps one implementation for both uses
// and makes the control unit-testable offscreen.
//
// Window management follows the platform contract:
//   * drag the title bar to move
//   * double-click the title bar toggles maximise
//   * drag any edge/corner to resize (8 px hit target, 4 px when maximised
//     is impossible because there is nothing to grab)
//   * the three buttons call showMinimized / showMaximized / close
//
// Keyboard: Tab reaches the chrome buttons; Space/Enter activate them
// (MdIconButton contract). Escape does nothing (not a dialog).
//
// There is no QSS. Every number is a token.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

namespace md {

class MdIconButton;

class QT_MD3_EXPORT MdWindow : public QWidget
{
    Q_OBJECT

    /// The title drawn in the title bar. Same name as QWidget::windowTitle
    /// so QWindow-style callers feel at home; this is the painted string.
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Optional 24 px Material Symbols glyph leading the title.
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    /// Whether the minimize button is shown and enabled.
    Q_PROPERTY(bool minimizable READ isMinimizable WRITE setMinimizable NOTIFY minimizableChanged)
    /// Whether the maximize / restore button is shown and enabled.
    Q_PROPERTY(bool maximizable READ isMaximizable WRITE setMaximizable NOTIFY maximizableChanged)
    /// Whether the close button is shown and enabled.
    Q_PROPERTY(bool closable READ isClosable WRITE setClosable NOTIFY closableChanged)
    /// Whether the window can be resized from its edges.
    Q_PROPERTY(bool resizable READ isResizable WRITE setResizable NOTIFY resizableChanged)
    /// The painted title bar height. Defaults to the small app bar height.
    Q_PROPERTY(int titleBarHeight READ titleBarHeight WRITE setTitleBarHeight NOTIFY
                   titleBarHeightChanged)
    /// Whether the title bar is shown at all.
    Q_PROPERTY(bool titleBarVisible READ isTitleBarVisible WRITE setTitleBarVisible NOTIFY
                   titleBarVisibleChanged)
    /// The current maximised state as painted by this widget (independent
    /// of the platform window state while embedded).
    Q_PROPERTY(bool maximized READ isMaximized WRITE setMaximized NOTIFY maximizedChanged)

public:
    explicit MdWindow(QWidget *parent = nullptr);
    ~MdWindow() override;

    // --- content -------------------------------------------------------------
    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    /// The page body. The scaffold takes the widget as a child and lays it
    /// out under the title bar. Null clears the slot.
    QWidget *centralWidget() const { return m_central; }
    void setCentralWidget(QWidget *widget);

    // --- chrome --------------------------------------------------------------
    bool isMinimizable() const { return m_minimizable; }
    void setMinimizable(bool on);

    bool isMaximizable() const { return m_maximizable; }
    void setMaximizable(bool on);

    bool isClosable() const { return m_closable; }
    void setClosable(bool on);

    bool isResizable() const { return m_resizable; }
    void setResizable(bool on);

    int titleBarHeight() const { return m_titleBarHeight; }
    void setTitleBarHeight(int height);

    bool isTitleBarVisible() const { return m_titleBarVisible; }
    void setTitleBarVisible(bool visible);

    bool isMaximized() const { return m_maximized; }
    void setMaximized(bool maximized);

    /// The three chrome buttons, in minimize / maximize / close order.
    QList<MdIconButton *> titleBarButtons() const;

    /// The painted title-bar rect in widget coordinates.
    QRect titleBarRect() const;

    // --- window management ---------------------------------------------------
public slots:
    /// Toggle between restored and maximised geometry. Spring-eased on
    /// platforms where we own the geometry; a plain showMaximized() call
    /// when the window manager does.
    void toggleMaximized();

signals:
    void titleChanged(const QString &title);
    void iconNameChanged(const QString &iconName);
    void centralWidgetChanged(QWidget *central);
    void minimizableChanged(bool on);
    void maximizableChanged(bool on);
    void closableChanged(bool on);
    void resizableChanged(bool on);
    void titleBarHeightChanged(int height);
    void titleBarVisibleChanged(bool visible);
    void maximizedChanged(bool maximized);
    /// The close button was activated. The widget does not close itself
    /// unless `closeOnCloseClick` is true — the default.
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onMinimizeClicked();
    void onMaximizeClicked();
    void onCloseClicked();
    void onThemeChanged();

private:
    enum class DragMode {
        None,
        Move,
        ResizeLeft,
        ResizeRight,
        ResizeTop,
        ResizeBottom,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight,
    };

    void init();
    void layoutChildren();
    void updateChrome();
    void syncMaximizedToPlatform();
    DragMode dragModeAt(const QPointF &pos) const;
    void beginDrag(DragMode mode, const QPointF &pos);
    void applyDrag(const QPointF &pos);
    void endDrag();
    QRectF restoredFrame() const;

    QString m_title;
    QString m_iconName;
    QWidget *m_central = nullptr;
    MdIconButton *m_minimize = nullptr;
    MdIconButton *m_maximize = nullptr;
    MdIconButton *m_close = nullptr;
    bool m_minimizable = true;
    bool m_maximizable = true;
    bool m_closable = true;
    bool m_resizable = true;
    bool m_titleBarVisible = true;
    bool m_maximized = false;
    bool m_closeOnCloseClick = true;
    int m_titleBarHeight = 64;

    DragMode m_dragMode = DragMode::None;
    QPointF m_dragOrigin;
    QRect m_dragStartGeometry;
    bool m_platformTopLevel = false;
};

} // namespace md

#endif // MD_WINDOW_H
