#include "MdCollapse.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QChildEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <algorithm>
#include <cmath>

namespace md {

namespace {

/// List one-line height (`md.comp.list.list-item.container.height` is 56).
constexpr int kDefaultHeaderHeight = 56;
/// Trailing chevron slot: the list trailing target (24 px icon in a 48 px hit
/// box), the same measurement MdListItem uses.
constexpr qreal kChevronSlot = 48.0;
constexpr qreal kChevronSize = 24.0;
/// Label inset, matching the list's `label-space` 16 px start padding.
constexpr qreal kLabelInset = 16.0;
/// Inward focus-ring margin, matching the list item.
constexpr qreal kFocusMargin = 2.0;
/// Vertical padding around the body while animating.
constexpr qreal kBodyPadding = 0.0;

/// The collapse header's corner: the list Standard style is corner-none at
/// rest. The body inherits the same reading — a section of a list, not a card.
constexpr ShapeCorner kHeaderCorner = ShapeCorner::None;

} // namespace

MdCollapse::MdCollapse(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdCollapse::MdCollapse(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
{
    init();
}

MdCollapse::~MdCollapse() = default;

void MdCollapse::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(m_title.isEmpty() ? QStringLiteral("collapse") : m_title);

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&MdCollapse::update));

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this,
            qOverload<>(&MdCollapse::update));

    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MdCollapse::onAnimationFrame);

    MdStyleBase::connectThemeUpdate(this, &MdCollapse::onThemeChanged);
}

void MdCollapse::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    setAccessibleName(m_title.isEmpty() ? QStringLiteral("collapse") : m_title);
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdCollapse::setSupportingText(const QString &text)
{
    if (m_supportingText == text) {
        return;
    }
    m_supportingText = text;
    updateGeometry();
    update();
    emit supportingTextChanged(m_supportingText);
}

void MdCollapse::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;
    m_from = bodyProgress();
    m_target = m_expanded ? 1.0 : 0.0;
    m_clock.restart();
    updateAnimationState();
    updateGeometry();
    update();
    emit expandedChanged(m_expanded);
}

void MdCollapse::setHeaderHeight(int height)
{
    const int clamped = qMax(1, height);
    if (m_headerHeight == clamped) {
        return;
    }
    m_headerHeight = clamped;
    updateGeometry();
    relayout();
    update();
    emit headerHeightChanged(m_headerHeight);
}

void MdCollapse::setBodyWidget(QWidget *body)
{
    if (m_body == body) {
        return;
    }
    if (m_body != nullptr) {
        m_body->hide();
        m_body->setParent(nullptr);
    }
    m_body = body;
    if (m_body != nullptr) {
        m_body->setParent(this);
        m_body->show();
    }
    updateGeometry();
    relayout();
    update();
}

QRectF MdCollapse::headerRect() const
{
    return QRectF(0.0, 0.0, width(), m_headerHeight);
}

QRectF MdCollapse::chevronRect() const
{
    const QRectF header = headerRect();
    return QRectF(header.right() - kChevronSlot, header.top(),
                  kChevronSlot, header.height());
}

QSize MdCollapse::sizeHint() const
{
    // Header plus the *expanded* body so a layout reserves the open height.
    return QSize(240, m_headerHeight + bodyContentHeight());
}

QSize MdCollapse::minimumSizeHint() const
{
    // Collapsed floor: the header alone.
    return QSize(0, m_headerHeight);
}

int MdCollapse::heightForWidth(int width) const
{
    Q_UNUSED(width);
    return m_headerHeight + qRound(bodyContentHeight() * bodyProgress()) + kBodyPadding;
}

int MdCollapse::bodyContentHeight() const
{
    if (m_body == nullptr) {
        return 0;
    }
    const int hint = m_body->sizeHint().height();
    const int minHint = m_body->minimumSizeHint().height();
    return qMax(hint, minHint);
}

