#include "MdWindow.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdIconButton.h"

#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// `md.comp.app-bar.small.container.height` — 64 px.
constexpr int kDefaultTitleBarHeight = 64;
/// `md.comp.icon-button.xsmall.container.height` — 32 px. The three chrome
/// buttons share this track so the title bar reads as one control group.
constexpr int kChromeButtonSide = 32;
/// The gap between chrome buttons. md.comp.icon-button default track leaves
/// the container square; the 0 gap keeps the Windows-style cluster tight.
constexpr int kChromeButtonGap = 0;
/// Edge hit target for window resize, in logical px.
constexpr int kResizeGrip = 8;
/// Horizontal padding before the leading icon / after the trailing buttons.
constexpr int kTitleBarInset = 12;
/// Gap between the leading icon and the title text.
constexpr int kIconTitleGap = 8;

/// Restore glyph: the same `crop_square` the maximize button uses when the
/// window is floating; a "restore" symbol is not in the classic set we ship,
/// so the maximize button swaps to `filter_none` when maximised.
inline QString maximizeGlyph(bool maximized)
{
    return maximized ? QStringLiteral("filter_none") : QStringLiteral("crop_square");
}

} // namespace

MdWindow::MdWindow(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdWindow::~MdWindow() = default;

void MdWindow::init()
{
    setWindowTitle(QStringLiteral(" "));
    setObjectName(QStringLiteral("MdWindow"));
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(240, 160);

    // Frameless when we are (or become) a top-level window. Embedded in a
    // gallery page the flag is irrelevant — the host owns the chrome.
    if (isWindow()) {
        setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        m_platformTopLevel = true;
    }

    m_minimize = new MdIconButton(QStringLiteral("minimize"), this);
    m_minimize->setVariant(IconButtonVariant::Standard);
    m_minimize->setButtonSize(ButtonSize::XSmall);
    m_minimize->setAccessibleName(QStringLiteral("Minimize"));

    m_maximize = new MdIconButton(maximizeGlyph(false), this);
    m_maximize->setVariant(IconButtonVariant::Standard);
    m_maximize->setButtonSize(ButtonSize::XSmall);
    m_maximize->setAccessibleName(QStringLiteral("Maximize"));

    m_close = new MdIconButton(QStringLiteral("close"), this);
    m_close->setVariant(IconButtonVariant::Standard);
    m_close->setButtonSize(ButtonSize::XSmall);
    m_close->setAccessibleName(QStringLiteral("Close"));

    connect(m_minimize, &MdIconButton::clicked, this, &MdWindow::onMinimizeClicked);
    connect(m_maximize, &MdIconButton::clicked, this, &MdWindow::onMaximizeClicked);
    connect(m_close, &MdIconButton::clicked, this, &MdWindow::onCloseClicked);

    // The central widget's focus should leave a trail back to the chrome.
    installEventFilter(this);

    layoutChildren();
    MdStyleBase::connectThemeUpdate(this, &MdWindow::onThemeChanged);
}

void MdWindow::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    QWidget::setWindowTitle(title);
    update();
    emit titleChanged(m_title);
}

void MdWindow::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    update();
    emit iconNameChanged(m_iconName);
}

void MdWindow::setCentralWidget(QWidget *widget)
{
    if (m_central == widget) {
        return;
    }
    if (m_central && m_central->parent() == this && m_central != widget) {
        m_central->hide();
    }
    m_central = widget;
    if (m_central) {
        m_central->setParent(this);
        m_central->show();
    }
    layoutChildren();
    emit centralWidgetChanged(m_central);
}

void MdWindow::setMinimizable(bool on)
{
    if (m_minimizable == on) {
        return;
    }
    m_minimizable = on;
    updateChrome();
    emit minimizableChanged(m_minimizable);
}

void MdWindow::setMaximizable(bool on)
{
    if (m_maximizable == on) {
        return;
    }
    m_maximizable = on;
    updateChrome();
    emit maximizableChanged(m_maximizable);
}

void MdWindow::setClosable(bool on)
{
    if (m_closable == on) {
        return;
    }
    m_closable = on;
    updateChrome();
    emit closableChanged(m_closable);
}

