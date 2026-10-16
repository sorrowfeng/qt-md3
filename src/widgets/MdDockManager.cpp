#include "MdDockManager.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdDockWidget.h"
#include "widgets/MdSplitter.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QResizeEvent>

namespace md {

namespace {

/// Default dock area extent when a widget is first docked (`md.comp.side-sheet`
/// standard width 256; we use the narrower detached reading).
constexpr int kDefaultLeftWidth = 220;
constexpr int kDefaultRightWidth = 220;
constexpr int kDefaultBottomHeight = 160;
/// Guide corner radius (ShapeCorner::Medium = 12 px).
constexpr int kGuideRadius = 12;

} // namespace

MdDockManager::MdDockManager(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDockManager::~MdDockManager()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. Cut the wires first (the MdInputNumber pattern).
    if (m_leftSplitter) {
        disconnect(m_leftSplitter, nullptr, this, nullptr);
    }
    if (m_rightSplitter) {
        disconnect(m_rightSplitter, nullptr, this, nullptr);
    }
    if (m_bottomSplitter) {
        disconnect(m_bottomSplitter, nullptr, this, nullptr);
    }
    for (MdDockWidget *w : m_leftWidgets) {
        if (w) {
            disconnect(w, nullptr, this, nullptr);
        }
    }
    for (MdDockWidget *w : m_rightWidgets) {
        if (w) {
            disconnect(w, nullptr, this, nullptr);
        }
    }
    for (MdDockWidget *w : m_bottomWidgets) {
        if (w) {
            disconnect(w, nullptr, this, nullptr);
        }
    }
}

void MdDockManager::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(QSize(320, 240));
    setAccessibleName(QStringLiteral("dock manager"));

    m_leftSplitter = new MdSplitter(Qt::Vertical, this);
    m_rightSplitter = new MdSplitter(Qt::Vertical, this);
    m_bottomSplitter = new MdSplitter(Qt::Horizontal, this);
    m_leftSplitter->setVisible(false);
    m_rightSplitter->setVisible(false);
    m_bottomSplitter->setVisible(false);

    connect(m_leftSplitter, &MdSplitter::splitterMoved, this, &MdDockManager::onSplitterMoved);
    connect(m_rightSplitter, &MdSplitter::splitterMoved, this, &MdDockManager::onSplitterMoved);
    connect(m_bottomSplitter, &MdSplitter::splitterMoved, this, &MdDockManager::onSplitterMoved);

    MdStyleBase::connectThemeUpdate(this, &MdDockManager::onThemeChanged);
}

void MdDockManager::setCenterWidget(QWidget *widget)
{
    if (m_center == widget) {
        return;
    }
    if (m_center) {
        m_center->setParent(nullptr);
    }
    m_center = widget;
    if (m_center) {
        m_center->setParent(this);
    }
    layoutAreas();
    emit centerWidgetChanged(m_center);
}

void MdDockManager::setLeftWidth(int width)
{
    const int clamped = qMax(0, width);
    if (m_leftWidth == clamped) {
        return;
    }
    m_leftWidth = clamped;
    layoutAreas();
    emit leftWidthChanged(m_leftWidth);
}

void MdDockManager::setRightWidth(int width)
{
    const int clamped = qMax(0, width);
    if (m_rightWidth == clamped) {
        return;
    }
    m_rightWidth = clamped;
    layoutAreas();
    emit rightWidthChanged(m_rightWidth);
}

void MdDockManager::setBottomHeight(int height)
{
    const int clamped = qMax(0, height);
    if (m_bottomHeight == clamped) {
        return;
    }
    m_bottomHeight = clamped;
    layoutAreas();
    emit bottomHeightChanged(m_bottomHeight);
}

void MdDockManager::addDockWidget(MdDockWidget *widget, MdDockArea area)
{
    if (!widget) {
        return;
    }
    widget->setParent(this);
    widget->setArea(area);
    switch (area) {
    case MdDockArea::Left:
        if (!m_leftWidgets.contains(widget)) {
            m_leftWidgets.append(widget);
        }
        if (m_leftWidth == 0) {
            m_leftWidth = kDefaultLeftWidth;
        }
        if (m_leftSplitter) {
            m_leftSplitter->addWidget(widget);
            m_leftSplitter->setVisible(true);
        }
        break;
    case MdDockArea::Right:
        if (!m_rightWidgets.contains(widget)) {
            m_rightWidgets.append(widget);
        }
        if (m_rightWidth == 0) {
            m_rightWidth = kDefaultRightWidth;
        }
        if (m_rightSplitter) {
            m_rightSplitter->addWidget(widget);
            m_rightSplitter->setVisible(true);
        }
        break;
    case MdDockArea::Bottom:
        if (!m_bottomWidgets.contains(widget)) {
            m_bottomWidgets.append(widget);
        }
        if (m_bottomHeight == 0) {
            m_bottomHeight = kDefaultBottomHeight;
        }
        if (m_bottomSplitter) {
            m_bottomSplitter->addWidget(widget);
            m_bottomSplitter->setVisible(true);
        }
        break;
    case MdDockArea::Center:
        setCenterWidget(widget);
        break;
    case MdDockArea::None:
    case MdDockArea::Floating:
    case MdDockArea::Count:
        widget->show();
        break;
    }
    layoutAreas();
    emit dockWidgetAdded(widget, area);
}

