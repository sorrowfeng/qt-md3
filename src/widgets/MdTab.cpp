#include "MdTab.h"

#include "styles/MdTabStyle.h"

#include "core/MdColorMath.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdMotion.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QFocusEvent>
#include <QtGui/QFontMetricsF>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdTab::MdTab(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdTab::MdTab(const QString &label, QWidget *parent)
    : QPushButton(parent)
    , m_label(label)
{
    init();
}

MdTab::~MdTab() = default;

void MdTab::init()
{
    // One selectable page of a set. Checkable is what gives it the selected
    // state, Space/Enter activation and the selected accessibility state;
    // making the set exclusive is the owning row's job.
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setAccessibleName(m_label.isEmpty() ? m_iconName : m_label);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    m_colourTimer = new QTimer(this);
    m_colourTimer->setInterval(8);
    m_colourTimer->setTimerType(Qt::PreciseTimer);
    connect(m_colourTimer, &QTimer::timeout, this, &MdTab::onColourTick);

    connect(this, &QAbstractButton::toggled, this, [this](bool checked) {
        emit selectedChanged(checked);
        restartColourAnimation();
    });

    MdStyleBase::connectThemeUpdate(this, &MdTab::onThemeChanged);
    MdTabStyle::shared();
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

void MdTab::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    updateGeometry();
    update();
    emit iconNameChanged(m_iconName);
}

void MdTab::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
    emit iconSetChanged(m_iconSet);
}

void MdTab::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
    emit iconFamilyChanged(m_iconFamily);
}

void MdTab::setLabel(const QString &label)
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

void MdTab::setSelected(bool selected)
{
    setChecked(selected);
}

void MdTab::setIconPosition(MdTabIconPosition position)
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

const MdTabsVariantTokens &MdTab::variantTokens() const
{
    if (m_hasPushedTokens) {
        return m_variantTokens;
    }
    if (m_tokensDirty) {
        m_variantTokens = MdTabsTokens::resolve(&m_componentTokens).forVariant(MdTabsVariant::Primary);
        m_tokensDirty = false;
    }
    return m_variantTokens;
}

