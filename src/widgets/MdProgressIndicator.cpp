#include "MdProgressIndicator.h"

#include "styles/MdProgressIndicatorStyle.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QShowEvent>

namespace md {

qreal MdProgressIndicator::fraction() const
{
    if (m_max <= 0.0) {
        return 0.0;
    }
    return qBound(0.0, m_value / m_max, 1.0);
}

MdProgressIndicator::MdProgressIndicator(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdProgressIndicator::MdProgressIndicator(ProgressIndicatorShape shape, QWidget *parent)
    : QWidget(parent)
    , m_shape(shape)
{
    init();
}

MdProgressIndicator::~MdProgressIndicator() = default;

void MdProgressIndicator::init()
{
    // A progress indicator is not interactive — the merged export publishes
    // no state rows at all.
    setFocusPolicy(Qt::NoFocus);

    // Accessible content: the value the eye can read must reach the screen
    // reader. The role mapping (aria progressbar) is a recorded
    // simplification — plain QWidget maps to a generic role; see
    // docs/porting-todo.md.
    setAccessibleName(isIndeterminate() ? QStringLiteral("loading")
                                        : QString::number(qRound(displayFraction() * 100.0))
                                              + QStringLiteral("%"));

    m_clock.start();

    m_timer = new QTimer(this);
    m_timer->setInterval(16);
    connect(m_timer, &QTimer::timeout, this, &MdProgressIndicator::onAnimationFrame);

    m_transitionTimer = new QTimer(this);
    m_transitionTimer->setInterval(16);
    m_transitionTimer->setTimerType(Qt::PreciseTimer);
    connect(m_transitionTimer, &QTimer::timeout, this, &MdProgressIndicator::onTransitionTick);

    MdStyleBase::connectThemeUpdate(this, &MdProgressIndicator::onThemeChanged);

    // Create and register the shared style as soon as the first indicator
    // exists, so one is never momentarily painted by the platform style.
    MdProgressIndicatorStyle::shared();

    updateAnimationState();
}

void MdProgressIndicator::setShape(ProgressIndicatorShape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    // The shape selects the whole metric table, so the cached set has to go;
    // the display fraction snaps because the two shapes transition at
    // different durations.
    invalidateTokens();
    m_transitionActive = false;
    if (m_transitionTimer) {
        m_transitionTimer->stop();
    }
    snapDisplayFraction();
    updateGeometry();
    updateAnimationState();
    update();
    emit shapeChanged(m_shape);
}

void MdProgressIndicator::setIndeterminate(bool indeterminate)
{
    if (m_indeterminate == indeterminate) {
        return;
    }
    m_indeterminate = indeterminate;
    m_transitionActive = false;
    if (m_transitionTimer) {
        m_transitionTimer->stop();
    }
    snapDisplayFraction();
    setAccessibleName(indeterminate ? QStringLiteral("loading")
                                    : QString::number(qRound(displayFraction() * 100.0))
                                          + QStringLiteral("%"));
    updateAnimationState();
    update();
    emit indeterminateChanged(m_indeterminate);
}

void MdProgressIndicator::setValue(qreal value)
{
    if (qFuzzyCompare(m_value, value)) {
        return;
    }
    m_value = value;
    if (m_indeterminate) {
        // Indeterminate mode ignores value; the change lands when the mode
        // switches back.
    } else {
        transitionDisplayFraction();
        setAccessibleName(QString::number(qRound(displayFraction() * 100.0))
                          + QStringLiteral("%"));
    }
    updateAnimationState();
    update();
    emit valueChanged(m_value);
}

void MdProgressIndicator::setMax(qreal max)
{
    if (qFuzzyCompare(m_max, max)) {
        return;
    }
    m_max = max;
    if (!m_indeterminate) {
        snapDisplayFraction();
    }
    update();
    emit maxChanged(m_max);
}

void MdProgressIndicator::setBuffer(qreal buffer)
{
    if (qFuzzyCompare(m_buffer, buffer)) {
        return;
    }
    m_buffer = buffer;
    // Buffer only means anything in the linear shape; circular ignores it,
    // matching material-web (the property only exists on LinearProgress).
    updateAnimationState();
    update();
    emit bufferChanged(m_buffer);
}

void MdProgressIndicator::setFourColor(bool fourColor)
{
    if (m_fourColor == fourColor) {
        return;
    }
    m_fourColor = fourColor;
    updateAnimationState();
    update();
    emit fourColorChanged(m_fourColor);
}

const MdProgressIndicatorTokens &MdProgressIndicator::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdProgressIndicatorTokens::resolve(m_shape, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdProgressIndicator::sizeHint() const
{
    const MdProgressIndicatorTokens &resolved = tokens();
    const MdProgressIndicatorStyle::Layout layout =
        MdProgressIndicatorStyle::layoutFor(*this, resolved);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdProgressIndicator::minimumSizeHint() const
{
    return sizeHint();
}

void MdProgressIndicator::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateAnimationState();
}

void MdProgressIndicator::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    updateAnimationState();
}

void MdProgressIndicator::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdProgressIndicator::onAnimationFrame()
{
    update();
}

void MdProgressIndicator::invalidateTokens()
{
    m_tokensDirty = true;
}

void MdProgressIndicator::updateAnimationState()
{
    if (m_timer == nullptr) {
        return;
    }
    const bool bufferDotsVisible =
        !m_indeterminate && m_buffer > 0.0 && m_buffer < m_max && m_value < m_max;
    const bool animated = (m_indeterminate || bufferDotsVisible) && isVisible();
    if (animated) {
        m_timer->start();
    } else {
        m_timer->stop();
    }
}

void MdProgressIndicator::snapDisplayFraction()
{
    m_displayFraction = fraction();
    update();
}

void MdProgressIndicator::transitionDisplayFraction()
{
    if (m_transitionTimer == nullptr) {
        return;
    }
    const qreal target = fraction();
    // Interruptible: retarget from the fraction currently on screen rather
    // than jumping. Critically damped effects spring — a progress bar must
    // never overshoot its target.
    m_displayFrom = m_displayFraction;
    m_displayTo = target;
    if (qFuzzyCompare(m_displayFrom + 1.0, m_displayTo + 1.0)) {
        m_displayFraction = target;
        m_transitionActive = false;
        m_transitionTimer->stop();
        update();
        return;
    }
    m_transitionActive = true;
    m_transitionClock.restart();
    m_transitionTimer->start();
}

void MdProgressIndicator::onTransitionTick()
{
    if (!m_transitionActive) {
        m_transitionTimer->stop();
        return;
    }
    const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
    const qreal seconds = qreal(m_transitionClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_displayFraction = m_displayFrom + (m_displayTo - m_displayFrom) * progress;
    // A damped spring approaches its target asymptotically; the elapsed-time
    // bound is what guarantees the timer stops.
    if (progress >= 1.0
        || qreal(m_transitionClock.elapsed()) >= spring.settlingDurationMs()) {
        m_displayFraction = m_displayTo;
        m_transitionActive = false;
        m_transitionTimer->stop();
    }
    update();
}

} // namespace md
