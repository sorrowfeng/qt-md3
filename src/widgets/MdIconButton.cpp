#include "MdIconButton.h"

#include "styles/MdIconButtonStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <QtGui/QFocusEvent>
#include <QtGui/QMouseEvent>
#include <QtCore/QTimer>

namespace md {

MdIconButton::MdIconButton(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdIconButton::MdIconButton(const QString &iconName, QWidget *parent)
    : QPushButton(parent)
    , m_iconName(iconName)
{
    init();
}

MdIconButton::~MdIconButton() = default;

void MdIconButton::init()
{
    // An icon button is reachable with Tab and activates on Space and Enter,
    // both of which QAbstractButton already implements.
    setFocusPolicy(Qt::StrongFocus);

    // The icon is the whole accessible content. QAbstractButton's accessible
    // name comes from text(), which is empty here by definition, so the icon
    // name doubles as the accessible name — an icon-only control with no name
    // is unusable with a screen reader.
    if (!m_iconName.isEmpty()) {
        setAccessibleName(m_iconName);
        setToolTip(m_iconName);
    }

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_morphTimer = new QTimer(this);
    m_morphTimer->setInterval(16);
    m_morphTimer->setTimerType(Qt::PreciseTimer);
    connect(m_morphTimer, &QTimer::timeout, this, &MdIconButton::onMorphTick);

    // QAbstractButton emits these for mouse *and* keyboard activation, so
    // Space produces the same ripple and the same corner morph a click does
    // without a second code path.
    connect(this, &QAbstractButton::pressed, this, &MdIconButton::onPressed);
    connect(this, &QAbstractButton::released, this, &MdIconButton::onReleased);

    // The selection is Qt's checked state, so it must be *toggled* — by a
    // click, by Space, or by setChecked — that announces it, not the
    // setSelected() convenience alone. Wiring the signal here means every path
    // reports it, and a click can never bypass it the way it would bypass a
    // signal emitted only from setSelected().
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        emit selectedChanged(checked);
        // The checked state changes both the colour family and the resting
        // corner shape, which is a paint change even with no interaction.
        update();
    });

    MdStyleBase::connectThemeUpdate(this, &MdIconButton::onThemeChanged);

    // Create and register the shared style as soon as the first icon button
    // exists, so a button is never momentarily painted by the platform style.
    MdIconButtonStyle::shared();
}

void MdIconButton::setVariant(IconButtonVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // The variant selects the colour families and (through the style file
    // fallbacks) nothing metric — but the cached token set is resolved per
    // variant, so it has to go.
    invalidateTokens();
    update();
    emit variantChanged(m_variant);
}

void MdIconButton::setButtonSize(ButtonSize size)
{
    if (m_size == size) {
        return;
    }
    m_size = size;
    // The size set carries the metrics — height, icon size, tracks, shapes —
    // so the cached table is stale the moment this lands.
    invalidateTokens();
    updateGeometry();
    update();
    emit buttonSizeChanged(m_size);
}

void MdIconButton::setButtonShape(ButtonShape shape)
{
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    // The shape knob chooses between two published resting corners, resolved
    // per shape at tokens() time.
    invalidateTokens();
    update();
    emit buttonShapeChanged(m_shape);
}

void MdIconButton::setSpaceTrack(IconButtonSpaceTrack track)
{
    if (m_track == track) {
        return;
    }
    m_track = track;
    // The track picks which published leading/trailing space pair the layout
    // reads, so the cache must go with it.
    invalidateTokens();
    updateGeometry();
    update();
    emit spaceTrackChanged(m_track);
}

void MdIconButton::setIconName(const QString &iconName)
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

void MdIconButton::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    emit iconSetChanged(m_iconSet);
    update();
}

void MdIconButton::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    emit iconFamilyChanged(m_iconFamily);
    update();
}

void MdIconButton::setToggleable(bool toggleable)
{
    if (m_toggleable == toggleable) {
        return;
    }
    m_toggleable = toggleable;
    setCheckable(toggleable);
    emit toggleableChanged(m_toggleable);
    // Leaving toggle mode drops the selected colours and the selected corner
    // shape with it, which is a paint change even when the checked state is
    // untouched.
    update();
}

void MdIconButton::setSelected(bool selected)
{
    // QAbstractButton refuses setChecked() on a non-checkable button, which is
    // also the right answer here: a non-toggle icon button has no selected
    // state to enter. selectedChanged itself is emitted by the toggled
    // connection in init(), so setChecked is the single path through.
    setChecked(selected);
}

QRectF MdIconButton::containerRect() const
{
    const MdIconButtonTokens &resolved = tokens();
    return MdIconButtonStyle::layoutFor(*this, resolved).container;
}

const MdIconButtonTokens &MdIconButton::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdIconButtonTokens::resolve(m_variant, m_size, m_shape, m_track,
                                               &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

QSize MdIconButton::sizeHint() const
{
    const MdIconButtonTokens &resolved = tokens();
    const MdIconButtonStyle::Layout layout = MdIconButtonStyle::layoutFor(*this, resolved);
    return QSize(int(std::ceil(layout.preferredSize.width())),
                 int(std::ceil(layout.preferredSize.height())));
}

QSize MdIconButton::minimumSizeHint() const
{
    return sizeHint();
}

void MdIconButton::animateMorphTo(qreal target)
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

void MdIconButton::onMorphTick()
{
    const MdIconButtonTokens &resolved = tokens();
    const MdSpring spring(resolved.springStiffness, resolved.springDampingRatio);

    const qreal seconds = qreal(m_morphClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_morph = m_morphFrom + (m_morphTo - m_morphFrom) * progress;

    // A damped spring approaches its target asymptotically, so the elapsed-time
    // bound is what guarantees the timer stops.
    const qreal settleMs = spring.settlingDurationMs();
    if (progress >= 1.0 || qreal(m_morphClock.elapsed()) >= settleMs) {
        m_morph = m_morphTo;
        m_morphTimer->stop();
    }
    update();
}

void MdIconButton::onPressed()
{
    animateMorphTo(1.0);

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

void MdIconButton::onReleased()
{
    animateMorphTo(0.0);
    if (m_ripple) {
        m_ripple->release();
    }
    update();
}

void MdIconButton::mousePressEvent(QMouseEvent *event)
{
    m_hasPressPosition = true;
    // The ripple controller works in container-local coordinates; the layout
    // centres the container in the widget, so the same offset applies here.
    m_pressPosition = QPointF(event->position()) - containerRect().topLeft();
    QPushButton::mousePressEvent(event);
}

void MdIconButton::mouseReleaseEvent(QMouseEvent *event)
{
    m_hasPressPosition = false;
    QPushButton::mouseReleaseEvent(event);
}

void MdIconButton::enterEvent(QEnterEvent *event)
{
    m_hovered = true;
    QPushButton::enterEvent(event);
    update();
}

void MdIconButton::leaveEvent(QEvent *event)
{
    m_hovered = false;
    QPushButton::leaveEvent(event);
    update();
}

void MdIconButton::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
    if (m_focusRing) {
        m_focusRing->start();
    }
    update();
}

void MdIconButton::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    if (m_focusRing) {
        m_focusRing->stop();
    }
    update();
}

void MdIconButton::changeEvent(QEvent *event)
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

void MdIconButton::onThemeChanged()
{
    invalidateTokens();
    update();
}

void MdIconButton::invalidateTokens()
{
    m_tokensDirty = true;
}

} // namespace md
