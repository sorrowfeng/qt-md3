#include "MdStackedWidget.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QTimer>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QGraphicsOpacityEffect>

namespace md {

namespace {

/// How long the effects-fast spring is allowed to run before snapping shut.
constexpr int kTransitionBudgetMs = 400;

QGraphicsOpacityEffect *ensureFadeEffect(QWidget *page)
{
    if (page == nullptr) {
        return nullptr;
    }
    if (auto *fx = qobject_cast<QGraphicsOpacityEffect *>(page->graphicsEffect())) {
        return fx;
    }
    auto *fx = new QGraphicsOpacityEffect(page);
    page->setGraphicsEffect(fx);
    return fx;
}

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
    clearPageEffect(page);
    if (page->parent() == this) {
        page->setParent(nullptr);
    }
    if (m_outgoing == page) {
        m_outgoing = nullptr;
        m_outgoingFrom = 1.0;
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
    m_outgoingFrom = 1.0;
    m_running = false;
    if (m_timer) {
        m_timer->stop();
    }
    m_currentIndex = -1;
    for (QWidget *page : pages) {
        clearPageEffect(page);
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

    // A page that was already fading out and is not the one we are leaving
    // is finished immediately so only one outgoing is live.
    if (m_running && m_outgoing != nullptr && m_outgoing != previous) {
        finishOutgoing();
    }

    m_currentIndex = clamped;

    if (m_animated && previous && previous->isVisible()) {
        // Retarget: the page we are leaving keeps whatever opacity it had on
        // screen so a mid-flight switch never jumps.
        m_outgoingFrom = pageOpacity(previous);
        m_outgoing = previous;
        startTransition();
    } else {
        for (int i = 0; i < m_pages.size(); ++i) {
            m_pages.at(i)->setVisible(i == m_currentIndex);
        }
        if (previous) {
            clearPageEffect(previous);
        }
        m_progress = 1.0;
        m_outgoingFrom = 1.0;
        m_running = false;
        if (m_timer) {
            m_timer->stop();
        }
        m_outgoing = nullptr;
    }

    QWidget *current = currentWidget();
    if (current) {
        current->setVisible(true);
        current->raise();
    }
    if (m_running && m_outgoing != nullptr && current != nullptr) {
        m_outgoing->setVisible(true);
        m_outgoing->lower();
        applyFadeOpacities();
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
        m_outgoingFrom = 1.0;
        if (m_timer) {
            m_timer->stop();
        }
        finishOutgoing();
        if (QWidget *current = currentWidget()) {
            clearPageEffect(current);
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

void MdStackedWidget::applyFadeOpacities()
{
    if (m_outgoing != nullptr) {
        if (QGraphicsOpacityEffect *fx = ensureFadeEffect(m_outgoing)) {
            fx->setOpacity(m_outgoingFrom * (1.0 - m_progress));
        }
    }
    if (QWidget *current = currentWidget()) {
        if (QGraphicsOpacityEffect *fx = ensureFadeEffect(current)) {
            fx->setOpacity(m_progress);
        }
    }
}

void MdStackedWidget::finishOutgoing()
{
    if (m_outgoing != nullptr) {
        clearPageEffect(m_outgoing);
        m_outgoing->setVisible(false);
        m_outgoing = nullptr;
    }
    m_outgoingFrom = 1.0;
}

qreal MdStackedWidget::pageOpacity(const QWidget *page)
{
    if (page == nullptr) {
        return 1.0;
    }
    if (const auto *fx = qobject_cast<const QGraphicsOpacityEffect *>(page->graphicsEffect())) {
        return fx->opacity();
    }
    return 1.0;
}

void MdStackedWidget::clearPageEffect(QWidget *page)
{
    if (page != nullptr && page->graphicsEffect() != nullptr) {
        page->setGraphicsEffect(nullptr);
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
        finishOutgoing();
        if (QWidget *current = currentWidget()) {
            clearPageEffect(current);
        }
    } else {
        applyFadeOpacities();
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
    // Honour contentsMargins so a host can reserve space for a floating
    // overlay (a mini player, say) without the pages sliding under it.
    const QMargins m = contentsMargins();
    const QRectF bounds = QRectF(rect()).adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    for (QWidget *page : m_pages) {
        page->setGeometry(bounds.toRect());
    }
}

bool MdStackedWidget::event(QEvent *event)
{
    // setContentsMargins() posts a LayoutRequest; without a layout manager
    // nothing else would re-place the pages.
    if (event->type() == QEvent::LayoutRequest) {
        relayout();
    }
    return QWidget::event(event);
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
