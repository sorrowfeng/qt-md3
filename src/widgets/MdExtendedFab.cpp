#include "MdExtendedFab.h"

#include "styles/MdExtendedFabStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdTheme.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdExtendedFab::MdExtendedFab(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdExtendedFab::MdExtendedFab(const QString &iconName, const QString &label, QWidget *parent)
    : QPushButton(label, parent)
    , m_iconName(iconName)
{
    init();
}

MdExtendedFab::MdExtendedFab(const QString &label, QWidget *parent)
    : QPushButton(label, parent)
{
    init();
}

MdExtendedFab::~MdExtendedFab() = default;

void MdExtendedFab::init()
{
    // Reachable with Tab, activates on Space and Enter — QAbstractButton
    // implements all of it.
    setFocusPolicy(Qt::StrongFocus);

    // An extended FAB without a label is a FAB with decoration; the label is
    // the accessible content, with the icon name folded into the name the
    // way a screen reader reads "icon, text".
    if (!m_iconName.isEmpty()) {
        setAccessibleName(m_iconName + QStringLiteral(" ") + text());
    } else {
        setAccessibleName(text());
    }

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    // QAbstractButton emits these for mouse *and* keyboard activation, so
    // Space produces the same ripple a click does without a second code path.
    connect(this, &QAbstractButton::pressed, this, &MdExtendedFab::onPressed);
    connect(this, &QAbstractButton::released, this, &MdExtendedFab::onReleased);

    MdStyleBase::connectThemeUpdate(this, &MdExtendedFab::onThemeChanged);

    // Create and register the shared style as soon as the first extended FAB
    // exists, so one is never momentarily painted by the platform style.
    MdExtendedFabStyle::shared();
}

void MdExtendedFab::setVariant(ExtendedFabVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant selects the colour set, so the cached token set is stale
    // the moment this lands.
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdExtendedFab::setFabSize(ExtendedFabSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    // The size set carries the metrics, the container shape and the label
    // type style, so the cached table has to go.
    invalidateTokens();
    updateGeometry();
    update();
    emit fabSizeChanged(m_size);
}

void MdExtendedFab::setLowered(bool lowered)
{
    if (m_lowered == lowered) {
        return;
    }
    m_lowered = lowered;
    // Lowered changes the elevation rows only in this family.
    invalidateTokens();
    update();
    emit loweredChanged(m_lowered);
}

void MdExtendedFab::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    // See init(): label + icon name make the accessible name, and a content
    // change changes the derived width.
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

void MdExtendedFab::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    emit iconSetChanged(m_iconSet);
    update();
}

void MdExtendedFab::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    emit iconFamilyChanged(m_iconFamily);
    update();
}

QRectF MdExtendedFab::containerRect() const
{
    const MdExtendedFabTokens &resolved = tokens();
    return MdExtendedFabStyle::layoutFor(*this, resolved).container;
}

const MdExtendedFabTokens &MdExtendedFab::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdExtendedFabTokens::resolve(m_variant, m_size, m_lowered, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdExtendedFab::sizeHint() const
{
    const MdExtendedFabTokens &resolved = tokens();
    const MdExtendedFabStyle::Layout layout = MdExtendedFabStyle::layoutFor(*this, resolved);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdExtendedFab::minimumSizeHint() const
{
    return sizeHint();
}

void MdExtendedFab::onPressed()
{
    if (m_ripple) {
        m_ripple->setBounds(containerRect().size());
        if (m_hasPressPosition) {
            m_ripple->press(m_pressPosition);
        } else {
            // Keyboard activation: the ripple starts from the middle.
            m_ripple->pressCentered();
        }
    }
    update();
}

void MdExtendedFab::onReleased()
{
    if (m_ripple) {
        m_ripple->release();
    }
    update();
}

void MdExtendedFab::mousePressEvent(QMouseEvent *event)
{
    m_hasPressPosition = true;
    // The ripple controller works in container-local coordinates; the layout
    // centres the container in the widget, so the same offset applies here.
    m_pressPosition = QPointF(event->position()) - containerRect().topLeft();
    QPushButton::mousePressEvent(event);
}

void MdExtendedFab::mouseReleaseEvent(QMouseEvent *event)
{
    m_hasPressPosition = false;
    QPushButton::mouseReleaseEvent(event);
}

void MdExtendedFab::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    QPushButton::enterEvent(event);
    update();
}

void MdExtendedFab::leaveEvent(QEvent *event)
{
    m_hovered = false;
    QPushButton::leaveEvent(event);
    update();
}

void MdExtendedFab::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
    // `:focus-visible` semantics, shared with MdButton and the other families
    // — pointer focus shows no ring, keyboard focus does.
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

void MdExtendedFab::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdExtendedFab::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);

    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::FontChange:
        // Never fires for this class (the style owns the font) but kept
        // symmetric with the text change below for future-proofing.
        break;
    default:
        break;
    }
}

void MdExtendedFab::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdExtendedFab::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
