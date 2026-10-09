#include "MdRadioButton.h"

#include "styles/MdRadioButtonStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdRadioButton::MdRadioButton(QWidget *parent)
    : QRadioButton(parent)
{
    init();
}

MdRadioButton::MdRadioButton(const QString &text, QWidget *parent)
    : QRadioButton(text, parent)
{
    init();
}

MdRadioButton::~MdRadioButton() = default;

void MdRadioButton::init()
{
    // The group exclusivity is QRadioButton's native autoExclusive; Space and
    // Enter check. A click on an already-checked button keeps it checked
    // (QRadioButton::nextCheckState's native behaviour, matching `selectable`).
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_dotTimer = new QTimer(this);
    m_dotTimer->setInterval(8);
    m_dotTimer->setTimerType(Qt::PreciseTimer);
    connect(m_dotTimer, &QTimer::timeout, this, &MdRadioButton::onDotTick);

    m_colourTimer = new QTimer(this);
    m_colourTimer->setInterval(8);
    m_colourTimer->setTimerType(Qt::PreciseTimer);
    connect(m_colourTimer, &QTimer::timeout, this, &MdRadioButton::onColourTick);

    MdStyleBase::connectThemeUpdate(this, &MdRadioButton::onThemeChanged);

    // `toggled` fires for a click's `nextCheckState` and for a programmatic
    // `setChecked` alike — both run the same transitions. The restart dedupes
    // a state it has already picked up.
    connect(this, &QRadioButton::toggled, this, [this](bool) { restartAnimations(); });

    MdRadioButtonStyle::shared();
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

bool MdRadioButton::hitButton(const QPoint &pos) const
{
    // QRadioButton defers to the platform style's indicator rect, which knows
    // nothing about this widget's self-drawn, centred circle — the 48 px touch
    // target is dead away from the native indicator sliver. The MD3 touch
    // target is the whole widget.
    return rect().contains(pos);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QRectF MdRadioButton::iconRect() const
{
    const MdRadioButtonTokens &t = radioButtonTokens();
    const qreal w = qreal(width());
    const qreal h = qreal(height());
    return QRectF((w - t.iconSize) / 2.0, (h - t.iconSize) / 2.0, t.iconSize, t.iconSize);
}

QRectF MdRadioButton::stateLayerRect() const
{
    const MdRadioButtonTokens &t = radioButtonTokens();
    const qreal w = qreal(width());
    const qreal h = qreal(height());
    return QRectF((w - t.stateLayerSize) / 2.0, (h - t.stateLayerSize) / 2.0, t.stateLayerSize,
                  t.stateLayerSize);
}

QSize MdRadioButton::sizeHint() const
{
    // Compose's `minimumInteractiveComponentSize` — the touch target the 20 px
    // canvas centres in. The size holds while disabled too (the checkbox's
    // recorded divergence, the same here).
    const qreal side = radioButtonTokens().minimumInteractiveSize;
    return QSize(int(std::ceil(side)), int(std::ceil(side)));
}

QSize MdRadioButton::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdRadioButtonTokens &MdRadioButton::radioButtonTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdRadioButtonTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdRadioButton::setRadioButtonTokens(const MdRadioButtonTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdRadioButton::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdRadioButton::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        // Compose's unbounded ripple runs in a circle of the state layer's
        // diameter, centred on the icon.
        m_ripple->setBounds(stateLayerRect().size());
        m_ripple->press(QPointF(event->pos()));
    }
    QRadioButton::mousePressEvent(event);
}

void MdRadioButton::mouseReleaseEvent(QMouseEvent *event)
{
    QRadioButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdRadioButton::enterEvent(md::MdEnterEvent *event)
{
    QRadioButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdRadioButton::leaveEvent(QEvent *event)
{
    QRadioButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdRadioButton::focusInEvent(QFocusEvent *event)
{
    QRadioButton::focusInEvent(event);
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

void MdRadioButton::focusOutEvent(QFocusEvent *event)
{
    QRadioButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdRadioButton::changeEvent(QEvent *event)
{
    QRadioButton::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        // Compose snaps the colours on the way into disabled — "there should
        // be no animations between enabled / disabled". The dot snaps with it.
        if (m_colourTimer != nullptr) {
            m_colourTimer->stop();
        }
        if (m_dotTimer != nullptr) {
            m_dotTimer->stop();
        }
        m_colourFrom = m_colourTo = isChecked() ? 1.0 : 0.0;
        m_colourProgress = m_colourTo;
        m_dotFrom = m_dotTo = isChecked() ? radioButtonTokens().dotSize : 0.0;
        m_dotDiameter = m_dotTo;
        update();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// The two animations
// ---------------------------------------------------------------------------

void MdRadioButton::restartAnimations()
{
    if (m_dotTimer == nullptr) {
        // Constructing: no animation to run.
        return;
    }
    const bool checked = isChecked();
    if (checked == m_lastChecked && !m_dotTimer->isActive() && !m_colourTimer->isActive()) {
        return;
    }
    m_lastChecked = checked;

    // The dot: Compose's `animateDpAsState` on the fast spatial spring — in
    // *and* out on the same spring, no snap delays anywhere.
    m_dotFrom = m_dotDiameter;
    m_dotTo = checked ? radioButtonTokens().dotSize : 0.0;
    m_dotClock.restart();
    m_dotTimer->start();

    // The colour fade: the default effects spring in, the fast one out.
    if (!qFuzzyCompare(m_colourTo + 1.0, (checked ? 1.0 : 0.0) + 1.0)) {
        m_colourFrom = m_colourProgress;
        m_colourTo = checked ? 1.0 : 0.0;
        m_colourClock.restart();
        m_colourTimer->start();
    }
    update();
}

void MdRadioButton::onDotTick()
{
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = qreal(m_dotClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_dotDiameter = m_dotFrom + (m_dotTo - m_dotFrom) * travelled;

    if (qFuzzyCompare(m_dotDiameter + 1.0, m_dotTo + 1.0)
        || qreal(m_dotClock.elapsed()) >= spring.settlingDurationMs()) {
        m_dotDiameter = m_dotTo;
        m_dotTimer->stop();
    }
    update();
}

void MdRadioButton::onColourTick()
{
    // Compose's `animateColorAsState` on the default effects spring, faded out
    // on the fast one (the checkbox's `colorAnimationSpecForState` shape).
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

void MdRadioButton::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
