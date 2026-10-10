#include "MdNavigationDrawerItemStyle.h"

#include "widgets/MdNavigationDrawerItem.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>

namespace md {

MdNavigationDrawerItemStyle::MdNavigationDrawerItemStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdNavigationDrawerItemStyle *MdNavigationDrawerItemStyle::shared()
{
    static QMutex mutex;
    static MdNavigationDrawerItemStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdNavigationDrawerItemStyle;
        installPaintFilter<MdNavigationDrawerItem>(instance);
    }
    return instance;
}

bool MdNavigationDrawerItemStyle::isInstalled()
{
    return hasPaintFilter(&MdNavigationDrawerItem::staticMetaObject);
}

MdFocusRingSpec
MdNavigationDrawerItemStyle::focusRingSpec(const MdNavigationDrawerItemTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    // `active-width` is the system constant the ring grows to on focus; the
    // export publishes only the resting `thickness`.
    spec.activeWidth = 8.0;
    // The export's `focus-indicator-outline-offset` is the **inner** offset:
    // the ring draws inside the pill and the item reserves no outward margin.
    spec.outwardOffset = 2.0;
    spec.inwardOffset = tokens.focusIndicatorOffset;
    spec.inward = true;
    spec.shape = tokens.indicatorShape;
    return spec;
}

MdNavigationItemState MdNavigationDrawerItemStyle::stateFor(const MdNavigationDrawerItem &item)
{
    if (item.isEffectivelyDisabled()) {
        return MdNavigationItemState::Disabled;
    }
    if (item.isDown()) {
        return MdNavigationItemState::Pressed;
    }
    if (item.hasKeyboardFocus()) {
        return MdNavigationItemState::Focused;
    }
    if (item.isHovered()) {
        return MdNavigationItemState::Hovered;
    }
    return MdNavigationItemState::Enabled;
}

namespace {

/// A `MdNavigationColourSlot` as a paintable colour — the bar item style's
/// helper, repeated because the paint helpers live in each style's anonymous
/// namespace.
QColor resolveSlot(const MdNavigationColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (!colour.isValid()) {
        return QColor();
    }
    colour.setAlphaF(qBound(0.0, colour.alphaF() * slot.opacity, 1.0));
    return colour;
}

QColor stateLayerOverlay(const MdNavigationColourSlot &slot,
                         StateLayerKind kind,
                         qreal tokenOpacity)
{
    const QColor base = resolveSlot(slot);
    if (!base.isValid()) {
        return QColor();
    }
    const qreal systemOpacity = MdStateLayer::opacity(kind);
    const qreal scale = systemOpacity > 0.0 ? qBound(0.0, tokenOpacity / systemOpacity, 1.0) : 0.0;
    return MdStateLayer::overlayScaled(base, kind, scale);
}

qreal stateLayerOpacityFor(const MdNavigationDrawerItemTokens &tokens, MdNavigationItemState state)
{
    switch (state) {
    case MdNavigationItemState::Hovered:
        return tokens.hoverStateLayerOpacity;
    case MdNavigationItemState::Focused:
        return tokens.focusStateLayerOpacity;
    case MdNavigationItemState::Pressed:
        return tokens.pressedStateLayerOpacity;
    default:
        return 0.0;
    }
}

} // namespace

void MdNavigationDrawerItemStyle::paintNavigationDrawerItem(
    QPainter &painter, const MdNavigationDrawerItem &item, const MdNavigationDrawerItemTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationDrawerItem::Boxes boxes = item.boxes();
    const MdNavigationItemState state = stateFor(item);
    const bool selected = item.isSelected();
    const MdNavigationItemColours &colours = tokens.coloursFor(selected);

    // The pill is the item: one rectangle, one shape, no animation.
    const QList<qreal> pillRadii =
        MdShape::resolvedRadii(tokens.indicatorShape, boxes.pill.size());
    const QPainterPath pillPath = MdShape::roundedRect(boxes.pill, pillRadii);

    // 1. The pill — nothing painted when unselected, which is what "the
    //    unselected container is transparent" means.
    if (selected && colours.indicator.isPresent() && !boxes.pill.isEmpty()) {
        const QColor fill = resolveSlot(colours.indicator);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(pillPath);
        }
    }

    const bool interactive = !item.isEffectivelyDisabled();

    // 2. State layer — hover and keyboard focus only; press rides on the
    //    ripple, as in the bar item.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const QColor overlay =
            stateLayerOverlay(colours.stateLayerFor(state), kind, stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(pillPath);
        }
    }

    // 3. The ripple, clipped to the pill. It takes the pressed state's
    //    published colour — for the unselected side that is the table's
    //    special case (`on-secondary-container` where hover and focus read
    //    `on-surface`).
    if (MdRippleController *ripple = item.rippleController()) {
        ripple->setBounds(boxes.pill.size());
        ripple->setClipPath(MdShape::roundedRect(
            QRectF(QPointF(0.0, 0.0), boxes.pill.size()), pillRadii));
        const MdNavigationColourSlot &pressSlot =
            colours.stateLayerFor(MdNavigationItemState::Pressed);
        const QColor pressColour = pressSlot.isPresent()
                                       ? resolveSlot(pressSlot)
                                       : MdTheme::instance().color(ColorRole::OnSurface);
        ripple->setContentColor(pressColour);
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            MdRipple::paint(&painter, frame, ripple->clipPath(), ripple->contentColor());
        }
    }

    // 4. The icon.
    const QColor iconColour = resolveSlot(colours.iconFor(state));
    if (iconColour.isValid() && !item.iconName().isEmpty()) {
        MdIcon::paint(&painter, boxes.icon, item.iconName(), iconColour, item.iconSet(),
                      item.iconFamily());
    }

    // 5. The label, left-aligned in its `weight(1f)` box.
    if (!boxes.label.isEmpty() && !item.label().isEmpty()) {
        QColor textColour = resolveSlot(colours.labelFor(state));
        if (textColour.isValid()) {
            painter.setPen(textColour);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(item.labelFont());
            painter.drawText(boxes.label, int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item)), item.label());
        }
    }

    // 6. The badge — the `large-badge-label-*` rows, in the label's colour
    //    (Compose's default `badgeColor` is the text colour, both sides).
    if (boxes.hasBadge && !boxes.badge.isEmpty()) {
        QColor badgeColour = resolveSlot(colours.labelFor(state));
        if (badgeColour.isValid()) {
            painter.setPen(badgeColour);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(item.badgeFont());
            painter.drawText(boxes.badge, int(Qt::AlignVCenter | Qt::AlignRight), item.badge());
        }
    }

    // 7. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, inward on the pill.
    if (interactive && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, boxes.pill, pillRadii,
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdNavigationDrawerItemStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *item = qobject_cast<MdNavigationDrawerItem *>(widget);
    if (painter == nullptr || item == nullptr) {
        return;
    }
    paintNavigationDrawerItem(*painter, *item, item->itemTokens());
}

} // namespace md
