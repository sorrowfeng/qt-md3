#include "MdSnackbar.h"

#include "styles/MdSnackbarStyle.h"

#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QStyle>

#include <cmath>

namespace md {

MdSnackbar::MdSnackbar(QWidget *parent)
    : QWidget(parent)
    , m_actionRipple(this)
    , m_iconRipple(this)
{
    init();
}

MdSnackbar::MdSnackbar(const QString &message, QWidget *parent)
    : QWidget(parent)
    , m_message(message)
    , m_actionRipple(this)
    , m_iconRipple(this)
{
    init();
}

MdSnackbar::~MdSnackbar() = default;

void MdSnackbar::init()
{
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);
    MdSnackbarStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdSnackbar::onThemeChanged);
    connect(&m_actionRipple, &MdRippleController::repaintRequested, this,
            qOverload<>(&QWidget::update));
    connect(&m_iconRipple, &MdRippleController::repaintRequested, this,
            qOverload<>(&QWidget::update));
}

void MdSnackbar::setMessage(const QString &message)
{
    if (m_message == message) {
        return;
    }
    m_message = message;
    updateGeometry();
    update();
    emit messageChanged(m_message);
}

void MdSnackbar::setActionLabel(const QString &actionLabel)
{
    if (m_actionLabel == actionLabel) {
        return;
    }
    m_actionLabel = actionLabel;
    if (m_actionLabel.isEmpty()) {
        m_hovered = m_hovered == Action ? None : m_hovered;
        m_pressed = m_pressed == Action ? None : m_pressed;
        if (m_focusIndex == 0) {
            m_focusIndex = m_hasDismissAction ? 1 : -1;
        }
    }
    updateGeometry();
    update();
    emit actionLabelChanged(m_actionLabel);
}

void MdSnackbar::setHasDismissAction(bool show)
{
    if (m_hasDismissAction == show) {
        return;
    }
    m_hasDismissAction = show;
    if (!show) {
        m_hovered = m_hovered == Dismiss ? None : m_hovered;
        m_pressed = m_pressed == Dismiss ? None : m_pressed;
        if (m_focusIndex == 1) {
            m_focusIndex = hasAction() ? 0 : -1;
        }
    }
    updateGeometry();
    update();
    emit hasDismissActionChanged(m_hasDismissAction);
}

void MdSnackbar::setActionOnNewLine(bool onNewLine)
{
    if (m_actionOnNewLine == onNewLine) {
        return;
    }
    m_actionOnNewLine = onNewLine;
    updateGeometry();
    update();
    emit actionOnNewLineChanged(m_actionOnNewLine);
}

void MdSnackbar::setPaintOpacity(qreal opacity)
{
    const qreal clamped = qBound(0.0, opacity, 1.0);
    if (m_paintOpacity == clamped) {
        return;
    }
    m_paintOpacity = clamped;
    setVisible(m_paintOpacity > 0.0);
    update();
}

const MdSnackbarTokens &MdSnackbar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdSnackbarTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdSnackbar::invalidateTokens()
{
    m_tokensDirty = true;
    update();
}

MdSnackbarState MdSnackbar::actionState() const
{
    if (m_pressed == Action) {
        return MdSnackbarState::Pressed;
    }
    if (actionHasKeyboardFocus()) {
        return MdSnackbarState::Focused;
    }
    if (m_hovered == Action) {
        return MdSnackbarState::Hovered;
    }
    return MdSnackbarState::Enabled;
}

MdSnackbarState MdSnackbar::iconState() const
{
    if (m_pressed == Dismiss) {
        return MdSnackbarState::Pressed;
    }
    if (dismissHasKeyboardFocus()) {
        return MdSnackbarState::Focused;
    }
    if (m_hovered == Dismiss) {
        return MdSnackbarState::Hovered;
    }
    return MdSnackbarState::Enabled;
}

QRectF MdSnackbar::containerRect() const
{
    const qreal margin = tokens().kShadowMargin;
    return QRectF(rect()).adjusted(margin, margin, -margin, -margin);
}

QRectF MdSnackbar::actionRect() const
{
    if (!hasAction()) {
        return {};
    }
    const MdSnackbarTokens t = tokens();
    const QFont supporting = MdTypeScale::font(t.supportingTextStyle);
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QRectF bounds(containerRect());
    if (m_actionOnNewLine) {
        return MdSnackbarStyle::newLineLayout(bounds, m_message, m_actionLabel,
                                              m_hasDismissAction, supporting, action, t.iconSize,
                                              t)
            .actionRect;
    }
    return MdSnackbarStyle::oneRowLayout(bounds, m_message, m_actionLabel, m_hasDismissAction,
                                         supporting, action, t.iconSize, t)
        .actionRect;
}

