#include "MdStackedWidget.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QTimer>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// How long the effects-fast spring is allowed to run before snapping shut.
constexpr int kTransitionBudgetMs = 400;

} // namespace

MdStackedWidget::MdStackedWidget(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdStackedWidget::~MdStackedWidget()
{
    // Cut theme / timer wires before members die (the MdInputNumber pattern).
    disconnect(this, nullptr, nullptr, nullptr);
    if (m_timer) {
        m_timer->stop();
        disconnect(m_timer, nullptr, nullptr, nullptr);
        delete m_timer;
        m_timer = nullptr;
    }
}

void MdStackedWidget::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("stacked widget"));

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &MdStackedWidget::onAnimationFrame);

    MdStyleBase::connectThemeUpdate(this, &MdStackedWidget::onThemeChanged);
}

int MdStackedWidget::addWidget(QWidget *page)
{
    if (!page) {
        return -1;
    }
    page->setParent(this);
    m_pages.append(page);
    if (m_currentIndex < 0) {
        m_currentIndex = 0;
        page->setVisible(true);
        emit currentIndexChanged(m_currentIndex);
    } else {
        page->setVisible(false);
    }
    relayout();
    emit countChanged(count());
    emit widgetAdded(int(m_pages.size()) - 1);
    return int(m_pages.size()) - 1;
}

int MdStackedWidget::insertWidget(int index, QWidget *page)
{
    if (!page) {
        return -1;
    }
    if (index < 0 || index > int(m_pages.size())) {
        return addWidget(page);
    }
    page->setParent(this);
    m_pages.insert(index, page);
    if (m_currentIndex >= index) {
        ++m_currentIndex;
    } else if (m_currentIndex < 0) {
        m_currentIndex = 0;
        emit currentIndexChanged(m_currentIndex);
    }
    for (int i = 0; i < m_pages.size(); ++i) {
        m_pages.at(i)->setVisible(i == m_currentIndex);
    }
    relayout();
    emit countChanged(count());
    emit widgetAdded(index);
    return index;
}

void MdStackedWidget::removeWidget(QWidget *page)
{
    const int index = m_pages.indexOf(page);
    if (index < 0) {
        return;
    }
    m_pages.removeAt(index);
    if (page->parent() == this) {
        page->setParent(nullptr);
    }
    if (m_outgoing == page) {
        m_outgoing = nullptr;
    }
    if (m_currentIndex == index) {
        m_currentIndex = qMin(index, int(m_pages.size()) - 1);
        if (m_currentIndex >= 0 && m_currentIndex < int(m_pages.size())) {
            m_pages.at(m_currentIndex)->setVisible(true);
        }
        emit currentIndexChanged(m_currentIndex);
    } else if (m_currentIndex > index) {
        --m_currentIndex;
    }
    relayout();
    emit countChanged(count());
    emit widgetRemoved(index);
}

QWidget *MdStackedWidget::widget(int index) const
{
    if (index < 0 || index >= int(m_pages.size())) {
        return nullptr;
    }
    return m_pages.at(index);
}

int MdStackedWidget::indexOf(QWidget *page) const
{
    return m_pages.indexOf(page);
}

void MdStackedWidget::clear()
{
    if (m_pages.isEmpty()) {
        return;
    }
    const QList<QWidget *> pages = m_pages;
    m_pages.clear();
    m_outgoing = nullptr;
    m_running = false;
    if (m_timer) {
        m_timer->stop();
    }
    m_currentIndex = -1;
    for (QWidget *page : pages) {
        if (page->parent() == this) {
            page->setParent(nullptr);
        }
    }
    emit countChanged(0);
    emit currentIndexChanged(m_currentIndex);
}

void MdStackedWidget::setCurrentIndex(int index)
{
    if (m_pages.isEmpty()) {
        if (m_currentIndex != -1) {
            m_currentIndex = -1;
            emit currentIndexChanged(m_currentIndex);
        }
        return;
    }
    const int clamped = qBound(0, index, int(m_pages.size()) - 1);
    if (m_currentIndex == clamped) {
        return;
    }

    QWidget *previous = currentWidget();
    m_currentIndex = clamped;

    if (m_animated && previous && previous->isVisible()) {
        m_outgoing = previous;
        startTransition();
    } else {
        for (int i = 0; i < m_pages.size(); ++i) {
            m_pages.at(i)->setVisible(i == m_currentIndex);
        }
        m_progress = 1.0;
        m_running = false;
    }

    QWidget *current = currentWidget();
    if (current) {
        current->setVisible(true);
        current->raise();
    }
    relayout();
    update();
    emit currentIndexChanged(m_currentIndex);
}

QWidget *MdStackedWidget::currentWidget() const
{
    return widget(m_currentIndex);
}

void MdStackedWidget::setCurrentWidget(QWidget *page)
{
    setCurrentIndex(indexOf(page));
}

void MdStackedWidget::setAnimated(bool on)
{
    if (m_animated == on) {
        return;
    }
    m_animated = on;
    if (!m_animated && m_running) {
        m_running = false;
        m_progress = 1.0;
        if (m_timer) {
            m_timer->stop();
        }
        if (m_outgoing) {
            m_outgoing->setVisible(false);
            m_outgoing = nullptr;
        }
        update();
    }
    emit animatedChanged(m_animated);
}

QColor MdStackedWidget::resolvedSurfaceColor() const
{
    return MdTheme::instance().color(ColorRole::Surface);
}

QSize MdStackedWidget::sizeHint() const
{
    if (QWidget *page = currentWidget()) {
        return page->sizeHint();
    }
    return QSize(240, 180);
}

QSize MdStackedWidget::minimumSizeHint() const
{
    if (QWidget *page = currentWidget()) {
        return page->minimumSizeHint();
    }
    return QSize(96, 72);
}

qreal MdStackedWidget::transitionProgress() const
{
    return m_progress;
}

void MdStackedWidget::startTransition()
{
    m_clock.restart();
    m_running = true;
    m_progress = 0.0;
    if (m_timer) {
        m_timer->start();
    }
}

void MdStackedWidget::updateTransitionState()
{
    const qreal seconds = m_clock.elapsed() / 1000.0;
    const MdSpring spring = MdMotion::spring(MotionSpring::EffectsFast);
    const qreal raw = spring.valueAt(seconds);
    m_progress = qBound(0.0, raw, 1.0);

    if (m_clock.elapsed() >= kTransitionBudgetMs || m_progress >= 0.999) {
        m_progress = 1.0;
        m_running = false;
        if (m_timer) {
            m_timer->stop();
        }
        if (m_outgoing) {
            m_outgoing->setVisible(false);
            m_outgoing = nullptr;
        }
    }

    if (currentWidget()) {
        currentWidget()->setUpdatesEnabled(true);
    }
    update();
}

void MdStackedWidget::onAnimationFrame()
{
    updateTransitionState();
}

void MdStackedWidget::relayout()
{
    const QRectF bounds(0, 0, width(), height());
    for (QWidget *page : m_pages) {
        page->setGeometry(bounds.toRect());
    }
}

void MdStackedWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds(0, 0, width(), height());
    const QPainterPath path = MdShape::roundedRect(bounds, ShapeCorner::Medium);
    painter.fillPath(path, resolvedSurfaceColor());
}

void MdStackedWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdStackedWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        relayout();
        update();
    }
    QWidget::changeEvent(event);
}

void MdStackedWidget::onThemeChanged()
{
    update();
}

} // namespace md
