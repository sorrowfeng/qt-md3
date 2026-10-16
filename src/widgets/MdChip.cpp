#include "MdChip.h"

#include "styles/MdChipStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QFontMetricsF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdChip::MdChip(QWidget *parent)
    : QAbstractButton(parent)
{
    init();
}

MdChip::MdChip(const QString &label, QWidget *parent)
    : QAbstractButton(parent)
{
    setText(label);
    init();
}

MdChip::~MdChip() = default;

void MdChip::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setCheckable(false); // the variant decides; assist and suggestion are click-only
    setAccessibleName(text());

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_morphTimer = new QTimer(this);
    m_morphTimer->setInterval(16);
    m_morphTimer->setTimerType(Qt::PreciseTimer);
    connect(m_morphTimer, &QTimer::timeout, this, &MdChip::onMorphTick);

    // Selection and press both move the morph target (Compose's
    // shapeByInteraction).
    connect(this, &QAbstractButton::toggled, this, [this](bool) {
        if (m_cornerMorph) {
            retargetMorph();
        }
    });
    connect(this, &QAbstractButton::pressed, this, [this]() {
        if (m_cornerMorph) {
            retargetMorph();
        }
    });
    connect(this, &QAbstractButton::released, this, [this]() {
        if (m_cornerMorph) {
            retargetMorph();
        }
    });

    MdStyleBase::connectThemeUpdate(this, &MdChip::onThemeChanged);
    MdChipStyle::shared();
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

void MdChip::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    updateGeometry();
    update();
    emit iconNameChanged(m_iconName);
}

void MdChip::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
    emit iconSetChanged(m_iconSet);
}

void MdChip::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
    emit iconFamilyChanged(m_iconFamily);
}

void MdChip::setTrailingIconName(const QString &iconName)
{
    if (m_trailingIconName == iconName) {
        return;
    }
    m_trailingIconName = iconName;
    updateGeometry();
    update();
    emit trailingIconNameChanged(m_trailingIconName);
}

void MdChip::setAvatarIconName(const QString &iconName)
{
    if (m_avatarIconName == iconName) {
        return;
    }
    m_avatarIconName = iconName;
    updateGeometry();
    update();
    emit avatarIconNameChanged(m_avatarIconName);
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void MdChip::setVariant(MdChipVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    // Filter and input chips are selectable; assist and suggestion click.
    setCheckable(chipTokens().isSelectable());
    invalidateTokens();
    updateGeometry();
    update();
    emit variantChanged(m_variant);
}

void MdChip::setKind(MdChipKind kind)
{
    if (m_kind == kind) {
        return;
    }
    m_kind = kind;
    invalidateTokens();
    updateGeometry();
    update();
    emit kindChanged(m_kind);
}

void MdChip::setSelected(bool selected)
{
    setChecked(selected);
}

void MdChip::setDragged(bool dragged)
{
    if (m_dragged == dragged) {
        return;
    }
    m_dragged = dragged;
    update();
    emit draggedChanged(m_dragged);
}

void MdChip::setCornerMorph(bool enabled)
{
    if (m_cornerMorph == enabled) {
        return;
    }
    m_cornerMorph = enabled;
    if (enabled) {
        // Snap to the target and morph from there.
        m_morphRadius = MdShape::resolvedRadius(morphTargetCorner(), QSizeF(width(), height()));
        retargetMorph();
    } else {
        m_morphActive = false;
        if (m_morphTimer) {
            m_morphTimer->stop();
        }
    }
    update();
    emit cornerMorphChanged(m_cornerMorph);
}

ShapeCorner MdChip::morphTargetCorner() const
{
    // Compose Chip.kt shapeByInteraction:
    //   if (isPressed) shapes.pressedShape
    //   else if (selected) shapes.selectedShape
    //   else shapes.shape
    // ChipsTokens: PressedShape = CornerSmall, SelectedShape = CornerFull,
    // UnselectedShape = CornerMedium.
    if (isDown()) {
        return ShapeCorner::Small;
    }
    if (isSelected()) {
        return ShapeCorner::Full;
    }
    return ShapeCorner::Medium;
}

void MdChip::retargetMorph()
{
    if (m_morphTimer == nullptr) {
        return;
    }
    const ShapeCorner target = morphTargetCorner();
    const qreal targetRadius =
        MdShape::resolvedRadius(target, QSizeF(qreal(width()), qreal(height())));
    m_morphTargetCorner = target;
    if (qFuzzyCompare(m_morphRadius + 1.0, targetRadius + 1.0)) {
        m_morphTo = targetRadius;
        m_morphActive = false;
        m_morphTimer->stop();
        update();
        return;
    }
    // Interruptible: freeze the currently shown radius and morph from it
    // (Compose AnimatedShapeState.animateToShape).
    m_morphFrom = m_morphRadius;
    m_morphTo = targetRadius;
    m_morphActive = true;
    m_morphClock.restart();
    m_morphTimer->start();
}

void MdChip::onMorphTick()
{
    if (!m_morphActive) {
        m_morphTimer->stop();
        return;
    }
    // Compose runs the morph on MotionSchemeKeyTokens.FastSpatial —
    // MdSpring SpatialFast, the engineering-spec spring rule.
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialFast);
    const qreal seconds = qreal(m_morphClock.elapsed()) / 1000.0;
    const qreal progress = spring.valueAt(seconds);
    m_morphRadius = m_morphFrom + (m_morphTo - m_morphFrom) * progress;
    if (progress >= 1.0 || qreal(m_morphClock.elapsed()) >= spring.settlingDurationMs()) {
        m_morphRadius = m_morphTo;
        m_morphActive = false;
        m_morphTimer->stop();
    }
    update();
}

