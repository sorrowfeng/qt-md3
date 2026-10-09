#include "MdNavigationBarItemStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdNavigationBarItem.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>

namespace md {

MdNavigationBarItemStyle::MdNavigationBarItemStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdNavigationBarItemStyle *MdNavigationBarItemStyle::shared()
{
    static QMutex mutex;
    static MdNavigationBarItemStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdNavigationBarItemStyle;
        installPaintFilter<MdNavigationBarItem>(instance);
    }
    return instance;
}

bool MdNavigationBarItemStyle::isInstalled()
{
    return hasPaintFilter(&MdNavigationBarItem::staticMetaObject);
}

MdFocusRingSpec MdNavigationBarItemStyle::focusRingSpec(const MdNavigationBarVariantTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    // `active-width` is the system constant the ring grows to on focus; the
    // export publishes only the resting `thickness`.
    spec.activeWidth = 8.0;
    // The export's `focus-indicator-outline-offset` is the **inner** offset, so
    // the ring draws inside the pill and the item needs no outward margin.
    spec.outwardOffset = 2.0;
    spec.inwardOffset = tokens.focusIndicatorOffset;
    spec.inward = true;
    spec.shape = tokens.indicatorShape;
    return spec;
}

MdNavigationItemState MdNavigationBarItemStyle::stateFor(const MdNavigationBarItem &item)
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

/// A `MdNavigationColourSlot` as a paintable colour: the theme's role with the
/// export's opacity applied on top of whatever alpha the role carries.
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

/// The state layer's overlay at the *token's* opacity rather than the system
/// default. The two agree at the published values, but the rows are published
/// and resolved, so they are what is applied.
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

qreal stateLayerOpacityFor(const MdNavigationBarVariantTokens &tokens, MdNavigationItemState state)
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

void MdNavigationBarItemStyle::paintNavigationBarItem(QPainter &painter,
                                                      const MdNavigationBarItem &item,
                                                      const MdNavigationBarVariantTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationBarItem::Boxes boxes = item.boxes();
    const MdNavigationItemState state = stateFor(item);
    const bool selected = item.isSelected();
    const MdNavigationItemColours &colours = tokens.coloursFor(selected);

    // The pill is both the shape that is filled and the clip that everything
    // interactive is drawn through. Two paths from the same rectangle: the
    // animated one for the fill, the full-size one for the clip, so the ripple
    // and the ring keep their final geometry while the pill is still opening.
    const QList<qreal> openRadii =
        MdShape::resolvedRadii(tokens.indicatorShape, boxes.indicator.size());
    const QPainterPath openPath = MdShape::roundedRect(boxes.indicator, openRadii);
    const QList<qreal> rippleRadii =
        MdShape::resolvedRadii(tokens.indicatorShape, boxes.indicatorRipple.size());
    const QPainterPath ripplePath = MdShape::roundedRect(boxes.indicatorRipple, rippleRadii);

    // 1. The indicator. Nothing is painted when the item is unselected, which is
    //    what "no indicator row on the unselected side" means.
    if (colours.indicator.isPresent() && selected && !boxes.indicator.isEmpty()) {
        const QColor fill = resolveSlot(colours.indicator);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(openPath);
        }
    }

    const bool interactive = !item.isEffectivelyDisabled();

    // 2. State layer — hover and keyboard focus only. Press rides on the ripple,
    //    as in MdButtonStyle and MdIconButtonStyle.
    if (interactive && (state == MdNavigationItemState::Hovered
                        || state == MdNavigationItemState::Focused)) {
        StateLayerKind kind = state == MdNavigationItemState::Focused ? StateLayerKind::Focus
                                                                     : StateLayerKind::Hover;
        const MdNavigationColourSlot &slot = colours.stateLayerFor(state);
        const QColor overlay =
            stateLayerOverlay(slot, kind, stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.save();
            painter.setClipPath(ripplePath);
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(ripplePath);
            painter.restore();
        }
    }

    // 3. The ripple, clipped to the pill. In the `Start` position the pill
    //    contains the label, so the ripple stays *under* the content and the
    //    label is painted afterwards.
    if (MdRippleController *ripple = item.rippleController()) {
        ripple->setBounds(boxes.indicatorRipple.size());
        ripple->setClipPath(MdShape::roundedRect(
            QRectF(QPointF(0.0, 0.0), boxes.indicatorRipple.size()), rippleRadii));
        // The ripple is the pressed state layer, so it takes that state's
        // published colour for this side of the selection.
        const MdNavigationColourSlot &pressSlot =
            colours.stateLayerFor(MdNavigationItemState::Pressed);
        const QColor pressColour = pressSlot.isPresent()
                                       ? resolveSlot(pressSlot)
                                       : MdTheme::instance().color(ColorRole::OnSurface);
        ripple->setContentColor(pressColour);
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(boxes.indicatorRipple.topLeft());
            MdRipple::paint(&painter, frame, ripple->clipPath(), ripple->contentColor());
            painter.restore();
        }
    }

    // 4. The icon.
    const QColor iconColour = resolveSlot(colours.iconFor(state));
    if (iconColour.isValid() && !item.iconName().isEmpty()) {
        MdIcon::paint(&painter, boxes.icon, item.iconName(), iconColour, item.iconSet(),
                      item.iconFamily());
    }

    // 5. The label, if it is being shown.
    if (boxes.labelVisible && !boxes.label.isEmpty()) {
        // In the `Start` position the label sits inside the pill, so it is
        // painted in the pill's content colour — the flexible family's
        // `nav-bar-item-horizontal.active-label-text.color`. In the `Top`
        // position it is outside the pill and takes the item's own table.
        const bool insideThePill = !boxes.iconAboveLabel;
        const ColorRole roleOverride =
            (insideThePill && tokens.labelColorStart != ColorRole::Count) ? tokens.labelColorStart
                                                                         : ColorRole::Count;
        QColor textColour;
        if (roleOverride != ColorRole::Count && selected) {
            textColour = MdTheme::instance().color(roleOverride);
        } else {
            textColour = resolveSlot(colours.labelFor(state));
        }
        if (textColour.isValid()) {
            textColour.setAlphaF(
                qBound(0.0, textColour.alphaF() * boxes.labelOpacity, 1.0));
            painter.setPen(textColour);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(item.labelFont());
            painter.drawText(boxes.label, int(Qt::AlignCenter), item.label());
        }
    }

    // 6. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, and on the pill — see the header.
    if (interactive && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, boxes.indicatorRipple, rippleRadii,
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdNavigationBarItemStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *item = qobject_cast<MdNavigationBarItem *>(widget);
    if (painter == nullptr || item == nullptr) {
        return;
    }
    paintNavigationBarItem(*painter, *item, item->variantTokens());
}

} // namespace md
