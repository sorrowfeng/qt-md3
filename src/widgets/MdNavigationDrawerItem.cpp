#include "MdNavigationDrawerItem.h"

#include "styles/MdNavigationDrawerItemStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtGui/QFocusEvent>
#include <QtGui/QFontMetricsF>
#include <QtGui/QMouseEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdNavigationDrawerItem::MdNavigationDrawerItem(QWidget *parent)
    : QPushButton(parent)
{
    init();
}

MdNavigationDrawerItem::MdNavigationDrawerItem(const QString &label, const QString &iconName,
                                               QWidget *parent)
    : QPushButton(parent)
    , m_iconName(iconName)
    , m_label(label)
{
    init();
}

MdNavigationDrawerItem::~MdNavigationDrawerItem() = default;

void MdNavigationDrawerItem::init()
{
    // One destination of a set: checkable is what gives it the selected
    // state, Space/Enter activation and the accessibility role; exclusivity
    // is the owning drawer's job.
    setCheckable(true);
    setFocusPolicy(Qt::StrongFocus);
    // The width is the drawer's to stretch; the height is the row's own 56.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(m_label.isEmpty() ? m_iconName : m_label);

    m_ripple = new MdRippleController(this);
    connect(m_ripple, &MdRippleController::repaintRequested, this, qOverload<>(&QWidget::update));

    m_focusRing = new MdFocusRingController(this);
    connect(m_focusRing, &MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));

    connect(this, &QAbstractButton::toggled, this,
            [this](bool checked) { emit selectedChanged(checked); });

    MdStyleBase::connectThemeUpdate(this, &MdNavigationDrawerItem::onThemeChanged);
    MdNavigationDrawerItemStyle::shared();
}

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

void MdNavigationDrawerItem::setIconName(const QString &iconName)
{
    if (m_iconName == iconName) {
        return;
    }
    m_iconName = iconName;
    updateGeometry();
    update();
    emit iconNameChanged(m_iconName);
}

void MdNavigationDrawerItem::setIconSet(MdIconSet set)
{
    if (m_iconSet == set) {
        return;
    }
    m_iconSet = set;
    update();
    emit iconSetChanged(m_iconSet);
}

void MdNavigationDrawerItem::setIconFamily(MdIconFamily family)
{
    if (m_iconFamily == family) {
        return;
    }
    m_iconFamily = family;
    update();
    emit iconFamilyChanged(m_iconFamily);
}

void MdNavigationDrawerItem::setLabel(const QString &label)
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

