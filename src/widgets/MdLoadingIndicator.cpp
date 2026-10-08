#include "MdLoadingIndicator.h"

#include "styles/MdLoadingIndicatorStyle.h"
#include "core/MdTheme.h"

#include <cmath> // std::lround — MinGW 8.1 needs the explicit include

#include <QtCore/QTimer>
#include <QtWidgets/QStyle>

namespace md {

MdLoadingIndicator::MdLoadingIndicator(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdLoadingIndicator::MdLoadingIndicator(LoadingIndicatorVariant variant, QWidget *parent)
    : QWidget(parent)
    , m_variant(variant)
{
    init();
}

MdLoadingIndicator::~MdLoadingIndicator() = default;

void MdLoadingIndicator::init()
{
    // A loading indicator is not interactive — the export publishes no
    // state rows at all.
    setFocusPolicy(Qt::NoFocus);

    // Accessible content: the role mapping (aria progressbar) is a recorded
    // simplification — plain QWidget maps to a generic role; see
    // docs/porting-todo.md.
    setAccessibleName(QStringLiteral("loading"));

    m_clock.start();

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &MdLoadingIndicator::onAnimationFrame);

    MdStyleBase::connectThemeUpdate(this, &MdLoadingIndicator::onThemeChanged);

    // Create and register the shared style as soon as the first indicator
    // exists, so one is never momentarily painted by the platform style.
    MdLoadingIndicatorStyle::shared();

    updateAnimationState();
}

void MdLoadingIndicator::setVariant(LoadingIndicatorVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant selects the colour mapping (and the override keys), so
    // the cached set has to go.
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdLoadingIndicator::setIndeterminate(bool indeterminate)
{
    if (m_indeterminate == indeterminate) {
        return;
    }
    m_indeterminate = indeterminate;
    updateAnimationState();
    update();
    emit indeterminateChanged(m_indeterminate);
}

void MdLoadingIndicator::setProgress(qreal progress)
{
    const qreal clamped = qBound(0.0, progress, 1.0);
    if (m_progress == clamped) {
        return;
    }
    m_progress = clamped;
    update();
    emit progressChanged(m_progress);
}

void MdLoadingIndicator::setRunning(bool running)
{
    if (m_running == running) {
        return;
    }
    m_running = running;
    updateAnimationState();
    emit runningChanged(m_running);
}

const MdLoadingIndicatorTokens &MdLoadingIndicator::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdLoadingIndicatorTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdLoadingIndicator::sizeHint() const
{
    const MdLoadingIndicatorTokens &t = tokens();
    return QSize(int(std::lround(t.containerWidth)), int(std::lround(t.containerHeight)));
}

QSize MdLoadingIndicator::minimumSizeHint() const
{
    return sizeHint();
}

void MdLoadingIndicator::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateAnimationState();
}

void MdLoadingIndicator::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    updateAnimationState();
}

void MdLoadingIndicator::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdLoadingIndicator::onAnimationFrame()
{
    update();
}

void MdLoadingIndicator::invalidateTokens()
{
    m_tokensDirty = true;
}

void MdLoadingIndicator::updateAnimationState()
{
    if (!m_timer) {
        return;
    }
    const bool shouldRun = m_running && m_indeterminate && isVisible();
    if (shouldRun && !m_timer->isActive()) {
        m_timer->start();
    } else if (!shouldRun && m_timer->isActive()) {
        m_timer->stop();
    }
}

} // namespace md