qreal MdCollapse::bodyProgress() const
{
    if (!m_timer->isActive()) {
        return m_target;
    }
    // FastSpatial spring: the spatial expand/collapse slot.
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = m_clock.elapsed() / 1000.0;
    const qreal t = spring.valueAt(seconds);
    const qreal value = m_from + (m_target - m_from) * t;
    return qBound(0.0, value, 1.0);
}

void MdCollapse::updateAnimationState()
{
    const bool animating = !qFuzzyCompare(m_from, m_target);
    if (animating && !m_timer->isActive()) {
        m_timer->start(16);
    } else if (!animating && m_timer->isActive()) {
        m_timer->stop();
    }
}

void MdCollapse::onAnimationFrame()
{
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = m_clock.elapsed() / 1000.0;
    const bool settled = seconds * 1000.0 >= spring.settlingDurationMs();
    if (settled) {
        m_timer->stop();
        m_from = m_target;
    }
    updateGeometry();
    relayout();
    update();
}

void MdCollapse::relayout()
{
    if (m_body == nullptr) {
        return;
    }
    const int bodyHeight = qRound(bodyContentHeight() * bodyProgress());
    const QRectF bodyRect(0.0, m_headerHeight + kBodyPadding,
                          width(), bodyHeight);
    m_body->setGeometry(bodyRect.toRect());
    // Clip the body to the animated height so content does not spill while
    // the spring is still travelling.
    m_body->setVisible(bodyHeight > 0);
}

// ---------------------------------------------------------------------------
// Painting (Pattern C — the header is list-row chrome)
// ---------------------------------------------------------------------------

