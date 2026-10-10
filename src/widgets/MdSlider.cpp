#include "MdSlider.h"

#include "styles/MdSliderStyle.h"

#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QPainter>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyleOption>

namespace md {

namespace {

/// The reveal / width animations step on a 8 ms timer (the same shape as
/// MdSwitch's thumb loop): the handle width moves on the fast spatial spring
/// equivalent — critically damped toward the target — and the label reveal on
/// the emphasized easing. Both settle well inside 200 ms.
constexpr int kTickMs = 8;

qreal approach(qreal current, qreal target, qreal rate)
{
    return current + (target - current) * rate;
}

} // namespace

MdSlider::MdSlider(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QTimer *timer = new QTimer(this);
    timer->setInterval(kTickMs);
    connect(timer, &QTimer::timeout, this, &MdSlider::tickAnimations);
    timer->start();

    retargetAnimations();
    m_handleWidth = m_handleWidthTarget;
}

MdSlider::~MdSlider() = default;

void MdSlider::setValue(qreal value)
{
    const qreal v = snap(clampValue(value));
    if (qFuzzyCompare(v, m_value)) {
        return;
    }
    m_value = v;
    emit valueChanged(v);
    update();
}

void MdSlider::setValueStart(qreal value)
{
    const qreal v = snap(clampValue(value));
    if (qFuzzyCompare(v, m_valueStart)) {
        return;
    }
    m_valueStart = v;
    emit valueStartChanged(v);
    update();
}

void MdSlider::setValueEnd(qreal value)
{
    const qreal v = snap(clampValue(value));
    if (qFuzzyCompare(v, m_valueEnd)) {
        return;
    }
    m_valueEnd = v;
    emit valueEndChanged(v);
    update();
}

void MdSlider::setMin(qreal min)
{
    if (qFuzzyCompare(min, m_min)) {
        return;
    }
    m_min = min;
    emit minChanged(min);
    update();
}

void MdSlider::setMax(qreal max)
{
    if (qFuzzyCompare(max, m_max)) {
        return;
    }
    m_max = max;
    emit maxChanged(max);
    update();
}

void MdSlider::setStep(qreal step)
{
    if (qFuzzyCompare(step, m_step)) {
        return;
    }
    m_step = step;
    emit stepChanged(step);
    update();
}

void MdSlider::setTicks(bool ticks)
{
    if (ticks == m_ticks) {
        return;
    }
    m_ticks = ticks;
    emit ticksChanged(ticks);
    update();
}

void MdSlider::setLabeled(bool labeled)
{
    if (labeled == m_labeled) {
        return;
    }
    m_labeled = labeled;
    emit labeledChanged(labeled);
    retargetAnimations();
    update();
}

void MdSlider::setRange(bool range)
{
    if (range == m_range) {
        return;
    }
    m_range = range;
    emit rangeChanged(range);
    update();
}

void MdSlider::setSliderSize(MdSliderSize size)
{
    if (size == m_size) {
        return;
    }
    m_size = size;
    emit sliderSizeChanged(size);
    update();
}

void MdSlider::setValueLabel(const QString &label)
{
    if (label == m_valueLabel) {
        return;
    }
    m_valueLabel = label;
    emit valueLabelChanged(label);
    update();
}

// --- geometry -------------------------------------------------------------

QRectF MdSlider::trackRect() const
{
    const MdSliderTokens &tokens = sliderTokens();
    // The track is inset by half the state layer so the handle's pill can sit
    // flush with the ends without clipping — material-web's
    // `handleContainerPadded` contract.
    const qreal inset = tokens.stateLayerSize / 2.0;
    const qreal height = tokens.trackHeight;
    const qreal y = (rect().height() - height) / 2.0;
    return QRectF(inset, y, qMax(0.0, rect().width() - 2.0 * inset), height);
}

QRectF MdSlider::handleRect(bool startHandle) const
{
    const MdSliderTokens &tokens = sliderTokens();
    const QRectF track = trackRect();
    const qreal value = (m_range && startHandle) ? m_valueStart
                        : m_range                    ? m_valueEnd
                                                     : m_value;
    // The handle rides the fraction measured from the leading edge — the right
    // side under RTL, so the travel mirrors.
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal centre = rtl ? track.right() - fractionOf(value) * track.width()
                             : track.left() + fractionOf(value) * track.width();
    const qreal width = animatedHandleWidth();
    const qreal height = tokens.handleHeight;
    return QRectF(centre - width / 2.0, (rect().height() - height) / 2.0, width, height);
}

QRectF MdSlider::stateLayerRect(bool startHandle) const
{
    const MdSliderTokens &tokens = sliderTokens();
    const QRectF handle = handleRect(startHandle);
    const qreal size = tokens.stateLayerSize;
    return QRectF(handle.center().x() - size / 2.0, (rect().height() - size) / 2.0, size, size);
}

QSize MdSlider::sizeHint() const
{
    const MdSliderTokens &tokens = sliderTokens();
    return QSize(200, int(tokens.stateLayerSize));
}

QSize MdSlider::minimumSizeHint() const
{
    return QSize(48, sizeHint().height());
}

// --- tokens -----------------------------------------------------------------

const MdSliderTokens &MdSlider::sliderTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSliderTokens::resolve(m_size);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdSlider::setSliderTokens(const MdSliderTokens &tokens)
{
    m_tokens = tokens;
    m_tokensDirty = false;
    update();
}

MdSliderInteraction MdSlider::interactionState() const
{
    if (isEffectivelyDisabled()) {
        return MdSliderInteraction::Disabled;
    }
    if (m_pressed) {
        return MdSliderInteraction::Pressed;
    }
    if (hasKeyboardFocus()) {
        return MdSliderInteraction::Focused;
    }
    if (m_hovered) {
        return MdSliderInteraction::Hovered;
    }
    return MdSliderInteraction::Enabled;
}

// --- value helpers ------------------------------------------------------------

qreal MdSlider::clampValue(qreal value) const
{
    return qBound(m_min, value, m_max);
}

qreal MdSlider::snap(qreal value) const
{
    if (m_step <= 0.0 || m_max <= m_min) {
        return value;
    }
    const qreal steps = (value - m_min) / m_step;
    return m_min + qRound(steps) * m_step;
}

qreal MdSlider::fractionOf(qreal value) const
{
    if (m_max <= m_min) {
        return 0.0;
    }
    return qBound(0.0, (value - m_min) / (m_max - m_min), 1.0);
}

qreal MdSlider::valueAt(qreal x) const
{
    const QRectF track = trackRect();
    if (track.width() <= 0.0) {
        return m_min;
    }
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal f = qBound(0.0, rtl ? (track.right() - x) / track.width()
                                   : (x - track.left()) / track.width(), 1.0);
    return snap(m_min + f * (m_max - m_min));
}

bool MdSlider::startHandleNear(const QPointF &pos) const
{
    const qreal ds = qAbs(pos.x() - handleRect(true).center().x());
    const qreal de = qAbs(pos.x() - handleRect(false).center().x());
    return ds <= de;
}

// --- interaction ---------------------------------------------------------------

void MdSlider::setActiveFromPoint(const QPointF &pos)
{
    if (m_range) {
        m_activeIsStart = startHandleNear(pos);
        if (m_activeIsStart) {
            setValueStart(valueAt(pos.x()));
            emit sliderMoved(m_valueStart);
        } else {
            setValueEnd(valueAt(pos.x()));
            emit sliderMoved(m_valueEnd);
        }
    } else {
        setValue(valueAt(pos.x()));
        emit sliderMoved(m_value);
    }
}

void MdSlider::moveActiveBy(qreal delta)
{
    if (m_range) {
        if (m_activeIsStart) {
            setValueStart(m_valueStart + delta);
            emit sliderMoved(m_valueStart);
        } else {
            setValueEnd(m_valueEnd + delta);
            emit sliderMoved(m_valueEnd);
        }
    } else {
        setValue(m_value + delta);
        emit sliderMoved(m_value);
    }
}

void MdSlider::retargetAnimations()
{
    const MdSliderTokens &tokens = sliderTokens();
    switch (interactionState()) {
    case MdSliderInteraction::Focused:
        m_handleWidthTarget = tokens.focusHandleWidth;
        break;
    case MdSliderInteraction::Pressed:
        m_handleWidthTarget = tokens.pressedHandleWidth;
        break;
    case MdSliderInteraction::Disabled:
        m_handleWidthTarget = tokens.disabledHandleWidth;
        break;
    case MdSliderInteraction::Hovered:
        m_handleWidthTarget = tokens.hoverHandleWidth;
        break;
    case MdSliderInteraction::Enabled:
    case MdSliderInteraction::Count:
        m_handleWidthTarget = tokens.handleWidth;
        break;
    }
    m_labelRevealTarget = m_labeled && (m_hovered || m_pressed || hasKeyboardFocus()) ? 1.0 : 0.0;
}

void MdSlider::tickAnimations()
{
    bool dirty = false;
    if (!qFuzzyCompare(m_handleWidth, m_handleWidthTarget)) {
        m_handleWidth = approach(m_handleWidth, m_handleWidthTarget, 0.35);
        if (qAbs(m_handleWidth - m_handleWidthTarget) < 0.01) {
            m_handleWidth = m_handleWidthTarget;
        }
        dirty = true;
    }
    if (!qFuzzyCompare(m_labelReveal, m_labelRevealTarget)) {
        m_labelReveal = approach(m_labelReveal, m_labelRevealTarget, 0.30);
        if (qAbs(m_labelReveal - m_labelRevealTarget) < 0.01) {
            m_labelReveal = m_labelRevealTarget;
        }
        dirty = true;
    }
    if (dirty) {
        update();
    }
}

QString MdSlider::labelFor(qreal value) const
{
    if (!m_valueLabel.isEmpty() && !m_range) {
        return m_valueLabel;
    }
    if (value == qRound(value)) {
        return QString::number(qRound(value));
    }
    return QString::number(value, 'f', 1);
}

// --- events ---------------------------------------------------------------------

void MdSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QStyleOption option;
    option.initFrom(this);
    QPainter painter(this);
    MdSliderStyle::paintSlider(painter, *this, sliderTokens());
}

