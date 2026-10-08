#include "MdFab.h"

#include "styles/MdFabStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdTheme.h"

#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdFab::MdFab(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdFab::MdFab(const QString &iconName, QWidget *parent)
    : QPushButton(parent)
    , m_iconName(iconName)
{
    init();
}

MdFab::~MdFab() = default;

void MdFab::init()
{
    // Reachable with Tab, activates on Space and Enter — QAbstractButton
    // implements all of it.
    setFocusPolicy(Qt::StrongFocus);

    // The icon is the whole accessible content; the icon name doubles as the
    // accessible name — an icon-only control with no name is unusable with a
    // screen reader.
    if (!m_iconName.isEmpty()) {
        setAccessibleName(m_iconName);
        setToolTip(m_iconName);
    }

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    // QAbstractButton emits these for mouse *and* keyboard activation, so
    // Space produces the same ripple a click does without a second code path.
    connect(this, &QAbstractButton::pressed, this, &MdFab::onPressed);
    connect(this, &QAbstractButton::released, this, &MdFab::onReleased);

    MdStyleBase::connectThemeUpdate(this, &MdFab::onThemeChanged);

    // Create and register the shared style as soon as the first FAB exists,
    // so a FAB is never momentarily painted by the platform style.
    MdFabStyle::shared();
}

void MdFab::setVariant(FabVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant selects the colour set (and, for a lowered surface FAB, the
    // lowered container), so the cached token set is stale the moment this
    // lands.
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdFab::setFabSize(FabSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    // The size set carries the metrics and the container shape, so the cached
    // table has to go.
    invalidateTokens();
    updateGeometry();
    update();
    emit fabSizeChanged(m_size);
}

void MdFab::setLowered(bool lowered)
{
    if (m_lowered == lowered) {
        return;
    }
    m_lowered = lowered;
    // Lowered changes the elevation rows and — for the surface variant — the
    // container colour, both resolved at tokens() time.
    invalidateTokens();
    update();
    emit loweredChanged(m_lowered);
}

void MdFab::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    // See init(): the icon name is the accessible content.
    setAccessibleName(m_iconName);
    setToolTip(m_iconName);
    emit iconNameChanged(m_iconName);
    update();
}

void MdFab::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    emit iconSetChanged(m_iconSet);
    update();
}

void MdFab::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    emit iconFamilyChanged(m_iconFamily);
    update();
}

QRectF MdFab::containerRect() const
{
    const MdFabTokens &resolved = tokens();
    return MdFabStyle::layoutFor(*this, resolved).container;
}

const MdFabTokens &MdFab::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdFabTokens::resolve(m_variant, m_size, m_lowered, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdFab::sizeHint() const
{
    const MdFabTokens &resolved = tokens();
    const MdFabStyle::Layout layout = MdFabStyle::layoutFor(*this, resolved);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdFab::minimumSizeHint() const
{
    return sizeHint();
}

void MdFab::onPressed()
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

void MdFab::onReleased()
{
    if (m_ripple) {
        m_ripple->release();
    }
    update();
}

void MdFab::mousePressEvent(QMouseEvent *event)
{
    m_hasPressPosition = true;
    // The ripple controller works in container-local coordinates; the layout
    // centres the container in the widget, so the same offset applies here.
    m_pressPosition = QPointF(event->position()) - containerRect().topLeft();
    QPushButton::mousePressEvent(event);
}

void MdFab::mouseReleaseEvent(QMouseEvent *event)
{
    m_hasPressPosition = false;
    QPushButton::mouseReleaseEvent(event);
}

void MdFab::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    QPushButton::enterEvent(event);
    update();
}

void MdFab::leaveEvent(QEvent *event)
{
    m_hovered = false;
    QPushButton::leaveEvent(event);
    update();
}

void MdFab::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
    // `:focus-visible` semantics, shared with MdButton and MdIconButton —
    // pointer focus shows no ring, keyboard focus does.
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

void MdFab::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdFab::changeEvent(QEvent *event)
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

void MdFab::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdFab::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
