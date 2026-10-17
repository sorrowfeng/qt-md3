#include "MdDrawer.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QTimer>
#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

constexpr qreal kHeadlineHeight = 56.0;
constexpr qreal kHeadlinePad = 16.0;
constexpr int kAnimBudgetMs = 400;
/// The navigation drawer's headline style: `title-small`.
constexpr TypeStyle kHeadlineStyle = TypeStyle::TitleSmall;

} // namespace

MdDrawer::MdDrawer(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDrawer::~MdDrawer()
{
    disconnect(this, nullptr, nullptr, nullptr);
    if (m_timer) {
        m_timer->stop();
        disconnect(m_timer, nullptr, nullptr, nullptr);
        delete m_timer;
        m_timer = nullptr;
    }
}

void MdDrawer::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("drawer"));

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &MdDrawer::onAnimationFrame);

    MdStyleBase::connectThemeUpdate(this, &MdDrawer::onThemeChanged);
}

void MdDrawer::setEdge(MdDrawerEdge edge)
{
    if (edge >= MdDrawerEdge::Count || m_edge == edge) {
        return;
    }
    m_edge = edge;
    relayout();
    update();
    emit edgeChanged(m_edge);
}

void MdDrawer::setKind(MdDrawerKind kind)
{
    if (kind >= MdDrawerKind::Count || m_kind == kind) {
        return;
    }
    m_kind = kind;
    update();
    emit kindChanged(m_kind);
}

void MdDrawer::setOpen(bool open)
{
    if (m_open == open) {
        return;
    }
    m_open = open;
    startAnimation(open);
    emit openChanged(m_open);
}

void MdDrawer::open()
{
    setOpen(true);
}

void MdDrawer::close()
{
    setOpen(false);
}

void MdDrawer::toggle()
{
    setOpen(!m_open);
}

void MdDrawer::setPanelSize(int size)
{
    const int clamped = qBound(96, size, 640);
    if (m_panelSize == clamped) {
        return;
    }
    m_panelSize = clamped;
    relayout();
    updateGeometry();
    update();
    emit panelSizeChanged(m_panelSize);
}

void MdDrawer::setHeadline(const QString &headline)
{
    if (m_headline == headline) {
        return;
    }
    m_headline = headline;
    relayout();
    update();
    emit headlineChanged(m_headline);
}

void MdDrawer::setContentWidget(QWidget *content)
{
    if (m_content == content) {
        return;
    }
    if (m_content && m_content->parent() == this) {
        m_content->setVisible(false);
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
    }
    relayout();
    emit contentWidgetChanged(m_content);
}

ColorRole MdDrawer::scrimColor() const
{
    // The navigation-drawer export's scrim row resolves to Scrim; the library
    // publishes it as the neutral overlay. Record kept in porting-todo.
    return ColorRole::Scrim;
}

qreal MdDrawer::scrimOpacity() const
{
    return m_kind == MdDrawerKind::Modal ? 0.32 : 0.0;
}

QColor MdDrawer::resolvedSurfaceColor() const
{
    const MdTheme &theme = MdTheme::instance();
    return m_kind == MdDrawerKind::Modal ? theme.color(ColorRole::SurfaceContainerLow)
                                         : theme.color(ColorRole::Surface);
}

bool MdDrawer::isMirrored() const
{
    return layoutDirection() == Qt::RightToLeft;
}

QRectF MdDrawer::panelRect() const
{
    const bool horizontal = m_edge == MdDrawerEdge::Start || m_edge == MdDrawerEdge::End;
    const qreal w = width();
    const qreal h = height();
    const qreal extent = horizontal ? m_panelSize : m_panelSize;
    const qreal t = m_progress;

    switch (m_edge) {
    case MdDrawerEdge::Start: {
        const bool rtl = isMirrored();
        const qreal resting = rtl ? w - extent : 0.0;
        const qreal hidden = rtl ? w : -extent;
        const qreal x = hidden + (resting - hidden) * t;
        return QRectF(x, 0, extent, h);
    }
    case MdDrawerEdge::End: {
        const bool rtl = isMirrored();
        const qreal resting = rtl ? 0.0 : w - extent;
        const qreal hidden = rtl ? -extent : w;
        const qreal x = hidden + (resting - hidden) * t;
        return QRectF(x, 0, extent, h);
    }
    case MdDrawerEdge::Top: {
        const qreal y = -extent + extent * t;
        return QRectF(0, y, w, extent);
    }
    case MdDrawerEdge::Bottom: {
        const qreal y = h - extent * t;
        return QRectF(0, y, w, extent);
    }
    case MdDrawerEdge::Count:
        break;
    }
    return QRectF();
}

QSize MdDrawer::sizeHint() const
{
    return QSize(m_panelSize * 2, m_panelSize);
}

QSize MdDrawer::minimumSizeHint() const
{
    return QSize(96, 96);
}

void MdDrawer::startAnimation(bool opening)
{
    m_targetOpen = opening;
    m_clock.restart();
    m_running = true;
    if (m_timer) {
        m_timer->start();
    }
}

void MdDrawer::onAnimationFrame()
{
    const qreal seconds = m_clock.elapsed() / 1000.0;
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal raw = spring.valueAt(seconds);
    // Progress is the open fraction; closing inverts the spring displacement.
    const qreal target = m_targetOpen ? 1.0 : 0.0;
    const qreal start = m_targetOpen ? 0.0 : 1.0;
    m_progress = start + (target - start) * qBound(0.0, raw, 1.0);

    if (m_clock.elapsed() >= kAnimBudgetMs || qFuzzyCompare(m_progress, target)) {
        m_progress = target;
        m_running = false;
        if (m_timer) {
            m_timer->stop();
        }
    }
    relayout();
    update();
}

void MdDrawer::relayout()
{
    const QRectF panel = panelRect();
    if (m_content) {
        const qreal top = m_headline.isEmpty() ? 0.0 : kHeadlineHeight;
        m_content->setGeometry(QRectF(panel.left(), panel.top() + top, panel.width(),
                                      panel.height() - top)
                                   .toRect());
        m_content->setVisible(m_open || m_running);
    }
}

void MdDrawer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (m_kind == MdDrawerKind::Modal && (m_open || m_running)) {
        QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
        scrim.setAlphaF(scrimOpacity() * m_progress);
        painter.fillRect(QRectF(0, 0, width(), height()), scrim);
    }

    const QRectF panel = panelRect();
    if (panel.isEmpty()) {
        return;
    }

    const QPainterPath path = MdShape::roundedRect(panel, ShapeCorner::None);
    painter.fillPath(path, resolvedSurfaceColor());

    if (!m_headline.isEmpty()) {
        painter.setFont(MdTypeScale::font(kHeadlineStyle));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        const QRectF box(panel.left() + kHeadlinePad, panel.top(),
                         panel.width() - 2 * kHeadlinePad, kHeadlineHeight);
        painter.drawText(box, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_headline);
    }
}

void MdDrawer::mousePressEvent(QMouseEvent *event)
{
    // Modal: clicking the scrim closes the drawer.
    if (m_kind == MdDrawerKind::Modal && m_open && !panelRect().contains(md::mousePosition(event))) {
        close();
        return;
    }
    QWidget::mousePressEvent(event);
}

void MdDrawer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdDrawer::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::FontChange) {
        relayout();
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdDrawer::onThemeChanged()
{
    update();
}

} // namespace md