void MdCollapse::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTheme &theme = MdTheme::instance();
    const QRectF header = headerRect();

    // Interactive list-row state layer: hover 0.08 / focus 0.12 / press 0.12,
    // painted over `surface` with `on-surface` as the layer colour.
    const QColor surface = theme.color(ColorRole::Surface);
    const QColor onSurface = theme.color(ColorRole::OnSurface);
    if (m_headerHovered || m_headerPressed || headerHasKeyboardFocus()) {
        QColor layer = surface;
        if (m_headerPressed) {
            layer = MdStateLayer::overlay(onSurface, StateLayerKind::Pressed);
        } else if (headerHasKeyboardFocus()) {
            layer = MdStateLayer::overlay(onSurface, StateLayerKind::Focus);
        } else {
            layer = MdStateLayer::overlay(onSurface, StateLayerKind::Hover);
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(layer);
        painter.drawRect(header);
    }

    // Ripple on the header when pressed.
    if (m_ripple != nullptr) {
        const MdRippleFrame frame = m_ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.setClipRect(header);
            painter.setPen(Qt::NoPen);
            QColor rippleColour = onSurface;
            rippleColour.setAlphaF(frame.opacity);
            painter.setBrush(rippleColour);
            painter.drawEllipse(frame.center, frame.radius, frame.radius);
            painter.restore();
        }
    }

    // Headline (body-large) + optional supporting text (body-medium).
    const qreal textLeft = header.left() + kLabelInset;
    const qreal textRight = header.right() - kChevronSlot - kLabelInset / 2.0;
    const QRectF textRect(textLeft, header.top(),
                          qMax(0.0, textRight - textLeft), header.height());

    const QFont headlineFont = MdTypeScale::font(TypeStyle::BodyLarge);
    const QFont supportFont = MdTypeScale::font(TypeStyle::BodyMedium);
    if (m_supportingText.isEmpty()) {
        painter.setFont(headlineFont);
        painter.setPen(theme.color(ColorRole::OnSurface));
        painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    } else {
        const qreal line = qMin(header.height() / 2.0, 28.0);
        QRectF top(textRect.left(), header.center().y() - line,
                   textRect.width(), line);
        QRectF bottom(textRect.left(), header.center().y(),
                      textRect.width(), line);
        painter.setFont(headlineFont);
        painter.setPen(theme.color(ColorRole::OnSurface));
        painter.drawText(top, Qt::AlignLeft | Qt::AlignVCenter, m_title);
        painter.setFont(supportFont);
        painter.setPen(theme.color(ColorRole::OnSurfaceVariant));
        painter.drawText(bottom, Qt::AlignLeft | Qt::AlignVCenter, m_supportingText);
    }

    // Trailing chevron. expand_less when open, expand_more when closed —
    // the list's disclosure indicator. Rotated 180° by swapping the glyph
    // name rather than rotating the painter, so the glyph stays upright.
    const QRectF chevron = chevronRect();
    const QRectF iconBox(chevron.center().x() - kChevronSize / 2.0,
                         chevron.center().y() - kChevronSize / 2.0,
                         kChevronSize, kChevronSize);
    const QString glyph = m_expanded ? QStringLiteral("expand_less")
                                     : QStringLiteral("expand_more");
    if (!MdIcon::paint(&painter, iconBox, glyph,
                       theme.color(ColorRole::OnSurfaceVariant))) {
        // Fallback chevron for font-less platforms: a simple polyline.
        QPen pen(theme.color(ColorRole::OnSurfaceVariant));
        pen.setWidthF(2.0);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        QPainterPath path;
        const QPointF c = chevron.center();
        if (m_expanded) {
            path.moveTo(c.x() - 5.0, c.y() + 2.5);
            path.lineTo(c.x(), c.y() - 2.5);
            path.lineTo(c.x() + 5.0, c.y() + 2.5);
        } else {
            path.moveTo(c.x() - 5.0, c.y() - 2.5);
            path.lineTo(c.x(), c.y() + 2.5);
            path.lineTo(c.x() + 5.0, c.y() - 2.5);
        }
        painter.drawPath(path);
    }

    // :focus-visible ring — keyboard focus only, inward like the list item.
    if (headerHasKeyboardFocus() && m_focusRing != nullptr) {
        const QRectF bounds = header.adjusted(kFocusMargin, kFocusMargin,
                                              -kFocusMargin, -kFocusMargin);
        MdFocusRingSpec spec;
        spec.inward = true;
        const QList<qreal> radii = MdShape::radii(kHeaderCorner);
        MdFocusRing::paint(&painter, bounds, radii,
                           theme.color(ColorRole::Secondary), spec,
                           m_focusRing->isAnimating() ? m_focusRing->elapsedMs() : -1);
    }
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdCollapse::mousePressEvent(QMouseEvent *event)
{
    if (!isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    const QPointF pos = mousePosition(event);
    if (headerRect().contains(pos)) {
        m_headerPressed = true;
        if (m_ripple != nullptr) {
            m_ripple->setBounds(headerRect().size());
            m_ripple->setContentColor(MdTheme::instance().color(ColorRole::OnSurface));
            m_ripple->press(pos);
        }
        update();
    }
    QWidget::mousePressEvent(event);
}

void MdCollapse::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_headerPressed;
    m_headerPressed = false;
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
    if (wasPressed && isEnabled() && headerRect().contains(mousePosition(event))) {
        toggle();
    }
    update();
    QWidget::mouseReleaseEvent(event);
}

void MdCollapse::enterEvent(MdEnterEvent *event)
{
    m_headerHovered = true;
    QWidget::enterEvent(event);
    update();
}

void MdCollapse::leaveEvent(QEvent *event)
{
    m_headerHovered = false;
    QWidget::leaveEvent(event);
    update();
}

void MdCollapse::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    // `:focus-visible` semantics, shared with the button families.
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
    if (m_focusRing != nullptr) {
        if (m_focusIsKeyboard) {
            m_focusRing->start();
        } else {
            m_focusRing->stop();
        }
    }
    update();
}

void MdCollapse::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdCollapse::keyPressEvent(QKeyEvent *event)
{
    if (isEnabled()
        && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Return)) {
        if (m_ripple != nullptr) {
            m_ripple->pressCentered();
            m_ripple->release();
        }
        toggle();
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdCollapse::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdCollapse::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdCollapse::childEvent(QChildEvent *event)
{
    QWidget::childEvent(event);
    if (event->type() == QEvent::ChildRemoved && event->child() == m_body) {
        m_body = nullptr;
        updateGeometry();
    }
}

void MdCollapse::onThemeChanged()
{
    update();
}

} // namespace md