QRectF MdSnackbar::dismissRect() const
{
    if (!m_hasDismissAction) {
        return {};
    }
    const MdSnackbarTokens t = tokens();
    const QFont supporting = MdTypeScale::font(t.supportingTextStyle);
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QRectF bounds(containerRect());
    if (m_actionOnNewLine && hasAction()) {
        return MdSnackbarStyle::newLineLayout(bounds, m_message, m_actionLabel,
                                              m_hasDismissAction, supporting, action, t.iconSize,
                                              t)
            .dismissRect;
    }
    return MdSnackbarStyle::oneRowLayout(bounds, m_message, m_actionLabel, m_hasDismissAction,
                                         supporting, action, t.iconSize, t)
        .dismissRect;
}

QRectF MdSnackbar::actionHitRect() const
{
    if (!hasAction()) {
        return {};
    }
    const MdSnackbarTokens t = tokens();
    const QFont supporting = MdTypeScale::font(t.supportingTextStyle);
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QRectF bounds(containerRect());
    if (m_actionOnNewLine) {
        return MdSnackbarStyle::newLineLayout(bounds, m_message, m_actionLabel,
                                              m_hasDismissAction, supporting, action, t.iconSize,
                                              t)
            .actionHitRect;
    }
    return MdSnackbarStyle::oneRowLayout(bounds, m_message, m_actionLabel, m_hasDismissAction,
                                         supporting, action, t.iconSize, t)
        .actionHitRect;
}

QRectF MdSnackbar::dismissHitRect() const
{
    if (!m_hasDismissAction) {
        return {};
    }
    const MdSnackbarTokens t = tokens();
    const QFont supporting = MdTypeScale::font(t.supportingTextStyle);
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QRectF bounds(containerRect());
    if (m_actionOnNewLine && hasAction()) {
        return MdSnackbarStyle::newLineLayout(bounds, m_message, m_actionLabel,
                                              m_hasDismissAction, supporting, action, t.iconSize,
                                              t)
            .dismissHitRect;
    }
    return MdSnackbarStyle::oneRowLayout(bounds, m_message, m_actionLabel, m_hasDismissAction,
                                         supporting, action, t.iconSize, t)
        .dismissHitRect;
}

QSize MdSnackbar::sizeHint() const
{
    const MdSnackbarTokens t = tokens();
    // Compose fills the available width up to 600 px; standalone the natural
    // width is the content width with the one-row paddings, capped at 600.
    const QFont supporting = MdTypeScale::font(t.supportingTextStyle);
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QFontMetricsF actionMetrics(action);
    qreal width = t.kHorizontalSpacing * 2.0
        + QFontMetricsF(supporting).horizontalAdvance(m_message)
        + (hasAction() ? actionMetrics.horizontalAdvance(m_actionLabel) + 24.0 : 0.0)
        + (m_hasDismissAction ? 40.0 : t.kTextEndExtraSpacing);
    width = qMin<qreal>(width, t.kContainerMaxWidth);
    return QSize(int(std::ceil(width + t.kShadowMargin * 2.0)),
                 int(std::ceil(heightForWidth(int(width)))));
}

QSize MdSnackbar::minimumSizeHint() const
{
    const MdSnackbarTokens t = tokens();
    const QFont action = MdTypeScale::font(t.actionLabelStyle);
    const QFontMetricsF actionMetrics(action);
    qreal width = t.kHorizontalSpacing * 2.0
        + (hasAction() ? actionMetrics.horizontalAdvance(m_actionLabel) + 24.0 : 0.0)
        + (m_hasDismissAction ? 40.0 : t.kTextEndExtraSpacing);
    return QSize(int(std::ceil(width + t.kShadowMargin * 2.0)),
                 int(std::ceil(heightForWidth(int(width)))));
}

int MdSnackbar::heightForWidth(int width) const
{
    const MdSnackbarTokens t = tokens();
    // `width` is the widget width; the layout runs on the container inside
    // the shadow margin, and the height comes back with the margin added.
    const qreal inner = qMax<qreal>(0.0, width - t.kShadowMargin * 2.0);
    return int(std::ceil(MdSnackbarStyle::heightForWidth(inner, m_message, m_actionOnNewLine,
                                                         m_actionLabel, m_hasDismissAction, t)
                         + t.kShadowMargin * 2.0));
}