void MdNavigationDrawerItem::setBadge(const QString &badge)
{
    if (m_badge == badge) {
        return;
    }
    m_badge = badge;
    updateGeometry();
    update();
    emit badgeChanged(m_badge);
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void MdNavigationDrawerItem::setSelected(bool selected)
{
    // `setChecked` is what emits `toggled`, and the lambda above is what
    // emits `selectedChanged` — no second code path.
    setChecked(selected);
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdNavigationDrawerItemTokens &MdNavigationDrawerItem::itemTokens() const
{
    if (m_hasPushedTokens) {
        return m_itemTokens;
    }
    if (m_tokensDirty) {
        m_itemTokens = MdNavigationDrawerTokens::resolve(&m_componentTokens)
                           .forVariant(MdNavigationDrawerVariant::Standard)
                           .item;
        m_tokensDirty = false;
    }
    return m_itemTokens;
}

void MdNavigationDrawerItem::setItemTokens(const MdNavigationDrawerItemTokens &tokens)
{
    m_itemTokens = tokens;
    m_hasPushedTokens = true;
    updateGeometry();
    update();
}

void MdNavigationDrawerItem::invalidateTokens()
{
    m_tokensDirty = true;
}

QFont MdNavigationDrawerItem::labelFont() const
{
    // `active-label-text-weight` is `label-large-weight-prominent` — the
    // emphasized cut of the same size, exactly the bar's rule.
    const TypeEmphasis emphasis = isSelected() ? TypeEmphasis::Emphasized : TypeEmphasis::Baseline;
    return MdTypeScale::font(itemTokens().labelTextType, emphasis,
                             MdTheme::instance().scriptCategory());
}

QFont MdNavigationDrawerItem::badgeFont() const
{
    // The `large-badge-label-*` rows resolve to `label-large` at the baseline
    // weight — the badge is never emphasized.
    return MdTypeScale::font(TypeStyle::LabelLarge, TypeEmphasis::Baseline,
                             MdTheme::instance().scriptCategory());
}

// ---------------------------------------------------------------------------
// The paint's own arithmetic
// ---------------------------------------------------------------------------

MdNavigationDrawerItem::Boxes MdNavigationDrawerItem::boxes() const
{
    const MdNavigationDrawerItemTokens &t = itemTokens();
    Boxes out;

    out.pill = QRectF(0.0, 0.0, qreal(width()), qreal(height()));
    out.hasBadge = !m_badge.isEmpty();

    const QSizeF icon = MdIcon::preferredSize(t.iconSize);

    // The label measures in the emphasized cut even while unselected — the
    // bar's rule, for the same reason: a weight change must not resize the
    // item when the selection moves.
    const QFontMetricsF measure(MdTypeScale::font(t.labelTextType, TypeEmphasis::Emphasized,
                                                  MdTheme::instance().scriptCategory()));
    const QSizeF labelSize =
        m_label.isEmpty()
            ? QSizeF()
            : QSizeF(std::ceil(measure.horizontalAdvance(m_label)), std::ceil(measure.height()));

    // Compose's content Row: `padding(start = 16, end = 24)`, the icon and a
    // 12 px spacer, then the label in a `weight(1f)` box, then (with another
    // 12 px spacer) the badge. The label's box *is* the remaining width — it
    // is left-aligned inside it, which is what `Alignment.CenterVertically`
    // plus the default start alignment do.
    //
    // The trailing content is placed against the **content width** — the
    // widget's width when the drawer has stretched it, the content's own
    // minimum otherwise — because this same arithmetic answers `sizeHint()`
    // on a widget no container has sized yet.
    const qreal rowHeight = qreal(height());
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal leading = rtl ? t.contentTrailingSpace : t.contentLeadingSpace;
    const qreal trailing = rtl ? t.contentLeadingSpace : t.contentTrailingSpace;
    const bool hasIcon = !m_iconName.isEmpty();

    const qreal labelW = m_label.isEmpty() ? 0.0 : labelSize.width();
    QSizeF badgeSize;
    if (!m_badge.isEmpty()) {
        const QFontMetricsF badgeMeasure(badgeFont());
        badgeSize = QSizeF(std::ceil(badgeMeasure.horizontalAdvance(m_badge)),
                           std::ceil(badgeMeasure.height()));
    }
    const qreal contentMin = leading + (hasIcon ? icon.width() + t.iconLabelSpace : 0.0)
                             + labelW
                             + (badgeSize.isEmpty() ? 0.0 : t.iconLabelSpace + badgeSize.width())
                             + trailing;
    const qreal contentW = qMax<qreal>(qreal(width()), contentMin);

    out.icon = QRectF(leading, (rowHeight - icon.height()) / 2.0, icon.width(), icon.height());

    qreal labelLeft = leading + (hasIcon ? icon.width() + t.iconLabelSpace : 0.0);
    qreal labelRight = contentW - trailing;
    if (!badgeSize.isEmpty()) {
        // The badge sits at the trailing edge, a 12 px gap left of it.
        out.badge = QRectF(labelRight - badgeSize.width(), (rowHeight - badgeSize.height()) / 2.0,
                           badgeSize.width(), badgeSize.height());
        labelRight = out.badge.left() - t.iconLabelSpace;
    }

    if (!m_label.isEmpty()) {
        // Left-aligned in the `weight(1f)` box; the *measured* height is what
        // the natural size reports, the painted box takes the rest.
        out.label = QRectF(labelLeft, (rowHeight - labelSize.height()) / 2.0,
                           qMax<qreal>(0.0, labelRight - labelLeft), labelSize.height());
    }

    out.naturalSize = QSizeF(contentMin, t.activeIndicatorHeight);
    return out;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdNavigationDrawerItem::sizeHint() const
{
    const QSizeF natural = boxes().naturalSize;
    return QSize(int(std::ceil(natural.width())), int(std::ceil(natural.height())));
}

QSize MdNavigationDrawerItem::minimumSizeHint() const
{
    return sizeHint();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void MdNavigationDrawerItem::mousePressEvent(QMouseEvent *event)
{
    // The pill is the item, so the press is already in the ripple's space —
    // the bar's `MappedInteractionSource` has nothing to remap here.
    if (isEffectivelyDisabled()) {
        event->accept();
        return;
    }
    if (m_ripple != nullptr) {
        m_ripple->setBounds(QSizeF(width(), height()));
        m_ripple->press(QPointF(event->pos()));
    }
    QPushButton::mousePressEvent(event);
}

void MdNavigationDrawerItem::mouseReleaseEvent(QMouseEvent *event)
{
    QPushButton::mouseReleaseEvent(event);
    if (m_ripple != nullptr) {
        m_ripple->release();
    }
}

void MdNavigationDrawerItem::enterEvent(md::MdEnterEvent *event)
{
    m_hovered = true;
    update();
    QPushButton::enterEvent(event);
}

void MdNavigationDrawerItem::leaveEvent(QEvent *event)
{
    m_hovered = false;
    update();
    QPushButton::leaveEvent(event);
}

void MdNavigationDrawerItem::focusInEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = event->reason() == Qt::TabFocusReason
                        || event->reason() == Qt::BacktabFocusReason
                        || event->reason() == Qt::ShortcutFocusReason;
    update();
    QPushButton::focusInEvent(event);
}

void MdNavigationDrawerItem::focusOutEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = false;
    update();
    QPushButton::focusOutEvent(event);
}

void MdNavigationDrawerItem::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::LayoutDirectionChange:
        updateGeometry();
        update();
        break;
    default:
        break;
    }
}

void MdNavigationDrawerItem::onThemeChanged()
{
    invalidateTokens();
    update();
}

} // namespace md
