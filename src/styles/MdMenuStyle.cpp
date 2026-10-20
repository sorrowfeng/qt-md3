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
#include <QtGui/QFontMetrics>
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
        const QFont labelFont = MdTypeScale::font(tokens.labelTextType, TypeEmphasis::Baseline,
                                                  MdTheme::instance().scriptCategory());
        painter.setFont(labelFont);
        // sizeHint caps the row at itemMaxWidth; a longer label must elide
        // rather than run past the trailing slot or the row edge.
        const QRectF labelBox = mirror(QRectF(x, 0, item.width() - pad - trailingWidth - x,
                                              item.height()));
        const QFontMetrics labelFm(labelFont);
        painter.drawText(labelBox,
                         int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item)),
                         labelFm.elidedText(item.text(), Qt::ElideRight, int(labelBox.width())));
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
    // and alpha, Compose's graphicsLayer pair on the *whole* surface.
    const qreal progress = qBound(0.0, menu.openProgress(), 1.0);
    const qreal scale = tokens.closedScale + (1.0 - tokens.closedScale) * progress;
    const qreal alpha = tokens.closedAlpha + (1.0 - tokens.closedAlpha) * menu.openAlpha();
    const bool animating = menu.openAnimationRunning();
    if (animating || alpha < 0.999) {
        painter.setOpacity(painter.opacity() * qBound(0.0, alpha, 1.0));
    }

    // Compose scales the entire popup graphically; the window geometry never
    // moves mid-flight. The rows ride the same transform — they cannot be
    // real child widgets under a painter scale, so the surface paints them
    // itself whenever the flight (or a still-hidden row) owns the frame.
    // `child->render()` here was the flicker: the application paint filter
    // opens a QPainter on the *child*, which lands on the child's own paint
    // device instead of this transformed painter, so the open mapped a
    // surface with no rows and then popped the rows in when the springs
    // settled and the children became visible.
    bool rowsHidden = false;
    for (MdMenuItem *item : menu.items()) {
        if (item != nullptr && !item->isVisible()) {
            rowsHidden = true;
            break;
        }
    }
    const bool surfaceCarriesRows = animating || rowsHidden;
    if (animating && scale < 0.999) {
        const QPointF origin(0.0, 0.0); // MenuAnchorPosition.Below: top-start
        painter.translate(origin);
        painter.scale(scale, scale);
        painter.translate(-origin);
    }

    // 1. The level-2 shadow, the surface fill and the container outline.
    // The outline is a 1 px stroke on the *full* container rect (inset half a
    // pixel so the stroke is not clipped by the widget edge) — previously the
    // surface filled the rounded path but never stroked it, so the menu had
    // no complete edge against a same-coloured page.
    // The shadow is skipped while the scale pair is flying: the rings are
    // near-black (ColorRole::Shadow = neutral0) and, under the painter scale,
    // land inside the widget as a dark halo along the edge (frame probe:
    // dedge=439 on the first open frame, dedge=673 mid-close). When settled
    // the rings sit outside the surface and the clip leaves only the soft
    // corner falloff, which is the intended elevation cue.
    if (!animating || scale >= 0.999) {
        MdElevation::drawShadowDp(&painter, surfaceRect, radius,
                                  MdElevation::shadowDp(tokens.containerElevation),
                                  MdTheme::instance().color(tokens.containerShadowColor));
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawPath(surfacePath);
    {
        const QColor outline = MdTheme::instance().color(ColorRole::OutlineVariant);
        if (outline.isValid() && outline.alpha() > 0) {
            QPen pen(outline);
            pen.setWidthF(1.0);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(MdStyleBase::crispRoundedRectPath(
                surfaceRect.adjusted(0.5, 0.5, -0.5, -0.5), radii));
        }
    }

    // 2. The rows. While the flight owns the frame the surface paints every
    //    row through `paintMenuItem` (same paint the filter uses on the real
    //    children). Dividers are a 1 px `surface-variant` rule — a plain
    //    QWidget paints nothing of its own, so the surface draws them in
    //    both states.
    {
        const QColor dividerColour = MdTheme::instance().color(tokens.dividerColor);
        const QList<QObject *> children = menu.children();
        for (QObject *childObject : children) {
            auto *child = qobject_cast<QWidget *>(childObject);
            if (child == nullptr || child->parent() != &menu) {
                continue;
            }
            if (auto *item = qobject_cast<MdMenuItem *>(child)) {
                if (!surfaceCarriesRows && item->isVisible()) {
                    continue; // the real child paints itself
                }
                painter.save();
                painter.translate(item->pos());
                paintMenuItem(painter, *item, tokens);
                painter.restore();
            } else if (child->width() > 0 && child->height() > 0
                       && child->height() <= int(tokens.dividerHeight) + 1) {
                if (dividerColour.isValid() && dividerColour.alpha() > 0) {
                    painter.fillRect(QRectF(qreal(child->x()), qreal(child->y()),
                                            qreal(child->width()), qreal(child->height())),
                                     dividerColour);
                }
            }
        }
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
