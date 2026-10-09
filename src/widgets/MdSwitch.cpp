#include "MdSwitch.h"

#include "styles/MdSwitchStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdSwitch::MdSwitch(QWidget *parent)
    : QCheckBox(parent)
{
    init();
}

MdSwitch::MdSwitch(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    init();
}

MdSwitch::~MdSwitch() = default;

void MdSwitch::init()
{
    // Space and Enter toggle; a click does the same. Switches are independent
    // toggles — QCheckBox's non-autoexclusive semantics match Compose's
    // `toggleable`.
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_thumbTimer = new QTimer(this);
    m_thumbTimer->setInterval(8);
    m_thumbTimer->setTimerType(Qt::PreciseTimer);
    connect(m_thumbTimer, &QTimer::timeout, this, &MdSwitch::onThumbTick);

    MdStyleBase::connectThemeUpdate(this, &MdSwitch::onThemeChanged);

    // The thumb targets depend on checked *and* pressed: a check re-targets
    // the spring, a press/release snaps there and springs back.
    connect(this, &QCheckBox::toggled, this, [this](bool) { retargetThumb(); });

    MdSwitchStyle::shared();

    // First layout lands directly (Compose records `initialSize` /
    // `initialOffset` on the first measure, no animation from nothing).
    retargetThumb();
}

void MdSwitch::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    retargetThumb();
    update();
    emit iconNameChanged(m_iconName);
}

void MdSwitch::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
    emit iconSetChanged(m_iconSet);
}

void MdSwitch::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
    emit iconFamilyChanged(m_iconFamily);
}

// ---------------------------------------------------------------------------
// Compose's ThumbNode.measure targets, verbatim
// ---------------------------------------------------------------------------

MdSwitch::ThumbTarget MdSwitch::thumbTarget() const
{
    const MdSwitchTokens &t = switchTokens();
    const bool pressed = isDown();

    const qreal size = [&] {
        if (pressed) {
            return t.pressedHandleSize;
        }
        if (isChecked() || !m_iconName.isEmpty()) {
            return t.selectedHandleSize;
        }
        return t.unselectedHandleSize;
    }();

    // `thumbPaddingStart = (SwitchHeight - size) / 2` — the x inset the small
    // thumb keeps from the track's left edge.
    const qreal minBound = (t.trackHeight - size) / 2.0;
    // `thumbPathLength = (SwitchWidth - ThumbDiameter) - ThumbPadding` where
    // ThumbDiameter is the selected 24 and ThumbPadding = (32 - 24) / 2 = 4.
    const qreal thumbPadding = (t.trackHeight - t.selectedHandleSize) / 2.0;
    const qreal maxBound = (t.trackWidth - t.selectedHandleSize) - thumbPadding;

    ThumbTarget target;
    target.diameter = size;
    if (pressed && isChecked()) {
        target.offset = maxBound - t.trackOutlineWidth;
    } else if (pressed && !isChecked()) {
        target.offset = t.trackOutlineWidth;
    } else if (isChecked()) {
        target.offset = maxBound;
    } else {
        target.offset = minBound;
    }
    return target;
}

void MdSwitch::retargetThumb()
{
    if (m_thumbTimer == nullptr) {
        return;
    }
    const bool pressed = isDown();
    const ThumbTarget target = thumbTarget();
    // First layout lands directly (Compose's `initialSize` / `initialOffset`
    // are recorded on the first measure, no animation from nothing).
    if (!m_thumbInitialised) {
        m_thumbDiameter = m_thumbFromDiameter = m_thumbToDiameter = target.diameter;
        m_thumbOffset = m_thumbFromOffset = m_thumbToOffset = target.offset;
        m_thumbInitialised = true;
        m_lastChecked = isChecked();
        m_wasPressed = pressed;
        update();
        return;
    }
    // Same-target debounce: a release runs `click()` (whose `toggled` already
    // retargeted) and the explicit release path both; retargeting to values
    // the thumb already holds restarts nothing.
    if (!pressed && !m_thumbTimer->isActive()
        && qFuzzyCompare(target.diameter + 1.0, m_thumbDiameter + 1.0)
        && qFuzzyCompare(target.offset + 1.0, m_thumbOffset + 1.0)) {
        m_lastChecked = isChecked();
        m_wasPressed = pressed;
        return;
    }
    // While pressed both animations run `SnapSpec`; otherwise the fast
    // spatial spring carries them. A press that changes nothing re-snaps to
    // the same place — harmless.
    restartThumbAnimation(pressed);
    m_thumbFromDiameter = m_thumbDiameter;
    m_thumbToDiameter = target.diameter;
    m_thumbFromOffset = m_thumbOffset;
    m_thumbToOffset = target.offset;
    m_thumbClock.restart();
    m_thumbTimer->start();

    m_lastChecked = isChecked();
    m_wasPressed = pressed;
    update();
}