void MdDockManager::removeDockWidget(MdDockWidget *widget)
{
    if (!widget) {
        return;
    }
    m_leftWidgets.removeAll(widget);
    m_rightWidgets.removeAll(widget);
    m_bottomWidgets.removeAll(widget);
    if (m_center == widget) {
        m_center = nullptr;
    }
    widget->setParent(nullptr);
    widget->setArea(MdDockArea::Floating);
    syncAreaSizes();
    layoutAreas();
    emit dockWidgetRemoved(widget);
}

QList<MdDockWidget *> MdDockManager::dockWidgets(MdDockArea area) const
{
    switch (area) {
    case MdDockArea::Left:
        return m_leftWidgets;
    case MdDockArea::Right:
        return m_rightWidgets;
    case MdDockArea::Bottom:
        return m_bottomWidgets;
    default:
        break;
    }
    return QList<MdDockWidget *>();
}

void MdDockManager::showGuide(MdDockArea area)
{
    if (area == MdDockArea::None || area == MdDockArea::Count) {
        hideGuide();
        return;
    }
    const bool wasVisible = m_guideVisible;
    m_guideArea = area;
    m_guideVisible = true;
    update();
    if (!wasVisible) {
        emit guideVisibleChanged(true);
    }
}

void MdDockManager::hideGuide()
{
    if (!m_guideVisible) {
        return;
    }
    m_guideVisible = false;
    m_guideArea = MdDockArea::None;
    update();
    emit guideVisibleChanged(false);
}

QColor MdDockManager::resolvedGuideColor() const
{
    // md.sys.color.secondary-container — the dock-guide highlight.
    return MdTheme::instance().color(ColorRole::SecondaryContainer);
}

QSize MdDockManager::sizeHint() const
{
    return QSize(800, 600);
}

QRect MdDockManager::areaRect(MdDockArea area) const
{
    const QRectF bounds = QRectF(rect());
    qreal left = bounds.left();
    qreal right = bounds.right();
    qreal top = bounds.top();
    qreal bottom = bounds.bottom();

    if (m_leftWidth > 0 && m_leftWidgets.size() > 0) {
        if (area == MdDockArea::Left) {
            return QRect(qRound(left), qRound(top), m_leftWidth, qRound(bounds.height()));
        }
        left += m_leftWidth;
    }
    if (m_rightWidth > 0 && m_rightWidgets.size() > 0) {
        if (area == MdDockArea::Right) {
            return QRect(qRound(right - m_rightWidth), qRound(top),
                         m_rightWidth, qRound(bounds.height()));
        }
        right -= m_rightWidth;
    }
    if (m_bottomHeight > 0 && m_bottomWidgets.size() > 0) {
        if (area == MdDockArea::Bottom) {
            return QRect(qRound(left), qRound(bottom - m_bottomHeight),
                         qRound(right - left), m_bottomHeight);
        }
        bottom -= m_bottomHeight;
    }
    if (area == MdDockArea::Center) {
        return QRect(qRound(left), qRound(top),
                     qRound(right - left), qRound(bottom - top));
    }
    return QRect();
}

void MdDockManager::layoutAreas()
{
    if (m_leftSplitter) {
        m_leftSplitter->setGeometry(areaRect(MdDockArea::Left));
    }
    if (m_rightSplitter) {
        m_rightSplitter->setGeometry(areaRect(MdDockArea::Right));
    }
    if (m_bottomSplitter) {
        m_bottomSplitter->setGeometry(areaRect(MdDockArea::Bottom));
    }
    if (m_center) {
        m_center->setGeometry(areaRect(MdDockArea::Center));
        m_center->show();
    }
}

void MdDockManager::syncAreaSizes()
{
    if (m_leftWidgets.isEmpty()) {
        m_leftWidth = 0;
    }
    if (m_rightWidgets.isEmpty()) {
        m_rightWidth = 0;
    }
    if (m_bottomWidgets.isEmpty()) {
        m_bottomHeight = 0;
    }
    if (m_leftSplitter) {
        m_leftSplitter->setVisible(!m_leftWidgets.isEmpty());
    }
    if (m_rightSplitter) {
        m_rightSplitter->setVisible(!m_rightWidgets.isEmpty());
    }
    if (m_bottomSplitter) {
        m_bottomSplitter->setVisible(!m_bottomWidgets.isEmpty());
    }
}

void MdDockManager::onSplitterMoved()
{
    // Read the live sizes back off the splitters so the properties stay honest.
    if (m_leftSplitter && m_leftSplitter->isVisible()) {
        m_leftWidth = m_leftSplitter->width();
    }
    if (m_rightSplitter && m_rightSplitter->isVisible()) {
        m_rightWidth = m_rightSplitter->width();
    }
    if (m_bottomSplitter && m_bottomSplitter->isVisible()) {
        m_bottomHeight = m_bottomSplitter->height();
    }
}

void MdDockManager::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (!m_guideVisible || m_guideArea == MdDockArea::None) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect target = areaRect(m_guideArea);
    if (target.isEmpty()) {
        return;
    }

    // Dock guides use shape highlight (appendix D).
    const QRectF guide = QRectF(target).adjusted(4.0, 4.0, -4.0, -4.0);
    const QList<qreal> radii = MdShape::resolvedRadii(ShapeCorner::Medium, guide.size());
    const QPainterPath path = MdShape::roundedRect(guide, radii);

    QColor fill = resolvedGuideColor();
    fill.setAlphaF(0.55);
    painter.fillPath(path, fill);
}

void MdDockManager::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutAreas();
}

void MdDockManager::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        layoutAreas();
        update();
    }
}

void MdDockManager::onThemeChanged()
{
    update();
}

} // namespace md
