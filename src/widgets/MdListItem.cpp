#include "MdListItem.h"

#include "styles/MdListItemStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtGui/QFocusEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdListItem::MdListItem(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdListItem::MdListItem(const QString &headline, QWidget *parent)
    : QWidget(parent)
    , m_headline(headline)
{
    init();
}

MdListItem::~MdListItem() = default;

void MdListItem::init()
{
    // The paint hub owns painting — registering the shared style as soon as
    // the first item exists means an item is never momentarily painted by the
    // platform style.
    MdListItemStyle::shared();

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    // The long axis is filled by the list; the cross axis comes from the
    // measured height.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    applyInteractivity();
    MdStyleBase::connectThemeUpdate(this, &MdListItem::onThemeChanged);
}

void MdListItem::applyInteractivity()
{
    // Exactly the material-web split: `type="button"` items are focusable,
    // `type="text"` items are not.
    setFocusPolicy(m_interactive ? Qt::StrongFocus : Qt::NoFocus);
    updateGeometry();
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

void MdListItem::setOverline(const QString &text)
{
    if (m_overline == text) {
        return;
    }
    m_overline = text;
    updateGeometry();
    update();
    emit overlineChanged(m_overline);
}

void MdListItem::setHeadline(const QString &text)
{
    if (m_headline == text) {
        return;
    }
    m_headline = text;
    updateGeometry();
    update();
    emit headlineChanged(m_headline);
}

void MdListItem::setSupporting(const QString &text)
{
    if (m_supporting == text) {
        return;
    }
    m_supporting = text;
    updateGeometry();
    update();
    emit supportingChanged(m_supporting);
}

void MdListItem::setTrailingSupporting(const QString &text)
{
    if (m_trailingSupporting == text) {
        return;
    }
    m_trailingSupporting = text;
    updateGeometry();
    update();
    emit trailingSupportingChanged(m_trailingSupporting);
}

int MdListItem::lineCount() const
{
    return MdListItemStyle::lineCount(*this);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MdListItem::setLeadingKind(LeadingKind kind)
{
    if (m_leadingKind == kind) {
        return;
    }
    m_leadingKind = kind;
    updateGeometry();
    update();
}

void MdListItem::setLeadingIcon(const QString &name)
{
    if (m_leadingIcon == name) {
        return;
    }
    m_leadingIcon = name;
    if (!name.isEmpty() && m_leadingKind == LeadingKind::None) {
        m_leadingKind = LeadingKind::Icon;
    } else if (name.isEmpty() && m_leadingKind == LeadingKind::Icon) {
        m_leadingKind = LeadingKind::None;
    }
    updateGeometry();
    update();
    emit leadingIconChanged(m_leadingIcon);
}

void MdListItem::setAvatarLabel(const QString &text)
{
    if (m_avatarLabel == text) {
        return;
    }
    m_avatarLabel = text;
    update();
}

void MdListItem::setLeadingWidget(QWidget *widget)
{
    if (m_leadingWidget == widget) {
        return;
    }
    m_leadingWidget = widget;
    if (widget) {
        widget->setParent(this);
        widget->show();
    }
    placeSlotWidgets();
    update();
}

QList<qreal> MdListItem::leadingSlotRadii() const
{
    if (m_leadingKind == LeadingKind::None && m_leadingWidget == nullptr) {
        return {};
    }
    return MdListItemStyle::radiiFor(*this, tokens(), QSizeF());
}

void MdListItem::setTrailingIcon(const QString &name)
{
    if (m_trailingIcon == name) {
        return;
    }
    m_trailingIcon = name;
    updateGeometry();
    update();
    emit trailingIconChanged(m_trailingIcon);
}

void MdListItem::setTrailingWidget(QWidget *widget)
{
    if (m_trailingWidget == widget) {
        return;
    }
    m_trailingWidget = widget;
    if (widget) {
        widget->setParent(this);
        widget->show();
    }
    placeSlotWidgets();
    update();
}

void MdListItem::placeSlotWidgets()
{
    const MdListItemStyle::Layout layout = MdListItemStyle::layoutFor(*this, tokens());
    if (m_leadingWidget && layout.leading.isValid()) {
        m_leadingWidget->setGeometry(layout.leading.toRect());
    }
    if (m_trailingWidget && layout.trailing.isValid()) {
        m_trailingWidget->setGeometry(layout.trailing.toRect());
    }
}

// ---------------------------------------------------------------------------
// Style / state
// ---------------------------------------------------------------------------

void MdListItem::setVariant(MdListVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant owns the whole shape table.
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdListItem::setInteractive(bool interactive)
{
    if (m_interactive == interactive) {
        return;
    }
    m_interactive = interactive;
    if (!m_interactive) {
        m_hovered = false;
        m_pressed = false;
        m_focusIsKeyboard = false;
        if (m_ripple) {
            m_ripple->release();
        }
        if (m_focusRing) {
            m_focusRing->stop();
        }
    }
    applyInteractivity();
    update();
    emit interactiveChanged(m_interactive);
}

void MdListItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
    emit selectedChanged(m_selected);
}

void MdListItem::setDragged(bool dragged)
{
    if (m_dragged == dragged) {
        return;
    }
    m_dragged = dragged;
    update();
    emit draggedChanged(m_dragged);
}

void MdListItem::setSegmentedPosition(int index, int count)
{
    if (m_segmentIndex == index && m_segmentCount == count) {
        return;
    }
    m_segmentIndex = index;
    m_segmentCount = count;
    update();
}

// ---------------------------------------------------------------------------
// Tokens / geometry
// ---------------------------------------------------------------------------

const MdListTokens &MdListItem::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdListTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QRectF MdListItem::containerRect() const
{
    return MdListItemStyle::layoutFor(*this, tokens()).container;
}

QSize MdListItem::sizeHint() const
{
    const int height = int(std::ceil(heightForWidth(width() > 0 ? width() : 360)));
    return QSize(QWidget::sizeHint().width(), height);
}

QSize MdListItem::minimumSizeHint() const
{
    return QSize(0, sizeHint().height());
}

int MdListItem::heightForWidth(int width) const
{
    return int(std::ceil(MdListItemStyle::measuredHeight(*this, tokens(), qreal(width))));
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdListItem::syncRippleGeometry()
{
    if (!m_ripple) {
        return;
    }
    const MdListItemStyle::Layout layout = MdListItemStyle::layoutFor(*this, tokens());
    m_ripple->setBounds(layout.container.size());
    const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
    m_ripple->setClipPath(MdShape::roundedRect(localRect, layout.radii));
    m_ripple->setContentColor(MdTheme::instance().color(
        m_selected ? tokens().selectedFamily.pressed.stateLayer : tokens().family.pressed.stateLayer));
}

void MdListItem::mousePressEvent(QMouseEvent *event)
{
    if (!m_interactive || !isEnabled()) {
        QWidget::mousePressEvent(event);
        return;
    }
    m_pressed = true;
    if (m_ripple) {
        syncRippleGeometry();
        m_ripple->press(mousePosition(event) - containerRect().topLeft());
    }
    update();
    QWidget::mousePressEvent(event);
}

void MdListItem::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_pressed;
    m_pressed = false;
    if (m_ripple) {
        m_ripple->release();
    }
    if (wasPressed && m_interactive && isEnabled()
        && rect().contains(mousePosition(event).toPoint())) {
        emit activated();
    }
    update();
    QWidget::mouseReleaseEvent(event);
}

void MdListItem::enterEvent(MdEnterEvent *event)
{
    m_hovered = true;
    QWidget::enterEvent(event);
    update();
}

void MdListItem::leaveEvent(QEvent *event)
{
    m_hovered = false;
    QWidget::leaveEvent(event);
    update();
}

void MdListItem::focusInEvent(QFocusEvent *event)
{
    QWidget::focusInEvent(event);
    // `:focus-visible` semantics, shared with the button families and cards.
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
    update();
}

void MdListItem::focusOutEvent(QFocusEvent *event)
{
    QWidget::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdListItem::keyPressEvent(QKeyEvent *event)
{
    if (m_interactive && isEnabled()
        && (event->key() == Qt::Key_Space || event->key() == Qt::Key_Enter
            || event->key() == Qt::Key_Return)) {
        if (m_ripple) {
            syncRippleGeometry();
            m_ripple->pressCentered();
            m_ripple->release();
        }
        emit activated();
        event->accept();
        return;
    }
    // Navigation keys belong to the host list; let them through.
    QWidget::keyPressEvent(event);
}

void MdListItem::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        // The disabled row's colours and opacities change with the state.
        update();
        break;
    default:
        break;
    }
}

void MdListItem::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    syncRippleGeometry();
    placeSlotWidgets();
}

void MdListItem::onThemeChanged()
{
    invalidateTokens();
    syncRippleGeometry();
    update();
}

void MdListItem::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
