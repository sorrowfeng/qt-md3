#include "MdSplitButton.h"

#include "styles/MdSplitButtonStyle.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtCore/QTimer>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>

namespace md {

namespace {

constexpr int kMorphTickMs = 16;
constexpr int kLeadingIndex = 0;
constexpr int kTrailingIndex = 1;

int indexFor(MdSplitButton::Zone zone)
{
    return zone == MdSplitButton::Zone::Trailing ? kTrailingIndex : kLeadingIndex;
}

} // namespace

MdSplitButton::MdSplitButton(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSplitButton::MdSplitButton(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
{
    init();
}

MdSplitButton::~MdSplitButton() = default;

void MdSplitButton::init()
{
    // Tab focus only: a pointer press must not move focus, because the focus
    // indicator and the Focused colour rows are `:focus-visible` semantics —
    // exactly the rule MdButton's interaction audit pinned down.
    setFocusPolicy(Qt::TabFocus);
    // Hover moves between two halves inside one widget, so move events are
    // needed even with no button held.
    setMouseTracking(true);

    for (MdRippleController *&ripple : m_ripples) {
        ripple = new MdRippleController(this);
        connect(ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));
    }
    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_morphTimer = new QTimer(this);
    m_morphTimer->setInterval(kMorphTickMs);
    connect(m_morphTimer, &QTimer::timeout, this, &MdSplitButton::onMorphTick);

    MdStyleBase::connectThemeUpdate(this, &MdSplitButton::onThemeChanged);

    // Install the shared style with the first split button, so the widget is
    // never momentarily painted by the platform style.
    MdSplitButtonStyle::shared();
}

// ---------------------------------------------------------------------------
// configuration
// ---------------------------------------------------------------------------

void MdSplitButton::setVariant(ButtonVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    emit variantChanged(variant);
}

void MdSplitButton::setSplitSize(SplitButtonSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    invalidateTokens();
    emit splitSizeChanged(size);
}

void MdSplitButton::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateGeometry();
    update();
    emit textChanged(text);
}

void MdSplitButton::setLeadingIcon(const QString &icon)
{
    if (m_leadingIcon == icon) {
        return;
    }
    m_leadingIcon = icon;
    updateGeometry();
    update();
    emit leadingIconChanged(icon);
}

void MdSplitButton::setTrailingIcon(const QString &icon)
{
    if (m_trailingIcon == icon) {
        return;
    }
    m_trailingIcon = icon;
    updateGeometry();
    update();
    emit trailingIconChanged(icon);
}

void MdSplitButton::setTrailingSelected(bool selected)
{
    if (m_trailingSelected == selected) {
        return;
    }
    m_trailingSelected = selected;
    // The selected inner corner is a different radius, so the trailing half
    // animates to it with the same spring a hover morph uses.
    animateMorphTo(Zone::Trailing, innerRadiusTarget(Zone::Trailing));
    update();
    emit trailingSelectedChanged(selected);
}

void MdSplitButton::setSoftDisabled(bool softDisabled)
{
    if (m_softDisabled == softDisabled) {
        return;
    }
    m_softDisabled = softDisabled;
    update();
    emit softDisabledChanged(softDisabled);
}

bool MdSplitButton::isEffectivelyDisabled() const
{
    return !isEnabled() || m_softDisabled;
}

// ---------------------------------------------------------------------------
// interaction state
// ---------------------------------------------------------------------------

void MdSplitButton::setHoveredZone(Zone zone)
{
    if (m_hoveredZone == zone) {
        return;
    }
    const Zone previous = m_hoveredZone;
    m_hoveredZone = zone;
    // The half that stopped being hovered relaxes back to its resting corner.
    if (previous != Zone::None && previous != m_pressedZone) {
        animateMorphTo(previous, innerRadiusTarget(previous));
    }
    animateMorphTo(zone, innerRadiusTarget(zone));
    update();
}

qreal MdSplitButton::innerRadiusTarget(Zone zone) const
{
    if (zone == Zone::None) {
        return tokens().metrics.innerCornerRest;
    }
    const MdSplitButtonMetrics &metrics = tokens().metrics;
    // Precedence is the published rows': selected (trailing only) beats
    // pressed, pressed beats hovered, everything else rests.
    if (zone == Zone::Trailing && m_trailingSelected) {
        return metrics.selectedInnerCornerRadius();
    }
    if (m_pressedZone == zone) {
        return metrics.innerCornerPressed;
    }
    if (m_hoveredZone == zone) {
        return metrics.innerCornerHovered;
    }
    return metrics.innerCornerRest;
}

qreal MdSplitButton::innerCornerRadius(Zone zone) const
{
    if (zone == Zone::None) {
        return tokens().metrics.innerCornerRest;
    }
    return m_innerRadius[indexFor(zone)];
}

void MdSplitButton::animateMorphTo(Zone zone, qreal target)
{
    if (zone == Zone::None) {
        return;
    }
    const int index = indexFor(zone);
    m_radiusFrom[index] = m_innerRadius[index];
    m_radiusTo[index] = target;

    if (qFuzzyCompare(m_radiusFrom[index], m_radiusTo[index])) {
        m_innerRadius[index] = target;
        return;
    }
    m_morphClock[index].restart();
    m_morphTimer->start();
}

void MdSplitButton::onMorphTick()
{
    bool anyActive = false;
    const MdSplitButtonTokens &resolved = tokens();
    // The split-button export publishes no motion rows; the morph runs on the
    // button family's press spring (see MdSplitButtonTokens.h).
    const MdSpring spring(resolved.button.springStiffness, resolved.button.springDampingRatio);

    for (int index = 0; index < 2; ++index) {
        if (qFuzzyCompare(m_innerRadius[index], m_radiusTo[index])) {
            continue;
        }
        const qreal seconds = qreal(m_morphClock[index].elapsed()) / 1000.0;
        const qreal progress = spring.valueAt(seconds);
        m_innerRadius[index] =
            m_radiusFrom[index] + (m_radiusTo[index] - m_radiusFrom[index]) * progress;

        // A damped spring approaches asymptotically; the elapsed bound is
        // what makes the timer stop (same reasoning as MdButton's morph).
        if (progress >= 1.0 || qreal(m_morphClock[index].elapsed()) >= spring.settlingDurationMs()) {
            m_innerRadius[index] = m_radiusTo[index];
        } else {
            anyActive = true;
        }
    }
    update();
    if (!anyActive) {
        m_morphTimer->stop();
    }
}

// ---------------------------------------------------------------------------
// tokens
// ---------------------------------------------------------------------------

const MdSplitButtonTokens &MdSplitButton::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSplitButtonTokens::resolve(m_variant, m_size, &m_componentTokens);
        m_tokensDirty = false;
        // A config change snaps the corners rather than animating between
        // token sets — the morph spring belongs to a state change, not to a
        // property edit.
        for (int index = 0; index < 2; ++index) {
            m_innerRadius[index] = m_tokens.metrics.innerCornerRest;
            m_radiusTo[index] = m_tokens.metrics.innerCornerRest;
        }
    }
    return m_tokens;
}