void MdTab::setVariantTokens(const MdTabsVariantTokens &tokens)
{
    m_variantTokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdTab::invalidateTokens()
{
    m_tokensDirty = true;
}

QFont MdTab::labelFont() const
{
    const MdTabsVariantTokens &t = variantTokens();
    // `title-small` in both families. Compose measures the label in the same
    // cut for both states — the transition is a colour fade, not a weight
    // morph — and this tab's own measure reserves the emphasized width anyway.
    return MdTypeScale::font(t.labelTextType, TypeEmphasis::Baseline,
                             MdTheme::instance().scriptCategory());
}

// ---------------------------------------------------------------------------
// The paint's own arithmetic
// ---------------------------------------------------------------------------

MdTab::Boxes MdTab::boxes() const
{
    const MdTabsVariantTokens &t = variantTokens();
    Boxes out;

    const bool start = m_iconPosition == MdTabIconPosition::Start;
    out.iconAboveLabel = !start;
    out.hasLabel = !m_label.isEmpty();
    out.hasIcon = !m_iconName.isEmpty();

    const QSizeF icon = out.hasIcon ? MdIcon::preferredSize(t.iconSize) : QSizeF();
    const QFontMetricsF measure(labelFont());
    const QSizeF labelSize = out.hasLabel
                                 ? QSizeF(std::ceil(measure.horizontalAdvance(m_label)),
                                          std::ceil(measure.height()))
                                 : QSizeF();

    const qreal w = qreal(width());
    const qreal h = qreal(height());

    if (start) {
        // --- icon beside label --------------------------------------------
        // Compose's `LeadingIconTab`: a Row, content centred, the label box
        // carrying the shared 16 px horizontal padding. The group is centred
        // in whatever width the row gives the tab.
        const qreal gap = t.textDistanceFromLeadingIcon;
        const qreal groupW =
            icon.width() + (out.hasLabel ? gap + labelSize.width() : 0.0);
        const qreal x = (w - groupW) / 2.0;
        out.icon = QRectF(x, (h - icon.height()) / 2.0, icon.width(), icon.height());
        if (out.hasLabel) {
            out.label = QRectF(x + icon.width() + gap, (h - labelSize.height()) / 2.0,
                               labelSize.width(), labelSize.height());
        }
        out.naturalSize = QSizeF(
            2.0 * t.horizontalTextPadding + groupW, t.containerHeight);
        return out;
    }

    // --- icon above label -----------------------------------------------------
    // `TabBaselineLayout` aligns the pair on the text baseline; Qt has no
    // cross-widget baseline alignment, so the icon + gap + label block is
    // centred instead. With a single-line `title-small` label the block is
    // 48 px and Compose's own arithmetic puts it within a few pixels of
    // centre — the difference is recorded in porting-todo.md.
    const qreal gap = t.textDistanceFromLeadingIcon;
    const qreal contentH = icon.height() + (out.hasLabel ? gap + labelSize.height() : 0.0);
    const qreal y = (h - contentH) / 2.0;

    if (out.hasIcon) {
        out.icon = QRectF((w - icon.width()) / 2.0, y, icon.width(), icon.height());
    }
    if (out.hasLabel) {
        out.label = QRectF((w - labelSize.width()) / 2.0, y + icon.height() + gap,
                           labelSize.width(), labelSize.height());
    }
    out.naturalSize = QSizeF(
        qMax<qreal>(out.hasIcon ? icon.width() : 0.0,
                    out.hasLabel ? labelSize.width() + 2.0 * t.horizontalTextPadding : 0.0),
        out.hasIcon && out.hasLabel ? t.iconLabelTextContainerHeight : t.containerHeight);
    return out;
}

qreal MdTab::indicatorContentWidth(qreal availableWidth) const
{
    const MdTabsVariantTokens &t = variantTokens();

    // Compose, `TabRowImpl`: `contentWidth = min(maxIntrinsicWidth, tabWidth)
    // - 2 * HorizontalTextPadding`, floored at the 24 dp touch target. The
    // intrinsic width is the tab's own ideal width — the text box carries the
    // 16 px padding on both sides, the icon does not.
    qreal intrinsic;
    if (m_iconPosition == MdTabIconPosition::Start) {
        const QSizeF icon = MdIcon::preferredSize(t.iconSize);
        QFontMetricsF measure(labelFont());
        const qreal labelW = m_label.isEmpty() ? 0.0 : std::ceil(measure.horizontalAdvance(m_label));
        intrinsic = 2.0 * t.horizontalTextPadding + icon.width()
                    + (m_label.isEmpty() ? 0.0 : t.textDistanceFromLeadingIcon + labelW);
    } else {
        const Boxes b = boxes();
        intrinsic = qMax<qreal>(b.hasIcon ? b.icon.width() : 0.0,
                                b.hasLabel ? b.label.width() + 2.0 * t.horizontalTextPadding : 0.0);
    }

    return qMax(qMin(intrinsic, availableWidth) - 2.0 * t.horizontalTextPadding,
                t.indicatorMinimumWidth);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdTab::sizeHint() const
{
    const QSizeF size = boxes().naturalSize;
    return QSize(int(std::ceil(size.width())), int(std::ceil(size.height())));
}

QSize MdTab::minimumSizeHint() const
{
    const MdTabsVariantTokens &t = variantTokens();
    const QSizeF icon = MdIcon::preferredSize(t.iconSize);
    // The narrowest a tab can be without clipping its own content: one icon,
    // or the 24 px minimum the indicator enforces — the smaller floor.
    const qreal w = qMax<qreal>(m_iconName.isEmpty() ? 0.0 : icon.width(),
                                t.indicatorMinimumWidth);
    const bool both = m_iconPosition == MdTabIconPosition::Top && !m_iconName.isEmpty()
                      && !m_label.isEmpty();
    return QSize(int(std::ceil(w)),
                 int(std::ceil(both ? t.iconLabelTextContainerHeight : t.containerHeight)));
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

void MdTab::mousePressEvent(QMouseEvent *event)
{
    if (m_ripple != nullptr) {
        // Compose bounds the ripple to the tab and colours it with the
        // *selected* content colour, "because we want to show the color
        // before the item is considered selected".
        m_ripple->setBounds(QSizeF(qreal(width()), qreal(height())));
        m_ripple->press(QPointF(event->pos()));
    }
    QPushButton::mousePressEvent(event);
}

void MdTab::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdTab::enterEvent(md::MdEnterEvent *event)
{
    QPushButton::enterEvent(event);
    m_hovered = true;
    update();
}

void MdTab::leaveEvent(QEvent *event)
{
    QPushButton::leaveEvent(event);
    m_hovered = false;
    update();
}

void MdTab::focusInEvent(QFocusEvent *event)
{
    QPushButton::focusInEvent(event);
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

void MdTab::focusOutEvent(QFocusEvent *event)
{
    QPushButton::focusOutEvent(event);
    m_focusIsKeyboard = false;
    if (m_focusRing != nullptr) {
        m_focusRing->stop();
    }
    update();
}

void MdTab::changeEvent(QEvent *event)
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
// The colour fade
// ---------------------------------------------------------------------------

void MdTab::restartColourAnimation()
{
    const qreal target = isSelected() ? 1.0 : 0.0;
    if (qFuzzyCompare(target + 1.0, m_colourTo + 1.0)) {
        return;
    }
    m_colourFrom = m_colourProgress;
    m_colourTo = target;
    m_colourClock.restart();
    m_colourTimer->start();
    update();
}

void MdTab::onColourTick()
{
    // Compose's `TabTransition`: the fade-in runs on the *default* effects
    // spring, the fade-out on the *fast* one.
    const MotionSpring slot =
        m_colourTo > m_colourFrom ? MotionSpring::EffectsDefault : MotionSpring::EffectsFast;
    const MdSpring spring = MdMotion::spring(slot);
    const qreal seconds = qreal(m_colourClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_colourProgress = m_colourFrom + (m_colourTo - m_colourFrom) * travelled;

    if (qFuzzyCompare(m_colourProgress + 1.0, m_colourTo + 1.0)
        || qreal(m_colourClock.elapsed()) >= spring.settlingDurationMs()) {
        m_colourProgress = m_colourTo;
        m_colourTimer->stop();
    }
    update();
}

void MdTab::onThemeChanged()
{
    invalidateTokens();
    updateGeometry();
    update();
}

} // namespace md
