#include "MdTooltip.h"

#include "styles/MdTooltipStyle.h"

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

MdTooltip::MdTooltip(QWidget *parent)
    : QWidget(parent)
    , m_actionRipple(this)
{
    init();
}

MdTooltip::MdTooltip(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
    , m_actionRipple(this)
{
    init();
}

MdTooltip::~MdTooltip() = default;

void MdTooltip::init()
{
    setFocusPolicy(hasAction() ? Qt::TabFocus : Qt::NoFocus);
    setMouseTracking(true);
    MdTooltipStyle::shared();
    MdStyleBase::connectThemeUpdate(this, &MdTooltip::onThemeChanged);
    connect(&m_actionRipple, &MdRippleController::repaintRequested, this,
            qOverload<>(&QWidget::update));
}

void MdTooltip::setVariant(MdTooltipVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The action is a rich-tooltip element; dropping to Plain clears it the
    // way MdSnackbar clears the dismiss when its row disappears.
    if (variant == MdTooltipVariant::Plain) {
        m_title.clear();
        m_actionLabel.clear();
        setFocusPolicy(Qt::NoFocus);
        emit titleChanged(m_title);
        emit actionLabelChanged(m_actionLabel);
    }
    updateGeometry();
    update();
    emit variantChanged(m_variant);
}

void MdTooltip::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    updateGeometry();
    update();
    emit textChanged(m_text);
}

void MdTooltip::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    if (m_variant == MdTooltipVariant::Rich) {
        updateGeometry();
        update();
    }
    emit titleChanged(m_title);
}

void MdTooltip::setActionLabel(const QString &actionLabel)
{
    if (m_actionLabel == actionLabel) {
        return;
    }
    m_actionLabel = actionLabel;
    if (m_variant == MdTooltipVariant::Rich) {
        setFocusPolicy(hasAction() ? Qt::TabFocus : Qt::NoFocus);
        updateGeometry();
        update();
    }
    emit actionLabelChanged(m_actionLabel);
}

void MdTooltip::setCaretSide(MdTooltipCaretSide side)
{
    if (m_caretSide == side) {
        return;
    }
    m_caretSide = side;
    updateGeometry();
    update();
    emit caretSideChanged(m_caretSide);
}

void MdTooltip::setPaintOpacity(qreal opacity)
{
    const qreal clamped = qBound(0.0, opacity, 1.0);
    if (m_paintOpacity == clamped) {
        return;
    }
    m_paintOpacity = clamped;
    setVisible(m_paintOpacity > 0.0);
    update();
}

const MdTooltipTokens &MdTooltip::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdTooltipTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTooltip::invalidateTokens()
{
    m_tokensDirty = true;
    update();
}

MdTooltipActionState MdTooltip::actionState() const
{
    if (!hasAction() || m_variant != MdTooltipVariant::Rich) {
        return MdTooltipActionState::Enabled;
    }
    if (m_pressed) {
        return MdTooltipActionState::Pressed;
    }
    if (actionHasKeyboardFocus()) {
        return MdTooltipActionState::Focused;
    }
    if (m_hovered) {
        return MdTooltipActionState::Hovered;
    }
    return MdTooltipActionState::Enabled;
}

// --- geometry ---------------------------------------------------------------

namespace {

/// Horizontal margins: the rich container's level-2 shadow headroom.
qreal horizontalMargin(const MdTooltip *tooltip)
{
    return tooltip->variant() == MdTooltipVariant::Rich ? MdTooltipTokens::kShadowMargin : 0.0;
}

/// Vertical margins: the shadow headroom plus the caret's protrusion on its
/// side of the container.
QPair<qreal, qreal> verticalMargins(const MdTooltip *tooltip)
{
    const qreal shadow = tooltip->variant() == MdTooltipVariant::Rich
        ? MdTooltipTokens::kShadowMargin
        : 0.0;
    qreal top = shadow;
    qreal bottom = shadow;
    switch (tooltip->caretSide()) {
    case MdTooltipCaretSide::Top:
        top += MdTooltipTokens::kCaretMargin;
        break;
    case MdTooltipCaretSide::Bottom:
        bottom += MdTooltipTokens::kCaretMargin;
        break;
    case MdTooltipCaretSide::None:
    case MdTooltipCaretSide::Count:
        break;
    }
    return {top, bottom};
}

} // namespace

QRectF MdTooltip::containerRect() const
{
    const qreal horizontal = horizontalMargin(this);
    const QPair<qreal, qreal> vertical = verticalMargins(this);
    return QRectF(rect()).adjusted(horizontal, vertical.first, -horizontal, -vertical.second);
}

QRectF MdTooltip::caretRect() const
{
    if (m_caretSide == MdTooltipCaretSide::None) {
        return {};
    }
    const QRectF container = containerRect();
    const qreal centerX = container.center().x();
    if (m_caretSide == MdTooltipCaretSide::Bottom) {
        return QRectF(centerX - MdTooltipTokens::kCaretWidth / 2.0, container.bottom(),
                      MdTooltipTokens::kCaretWidth, MdTooltipTokens::kCaretMargin);
    }
    return QRectF(centerX - MdTooltipTokens::kCaretWidth / 2.0,
                  container.top() - MdTooltipTokens::kCaretMargin, MdTooltipTokens::kCaretWidth,
                  MdTooltipTokens::kCaretMargin);
}

