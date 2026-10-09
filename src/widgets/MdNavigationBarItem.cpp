#include "MdNavigationBarItem.h"

#include "styles/MdNavigationBarItemStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QFontMetricsF>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdNavigationBarItem::MdNavigationBarItem(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdNavigationBarItem::MdNavigationBarItem(const QString &label, const QString &iconName,
                                         QWidget *parent)
    : QPushButton(parent)
    , m_iconName(iconName)
    , m_label(label)
{
    init();
}

MdNavigationBarItem::~MdNavigationBarItem() = default;

void MdNavigationBarItem::init()
{
    // The item is one destination of a set. Checkable is what gives it the
    // selected state, Space/Enter activation and the selected accessibility
    // state; making the set exclusive is the owning bar's job, because a
    // stand-alone item has no set to be exclusive within.
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(m_label.isEmpty() ? m_iconName : m_label);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_indicatorTimer = new QTimer(this);
    m_indicatorTimer->setInterval(8);
    m_indicatorTimer->setTimerType(Qt::PreciseTimer);
    connect(m_indicatorTimer, &QTimer::timeout, this, &MdNavigationBarItem::onIndicatorTick);

    // Every path — a click, Space, or setSelected() — reports the selection
    // through `toggled`, so the signal cannot be bypassed and the pill cannot
    // be left out of step with the state.
    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        emit selectedChanged(checked);
        restartIndicatorAnimation();
    });

    MdStyleBase::connectThemeUpdate(this, &MdNavigationBarItem::onThemeChanged);
    MdNavigationBarItemStyle::shared();
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

void MdNavigationBarItem::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    updateGeometry();
    update();
    emit iconNameChanged(m_iconName);
}

void MdNavigationBarItem::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
    emit iconSetChanged(m_iconSet);
}

void MdNavigationBarItem::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
    emit iconFamilyChanged(m_iconFamily);
}