void MdSplitButton::invalidateTokens()
{
    m_tokensDirty = true;
    updateGeometry();
    update();
}

void MdSplitButton::onThemeChanged()
{
    invalidateTokens();
}

// ---------------------------------------------------------------------------
// controllers
// ---------------------------------------------------------------------------

MdRippleController *MdSplitButton::rippleController(Zone zone) const
{
    if (zone == Zone::None) {
        return nullptr;
    }
    return m_ripples[indexFor(zone)];
}

// ---------------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------------

QRectF MdSplitButton::containerRect() const
{
    return MdSplitButtonStyle::layoutFor(*this, tokens()).container;
}

QRectF MdSplitButton::leadingRect() const
{
    return MdSplitButtonStyle::layoutFor(*this, tokens()).leading;
}

QRectF MdSplitButton::trailingRect() const
{
    return MdSplitButtonStyle::layoutFor(*this, tokens()).trailing;
}

MdSplitButton::Zone MdSplitButton::zoneAt(const QPointF &position) const
{
    if (leadingRect().contains(position)) {
        return Zone::Leading;
    }
    if (trailingRect().contains(position)) {
        return Zone::Trailing;
    }
    return Zone::None;
}

QSize MdSplitButton::sizeHint() const
{
    const MdSplitButtonTokens &resolved = tokens();
    const MdSplitButtonMetrics &metrics = resolved.metrics;
    const MdButtonTokens &button = resolved.button;
    const qreal inset = MdSplitButtonStyle::focusRingInset(button);

    const bool rtl = MdTheme::instance().isRightToLeft();
    const QString icon = rtl ? m_trailingIcon : m_leadingIcon; // inline-start icon
    const bool hasLabel = !m_text.isEmpty();
    const bool hasIcon = !icon.isEmpty();

    qreal leadingWidth = metrics.leadingLeadingSpace + metrics.leadingTrailingSpace;
    if (hasIcon) {
        leadingWidth += button.iconSize;
        if (hasLabel) {
            leadingWidth += button.iconLabelSpace;
        }
    }
    if (hasLabel) {
        const QFont font = MdTypeScale::font(button.labelStyle, TypeEmphasis::Baseline,
                                             MdTheme::instance().scriptCategory());
        leadingWidth += QFontMetricsF(font).horizontalAdvance(m_text);
    }

    const qreal trailingWidth =
        metrics.trailingLeadingSpace + metrics.trailingIconSize + metrics.trailingTrailingSpace;

    const qreal width = 2.0 * inset + leadingWidth + metrics.betweenSpace + trailingWidth;
    const qreal height = 2.0 * inset + metrics.containerHeight;
    return QSize(int(std::ceil(width)), int(std::ceil(height)));
}