void MdSlider::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    update();
}

void MdSlider::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        return;
    }
    m_pressed = true;
    emit sliderPressed();
    setActiveFromPoint(event->position());
    retargetAnimations();
    m_handleWidth = m_handleWidthTarget;
    update();
}

void MdSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_pressed || isEffectivelyDisabled()) {
        return;
    }
    setActiveFromPoint(event->position());
}

void MdSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    if (!m_pressed) {
        return;
    }
    m_pressed = false;
    emit sliderReleased();
    retargetAnimations();
    update();
}

void MdSlider::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_hovered = true;
    retargetAnimations();
    update();
}

void MdSlider::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hovered = false;
    retargetAnimations();
    update();
}

void MdSlider::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    // The focus ring and the Focused colour row are `:focus-visible` only —
    // mouse activation must not paint them (the library-wide rule).
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
        && event->reason() != Qt::PopupFocusReason;
    retargetAnimations();
    update();
}

void MdSlider::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    retargetAnimations();
    update();
}

void MdSlider::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        QWidget::keyPressEvent(event);
        return;
    }

    const qreal big = (m_max - m_min) / 10.0;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Down:
        moveActiveBy(m_step > 0.0 ? -m_step : -big);
        break;
    case Qt::Key_Right:
    case Qt::Key_Up:
        moveActiveBy(m_step > 0.0 ? m_step : big);
        break;
    case Qt::Key_PageDown:
        moveActiveBy(-big);
        break;
    case Qt::Key_PageUp:
        moveActiveBy(big);
        break;
    case Qt::Key_Home:
        if (m_range && m_activeIsStart) {
            setValueStart(m_min);
        } else if (m_range) {
            setValueEnd(m_min);
        } else {
            setValue(m_min);
        }
        break;
    case Qt::Key_End:
        if (m_range && m_activeIsStart) {
            setValueStart(m_max);
        } else if (m_range) {
            setValueEnd(m_max);
        } else {
            setValue(m_max);
        }
        break;
    case Qt::Key_Tab:
        if (m_range) {
            m_activeIsStart = !m_activeIsStart;
            update();
            break;
        }
        QWidget::keyPressEvent(event);
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }
    retargetAnimations();
    update();
}

void MdSlider::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        retargetAnimations();
        update();
    }
}

} // namespace md