qreal MdChip::morphCornerRadius() const
{
    if (!m_cornerMorph) {
        return MdShape::resolvedRadius(chipTokens().containerShape,
                                       QSizeF(qreal(width()), qreal(height())));
    }
    // Size changes move a Full/Small radius; keep the resting value honest
    // without restarting the spring.
    if (!m_morphActive) {
        return MdShape::resolvedRadius(morphTargetCorner(), QSizeF(qreal(width()), qreal(height())));
    }
    return m_morphRadius;
}

// ---------------------------------------------------------------------------
// The arrangement and the paint's own arithmetic
// ---------------------------------------------------------------------------

MdChip::Boxes MdChip::boxes() const
{
    const MdChipVariantTokens &t = chipTokens();
    Boxes out;

    out.hasAvatar = !m_avatarIconName.isEmpty();
    out.hasLeadingIcon = !out.hasAvatar && !m_iconName.isEmpty();
    out.hasTrailingIcon = !m_trailingIconName.isEmpty();
    out.hasLeadingIcon = out.hasAvatar || out.hasLeadingIcon;

    const QSizeF avatar = out.hasAvatar ? QSizeF(t.avatarSize, t.avatarSize) : QSizeF();
    const QSizeF icon = out.hasLeadingIcon ? MdIcon::preferredSize(t.iconSize) : QSizeF();
    const QSizeF trailing =
        out.hasTrailingIcon ? MdIcon::preferredSize(t.iconSize) : QSizeF();

    QFontMetricsF measure(MdTypeScale::font(t.labelTextType, TypeEmphasis::Baseline,
                                            MdTheme::instance().scriptCategory()));
    const QSizeF labelSize =
        text().isEmpty() ? QSizeF() : QSizeF(std::ceil(measure.horizontalAdvance(text())),
                                             std::ceil(measure.height()));

    // Compose's per-family arrangement: the spacing after the leading slot and
    // before the trailing one. Assist and suggestion keep the 8 px everywhere;
    // filter and input tighten the gap beside a leading icon to 4.
    qreal leadingSpacing = t.elementSpacing;
    qreal trailingSpacing = t.elementSpacing;
    const bool leadingPresent = out.hasLeadingIcon;
    if (m_variant == MdChipVariant::Filter || m_variant == MdChipVariant::Input) {
        if (leadingPresent && out.hasTrailingIcon) {
            leadingSpacing = trailingSpacing = t.compactSpacing;
        } else if (leadingPresent) {
            leadingSpacing = t.compactSpacing;
        } else if (out.hasTrailingIcon) {
            trailingSpacing = t.compactSpacing;
        }
    }

    const qreal leadW = out.hasAvatar ? avatar.width() : (out.hasLeadingIcon ? icon.width() : 0.0);
    const qreal h = t.containerHeight;
    const qreal naturalW = 2.0 * t.contentPadding + leadW
                           + (leadW > 0.0 ? leadingSpacing : 0.0) + labelSize.width()
                           + (out.hasTrailingIcon ? trailingSpacing + trailing.width() : 0.0);
    // A caller-stretched widget lays out at its own width: the label (the
    // weighted middle) absorbs the extra room and the trailing slot stays
    // pinned to the right padding, exactly as Compose's weighted label does.
    // The *natural* size keeps the content-only width for `sizeHint()`.
    const qreal w = qMax<qreal>(naturalW, qreal(width()));

    // Leading and trailing swap sides under RTL: the leading slot starts at
    // the *layout* start edge, the trailing slot stays pinned to the end.
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal innerLeft = t.contentPadding;
    qreal x = innerLeft;
    if (out.hasAvatar) {
        const QRectF r(x, (h - avatar.height()) / 2.0, avatar.width(), avatar.height());
        out.avatar = rtl ? QRectF(w - r.right(), r.y(), r.width(), r.height()) : r;
        x += avatar.width() + leadingSpacing;
    } else if (out.hasLeadingIcon) {
        const QRectF r(x, (h - icon.height()) / 2.0, icon.width(), icon.height());
        out.leadingIcon = rtl ? QRectF(w - r.right(), r.y(), r.width(), r.height()) : r;
        x += icon.width() + leadingSpacing;
    }
    // The label is the weighted middle: it runs to the trailing slot's start
    // (or to the right padding), so a stretched widget widens the label area,
    // never the trailing icon's position.
    const qreal labelRight = out.hasTrailingIcon ? w - t.contentPadding - trailing.width()
                                                       - trailingSpacing
                                                 : w - t.contentPadding;
    if (!text().isEmpty()) {
        const QRectF r(x, (h - labelSize.height()) / 2.0, qMax<qreal>(0.0, labelRight - x),
                       labelSize.height());
        out.label = rtl ? QRectF(w - r.right(), r.y(), r.width(), r.height()) : r;
    }
    if (out.hasTrailingIcon) {
        const QRectF r(w - t.contentPadding - trailing.width(),
                       (h - trailing.height()) / 2.0, trailing.width(), trailing.height());
        out.trailingIcon = rtl ? QRectF(w - r.right(), r.y(), r.width(), r.height()) : r;
    }
    out.naturalSize = QSizeF(naturalW, h);
    return out;
}