QSize MdSplitButton::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// pointer events
// ---------------------------------------------------------------------------

void MdSplitButton::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        event->accept();
        return;
    }
    const Zone zone = zoneAt(event->position());
    m_pressedZone = zone;
    if (zone != Zone::None) {
        const QRectF half = zone == Zone::Leading ? leadingRect() : trailingRect();
        rippleController(zone)->press(event->position() - half.topLeft());
        rippleController(zone)->setBounds(half.size());
        animateMorphTo(zone, innerRadiusTarget(zone));
        // Deliberately no setFocus(): a pointer press shows no focus ring.
    }
    update();
    event->accept();
}

void MdSplitButton::mouseMoveEvent(QMouseEvent *event)
{
    setHoveredZone(zoneAt(event->position()));
    event->accept();
}

void MdSplitButton::mouseReleaseEvent(QMouseEvent *event)
{
    const Zone pressed = m_pressedZone;
    m_pressedZone = Zone::None;
    if (pressed != Zone::None) {
        rippleController(pressed)->release();
        animateMorphTo(pressed, innerRadiusTarget(pressed));
    }
    const Zone releasedOn = zoneAt(event->position());
    update();
    if (pressed != Zone::None && releasedOn == pressed) {
        if (pressed == Zone::Leading) {
            emit leadingClicked();
        } else {
            emit trailingClicked();
        }
    }
    event->accept();
}

void MdSplitButton::enterEvent(QEnterEvent *event)
{
    setHoveredZone(zoneAt(event->position()));
    QWidget::enterEvent(event);
}

void MdSplitButton::leaveEvent(QEvent *event)
{
    setHoveredZone(Zone::None);
    QWidget::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------

MdSplitButton::Zone MdSplitButton::nextZone(Zone zone, int delta) const
{
    // Two halves, wrapped: trailing -> leading for delta < 0 and the reverse
    // for delta > 0, with None entering at the inline-start half.
    const int count = 2;
    int index = zone == Zone::Trailing ? 1 : 0;
    if (zone == Zone::None) {
        return delta < 0 ? Zone::Trailing : Zone::Leading;
    }
    index = (index + delta % count + count) % count;
    return index == 1 ? Zone::Trailing : Zone::Leading;
}

void MdSplitButton::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    // `:focus-visible` semantics, shared with MdButton: pointer and popup
    // focus show no ring and no Focused colours.
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
    if (m_focusIsKeyboard) {
        if (m_focusedZone == Zone::None) {
            m_focusedZone = Zone::Leading;
        }
        m_focusRing->start();
    }
    update();
}

void MdSplitButton::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    m_focusedZone = Zone::None;
    m_focusRing->stop();
    update();
}

void MdSplitButton::keyPressEvent(QKeyEvent *event)
{
    if (!isEffectivelyDisabled()) {
        const int key = event->key();
        // Left/Right walk the two halves (the split is a horizontal control);
        // Up/Down are consumed as no-ops so they never fall through to a
        // focus-widget change, mirroring the button group's rule.
        if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up
            || key == Qt::Key_Down) {
            if (key != Qt::Key_Up && key != Qt::Key_Down) {
                const bool rtl = MdTheme::instance().isRightToLeft();
                const bool forward = (key == Qt::Key_Right) != rtl;
                m_focusedZone = nextZone(m_focusedZone, forward ? 1 : -1);
            }
            event->accept();
            update();
            return;
        }
        if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter) {
            const Zone zone = m_focusedZone == Zone::None ? Zone::Leading : m_focusedZone;
            MdRippleController *ripple = rippleController(zone);
            const QRectF half = zone == Zone::Leading ? leadingRect() : trailingRect();
            ripple->setBounds(half.size());
            // Press-and-release in one go: the controller's own
            // minimum-press hold still gives the tap a visible ripple.
            ripple->pressCentered();
            ripple->release();
            if (zone == Zone::Leading) {
                emit leadingClicked();
            } else {
                emit trailingClicked();
            }
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdSplitButton::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
}

} // namespace md