void MdSwitch::restartThumbAnimation(bool snap)
{
    m_thumbSnap = snap;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

bool MdSwitch::hitButton(const QPoint &pos) const
{
    // The native hit area defers to the platform style's indicator rect; the
    // MD3 touch target is the whole widget.
    return rect().contains(pos);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QRectF MdSwitch::trackRect() const
{
    const MdSwitchTokens &t = switchTokens();
    const qreal w = qreal(width());
    const qreal h = qreal(height());
    return QRectF((w - t.trackWidth) / 2.0, (h - t.trackHeight) / 2.0, t.trackWidth,
                  t.trackHeight);
}

QRectF MdSwitch::thumbRect() const
{
    const QRectF track = trackRect();
    return QRectF(track.left() + m_thumbOffset, track.center().y() - m_thumbDiameter / 2.0,
                  m_thumbDiameter, m_thumbDiameter);
}

QRectF MdSwitch::stateLayerRect() const
{
    const MdSwitchTokens &t = switchTokens();
    const QPointF centre = thumbRect().center();
    return QRectF(centre.x() - t.stateLayerSize / 2.0, centre.y() - t.stateLayerSize / 2.0,
                  t.stateLayerSize, t.stateLayerSize);
}

QSize MdSwitch::sizeHint() const
{
    // Compose's `minimumInteractiveComponentSize` around the required
    // 52×32 track: 52 × 48.
    const MdSwitchTokens &t = switchTokens();
    const qreal w = qMax(t.trackWidth, t.minimumInteractiveSize);
    const qreal h = qMax(t.trackHeight, t.minimumInteractiveSize);
    return QSize(int(std::ceil(w)), int(std::ceil(h)));
}

QSize MdSwitch::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdSwitchTokens &MdSwitch::switchTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdSwitchTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdSwitch::setSwitchTokens(const MdSwitchTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdSwitch::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdSwitch::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        // The ripple rides the thumb: unbounded in a 40 px circle at the
        // thumb's centre.
        m_ripple->setBounds(stateLayerRect().size());
        m_ripple->press(QPointF(event->pos()));
    }
    QCheckBox::mousePressEvent(event);
    retargetThumb(); // the press snaps the thumb to its pressed target
}

void MdSwitch::mouseReleaseEvent(QMouseEvent *event)
{
    QCheckBox::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
    retargetThumb(); // release springs back from the pressed target
}

void MdSwitch::enterEvent(md::MdEnterEvent *event)
{
    QCheckBox::enterEvent(event);
    m_hovered = true;
    update();
}

void MdSwitch::leaveEvent(QEvent *event)
{
    QCheckBox::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdSwitch::focusInEvent(QFocusEvent *event)
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

void MdSwitch::focusOutEvent(QFocusEvent *event)
{
    QCheckBox::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdSwitch::changeEvent(QEvent *event)
{
    QCheckBox::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        // No rows animate into disabled: the colours resolve by state and the
        // thumb target doesn't change with enabled. A press interrupted by
        // disabling lands back on the resting target.
        if (m_thumbTimer != nullptr && !isDown()) {
            retargetThumb();
        }
        update();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// The thumb animation
// ---------------------------------------------------------------------------

void MdSwitch::onThumbTick()
{
    auto advance = [&](qreal from, qreal to, qreal *value) {
        if (m_thumbSnap) {
            *value = to;
            return true;
        }
        const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
        const qreal seconds = qreal(m_thumbClock.elapsed()) / 1000.0;
        const qreal travelled = spring.valueAt(seconds);
        *value = from + (to - from) * travelled;
        return qFuzzyCompare(*value + 1.0, to + 1.0)
               || m_thumbClock.elapsed() >= spring.settlingDurationMs();
    };

    const bool sizeDone = advance(m_thumbFromDiameter, m_thumbToDiameter, &m_thumbDiameter);
    const bool offsetDone = advance(m_thumbFromOffset, m_thumbToOffset, &m_thumbOffset);

    if (sizeDone && offsetDone) {
        m_thumbDiameter = m_thumbToDiameter;
        m_thumbOffset = m_thumbToOffset;
        m_thumbTimer->stop();
    }
    update();
}

void MdSwitch::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