QSize MdChip::sizeHint() const
{
    const QSizeF size = boxes().naturalSize;
    return QSize(int(std::ceil(size.width())), int(std::ceil(size.height())));
}

QSize MdChip::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdChipVariantTokens &MdChip::chipTokens() const
{
    if (m_hasPushedTokens) {
        return m_tokens;
    }
    if (m_tokensDirty) {
        m_tokens = MdChipVariantTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdChip::setChipTokens(const MdChipVariantTokens &tokens)
{
    m_tokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdChip::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdChip::paintEvent(QPaintEvent *event)
{
    // The shared paint filter paints the chip from the Paint event before
    // this runs; QAbstractButton's own painting is never wanted here.
    Q_UNUSED(event)
}

void MdChip::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        m_ripple->setBounds(QSizeF(qreal(width()), qreal(height())));
        m_ripple->press(QPointF(event->pos()));
    }
    QAbstractButton::mousePressEvent(event);
}

void MdChip::mouseReleaseEvent(QMouseEvent *event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdChip::enterEvent(md::MdEnterEvent *event)
{
    QAbstractButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdChip::leaveEvent(QEvent *event)
{
    QAbstractButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdChip::focusInEvent(QFocusEvent *event)
{
    QAbstractButton::focusInEvent(event);
    // `:focus-visible`: only a keyboard reason paints the ring. See MdButton
    // for the reason classification.
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
                        && event->reason() != Qt::ActiveWindowFocusReason
                        && event->reason() != Qt::PopupFocusReason;
    if (m_focusRing != nullptr && m_focusIsKeyboard) {
        m_focusRing->start();
    }
    update();
}

void MdChip::focusOutEvent(QFocusEvent *event)
{
    QAbstractButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdChip::keyPressEvent(QKeyEvent *event)
{
    if (isEffectivelyDisabled()) {
        switch (event->key()) {
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            event->accept();
            return;
        default:
            break;
        }
    }
    switch (event->key()) {
    case Qt::Key_Return:
    case Qt::Key_Enter:
        // QAbstractButton's ButtonPressKeys theme hint is Space/Select only.
        // M3 wants Enter to activate like Space (DoD: Space/Enter/Esc/arrow
        // keys) — the chip families document Space *and* Enter as click.
        click();
        event->accept();
        return;
    default:
        break;
    }
    QAbstractButton::keyPressEvent(event);
}

void MdChip::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
    case QEvent::LayoutDirectionChange:
        update();
        break;
    default:
        break;
    }
}

void MdChip::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
