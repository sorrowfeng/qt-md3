#include "MdFabMenuItem.h"

#include "styles/MdFabMenuStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdFabMenuItem::MdFabMenuItem(FabMenuElement elementRole, FabMenuVariant variant,
                             QWidget *parent)
    : QPushButton(parent)
    , m_elementRole(elementRole)
    , m_variant(variant)
{
    init();
}

MdFabMenuItem::MdFabMenuItem(FabMenuElement elementRole, FabMenuVariant variant,
                             const QString &iconName, const QString &label, QWidget *parent)
    : QPushButton(label, parent)
    , m_elementRole(elementRole)
    , m_variant(variant)
    , m_iconName(iconName)
{
    init();
}

MdFabMenuItem::~MdFabMenuItem() = default;

void MdFabMenuItem::init()
{
    setFocusPolicy(Qt::StrongFocus);

    if (!m_iconName.isEmpty() && !text().isEmpty()) {
        setAccessibleName(m_iconName + QStringLiteral(" ") + text());
    } else if (!m_iconName.isEmpty()) {
        setAccessibleName(m_iconName);
    } else {
        setAccessibleName(text());
    }

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    connect(this, &QAbstractButton::pressed, this, &MdFabMenuItem::onPressed);
    connect(this, &QAbstractButton::released, this, &MdFabMenuItem::onReleased);

    MdStyleBase::connectThemeUpdate(this, &MdFabMenuItem::onThemeChanged);

    MdFabMenuStyle::shared();
}

void MdFabMenuItem::setElementRole(FabMenuElement role)
{
    if (m_elementRole == role) {
        return;
    }
    m_elementRole = role;
    invalidateTokens();
    updateGeometry();
    update();
    emit elementRoleChanged(m_elementRole);
}

void MdFabMenuItem::setVariant(FabMenuVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdFabMenuItem::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    if (!m_iconName.isEmpty() && !text().isEmpty()) {
        setAccessibleName(m_iconName + QStringLiteral(" ") + text());
    } else if (!m_iconName.isEmpty()) {
        setAccessibleName(m_iconName);
    } else {
        setAccessibleName(text());
    }
    updateGeometry();
    emit iconNameChanged(m_iconName);
    update();
}

void MdFabMenuItem::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    emit iconSetChanged(m_iconSet);
    update();
}

void MdFabMenuItem::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    emit iconFamilyChanged(m_iconFamily);
    update();
}

void MdFabMenuItem::setReveal(qreal reveal)
{
    const qreal clamped = qBound<qreal>(0.0, reveal, 1.0);
    if (qFuzzyCompare(m_reveal, clamped)) {
        return;
    }
    m_reveal = clamped;
    setVisible(m_reveal > 0.0);
    update();
}

QRectF MdFabMenuItem::containerRect() const
{
    const MdFabMenuTokens &resolved = menuTokens();
    const MdFabMenuElementTokens &element =
        m_elementRole == FabMenuElement::CloseButton ? resolved.closeButton : resolved.listItem;
    return MdFabMenuStyle::layoutFor(*this, element).container;
}

const MdFabMenuTokens &MdFabMenuItem::menuTokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdFabMenuTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdFabMenuItem::sizeHint() const
{
    const MdFabMenuTokens &resolved = menuTokens();
    const MdFabMenuElementTokens &element =
        m_elementRole == FabMenuElement::CloseButton ? resolved.closeButton : resolved.listItem;
    const MdFabMenuStyle::Layout layout = MdFabMenuStyle::layoutFor(*this, element);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdFabMenuItem::minimumSizeHint() const
{
    return sizeHint();
}

void MdFabMenuItem::onPressed()
{
    if (m_ripple) {
        m_ripple->setBounds(containerRect().size());
        if (m_hasPressPosition) {
            m_ripple->press(m_pressPosition);
        } else {
            m_ripple->pressCentered();
        }
    }
    update();
}

void MdFabMenuItem::onReleased()
{
    if (m_ripple) {
        m_ripple->release();
    }
    update();
}

void MdFabMenuItem::mousePressEvent(QMouseEvent *event)
{
    m_hasPressPosition = true;
    m_pressPosition = QPointF(mousePosition(event)) - containerRect().topLeft();
    QPushButton::mousePressEvent(event);
}

void MdFabMenuItem::mouseReleaseEvent(QMouseEvent *event)
{
    m_hasPressPosition = false;
    QPushButton::mouseReleaseEvent(event);
}

void MdFabMenuItem::enterEvent(MdEnterEvent *event)
{
    m_hovered = true;
    QPushButton::enterEvent(event);
    update();
}

void MdFabMenuItem::leaveEvent(QEvent *event)
{
    m_hovered = false;
    QPushButton::leaveEvent(event);
    update();
}

void MdFabMenuItem::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
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
    if (m_focusRing) {
        if (m_focusIsKeyboard) {
            m_focusRing->start();
        } else {
            m_focusRing->stop();
        }
    }
    update();
}

void MdFabMenuItem::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdFabMenuItem::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    default:
        break;
    }
}

void MdFabMenuItem::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdFabMenuItem::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
