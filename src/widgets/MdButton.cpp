#include "MdButton.h"

#include "core/MdMotion.h"
#include "styles/MdButtonStyle.h"

#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

MdButton::MdButton(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdButton::MdButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    init();
}

MdButton::~MdButton() = default;

void MdButton::init()
{
    // A button is reachable with Tab and activates on Space and Enter, both of
    // which QAbstractButton already implements.
    setFocusPolicy(Qt::StrongFocus);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_morphTimer = new QTimer(this);
    m_morphTimer->setInterval(16);
    m_morphTimer->setTimerType(Qt::PreciseTimer);
    connect(m_morphTimer, &QTimer::timeout, this, &MdButton::onMorphTick);

    // QAbstractButton emits these for mouse *and* keyboard activation, so
    // wiring the ripple to them means Space produces the same feedback a click
    // does without a second code path.
    connect(this, &QAbstractButton::pressed, this, &MdButton::onPressed);
    connect(this, &QAbstractButton::released, this, &MdButton::onReleased);

    MdStyleBase::connectThemeUpdate(this, &MdButton::onThemeChanged);

    // Create and register the shared style as soon as the first button exists,
    // so a button is never momentarily painted by the platform style.
    MdButtonStyle::shared();
}

// ---------------------------------------------------------------------------
// properties
// ---------------------------------------------------------------------------

void MdButton::setVariant(ButtonVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    updateGeometry();
    update();
    emit variantChanged(m_variant);
}

void MdButton::setButtonSize(ButtonSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    invalidateTokens();
    updateGeometry();
    update();
    emit buttonSizeChanged(m_size);
}

void MdButton::setButtonShape(ButtonShape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    invalidateTokens();
    updateGeometry();
    update();
    emit buttonShapeChanged(m_shape);
}

void MdButton::setLeadingIcon(const QString &icon)
{
    if (m_leadingIcon == icon) {
        return;
    }
    m_leadingIcon = icon;
    updateGeometry();
    update();
    emit leadingIconChanged(m_leadingIcon);
}

void MdButton::setTrailingIcon(const QString &icon)
{
    if (m_trailingIcon == icon) {
        return;
    }
    m_trailingIcon = icon;
    updateGeometry();
    update();
    emit trailingIconChanged(m_trailingIcon);
}

void MdButton::setSoftDisabled(bool softDisabled)
{
    if (m_softDisabled == softDisabled) {
        return;
    }
    m_softDisabled = softDisabled;
    if (m_softDisabled) {
        // Soft-disabled exists precisely to stay reachable, so make sure a
        // caller that had switched the focus policy off does not lose it.
        setFocusPolicy(Qt::StrongFocus);
    }
    update();
    emit softDisabledChanged(m_softDisabled);
}

bool MdButton::isEffectivelyDisabled() const
{
    return !isEnabled() || m_softDisabled;
}

void MdButton::setCornerRadii(const QList<qreal> &resting, const QList<qreal> &pressed)
{
    if (m_restingRadii == resting && m_pressedRadii == pressed) {
        return;
    }
    m_restingRadii = resting;
    m_pressedRadii = pressed;
    // No updateGeometry(): the radii change the container's *corners*, not the
    // container's size, so the widget's sizeHint is unaffected. A button group
    // calls this on every relayout and a redundant geometry pass there would be
    // quadratic.
    update();
}

QList<qreal> MdButton::currentCornerRadii() const
{
    return MdButtonStyle::layoutFor(*this, tokens()).radii;
}

MdButtonState MdButton::paintState() const
{
    return MdButtonStyle::stateFor(*this);
}

// ---------------------------------------------------------------------------
// tokens
// ---------------------------------------------------------------------------

