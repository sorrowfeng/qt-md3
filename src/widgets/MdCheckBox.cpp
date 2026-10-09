#include "MdCheckBox.h"

#include "styles/MdCheckBoxStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

namespace {

/// The three-state cycle Compose's `toggleValue()` walks: Off → On →
/// (Indeterminate, when the widget is tristate) → Off.
Qt::CheckState cycleFrom(Qt::CheckState state, bool tristate)
{
    switch (state) {
    case Qt::Unchecked:
        return Qt::Checked;
    case Qt::Checked:
        return tristate ? Qt::PartiallyChecked : Qt::Unchecked;
    case Qt::PartiallyChecked:
    default:
        return Qt::Unchecked;
    }
}

} // namespace

MdCheckBox::MdCheckBox(QWidget *parent)
    : QCheckBox(parent)
{
    init();
}

MdCheckBox::MdCheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    init();
}

MdCheckBox::~MdCheckBox() = default;

void MdCheckBox::init()
{
    // Space and Enter toggle; a click does the same. The tristate cycle itself
    // is `nextCheckState()`'s, overridden below to pin the order.
    setTristate(false);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_checkTimer = new QTimer(this);
    m_checkTimer->setInterval(8);
    m_checkTimer->setTimerType(Qt::PreciseTimer);
    connect(m_checkTimer, &QTimer::timeout, this, &MdCheckBox::onCheckTick);

    m_colourTimer = new QTimer(this);
    m_colourTimer->setInterval(8);
    m_colourTimer->setTimerType(Qt::PreciseTimer);
    connect(m_colourTimer, &QTimer::timeout, this, &MdCheckBox::onColourTick);

    MdStyleBase::connectThemeUpdate(this, &MdCheckBox::onThemeChanged);

    // QCheckBox announces a check-state change through the checkStateSet()
    // virtual and — since 6.7 — the checkStateChanged() signal; Qt 5 spells the
    // signal `stateChanged(int)`. Either path lands here, so a programmatic
    // `setCheckState()` runs the same transitions a click's `nextCheckState()`
    // does. The restart dedupes a state it has already picked up.
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(this, &QCheckBox::checkStateChanged, this, [this](Qt::CheckState) {
        restartCheckAnimation();
        restartColourAnimation();
    });
#else
    connect(this, QOverload<int>::of(&QCheckBox::stateChanged), this, [this](int) {
        restartCheckAnimation();
        restartColourAnimation();
    });
#endif

    MdCheckBoxStyle::shared();
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void MdCheckBox::setError(bool error)
{
    if (m_error == error) {
        return;
    }
    m_error = error;
    update();
    emit errorChanged(m_error);
}

void MdCheckBox::nextCheckState()
{
    setCheckState(cycleFrom(checkState(), isTristate()));
}

void MdCheckBox::checkStateSet()
{
    QCheckBox::checkStateSet();
    restartCheckAnimation();
    restartColourAnimation();
}

bool MdCheckBox::hitButton(const QPoint &pos) const
{
    // QCheckBox defers to the platform style's indicator rect, which knows
    // nothing about this widget's self-drawn, centred box — with no text the
    // native geometry puts the clickable sliver at the left edge and the 48 px
    // touch target is dead everywhere else. The MD3 touch target is the whole
    // widget: the box centres in it and every point of it toggles.
    return rect().contains(pos);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QRectF MdCheckBox::boxRect() const
{
    const MdCheckBoxTokens &t = checkBoxTokens();
    const qreal w = qreal(width());
    const qreal h = qreal(height());
    return QRectF((w - t.containerSize) / 2.0, (h - t.containerSize) / 2.0, t.containerSize,
                  t.containerSize);
}

QRectF MdCheckBox::stateLayerRect() const
{
    const MdCheckBoxTokens &t = checkBoxTokens();
    const qreal w = qreal(width());
    const qreal h = qreal(height());
    return QRectF((w - t.stateLayerSize) / 2.0, (h - t.stateLayerSize) / 2.0, t.stateLayerSize,
                  t.stateLayerSize);
}

QSize MdCheckBox::sizeHint() const
{
    // Compose's `minimumInteractiveComponentSize` — the touch target the 18 px
    // canvas centres in. The size holds while disabled too: a checkbox that
    // shrinks its layout slot when it greys out would reshuffle everything
    // around it (the divergence is recorded in porting-todo.md).
    const qreal side = checkBoxTokens().minimumInteractiveSize;
    return QSize(int(std::ceil(side)), int(std::ceil(side)));
}

QSize MdCheckBox::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdCheckBoxTokens &MdCheckBox::checkBoxTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdCheckBoxTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdCheckBox::setCheckBoxTokens(const MdCheckBoxTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdCheckBox::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdCheckBox::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        // Compose's unbounded ripple runs in a circle of the state layer's
        // diameter, centred on the box.
        m_ripple->setBounds(stateLayerRect().size());
        m_ripple->press(QPointF(event->pos()));
    }
    QCheckBox::mousePressEvent(event);
}

void MdCheckBox::mouseReleaseEvent(QMouseEvent *event)
{
    QCheckBox::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdCheckBox::enterEvent(md::MdEnterEvent *event)
{
    QCheckBox::enterEvent(event);
    m_hovered = true;
    update();
}

void MdCheckBox::leaveEvent(QEvent *event)
{
    QCheckBox::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdCheckBox::focusInEvent(QFocusEvent *event)
{
    QCheckBox::focusInEvent(event);
    // `:focus-visible`: only a keyboard reason paints the ring. See MdButton
    // for the reason classification.
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
                        && event->reason() != Qt::ActiveWindowFocusReason
                        && event->reason() != Qt::PopupFocusReason;
    if (m_focusRing != nullptr && m_focusIsKeyboard) {
        m_focusRing->start();
    }
    update();
}

void MdCheckBox::focusOutEvent(QFocusEvent *event)
{
    QCheckBox::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdCheckBox::changeEvent(QEvent *event)
{
    QCheckBox::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        // Compose snaps the colours on the way into disabled — "there should
        // be no animations between enabled / disabled".
        if (m_colourTimer != nullptr) {
            m_colourTimer->stop();
        }
        m_colourFrom = m_colourTo = isChecked() ? 1.0 : 0.0;
        m_colourProgress = m_colourTo;
        update();
        break;
    case QEvent::LayoutDirectionChange:
        update();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// The check animation
// ---------------------------------------------------------------------------

void MdCheckBox::restartCheckAnimation()
{
    if (m_checkTimer == nullptr) {
        // Constructing: no animation to run.
        return;
    }
    const Qt::CheckState previous = m_lastState;
    const Qt::CheckState current = checkState();
    m_lastState = current;
    if (previous == current && !m_checkTimer->isActive()) {
        return;
    }

    // The fraction: Off → anything draws the path on the spatial default
    // spring; anything → Off holds for the snap delay, then vanishes.
    const qreal fractionTarget = current == Qt::Unchecked ? 0.0 : 1.0;
    m_fractionTransition.from = m_checkFraction;
    m_fractionTransition.to = fractionTarget;
    m_fractionTransition.snap = previous != Qt::Unchecked && current == Qt::Unchecked;
    m_fractionTransition.delayMs = m_fractionTransition.snap
                                       ? int(checkBoxTokens().snapAnimationDelayMs)
                                       : 0;
    m_fractionTransition.clock.restart();

    // The gravitation: Off → anything else snaps (an indeterminate dash draws
    // in from nothing, it does not grow out of a checkmark); anything → Off
    // holds for the delay; a check-to-dash morph springs.
    const qreal gravitationTarget = current == Qt::PartiallyChecked ? 1.0 : 0.0;
    m_gravitationTransition.from = m_gravitation;
    m_gravitationTransition.to = gravitationTarget;
    if (previous == Qt::Unchecked) {
        // Compose's `snap()` — no delay, no morph.
        m_gravitation = gravitationTarget;
        m_gravitationTransition.snap = true;
        m_gravitationTransition.delayMs = 0;
    } else {
        m_gravitationTransition.snap = current == Qt::Unchecked;
        m_gravitationTransition.delayMs =
            m_gravitationTransition.snap ? int(checkBoxTokens().snapAnimationDelayMs) : 0;
    }
    m_gravitationTransition.clock.restart();

    m_checkTimer->start();
    update();
}

void MdCheckBox::onCheckTick()
{
    auto advance = [this](Transition &transition, qreal *value) {
        if (transition.snap) {
            // Hold the current value until the delay elapses, then land.
            if (transition.clock.elapsed() < transition.delayMs) {
                *value = transition.from;
                return false;
            }
            *value = transition.to;
            return true;
        }
        const MdSpring spring = MdMotion::spring(MotionSpring::SpatialDefault);
        const qreal seconds = qreal(transition.clock.elapsed()) / 1000.0;
        const qreal travelled = spring.valueAt(seconds);
        *value = transition.from + (transition.to - transition.from) * travelled;
        return qFuzzyCompare(*value + 1.0, transition.to + 1.0)
               || transition.clock.elapsed() >= spring.settlingDurationMs();
    };

    const bool fractionDone = advance(m_fractionTransition, &m_checkFraction);
    const bool gravitationDone = advance(m_gravitationTransition, &m_gravitation);

    if (fractionDone && gravitationDone) {
        m_checkFraction = m_fractionTransition.to;
        m_gravitation = m_gravitationTransition.to;
        m_checkTimer->stop();
    }
    update();
}

// ---------------------------------------------------------------------------
// The colour fade
// ---------------------------------------------------------------------------

void MdCheckBox::restartColourAnimation()
{
    if (m_colourTimer == nullptr) {
        return;
    }
    const qreal target = checkState() == Qt::Unchecked ? 0.0 : 1.0;
    if (qFuzzyCompare(target + 1.0, m_colourTo + 1.0)) {
        return;
    }
    m_colourFrom = m_colourProgress;
    m_colourTo = target;
    m_colourClock.restart();
    m_colourTimer->start();
    update();
}

void MdCheckBox::onColourTick()
{
    // Compose's `colorAnimationSpecForState`: the fade-in runs on the *default*
    // effects spring, the fade-out on the *fast* one.
    const MotionSpring slot =
        m_colourTo > m_colourFrom ? MotionSpring::EffectsDefault : MotionSpring::EffectsFast;
    const MdSpring spring = MdMotion::spring(slot);
    const qreal seconds = qreal(m_colourClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_colourProgress = m_colourFrom + (m_colourTo - m_colourFrom) * travelled;

    if (qFuzzyCompare(m_colourProgress + 1.0, m_colourTo + 1.0)
        || qreal(m_colourClock.elapsed()) >= spring.settlingDurationMs()) {
        m_colourProgress = m_colourTo;
        m_colourTimer->stop();
    }
    update();
}

void MdCheckBox::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