QRectF MdTooltip::actionRect() const
{
    if (!hasAction() || m_variant != MdTooltipVariant::Rich) {
        return {};
    }
    const MdTooltipTokens t = tokens();
    return MdTooltipStyle::richLayout(
               containerRect(), m_title, m_text, m_actionLabel,
               MdTypeScale::font(t.richSubheadStyle), MdTypeScale::font(t.richTextStyle),
               MdTypeScale::font(t.richActionLabelStyle), t)
        .actionRect;
}

QRectF MdTooltip::actionHitRect() const
{
    if (!hasAction() || m_variant != MdTooltipVariant::Rich) {
        return {};
    }
    const MdTooltipTokens t = tokens();
    return MdTooltipStyle::richLayout(
               containerRect(), m_title, m_text, m_actionLabel,
               MdTypeScale::font(t.richSubheadStyle), MdTypeScale::font(t.richTextStyle),
               MdTypeScale::font(t.richActionLabelStyle), t)
        .actionHitRect;
}

QSize MdTooltip::sizeHint() const
{
    const MdTooltipTokens t = tokens();
    const qreal containerWidth = m_variant == MdTooltipVariant::Plain
        ? MdTooltipStyle::plainWidthFor(m_text, t)
        : MdTooltipStyle::richWidthFor(m_title, m_text, m_actionLabel, t);
    return QSize(int(std::ceil(containerWidth + horizontalMargin(this) * 2.0)),
                 int(std::ceil(heightForWidth(int(std::ceil(containerWidth))
                                       + int(horizontalMargin(this) * 2.0)))));
}

QSize MdTooltip::minimumSizeHint() const
{
    const MdTooltipTokens t = tokens();
    const qreal containerWidth = t.kMinWidth;
    return QSize(int(std::ceil(containerWidth + horizontalMargin(this) * 2.0)),
                 int(std::ceil(heightForWidth(int(std::ceil(containerWidth))
                                       + int(horizontalMargin(this) * 2.0)))));
}

int MdTooltip::heightForWidth(int width) const
{
    const MdTooltipTokens t = tokens();
    // `width` is the widget width; the layout runs on the container inside
    // the margins, and the height comes back with the margins added.
    const qreal horizontal = horizontalMargin(this);
    const QPair<qreal, qreal> vertical = verticalMargins(this);
    const qreal inner = qMax<qreal>(0.0, width - horizontal * 2.0);
    const qreal contentHeight = m_variant == MdTooltipVariant::Plain
        ? MdTooltipStyle::plainHeightForWidth(inner, m_text, t)
        : MdTooltipStyle::richHeightForWidth(inner, m_title, m_text, m_actionLabel, t);
    return int(std::ceil(contentHeight + vertical.first + vertical.second));
}

// --- interaction ------------------------------------------------------------

void MdTooltip::setHovered(bool hovered)
{
    if (m_hovered == hovered) {
        return;
    }
    m_hovered = hovered;
    update();
}

bool MdTooltip::actionRegionAt(const QPointF &position) const
{
    const QRectF hit = actionHitRect();
    return hit.isValid() && hit.contains(position);
}

void MdTooltip::mousePressEvent(QMouseEvent *event)
{
    if (actionRegionAt(mousePosition(event))) {
        m_pressed = true;
        const QRectF hit = actionHitRect();
        m_actionRipple.setBounds(QSizeF(hit.width(), hit.height()));
        m_actionRipple.setClipPath(MdShape::roundedRect(hit, ShapeCorner::Full));
        m_actionRipple.press(mousePosition(event));
    }
    // Deliberately no setFocus(): a pointer press shows no focus ring.
    update();
    event->accept();
}

void MdTooltip::mouseMoveEvent(QMouseEvent *event)
{
    setHovered(actionRegionAt(mousePosition(event)));
    event->accept();
}

void MdTooltip::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_pressed;
    m_pressed = false;
    if (wasPressed) {
        m_actionRipple.release();
        if (actionRegionAt(mousePosition(event))) {
            emit actionClicked();
        }
    }
    update();
    event->accept();
}

void MdTooltip::enterEvent(md::MdEnterEvent *event)
{
    QWidget::enterEvent(event);
    setHovered(actionRegionAt(enterPosition(this, event)));
}

void MdTooltip::leaveEvent(QEvent *event)
{
    setHovered(false);
    QWidget::leaveEvent(event);
}

void MdTooltip::focusInEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason;
    update();
    QWidget::focusInEvent(event);
}

void MdTooltip::focusOutEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = false;
    update();
    QWidget::focusOutEvent(event);
}

void MdTooltip::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Enter:
    case Qt::Key_Return:
    case Qt::Key_Space:
        if (hasFocus() && hasAction() && m_variant == MdTooltipVariant::Rich) {
            m_actionRipple.pressCentered();
            m_actionRipple.release();
            emit actionClicked();
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdTooltip::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::EnabledChange) {
        update();
    }
    QWidget::changeEvent(event);
}

bool MdTooltip::event(QEvent *event)
{
    const bool result = QWidget::event(event);
    if (event->type() == QEvent::LanguageChange) {
        update();
    }
    return result;
}

void MdTooltip::onThemeChanged()
{
    invalidateTokens();
}

} // namespace md
