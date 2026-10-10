#include "MdMenuStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdMenu.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

MdMenuStyle::MdMenuStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdMenuStyle *MdMenuStyle::shared()
{
    static QMutex mutex;
    static MdMenuStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdMenuStyle;
        installPaintFilter<MdMenuItem>(instance);
        // The menu surface paints through its own paintEvent (it renders its
        // children through a transform while animating).
    }
    return instance;
}

bool MdMenuStyle::isInstalled()
{
    return hasPaintFilter(&MdMenuItem::staticMetaObject);
}

MdFocusRingSpec MdMenuStyle::focusRingSpec(const MdMenuTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    // The export points at the system *inner* offset — the menu item's ring
    // draws inward, and the spec reads its dedicated inward-offset field.
    spec.inwardOffset = tokens.focusIndicatorInnerOffset;
    spec.inward = true;
    spec.shape = ShapeCorner::Small;
    return spec;
}

MdNavigationItemState MdMenuStyle::stateFor(const MdMenuItem &item)
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

/// A `MdNavigationColourSlot` as a paintable colour. An absent slot resolves
/// to an *invalid* colour, which the paint treats as transparent.
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

/// The state layer's overlay at the token's opacity.
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

qreal stateLayerOpacityFor(const MdMenuTokens &tokens, MdNavigationItemState state)
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

void MdMenuStyle::paintMenuItem(QPainter &painter, const MdMenuItem &item,
                                const MdMenuTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationItemState state = stateFor(item);
    const bool interactive = !item.isEffectivelyDisabled();
    const MdMenuSelection selection =
        item.isSelected() ? MdMenuSelection::Selected : MdMenuSelection::Unselected;
    const int s = int(selection);

    const QRectF itemRect(QPointF(0, 0), QSizeF(item.width(), item.height()));
    const qreal radius = 0.0; // the classic item is a square row inside the surface
    Q_UNUSED(radius);

    // 1. The selected container (`secondary-container`; the unselected item
    //    has no container row).
    const QColor containerColour = resolveSlot(tokens.itemContainerFor(selection, state));
    if (containerColour.isValid() && containerColour.alpha() > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(containerColour);
        painter.drawRect(itemRect);
    }

    // 2. The state layer — hover and keyboard focus flat; the press rides the
    //    ripple.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const QColor overlay =
            stateLayerOverlay(tokens.stateLayerFor(selection, state), kind,
                              stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawRect(itemRect);
        }
    }
    if (MdRippleController *ripple = item.rippleController()) {
        ripple->setBounds(itemRect.size());
        ripple->setContentColor(resolveSlot(tokens.stateLayerFor(
            selection, MdNavigationItemState::Pressed)));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            QPainterPath clip;
            clip.addRect(itemRect);
            MdRipple::paint(&painter, frame, clip, ripple->contentColor());
        }
    }

    // 3. The content row: 12 px horizontal padding, 24 px icons spaced 8 px
    //    from the label, the trailing icon right-aligned.
    const bool rtl = item.layoutDirection() == Qt::RightToLeft;
    const qreal pad = tokens.itemHorizontalPadding;
    const QColor iconColour = resolveSlot(tokens.iconFor(selection, state));
    qreal x = pad;
    auto mirror = [&](const QRectF &r) {
        return rtl ? QRectF(item.width() - r.right(), r.y(), r.width(), r.height()) : r;
    };
    if (!item.leadingIconName().isEmpty()) {
        MdIcon::paint(&painter,
                      mirror(QRectF(x, (item.height() - tokens.iconSize) / 2.0,
                                    tokens.iconSize, tokens.iconSize)),
                      item.leadingIconName(), iconColour, item.iconSet(), item.iconFamily());
        x += tokens.iconSize + tokens.itemIconTextSpacing;
    }
    const QColor labelColour = resolveSlot(tokens.labelFor(selection, state));
    const qreal trailingWidth =
        item.trailingIconName().isEmpty() ? 0.0 : tokens.iconSize + tokens.itemIconTextSpacing;
    if (labelColour.isValid() && !item.text().isEmpty()) {
        painter.setPen(labelColour);
        painter.setFont(MdTypeScale::font(tokens.labelTextType, TypeEmphasis::Baseline,
                                          MdTheme::instance().scriptCategory()));
        painter.drawText(mirror(QRectF(x, 0, item.width() - pad - trailingWidth - x,
                                      item.height())),
                         int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item)), item.text());
    }
    if (!item.trailingIconName().isEmpty()) {
        const QColor trailingColour = resolveSlot(
            item.trailingIconName() == QLatin1String("arrow_forward")
                ? MdNavigationColourSlot{tokens.cascadingIndicatorColor, 1.0}
                : tokens.iconFor(selection, state));
        MdIcon::paint(&painter,
                      mirror(QRectF(item.width() - pad - tokens.iconSize,
                                    (item.height() - tokens.iconSize) / 2.0, tokens.iconSize,
                                    tokens.iconSize)),
                      item.trailingIconName(), trailingColour, item.iconSet(),
                      item.iconFamily());
    }

    // 4. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, *inward* around the row.
    if (interactive && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, itemRect, {0, 0, 0, 0},
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdMenuStyle::paintMenu(QPainter &painter, MdMenu &menu, const MdMenuTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF surfaceRect(QPointF(0, 0), QSizeF(menu.width(), menu.height()));
    const qreal radius = MdShape::radius(tokens.containerShape);
    const QList<qreal> radii{radius, radius, radius, radius};
    const QPainterPath surfacePath = MdShape::roundedRect(surfaceRect, radii);

    // The open/close animation: scale around the anchor corner (top-start)
    // and alpha, Compose's graphicsLayer pair.
    const qreal progress = qBound(0.0, menu.openProgress(), 1.0);
    const qreal scale = tokens.closedScale + (1.0 - tokens.closedScale) * progress;
    const qreal alpha = tokens.closedAlpha + (1.0 - tokens.closedAlpha) * menu.openAlpha();
    const bool animating = menu.openAnimationRunning();
    if (animating && progress < 1.0) {
        painter.setOpacity(alpha);
    }

    // 1. The level-2 shadow and the surface fill.
    MdElevation::drawShadowDp(&painter, surfaceRect, radius,
                              MdElevation::shadowDp(tokens.containerElevation),
                              MdTheme::instance().color(tokens.containerShadowColor));
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawPath(surfacePath);

    // 2. The children through the transform. Compose scales graphically —
    //    the popup's geometry never changes mid-flight. Each child renders
    //    itself into the transformed painter (rendering the menu *itself*
    //    here would recurse into this very paintEvent); the children are
    //    hidden during the flight and render() paints hidden widgets too.
    if (animating && (scale < 0.999 || alpha < 0.999)) {
        const QPointF origin(0.0, 0.0); // MenuAnchorPosition.Below: top-start
        painter.save();
        painter.translate(origin);
        painter.scale(scale, scale);
        painter.translate(-origin);
        painter.setClipPath(surfacePath);
        const QList<QObject *> children = menu.children();
        for (QObject *childObject : children) {
            auto *child = qobject_cast<QWidget *>(childObject);
            if (child == nullptr) {
                continue;
            }
            child->render(&painter, child->pos(), QRegion(),
                          QWidget::DrawChildren | QWidget::IgnoreMask);
        }
        painter.restore();
        painter.restore();
        return;
    }

    painter.restore();
}

void MdMenuStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    if (auto *item = qobject_cast<MdMenuItem *>(widget)) {
        paintMenuItem(*painter, *item, item->menuTokens());
        return;
    }
}

} // namespace md
