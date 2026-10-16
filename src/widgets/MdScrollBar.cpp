#include "MdScrollBar.h"

#include "styles/MdScrollBarStyle.h"

#include "core/MdTheme.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>

namespace md {

namespace {

/// Thickness bounds: the overlay pill stays thin enough not to eat content
/// but wide enough to see. Compose Desktop defaults to 8.dp.
constexpr int kMinThickness = 4;
constexpr int kMaxThickness = 24;
constexpr int kDefaultThickness = 8;

} // namespace

MdScrollBar::MdScrollBar(QWidget *parent)
    : QScrollBar(parent)
{
    init();
}

MdScrollBar::MdScrollBar(Qt::Orientation orientation, QWidget *parent)
    : QScrollBar(orientation, parent)
{
    init();
}

MdScrollBar::~MdScrollBar() = default;

void MdScrollBar::init()
{
    setStyle(MdScrollBarStyle::shared());
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    pushMetricsToStyle();
}

void MdScrollBar::pushMetricsToStyle()
{
    // The style reads thickness/visibleMode off the widget at paint time
    // (see overlayWidthFor / chromeSuppressed); a repaint is all that is
    // needed after either property changes.
    update();
    updateGeometry();
}

void MdScrollBar::setThickness(int thickness)
{
    const int clamped = qBound(kMinThickness, thickness, kMaxThickness);
    if (m_thickness == clamped) {
        return;
    }
    m_thickness = clamped;
    pushMetricsToStyle();
    emit thicknessChanged(m_thickness);
}

void MdScrollBar::setVisibleMode(MdScrollBarVisibility mode)
{
    if (m_visibleMode == mode) {
        return;
    }
    m_visibleMode = mode;
    pushMetricsToStyle();
    emit visibleModeChanged(m_visibleMode);
}

void MdScrollBar::setLifted(bool lifted)
{
    if (m_lifted == lifted) {
        return;
    }
    m_lifted = lifted;
    emit liftedChanged(m_lifted);
}

QSize MdScrollBar::sizeHint() const
{
    const int extent = qMax(MdScrollBarStyle::hitExtent(), m_thickness);
    if (orientation() == Qt::Vertical) {
        return QSize(extent, 48);
    }
    return QSize(48, extent);
}

QSize MdScrollBar::minimumSizeHint() const
{
    return sizeHint();
}

void MdScrollBar::enterEvent(MdEnterEvent *event)
{
    m_hovered = true;
    setLifted(true);
    QScrollBar::enterEvent(event);
    update();
}

void MdScrollBar::leaveEvent(QEvent *event)
{
    m_hovered = false;
    setLifted(m_dragging);
    QScrollBar::leaveEvent(event);
    update();
}

void MdScrollBar::mousePressEvent(QMouseEvent *event)
{
    QScrollBar::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        setLifted(true);
        update();
    }
}

void MdScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    QScrollBar::mouseReleaseEvent(event);
    m_dragging = false;
    setLifted(m_hovered);
    update();
}

void MdScrollBar::paintEvent(QPaintEvent *event)
{
    if (m_visibleMode == MdScrollBarVisibility::Never) {
        return;
    }
    QScrollBar::paintEvent(event);
}

void MdScrollBar::changeEvent(QEvent *event)
{
    QScrollBar::changeEvent(event);
    if (event->type() == QEvent::StyleChange || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::FontChange) {
        update();
    }
}

} // namespace md