const MdButtonTokens &MdButton::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdButtonTokens::resolve(m_variant, m_size, m_shape, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdButton::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// text
// ---------------------------------------------------------------------------

QString MdButton::displayText() const
{
    const QString raw = text();
    if (!raw.contains(QLatin1Char('&'))) {
        return raw;
    }

    // QPushButton interprets '&' as a mnemonic marker and '&&' as a literal
    // ampersand. MD3 has no mnemonic underline, so the marker is dropped rather
    // than painted — but the shortcut QAbstractButton installed still works,
    // because that is bound to the action, not to the glyphs on screen.
    QString out;
    out.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i) {
        if (raw.at(i) != QLatin1Char('&')) {
            out.append(raw.at(i));
        } else if (i + 1 < raw.size() && raw.at(i + 1) == QLatin1Char('&')) {
            out.append(QLatin1Char('&'));
            ++i;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------------

QRectF MdButton::containerRect() const
{
    const MdButtonTokens &resolved = tokens();
    return MdButtonStyle::layoutFor(*this, resolved).container;
}

QSize MdButton::sizeHint() const
{
    const MdButtonTokens &resolved = tokens();
    const MdButtonStyle::Layout layout = MdButtonStyle::layoutFor(*this, resolved);
    const qreal inset =
        MdButtonStyle::focusRingInset(MdButtonStyle::focusRingSpec(resolved));

    // The widget is the container plus a transparent margin the outward focus
    // indicator paints into. Qt clips a child to its own rect, so without the
    // margin the indicator would simply be cut off — see focusRingInset().
    return QSize(int(std::ceil(layout.preferredContainerSize.width() + 2.0 * inset)),
                 int(std::ceil(layout.preferredContainerSize.height() + 2.0 * inset)));
}

QSize MdButton::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// interaction
// ---------------------------------------------------------------------------

void MdButton::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        // Swallow rather than forward: a disabled button must not take focus
        // either, which is exactly what separates it from a soft-disabled one.
        event->accept();
        return;
    }

    m_pressPosition = event->pos() - containerRect().topLeft();
    m_hasPressPosition = true;
    // QAbstractButton::mousePressEvent emits pressed() synchronously, which is
    // what lets onPressed() know whether a pointer position is available.
    QPushButton::mousePressEvent(event);
    m_hasPressPosition = false;
}

void MdButton::mouseReleaseEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        event->accept();
        return;
    }
    QPushButton::mouseReleaseEvent(event);
}

void MdButton::enterEvent(QEnterEvent *event)
{
    if (!m_hovered) {
        m_hovered = true;
        update();
    }
    QPushButton::enterEvent(event);
}

void MdButton::leaveEvent(QEvent *event)
{
    if (m_hovered) {
        m_hovered = false;
        update();
    }
    QPushButton::leaveEvent(event);
}

void MdButton::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
    if (m_focusRing) {
        m_focusRing->start();
    }
    update();
}

void MdButton::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdButton::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        switch (event->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            event->accept();
            return;
        default:
            break;
        }
    }
    QPushButton::keyPressEvent(event);
}

void MdButton::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        if (!isEnabled()) {
            // A button that became disabled has nothing in flight.
            if (m_ripple) {
                m_ripple->cancel();
            }
            animateMorphTo(0.0);
        }
        update();
        break;
    case QEvent::FontChange:
    case QEvent::StyleChange:
    case QEvent::LayoutDirectionChange:
    case QEvent::ApplicationLayoutDirectionChange:
        updateGeometry();
        update();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// state machines
// ---------------------------------------------------------------------------

void MdButton::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

void MdButton::onPressed()
{
    animateMorphTo(1.0);

    if (m_ripple) {
        if (m_hasPressPosition) {
            m_ripple->setBounds(containerRect().size());
            m_ripple->press(m_pressPosition);
        } else {
            // Keyboard activation: no pointer position exists, so the ripple
            // starts from the middle of the container.
            m_ripple->setBounds(containerRect().size());
            m_ripple->pressCentered();
        }
    }
    update();
}

void MdButton::onReleased()
{
    animateMorphTo(0.0);
    if (m_ripple) {
        m_ripple->release();
    }
    update();
}

void MdButton::animateMorphTo(qreal target)
{
    if (m_morphTimer->isActive() && qFuzzyCompare(m_morphTo, target)) {
        return;
    }
    m_morphFrom = m_morph;
    m_morphTo = target;

    if (qFuzzyCompare(m_morphFrom, m_morphTo)) {
        m_morph = target;
        m_morphTimer->stop();
        update();
        return;
    }

    m_morphClock.restart();
    m_morphTimer->start();
}

void MdButton::onMorphTick()
{
    const MdButtonTokens &resolved = tokens();
    const MdSpring spring(resolved.springStiffness, resolved.springDampingRatio);

    const qreal seconds = qreal(m_morphClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_morph = m_morphFrom + (m_morphTo - m_morphFrom) * progress;

    // A damped spring approaches its target asymptotically, so the elapsed-time
    // bound is what guarantees the timer stops even when `progress` never quite
    // reaches 1.0. That is the difference between "settles" and "runs forever".
    const qreal settleMs = spring.settlingDurationMs();
    if (progress >= 1.0 || qreal(m_morphClock.elapsed()) >= settleMs) {
        m_morph = m_morphTo;
        m_morphTimer->stop();
    }
    update();
}

} // namespace md
