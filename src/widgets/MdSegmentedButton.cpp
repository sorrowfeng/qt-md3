#include "MdSegmentedButton.h"

#include "styles/MdSegmentedButtonStyle.h"
#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QFontMetricsF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QApplication>

namespace md {

namespace {

constexpr int kMorphTickMs = 16;

/// spring-fast-spatial — the spring the Compose check scale-in uses
/// (MotionSchemeKeyTokens.FastSpatial).
MdSpring checkSpring()
{
    return MdMotion::spring(MotionSpring::SpatialFast);
}

} // namespace

MdSegmentedButton::MdSegmentedButton(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSegmentedButton::MdSegmentedButton(const QStringList &segments, QWidget *parent)
    : QWidget(parent)
    , m_segments(segments)
{
    init();
}

MdSegmentedButton::~MdSegmentedButton() = default;

void MdSegmentedButton::init()
{
    // The segments ctor sets m_segments in its init-list, before this runs:
    // sync every per-segment array here so both construction paths agree.
    m_checked.fill(false, m_segments.size());
    m_checkMorph.fill(0.0, m_segments.size());
    m_morphFrom.fill(0.0, m_segments.size());
    m_morphTo.fill(0.0, m_segments.size());

    // Tab focus only: a pointer press must not move focus (`:focus-visible`).
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_morphTimer = new QTimer(this);
    m_morphTimer->setInterval(kMorphTickMs);
    connect(m_morphTimer, &QTimer::timeout, this, &MdSegmentedButton::onMorphTick);

    rebuildControllers();

    MdStyleBase::connectThemeUpdate(this, &MdSegmentedButton::onThemeChanged);
    MdSegmentedButtonStyle::shared();
}

void MdSegmentedButton::rebuildControllers()
{
    const int count = m_segments.size();
    while (m_ripples.size() > count) {
        m_ripples.takeLast()->deleteLater();
    }
    while (m_ripples.size() < count) {
        auto *ripple = new MdRippleController(this);
        connect(ripple, &MdRippleController::repaintRequested, this,
                qOverload<>(&QWidget::update));
        m_ripples.append(ripple);
    }
}

// ---------------------------------------------------------------------------
// configuration
// ---------------------------------------------------------------------------

void MdSegmentedButton::setSegments(const QStringList &segments)
{
    if (m_segments == segments) {
        return;
    }
    m_segments = segments;
    m_checked.fill(false, m_segments.size());
    m_checkMorph.fill(0.0, m_segments.size());
    m_morphFrom.fill(0.0, m_segments.size());
    m_morphTo.fill(0.0, m_segments.size());
    m_hovered = m_pressed = m_focused = -1;
    rebuildControllers();
    invalidateTokens();
    emit segmentsChanged(segments);
}

void MdSegmentedButton::setLeadingIcons(const QStringList &leadingIcons)
{
    if (m_leadingIcons == leadingIcons) {
        return;
    }
    m_leadingIcons = leadingIcons;
    update();
    emit leadingIconsChanged(leadingIcons);
}

void MdSegmentedButton::setSingleChoice(bool singleChoice)
{
    if (m_singleChoice == singleChoice) {
        return;
    }
    m_singleChoice = singleChoice;
    if (m_singleChoice) {
        // Collapse to the first checked segment, radio-style.
        const QList<int> checked = checkedIndexes();
        for (int i = 1; i < checked.size(); ++i) {
            setChecked(checked.at(i), false);
        }
    }
    emit singleChoiceChanged(singleChoice);
}

void MdSegmentedButton::setSoftDisabled(bool softDisabled)
{
    if (m_softDisabled == softDisabled) {
        return;
    }
    m_softDisabled = softDisabled;
    update();
}

// ---------------------------------------------------------------------------
// selection
// ---------------------------------------------------------------------------

void MdSegmentedButton::setChecked(int index, bool checked)
{
    if (index < 0 || index >= m_segments.size() || isChecked(index) == checked) {
        return;
    }
    m_checked[index] = checked;
    animateCheckTo(index, checked ? 1.0 : 0.0);
    if (checked && m_singleChoice) {
        for (int i = 0; i < m_checked.size(); ++i) {
            if (i != index && m_checked.at(i)) {
                m_checked[i] = false;
                animateCheckTo(i, 0.0);
            }
        }
    }
    update();
    emit segmentChecked(index, checked);
}

bool MdSegmentedButton::isChecked(int index) const
{
    return index >= 0 && index < m_checked.size() && m_checked.at(index);
}

QList<int> MdSegmentedButton::checkedIndexes() const
{
    QList<int> result;
    for (int i = 0; i < m_checked.size(); ++i) {
        if (m_checked.at(i)) {
            result.append(i);
        }
    }
    return result;
}

int MdSegmentedButton::checkedIndex() const
{
    const QList<int> checked = checkedIndexes();
    return checked.isEmpty() ? -1 : checked.first();
}

void MdSegmentedButton::activate(int index)
{
    if (index < 0 || index >= m_segments.size() || isEffectivelyDisabled()) {
        return;
    }
    setChecked(index, !isChecked(index));
    emit segmentActivated(index);
}

// ---------------------------------------------------------------------------
// check morph
// ---------------------------------------------------------------------------

qreal MdSegmentedButton::checkMorph(int index) const
{
    if (index < 0 || index >= m_checkMorph.size()) {
        return 0.0;
    }
    return m_checkMorph.at(index);
}

void MdSegmentedButton::animateCheckTo(int index, qreal target)
{
    if (index < 0 || index >= m_checkMorph.size()) {
        return;
    }
    m_morphFrom[index] = m_checkMorph.at(index);
    m_morphTo[index] = target;
    if (qFuzzyCompare(m_morphFrom[index], m_morphTo[index])) {
        m_checkMorph[index] = target;
        return;
    }
    m_morphClock.restart();
    m_morphTimer->start();
}

void MdSegmentedButton::onMorphTick()
{
    const MdSpring spring = checkSpring();
    const qreal seconds = qreal(m_morphClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    bool anyActive = false;

    for (int i = 0; i < m_checkMorph.size(); ++i) {
        if (qFuzzyCompare(m_checkMorph.at(i), m_morphTo.at(i))) {
            continue;
        }
        m_checkMorph[i] = m_morphFrom.at(i) + (m_morphTo.at(i) - m_morphFrom.at(i)) * progress;
        if (progress >= 1.0 || qreal(m_morphClock.elapsed()) >= spring.settlingDurationMs()) {
            m_checkMorph[i] = m_morphTo.at(i);
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

const MdSegmentedButtonTokens &MdSegmentedButton::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSegmentedButtonTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdSegmentedButton::invalidateTokens()
{
    m_tokensDirty = true;
    updateGeometry();
    update();
}

void MdSegmentedButton::onThemeChanged()
{
    invalidateTokens();
}

MdRippleController *MdSegmentedButton::rippleController(int index) const
{
    return (index >= 0 && index < m_ripples.size()) ? m_ripples.at(index) : nullptr;
}

// ---------------------------------------------------------------------------
// geometry
// ---------------------------------------------------------------------------

QRectF MdSegmentedButton::segmentRect(int index) const
{
    if (index < 0 || index >= m_segments.size()) {
        return QRectF();
    }
    // Lay the segments out the way the style does; the geometry helpers and
    // the painter must agree to the pixel.
    const MdSegmentedButtonStyle::Layout layout =
        MdSegmentedButtonStyle::layoutFor(*this, tokens());
    if (index >= layout.segments.size()) {
        return QRectF();
    }
    return layout.segments.at(index).rect;
}

QSize MdSegmentedButton::sizeHint() const
{
    const MdSegmentedButtonTokens &resolved = tokens();
    const MdSegmentedButtonStyle::Layout layout =
        MdSegmentedButtonStyle::layoutFor(*this, resolved);
    const qreal inset = MdSegmentedButtonStyle::focusRingInset(resolved);
    const qreal width = 2.0 * inset + layout.contentWidth;
    const qreal height = 2.0 * inset + resolved.containerHeight;
    return QSize(int(std::ceil(width)), int(std::ceil(height)));
}

QSize MdSegmentedButton::minimumSizeHint() const
{
    return sizeHint();
}

int MdSegmentedButton::segmentAt(const QPointF &position) const
{
    for (int i = 0; i < m_segments.size(); ++i) {
        if (segmentRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// pointer events
// ---------------------------------------------------------------------------

void MdSegmentedButton::setHoveredSegment(int index)
{
    if (m_hovered == index) {
        return;
    }
    m_hovered = index;
    update();
}

void MdSegmentedButton::mousePressEvent(QMouseEvent *event)
{
    if (isEffectivelyDisabled()) {
        event->accept();
        return;
    }
    const int index = segmentAt(mousePosition(event));
    m_pressed = index;
    if (MdRippleController *ripple = rippleController(index)) {
        const QRectF rect = segmentRect(index);
        ripple->setBounds(rect.size());
        ripple->press(mousePosition(event) - rect.topLeft());
        // Deliberately no setFocus(): pointer focus shows no ring.
    }
    update();
    event->accept();
}

void MdSegmentedButton::mouseMoveEvent(QMouseEvent *event)
{
    setHoveredSegment(segmentAt(mousePosition(event)));
    event->accept();
}

void MdSegmentedButton::mouseReleaseEvent(QMouseEvent *event)
{
    const int pressed = m_pressed;
    m_pressed = -1;
    if (pressed >= 0) {
        if (MdRippleController *ripple = rippleController(pressed)) {
            ripple->release();
        }
        if (segmentAt(mousePosition(event)) == pressed) {
            activate(pressed);
        }
    }
    update();
    event->accept();
}

void MdSegmentedButton::enterEvent(MdEnterEvent *event)
{
    setHoveredSegment(segmentAt(enterPosition(this, event)));
    QWidget::enterEvent(event);
}

void MdSegmentedButton::leaveEvent(QEvent *event)
{
    setHoveredSegment(-1);
    QWidget::leaveEvent(event);
}

// ---------------------------------------------------------------------------
// keyboard
// ---------------------------------------------------------------------------

void MdSegmentedButton::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
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
    if (m_focusIsKeyboard && m_focused < 0) {
        m_focused = qMax(0, checkedIndex());
    }
    update();
}

void MdSegmentedButton::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    m_focused = -1;
    update();
}

void MdSegmentedButton::keyPressEvent(QKeyEvent *event)
{
    if (!isEffectivelyDisabled() && !m_segments.isEmpty()) {
        const int key = event->key();
        if (key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up
            || key == Qt::Key_Down) {
            if (key != Qt::Key_Up && key != Qt::Key_Down) {
                // Same-axis navigation with wrap; cross-axis keys are no-ops.
                const bool rtl = MdTheme::instance().isRightToLeft();
                const bool forward = (key == Qt::Key_Right) != rtl;
                const int count = m_segments.size();
                const int current = m_focused < 0 ? 0 : m_focused;
                m_focused = forward ? (current + 1) % count
                                    : (current + count - 1) % count;
            }
            event->accept();
            update();
            return;
        }
        if (key == Qt::Key_Space || key == Qt::Key_Return || key == Qt::Key_Enter) {
            const int index = m_focused < 0 ? 0 : m_focused;
            const QRectF rect = segmentRect(index);
            if (MdRippleController *ripple = rippleController(index)) {
                ripple->setBounds(rect.size());
                ripple->pressCentered();
                ripple->release();
            }
            activate(index);
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdSegmentedButton::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
}

} // namespace md