MdSnackbar::Region MdSnackbar::regionAt(const QPointF &position) const
{
    const QRectF action = actionHitRect();
    if (action.isValid() && action.contains(position)) {
        return Action;
    }
    const QRectF dismiss = dismissHitRect();
    if (dismiss.isValid() && dismiss.contains(position)) {
        return Dismiss;
    }
    return None;
}

void MdSnackbar::setHovered(Region region)
{
    if (m_hovered == region) {
        return;
    }
    m_hovered = region;
    update();
}

void MdSnackbar::mousePressEvent(QMouseEvent *event)
{
    const Region region = regionAt(mousePosition(event));
    m_pressed = region;
    if (region == Action) {
        const QRectF hit = actionHitRect();
        m_actionRipple.setBounds(QSizeF(hit.width(), hit.height()));
        m_actionRipple.setClipPath(MdShape::roundedRect(hit, ShapeCorner::Full));
        m_actionRipple.press(mousePosition(event));
    } else if (region == Dismiss) {
        const QRectF hit = dismissHitRect();
        m_iconRipple.setBounds(QSizeF(hit.width(), hit.height()));
        m_iconRipple.setClipPath(MdShape::roundedRect(hit, ShapeCorner::Full));
        m_iconRipple.press(mousePosition(event));
    }
    // Deliberately no setFocus(): a pointer press shows no focus ring.
    update();
    event->accept();
}

void MdSnackbar::mouseMoveEvent(QMouseEvent *event)
{
    setHovered(regionAt(mousePosition(event)));
    event->accept();
}

void MdSnackbar::mouseReleaseEvent(QMouseEvent *event)
{
    const Region pressed = m_pressed;
    m_pressed = None;
    if (pressed == Action) {
        m_actionRipple.release();
        if (regionAt(mousePosition(event)) == Action) {
            emit actionClicked();
        }
    } else if (pressed == Dismiss) {
        m_iconRipple.release();
        if (regionAt(mousePosition(event)) == Dismiss) {
            emit dismissClicked();
        }
    }
    update();
    event->accept();
}

void MdSnackbar::enterEvent(md::MdEnterEvent *event)
{
    QWidget::enterEvent(event);
    setHovered(regionAt(enterPosition(this, event)));
}

void MdSnackbar::leaveEvent(QEvent *event)
{
    setHovered(None);
    QWidget::leaveEvent(event);
}

void MdSnackbar::focusInEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason;
    if (m_focusIndex < 0) {
        m_focusIndex = hasAction() ? 0 : (m_hasDismissAction ? 1 : -1);
    }
    update();
    QWidget::focusInEvent(event);
}

void MdSnackbar::focusOutEvent(QFocusEvent *event)
{
    m_focusIndex = -1;
    m_focusIsKeyboard = false;
    update();
    QWidget::focusOutEvent(event);
}

void MdSnackbar::keyPressEvent(QKeyEvent *event)
{
    const int count = (hasAction() ? 1 : 0) + (m_hasDismissAction ? 1 : 0);
    switch (event->key()) {
    case Qt::Key_Tab:
        if (count > 1) {
            m_focusIndex = (m_focusIndex + 1) % count;
            m_focusIsKeyboard = true;
            update();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Backtab:
        if (count > 1) {
            m_focusIndex = (m_focusIndex + count - 1) % count;
            m_focusIsKeyboard = true;
            update();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Left:
    case Qt::Key_Right:
        if (count > 1) {
            m_focusIndex = m_focusIndex < 0 ? (hasAction() ? 0 : 1) : (m_focusIndex + 1) % count;
            m_focusIsKeyboard = true;
            update();
            event->accept();
            return;
        }
        break;
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Space:
        if (m_focusIndex >= 0) {
            activate(m_focusIndex == 0 ? Action : Dismiss);
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdSnackbar::activate(Region region)
{
    if (region == Action) {
        m_actionRipple.pressCentered();
        m_actionRipple.release();
        emit actionClicked();
    } else if (region == Dismiss) {
        m_iconRipple.pressCentered();
        m_iconRipple.release();
        emit dismissClicked();
    }
    update();
}

void MdSnackbar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
    QWidget::changeEvent(event);
}

bool MdSnackbar::event(QEvent *event)
{
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::LanguageChange) {
        update();
    }
    return result;
}

void MdSnackbar::onThemeChanged()
{
    invalidateTokens();
}

} // namespace md
