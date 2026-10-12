#include "MdCard.h"

#include "styles/MdCardStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QLayout>

namespace md {

namespace {

/// The elevation ladder springs on the default-effects motion — critically
/// damped, so a hover never overshoots past the target dp.
constexpr int kElevationTickMs = 8;

} // namespace

MdCard::MdCard(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdCard::MdCard(MdCardVariant variant, QWidget *parent)
    : QWidget(parent)
    , m_variant(variant)
{
    init();
}

MdCard::~MdCard() = default;

void MdCard::init()
{
    // The paint hub owns painting —registering the shared style as soon as
    // the first card exists means a card is never momentarily painted by the
    // platform style.
    MdCardStyle::shared();

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_elevationTimer = new QTimer(this);
    m_elevationTimer->setInterval(kElevationTickMs);
    m_elevationTimer->setTimerType(Qt::PreciseTimer);
    connect(m_elevationTimer, &QTimer::timeout, this, &MdCard::onElevationTick);

    m_animatedShadowDp = targetShadowDp();
    m_elevationFrom = m_animatedShadowDp;
    m_elevationTo = m_animatedShadowDp;

    applyClickability();
    MdStyleBase::connectThemeUpdate(this, &MdCard::onThemeChanged);
}

void MdCard::setVariant(MdCardVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant selects the whole state table, so the cached token set is
    // stale the moment this lands - and the elevation ladder (and therefore
    // the shadow headroom the contents margins reserve) changes with it.
    invalidateTokens();
    applyClickability();
    updateElevationTarget();
    update();
    emit variantChanged(m_variant);
}
void MdCard::setClickable(bool clickable)
{
    if (m_clickable == clickable) {
        return;
    }
    m_clickable = clickable;
    applyClickability();
    updateElevationTarget();
    update();
    emit clickableChanged(m_clickable);
}

void MdCard::setDragged(bool dragged)
{
    if (m_dragged == dragged) {
        return;
    }
    m_dragged = dragged;
    updateElevationTarget();
    update();
    emit draggedChanged(m_dragged);
}

void MdCard::applyClickability()
{
    // Exactly the Compose split: the clickable card is focusable and paints
    // its ring; the plain surface is not and paints neither. Both reserve the
    // paint inset —the focus ring and/or the elevation shadow —so the
    // painted container and the contents margins stay in step.
    if (m_clickable) {
        setFocusPolicy(Qt::StrongFocus);
    } else {
        setFocusPolicy(Qt::NoFocus);
    }
    const qreal inset = MdCardStyle::paintInset(*this, tokens());
    setContentsMargins(int(std::ceil(inset)), int(std::ceil(inset)), int(std::ceil(inset)),
                       int(std::ceil(inset)));
    updateGeometry();
}

QRectF MdCard::containerRect() const
{
    return MdCardStyle::layoutFor(*this, tokens()).container;
}

const MdCardTokens &MdCard::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdCardTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

qreal MdCard::targetShadowDp() const
{
    const MdCardTokens &resolved = tokens();
    const MdCardState state = MdCardStyle::stateFor(*this);
    return MdElevation::shadowDp(resolved.family.state(state).elevation);
}

bool MdCard::isElevationAnimating() const
{
    return m_elevationTimer != nullptr && m_elevationTimer->isActive();
}

void MdCard::updateElevationTarget()
{
    const qreal target = targetShadowDp();
    if (qFuzzyCompare(m_animatedShadowDp + 1.0, target + 1.0)
        && qFuzzyCompare(m_elevationTo + 1.0, target + 1.0)) {
        return;
    }
    // Compose snaps instead of transitioning when the card becomes disabled;
    // everything else springs.
    const bool snap = !m_clickable || !isEnabled()
        || MdCardStyle::stateFor(*this) == MdCardState::Disabled;
    if (snap) {
        if (m_elevationTimer) {
            m_elevationTimer->stop();
        }
        m_animatedShadowDp = target;
        m_elevationFrom = target;
        m_elevationTo = target;
        update();
        return;
    }
    // Interruptible: continue from the dp currently on screen.
    m_elevationFrom = m_animatedShadowDp;
    m_elevationTo = target;
    if (qFuzzyCompare(m_elevationFrom + 1.0, m_elevationTo + 1.0)) {
        m_animatedShadowDp = target;
        if (m_elevationTimer) {
            m_elevationTimer->stop();
        }
        update();
        return;
    }
    m_elevationClock.restart();
    if (m_elevationTimer) {
        m_elevationTimer->start();
    }
}

void MdCard::onElevationTick()
{
    const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
    const qreal seconds = qreal(m_elevationClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_animatedShadowDp = m_elevationFrom + (m_elevationTo - m_elevationFrom) * progress;
    if (progress >= 1.0
        || qreal(m_elevationClock.elapsed()) >= spring.settlingDurationMs()) {
        m_animatedShadowDp = m_elevationTo;
        if (m_elevationTimer) {
            m_elevationTimer->stop();
        }
    }
    update();
}

void MdCard::syncRippleGeometry()
{
    if (!m_ripple) {
        return;
    }
    const QRectF container = containerRect();
    m_ripple->setBounds(container.size());
    const QRectF localRect(QPointF(0.0, 0.0), container.size());
    m_ripple->setClipPath(MdShape::roundedRect(localRect, MdCardStyle::layoutFor(*this, tokens()).radii));
}

QSize MdCard::sizeHint() const
{
    // A card sizes to its content: the installed layout's hint plus the
    // contents margins. Without a layout the widget publishes no hint of its
    // own —a card with no children has no intrinsic size, exactly as a
    // plain QWidget would not.
    QSize hint = QWidget::sizeHint();
    if (layout()) {
        hint = layout()->totalSizeHint();
    }
    return hint;
}

void MdCard::mousePressEvent(QMouseEvent *event)
{
    if (!m_clickable || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    updateElevationTarget();
    if (m_ripple) {
        syncRippleGeometry();
        m_ripple->press(QPointF(mousePosition(event)) - containerRect().topLeft());
    }
    update();
    QWidget::mousePressEvent(event);
}

void MdCard::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_pressed;
    m_pressed = false;
    if (m_ripple) {
        m_ripple->release();
    }
    if (wasPressed && m_clickable && isEnabled()
        && rect().contains(mousePosition(event).toPoint())) {
        emit clicked();
    }
    updateElevationTarget();
    update();
    QWidget::mouseReleaseEvent(event);
}

void MdCard::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
}

void MdCard::enterEvent(MdEnterEvent *event)
{
    m_hovered = true;
    updateElevationTarget();
    QWidget::enterEvent(event);
    update();
}

void MdCard::leaveEvent(QEvent *event)
{
    m_hovered = false;
    updateElevationTarget();
    QWidget::leaveEvent(event);
    update();
}

void MdCard::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    // `:focus-visible` semantics, shared with MdButton and MdIconButton —    // pointer focus shows no ring and no focused row.
    switch (event->reason()) {
    case Qt::MouseFocusReason:
    case Qt::PopupFocusReason:
    case Qt::ActiveWindowFocusReason:
        m_focusIsKeyboard = false;
        break;
    default:
        m_focusIsKeyboard = true;
        break;
    }
    if (m_focusRing) {
        if (m_focusIsKeyboard) {
            m_focusRing->start();
        } else {
            m_focusRing->stop();
        }
    }
    updateElevationTarget();
    update();
}

void MdCard::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing) {
        m_focusRing->stop();
    }
    updateElevationTarget();
    update();
}

void MdCard::keyPressEvent(QKeyEvent *event)
{
    if (m_clickable && isEnabled()
        && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Return)) {
        if (m_ripple) {
            syncRippleGeometry();
            m_ripple->pressCentered();
            m_ripple->release();
        }
        emit clicked();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdCard::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        // Compose snaps to the disabled elevation with no transition.
        updateElevationTarget();
        update();
        break;
    default:
        break;
    }
}

void MdCard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncRippleGeometry();
}

void MdCard::onThemeChanged()
{
    invalidateTokens();
    updateElevationTarget();
    update();
}

void MdCard::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