void MdWindow::setResizable(bool on)
{
    if (m_resizable == on) {
        return;
    }
    m_resizable = on;
    emit resizableChanged(m_resizable);
}

void MdWindow::setTitleBarHeight(int height)
{
    const int clamped = qBound(24, height, 160);
    if (m_titleBarHeight == clamped) {
        return;
    }
    m_titleBarHeight = clamped;
    layoutChildren();
    emit titleBarHeightChanged(m_titleBarHeight);
}

void MdWindow::setTitleBarVisible(bool visible)
{
    if (m_titleBarVisible == visible) {
        return;
    }
    m_titleBarVisible = visible;
    layoutChildren();
    emit titleBarVisibleChanged(m_titleBarVisible);
}

void MdWindow::setMaximized(bool maximized)
{
    if (m_maximized == maximized) {
        return;
    }
    m_maximized = maximized;
    m_maximize->setIconName(maximizeGlyph(m_maximized));
    m_maximize->setAccessibleName(m_maximized ? QStringLiteral("Restore") : QStringLiteral("Maximize"));
    if (m_platformTopLevel) {
        if (m_maximized) {
            showMaximized();
        } else {
            showNormal();
        }
    }
    layoutChildren();
    emit maximizedChanged(m_maximized);
}

QList<MdIconButton *> MdWindow::titleBarButtons() const
{
    return QList<MdIconButton *>() << m_minimize << m_maximize << m_close;
}

QRect MdWindow::titleBarRect() const
{
    return QRect(0, 0, width(), m_titleBarVisible ? m_titleBarHeight : 0);
}

void MdWindow::toggleMaximized()
{
    setMaximized(!m_maximized);
}

void MdWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    // Floating windows round; maximised windows fill the screen square.
    const ShapeCorner corner = m_maximized ? ShapeCorner::None : ShapeCorner::ExtraLarge;
    const QList<qreal> radii = MdShape::resolvedRadii(corner, box.size());
    const QPainterPath shape = MdShape::roundedRect(box, radii);

    // `md.sys.color.surface`.
    painter.fillPath(shape, MdTheme::instance().color(ColorRole::Surface));

    if (!m_maximized) {
        // Hairline `md.sys.color.outline-variant` — the tonal-surface
        // language dialogs and cards use instead of a drop shadow.
        QPen pen(MdTheme::instance().color(ColorRole::OutlineVariant));
        pen.setWidthF(1.0);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(shape);
    }

    // Title: title-large / on-surface (app-bar title type).
    if (m_titleBarVisible && !m_title.isEmpty()) {
        qreal textLeft = kTitleBarInset;
        if (!m_iconName.isEmpty()) {
            const QRectF iconRect(textLeft,
                                  (m_titleBarHeight - 24.0) / 2.0,
                                  24.0,
                                  24.0);
            MdIcon::paint(&painter,
                          iconRect,
                          m_iconName,
                          MdTheme::instance().color(ColorRole::OnSurface));
            textLeft += 24.0 + kIconTitleGap;
        }
        const qreal buttonsWidth = 3.0 * (kChromeButtonSide + kChromeButtonGap);
        const QRectF textRect(textLeft,
                              0,
                              qMax(0.0, box.width() - textLeft - buttonsWidth - kTitleBarInset),
                              qreal(m_titleBarHeight));
        painter.setFont(MdTypeScale::font(TypeStyle::TitleLarge));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_title);
    }
}

void MdWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdWindow::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange) {
        const bool maximizedNow = isMaximized() || (windowState() & Qt::WindowMaximized);
        if (m_platformTopLevel && maximizedNow != m_maximized) {
            m_maximized = maximizedNow;
            m_maximize->setIconName(maximizeGlyph(m_maximized));
            emit maximizedChanged(m_maximized);
            layoutChildren();
        }
    } else if (event->type() == QEvent::LayoutDirectionChange) {
        layoutChildren();
        update();
    }
}

void MdWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPointF pos = mousePosition(event);
    const DragMode mode = dragModeAt(pos);
    if (mode != DragMode::None) {
        beginDrag(mode, pos);
        event->accept();
        return;
    }
    // Title-bar drag: move the window.
    if (m_titleBarVisible && pos.y() <= m_titleBarHeight && pos.x() > kTitleBarInset) {
        const int buttonsWidth = 3 * (kChromeButtonSide + kChromeButtonGap);
        if (pos.x() < width() - buttonsWidth - kTitleBarInset) {
            beginDrag(DragMode::Move, pos);
            event->accept();
            return;
        }
    }
    QWidget::mousePressEvent(event);
}

void MdWindow::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    if (m_dragMode != DragMode::None) {
        applyDrag(pos);
        event->accept();
        return;
    }
    // Hover cursor only when we actually manage the window.
    if (m_resizable && (isWindow() || m_platformTopLevel)) {
        switch (dragModeAt(pos)) {
        case DragMode::ResizeLeft:
        case DragMode::ResizeRight:
            setCursor(Qt::SizeHorCursor);
            break;
        case DragMode::ResizeTop:
        case DragMode::ResizeBottom:
            setCursor(Qt::SizeVerCursor);
            break;
        case DragMode::ResizeTopLeft:
        case DragMode::ResizeBottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case DragMode::ResizeTopRight:
        case DragMode::ResizeBottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        default:
            unsetCursor();
            break;
        }
    }
    QWidget::mouseMoveEvent(event);
}

void MdWindow::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragMode != DragMode::None) {
        endDrag();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MdWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    const QPointF pos = mousePosition(event);
    if (m_maximizable && m_titleBarVisible && pos.y() <= m_titleBarHeight) {
        const int buttonsWidth = 3 * (kChromeButtonSide + kChromeButtonGap);
        if (pos.x() > kTitleBarInset && pos.x() < width() - buttonsWidth - kTitleBarInset) {
            toggleMaximized();
            event->accept();
            return;
        }
    }
    QWidget::mouseDoubleClickEvent(event);
}