void MdNavigationBarItem::setLabel(const QString &label)
{
    if (m_label == label) {
        return;
    }
    m_label = label;
    if (m_iconName.isEmpty()) {
        setAccessibleName(m_label);
    }
    updateGeometry();
    update();
    emit labelChanged(m_label);
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void MdNavigationBarItem::setSelected(bool selected)
{
    // `setChecked` is what emits `toggled`, and the lambda above is what emits
    // `selectedChanged` and drives the pill, so there is deliberately no
    // second code path here.
    setChecked(selected);
}

void MdNavigationBarItem::setAlwaysShowLabel(bool alwaysShowLabel)
{
    if (m_alwaysShowLabel == alwaysShowLabel) {
        return;
    }
    m_alwaysShowLabel = alwaysShowLabel;
    // The label's presence changes the item's own width when it is wider than
    // the pill, and where the icon sits vertically.
    updateGeometry();
    update();
    emit alwaysShowLabelChanged(m_alwaysShowLabel);
}

void MdNavigationBarItem::setVariant(MdNavigationBarVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    updateGeometry();
    update();
    emit variantChanged(m_variant);
}

void MdNavigationBarItem::setIconPosition(MdNavigationItemIconPosition position)
{
    if (m_iconPosition == position) {
        return;
    }
    m_iconPosition = position;
    updateGeometry();
    update();
    emit iconPositionChanged(m_iconPosition);
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdNavigationBarVariantTokens &MdNavigationBarItem::variantTokens() const
{
    if (m_hasPushedTokens) {
        return m_variantTokens;
    }
    if (m_tokensDirty) {
        m_variantTokens = MdNavigationBarTokens::resolve(&m_componentTokens).forVariant(m_variant);
        m_tokensDirty = false;
    }
    return m_variantTokens;
}

void MdNavigationBarItem::setVariantTokens(const MdNavigationBarVariantTokens &tokens)
{
    m_variantTokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdNavigationBarItem::invalidateTokens()
{
    m_tokensDirty = true;
}

QFont MdNavigationBarItem::labelFont() const
{
    // The export gives the selected label `label-medium-weight-prominent`,
    // which is the emphasized cut of the same size. A family that styles the
    // `Start` position's label differently — the rail's expanded item is
    // `label-large` — publishes that on its own row, and the position picks.
    const MdNavigationBarVariantTokens &t = variantTokens();
    const bool start = m_iconPosition == MdNavigationItemIconPosition::Start;
    const TypeStyle style = start ? t.horizontalLabelTextType : t.labelTextType;
    const TypeEmphasis emphasis = isSelected() ? TypeEmphasis::Emphasized : TypeEmphasis::Baseline;
    return MdTypeScale::font(style, emphasis, MdTheme::instance().scriptCategory());
}

// ---------------------------------------------------------------------------
// The paint's own arithmetic
// ---------------------------------------------------------------------------

MdNavigationBarItem::Boxes MdNavigationBarItem::boxes() const
{
    const MdNavigationBarVariantTokens &t = variantTokens();
    Boxes out;

    const bool start = m_iconPosition == MdNavigationItemIconPosition::Start;
    out.iconAboveLabel = !start;
    out.hasLabel = !m_label.isEmpty();

    const QSizeF icon = MdIcon::preferredSize(t.iconSize);

    // The label's box is measured in the **emphasized** cut even while the item
    // is unselected. A weight change moves the advance width, and measuring in
    // the state's own cut would resize the item — and the whole bar with it —
    // every time the selection moved. Reserving the wider of the two is what
    // the export's single `label-text-size` row leaves to the implementation.
    // The `Start` position measures in its own type when the family styles it
    // differently (the rail's expanded item is `label-large`).
    const TypeStyle labelStyle = start ? t.horizontalLabelTextType : t.labelTextType;
    const QFontMetricsF measure(MdTypeScale::font(labelStyle, TypeEmphasis::Emphasized,
                                                  MdTheme::instance().scriptCategory()));
    const QSizeF labelSize =
        out.hasLabel ? QSizeF(std::ceil(measure.horizontalAdvance(m_label)),
                              std::ceil(measure.height()))
                     : QSizeF();

    if (start) {
        // --- icon beside label ---------------------------------------------
        // The pill wraps icon and label, so its width *is* its content:
        // leading + icon + gap + label + trailing. Its height is the
        // horizontal item's own (40 for the bar's flexible family, 56 for the
        // rail's expanded one). The icon-label gap is the horizontal item's
        // row when the family publishes one — the rail's reads 8 where its
        // vertical item reads 4.
        const qreal iconLabelSpace =
            t.horizontalIconLabelSpace > 0.0 ? t.horizontalIconLabelSpace
                                             : t.indicatorIconLabelSpace;
        const qreal rippleH = t.horizontalIndicatorHeight;
        const qreal rippleW = t.horizontalIndicatorLeadingSpace + icon.width()
                              + (out.hasLabel ? iconLabelSpace + labelSize.width() : 0.0)
                              + t.horizontalIndicatorTrailingSpace;
        out.naturalSize = QSizeF(rippleW, rippleH);

        const qreal contentW = qMax<qreal>(qreal(width()), rippleW);
        const qreal left = (contentW - rippleW) / 2.0;
        out.indicatorRipple = QRectF(left, 0.0, rippleW, rippleH);
        const qreal openW = rippleW * qBound<qreal>(0.0, m_indicatorProgress, 1.0);
        out.indicator = QRectF(left + (rippleW - openW) / 2.0, 0.0, openW, rippleH);

        const qreal groupW =
            icon.width() + (out.hasLabel ? iconLabelSpace + labelSize.width() : 0.0);
        const qreal iconX = left + (rippleW - groupW) / 2.0;
        out.icon = QRectF(iconX, (rippleH - icon.height()) / 2.0, icon.width(), icon.height());
        if (out.hasLabel) {
            out.label = QRectF(iconX + icon.width() + iconLabelSpace,
                               (rippleH - labelSize.height()) / 2.0, labelSize.width(),
                               labelSize.height());
            // The label rides inside the pill in this position, so it is
            // painted with the pill whether or not the item is selected.
            out.labelVisible = true;
            out.labelOpacity = 1.0;
        }
        return out;
    }

    // --- icon above label ---------------------------------------------------
    // Without a label the pill takes the family's own no-label height when it
    // publishes one — the baseline rail's `no-label-active-indicator-height`
    // makes the label-less pill a 56 x 56 square around the bare icon.
    const qreal pillH = (!out.hasLabel && t.noLabelIndicatorHeight > 0.0)
                            ? t.noLabelIndicatorHeight
                            : t.activeIndicatorHeight;
    const qreal padH = t.indicatorHorizontalPadding();
    const qreal padV = (pillH - icon.height()) / 2.0;
    const qreal rippleW = icon.width() + 2.0 * padH;
    const qreal rippleH = pillH;
    const qreal itemPad = t.containerBetweenSpace;

    const qreal labelW = out.hasLabel ? labelSize.width() : 0.0;
    const qreal labelH = out.hasLabel ? labelSize.height() : 0.0;
    const qreal naturalW = qMax(rippleW, labelW);
    const qreal contentH = out.hasLabel ? rippleH + t.indicatorIconLabelSpace + labelH : rippleH;

    // The item's own height reserves `itemPad` top and bottom whatever happens
    // to the label, which is what Compose measures.
    out.naturalSize = QSizeF(naturalW, contentH + 2.0 * itemPad);

    // A taller widget — the baseline bar is 80 px around a 52 px content, and
    // `NavigationBar` is stretched to it — centres the content instead.
    const qreal topPad = qMax(itemPad, (qreal(height()) - contentH) / 2.0);

    // Compose moves *everything* — pill, icon and label — by one offset when
    // `alwaysShowLabel` is off, interpolated by the same progress that grows
    // the pill: at rest the icon sits in the middle of the item, and it rises
    // to leave room for the label as the label fades in.
    const qreal centredIconY = (qreal(height()) - icon.height()) / 2.0;
    const qreal selectedIconY = topPad + padV;
    const qreal offset =
        m_alwaysShowLabel ? 0.0 : (centredIconY - selectedIconY) * (1.0 - m_indicatorProgress);

    const qreal left = (qreal(width()) - naturalW) / 2.0;
    out.indicatorRipple =
        QRectF(left + (naturalW - rippleW) / 2.0, topPad + offset, rippleW, rippleH);
    const qreal openW = rippleW * qBound<qreal>(0.0, m_indicatorProgress, 1.0);
    out.indicator =
        QRectF(left + (naturalW - openW) / 2.0, topPad + offset, openW, rippleH);
    out.icon = QRectF(left + (naturalW - icon.width()) / 2.0, selectedIconY + offset,
                      icon.width(), icon.height());

    out.labelOpacity = m_alwaysShowLabel ? 1.0 : qBound<qreal>(0.0, m_indicatorProgress, 1.0);
    bool labelDrawn = m_alwaysShowLabel || m_indicatorProgress > 0.0;
    out.labelVisible = out.hasLabel && labelDrawn;
    if (out.labelVisible) {
        const qreal labelY = selectedIconY + icon.height() + padV + t.indicatorIconLabelSpace;
        out.label = QRectF(left + (naturalW - labelSize.width()) / 2.0, labelY + offset,
                           labelSize.width(), labelSize.height());
    }

    return out;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdNavigationBarItem::sizeHint() const
{
    const QSizeF size = boxes().naturalSize;
    return QSize(int(std::ceil(size.width())), int(std::ceil(size.height())));
}

QSize MdNavigationBarItem::minimumSizeHint() const
{
    // Without the label the item is the pill plus its padding; that is the
    // narrowest the component can be without clipping its own indicator. Not
    // derived by building a label-less copy: `QWidget` is not copyable, and a
    // second widget would have to be parented and torn down to answer a
    // question about arithmetic. The no-label pill height is the family's own
    // row when it publishes one.
    const MdNavigationBarVariantTokens &t = variantTokens();
    const QSizeF icon = MdIcon::preferredSize(t.iconSize);

    if (m_iconPosition == MdNavigationItemIconPosition::Start) {
        const qreal rippleW = t.horizontalIndicatorLeadingSpace + icon.width()
                              + t.horizontalIndicatorTrailingSpace;
        return QSize(int(std::ceil(rippleW)), int(std::ceil(t.horizontalIndicatorHeight)));
    }

    const qreal pillH =
        t.noLabelIndicatorHeight > 0.0 ? t.noLabelIndicatorHeight : t.activeIndicatorHeight;
    const qreal rippleW = icon.width() + 2.0 * t.indicatorHorizontalPadding();
    return QSize(int(std::ceil(rippleW)), int(std::ceil(pillH + 2.0 * t.containerBetweenSpace)));
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdNavigationBarItem::enterEvent(md::MdEnterEvent *event)
{
    QPushButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdNavigationBarItem::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdNavigationBarItem::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
    // `:focus-visible`: only a keyboard reason paints the ring and the focused
    // colours. See MdButton for the reason classification.
    m_focusIsKeyboard = event->reason() != Qt::MouseFocusReason
                        && event->reason() != Qt::ActiveWindowFocusReason
                        && event->reason() != Qt::PopupFocusReason;
    if (m_focusRing != nullptr && m_focusIsKeyboard) {
        m_focusRing->start();
    }
    update();
}

void MdNavigationBarItem::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdNavigationBarItem::mousePressEvent(QMouseEvent *event)
{
    // The ripple controller works in the pill's local coordinates, so the
    // press travels from wherever it landed into the pill: the same re-mapping
    // Compose does with `MappedInteractionSource`.
    if (m_ripple != nullptr) {
        const Boxes current = boxes();
        m_ripple->setBounds(current.indicatorRipple.size());
        m_ripple->press(QPointF(event->pos()) - current.indicatorRipple.topLeft());
    }
    QPushButton::mousePressEvent(event);
}

void MdNavigationBarItem::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdNavigationBarItem::changeEvent(QEvent *event)
{
    QPushButton::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
    case QEvent::LayoutDirectionChange:
        update();
        break;
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// The pill's spring
// ---------------------------------------------------------------------------

void MdNavigationBarItem::restartIndicatorAnimation()
{
    const qreal target = isSelected() ? 1.0 : 0.0;
    if (qFuzzyCompare(target + 1.0, m_indicatorTo + 1.0)) {
        return;
    }
    m_indicatorFrom = m_indicatorProgress;
    m_indicatorTo = target;
    m_indicatorClock.restart();
    m_indicatorTimer->start();
    update();
}

void MdNavigationBarItem::onIndicatorTick()
{
    // Compose animates the indicator's width with a spatial spring — the pill
    // is a *shape* change, so it takes the spatial scheme rather than the
    // effects one the colours use.
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialDefault);
    const qreal seconds = qreal(m_indicatorClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_indicatorProgress = m_indicatorFrom + (m_indicatorTo - m_indicatorFrom) * travelled;

    // A damped spring approaches its target asymptotically, so the elapsed-time
    // bound is what guarantees the timer stops.
    if (qFuzzyCompare(m_indicatorProgress + 1.0, m_indicatorTo + 1.0)
        || qreal(m_indicatorClock.elapsed()) >= spring.settlingDurationMs()) {
        m_indicatorProgress = m_indicatorTo;
        m_indicatorTimer->stop();
    }
    update();
}

void MdNavigationBarItem::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