bool MdWindow::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);
    if (event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        // F11 is the conventional maximize toggle on Windows/Linux.
        if (keyEvent->key() == Qt::Key_F11 && m_maximizable) {
            toggleMaximized();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void MdWindow::onMinimizeClicked()
{
    if (m_platformTopLevel) {
        showMinimized();
    }
}

void MdWindow::onMaximizeClicked()
{
    if (m_maximizable) {
        toggleMaximized();
    }
}

void MdWindow::onCloseClicked()
{
    emit closeRequested();
    if (m_closeOnCloseClick) {
        close();
    }
}

void MdWindow::onThemeChanged()
{
    update();
}

void MdWindow::layoutChildren()
{
    const bool rtl = MdStyleBase::isRtl(this);
    const QRect box = rect();

    // Chrome buttons: trailing cluster, vertically centred in the title bar.
    const int clusterWidth = 3 * (kChromeButtonSide + kChromeButtonGap);
    const int clusterX = rtl ? kTitleBarInset : box.width() - kTitleBarInset - clusterWidth;
    const int clusterY = (m_titleBarHeight - kChromeButtonSide) / 2;
    int x = clusterX;
    // Under RTL the cluster mirrors, so minimize ends up on the right of the
    // three and close on the left — the platform convention there.
    MdIconButton *order[3] = {m_minimize, m_maximize, m_close};
    if (rtl) {
        order[0] = m_close;
        order[1] = m_maximize;
        order[2] = m_minimize;
    }
    for (int i = 0; i < 3; ++i) {
        order[i]->setGeometry(x, clusterY, kChromeButtonSide, kChromeButtonSide);
        x += kChromeButtonSide + kChromeButtonGap;
    }

    if (m_central) {
        const int top = m_titleBarVisible ? m_titleBarHeight : 0;
        m_central->setGeometry(0, top, box.width(), qMax(0, box.height() - top));
        m_central->show();
        m_central->lower();
    }

    updateChrome();
}

void MdWindow::updateChrome()
{
    if (m_minimize) {
        m_minimize->setVisible(m_titleBarVisible && m_minimizable);
        m_minimize->setEnabled(m_minimizable);
    }
    if (m_maximize) {
        m_maximize->setVisible(m_titleBarVisible && m_maximizable);
        m_maximize->setEnabled(m_maximizable);
    }
    if (m_close) {
        m_close->setVisible(m_titleBarVisible && m_closable);
        m_close->setEnabled(m_closable);
    }
    if (m_minimize) {
        m_minimize->raise();
    }
    if (m_maximize) {
        m_maximize->raise();
    }
    if (m_close) {
        m_close->raise();
    }
    update();
}

void MdWindow::syncMaximizedToPlatform()
{
    // Reserved for a future animated restore; today the platform window
    // state and our painted flag are kept in step by changeEvent.
}

MdWindow::DragMode MdWindow::dragModeAt(const QPointF &pos) const
{
    if (!m_resizable || m_maximized || !(isWindow() || m_platformTopLevel)) {
        return DragMode::None;
    }
    const qreal w = width();
    const qreal h = height();
    const bool left = pos.x() <= kResizeGrip;
    const bool right = pos.x() >= w - kResizeGrip;
    const bool top = pos.y() <= kResizeGrip;
    const bool bottom = pos.y() >= h - kResizeGrip;

    if (top && left) {
        return DragMode::ResizeTopLeft;
    }
    if (top && right) {
        return DragMode::ResizeTopRight;
    }
    if (bottom && left) {
        return DragMode::ResizeBottomLeft;
    }
    if (bottom && right) {
        return DragMode::ResizeBottomRight;
    }
    if (left) {
        return DragMode::ResizeLeft;
    }
    if (right) {
        return DragMode::ResizeRight;
    }
    if (top) {
        return DragMode::ResizeTop;
    }
    if (bottom) {
        return DragMode::ResizeBottom;
    }
    return DragMode::None;
}

void MdWindow::beginDrag(DragMode mode, const QPointF &pos)
{
    m_dragMode = mode;
    m_dragOrigin = pos;
    m_dragStartGeometry = geometry();
}

void MdWindow::applyDrag(const QPointF &pos)
{
    if (m_dragMode == DragMode::None) {
        return;
    }
    const QPointF delta = pos - m_dragOrigin;
    QRect g = m_dragStartGeometry;

    switch (m_dragMode) {
    case DragMode::Move:
        g.moveTopLeft(m_dragStartGeometry.topLeft() + delta.toPoint());
        break;
    case DragMode::ResizeLeft:
        g.setLeft(g.left() + delta.toPoint().x());
        break;
    case DragMode::ResizeRight:
        g.setRight(g.right() + delta.toPoint().x());
        break;
    case DragMode::ResizeTop:
        g.setTop(g.top() + delta.toPoint().y());
        break;
    case DragMode::ResizeBottom:
        g.setBottom(g.bottom() + delta.toPoint().y());
        break;
    case DragMode::ResizeTopLeft:
        g.setLeft(g.left() + delta.toPoint().x());
        g.setTop(g.top() + delta.toPoint().y());
        break;
    case DragMode::ResizeTopRight:
        g.setRight(g.right() + delta.toPoint().x());
        g.setTop(g.top() + delta.toPoint().y());
        break;
    case DragMode::ResizeBottomLeft:
        g.setLeft(g.left() + delta.toPoint().x());
        g.setBottom(g.bottom() + delta.toPoint().y());
        break;
    case DragMode::ResizeBottomRight:
        g.setRight(g.right() + delta.toPoint().x());
        g.setBottom(g.bottom() + delta.toPoint().y());
        break;
    case DragMode::None:
        break;
    }

    if (g.width() < minimumWidth()) {
        g.setWidth(minimumWidth());
    }
    if (g.height() < minimumHeight()) {
        g.setHeight(minimumHeight());
    }
    setGeometry(g);
}

void MdWindow::endDrag()
{
    m_dragMode = DragMode::None;
}

QRectF MdWindow::restoredFrame() const
{
    return QRectF(m_dragStartGeometry);
}

} // namespace md
