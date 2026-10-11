#include "MdExpressiveMenuStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdExpressiveMenu.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

MdExpressiveMenuStyle::MdExpressiveMenuStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdExpressiveMenuStyle *MdExpressiveMenuStyle::shared()
{
    static QMutex mutex;
    static MdExpressiveMenuStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdExpressiveMenuStyle;
        installPaintFilter<MdExpressiveMenuItem>(instance);
    }
    return instance;
}

bool MdExpressiveMenuStyle::isInstalled()
{
    return hasPaintFilter(&MdExpressiveMenuItem::staticMetaObject);
}

MdFocusRingSpec MdExpressiveMenuStyle::focusRingSpec(const MdExpressiveMenuTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    // The export points at the system *inner* offset.
    spec.inwardOffset = tokens.focusIndicatorInnerOffset;
    spec.inward = true;
    spec.shape = ShapeCorner::Medium;
    return spec;
}

MdNavigationItemState MdExpressiveMenuStyle::stateFor(const MdExpressiveMenuItem &item)
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
QColor stateLayerOverlay(const MdNavigationColourSlot &slot, StateLayerKind kind, qreal tokenOpacity)
{
    const QColor base = resolveSlot(slot);
    if (!base.isValid()) {
        return QColor();
    }
    const qreal systemOpacity = MdStateLayer::opacity(kind);
    const qreal scale = systemOpacity > 0.0 ? qBound(0.0, tokenOpacity / systemOpacity, 1.0) : 0.0;
    return MdStateLayer::overlayScaled(base, kind, scale);
}

qreal stateLayerOpacityFor(const MdExpressiveMenuTokens &tokens, MdNavigationItemState state)
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

/// The item's corner radius, resolved against the row size so `Full` works.
qreal itemRadius(const MdExpressiveMenuTokens &tokens, ShapeCorner corner, const QSizeF &size)
{
    return MdShape::resolvedRadius(corner, size);
}

} // namespace

void MdExpressiveMenuStyle::paintMenuItem(QPainter &painter, const MdExpressiveMenuItem &item,
                                          const MdExpressiveMenuTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationItemState state = stateFor(item);
    const bool interactive = !item.isEffectivelyDisabled();
    const MdExpressiveMenuSelection selection = item.isSelected()
        ? MdExpressiveMenuSelection::Selected
        : MdExpressiveMenuSelection::Unselected;

    const QRectF itemRect(QPointF(0, 0), QSizeF(item.width(), item.height()));

    // The row's shape inside its group. `item.parent()` is the menu when the
    // row is owned by one; standalone rows fall back to the neutral shape.
    int index = 0;
    int count = 1;
    if (auto *menu = qobject_cast<MdExpressiveMenu *>(item.parent())) {
        const QList<MdExpressiveMenuItem *> rows = menu->items();
        count = rows.size();
        index = rows.indexOf(const_cast<MdExpressiveMenuItem *>(&item));
        if (index < 0) {
            index = 0;
        }
    }
    const ShapeCorner shape = tokens.itemShapeFor(index, count, item.isSelected(),
                                                  state == MdNavigationItemState::Pressed);
    const qreal radius = itemRadius(tokens, shape, itemRect.size());

    // 1. The item container. Unselected rows carry the variant base (usually
    //    equal to the menu surface, so they read as transparent); selected
    //    rows fill with the variant's selected container.
    const QColor containerColour = resolveSlot(tokens.itemContainerFor(selection, state));
    if (containerColour.isValid() && containerColour.alpha() > 0) {
        // Only paint the unselected base when it actually differs from what
        // the surface already shows — otherwise the row would double-blend.
        const bool paintsUnselectedBase =
            selection == MdExpressiveMenuSelection::Selected
            || containerColour != MdTheme::instance().color(tokens.containerColor);
        if (paintsUnselectedBase) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(containerColour);
            painter.drawRoundedRect(itemRect, radius, radius);
        }
    }

    // 2. The state layer — hover and keyboard focus flat; the press rides the
    //    ripple.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind = state == MdNavigationItemState::Focused ? StateLayerKind::Focus
                                                                           : StateLayerKind::Hover;
        const QColor overlay = stateLayerOverlay(tokens.stateLayerFor(selection, state), kind,
                                                 stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawRoundedRect(itemRect, radius, radius);
        }
    }
    if (MdRippleController *ripple = item.rippleController()) {
        ripple->setBounds(itemRect.size());
        ripple->setContentColor(
            resolveSlot(tokens.stateLayerFor(selection, MdNavigationItemState::Pressed)));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            QPainterPath clip;
            clip.addRoundedRect(itemRect, radius, radius);
            MdRipple::paint(&painter, frame, clip, ripple->contentColor());
        }
    }

    // 3. The content row: leading space, 20 px icons, 12 px icon↔text, the
    //    label (label-large) and the optional supporting / trailing text.
    const bool rtl = item.layoutDirection() == Qt::RightToLeft;
    const QColor iconColour = resolveSlot(tokens.iconFor(selection, state));
    qreal x = tokens.itemLeadingSpace;
    auto mirror = [&](const QRectF &r) {
        return rtl ? QRectF(item.width() - r.right(), r.y(), r.width(), r.height()) : r;
    };

    if (!item.leadingIconName().isEmpty()) {
        MdIcon::paint(&painter,
                      mirror(QRectF(x, (item.height() - tokens.leadingIconSize) / 2.0,
                                    tokens.leadingIconSize, tokens.leadingIconSize)),
                      item.leadingIconName(), iconColour, item.iconSet(), item.iconFamily());
        x += tokens.leadingIconSize + tokens.itemIconTextSpacing;
    }

    qreal trailingWidth = tokens.itemTrailingSpace;
    if (!item.trailingIconName().isEmpty()) {
        trailingWidth += tokens.trailingIconSize + tokens.itemIconTextSpacing;
    }
    if (!item.trailingSupportingText().isEmpty()) {
        const QFont trailingFont = MdTypeScale::font(tokens.trailingSupportingTextType,
                                                     TypeEmphasis::Baseline,
                                                     MdTheme::instance().scriptCategory());
        const QFontMetricsF trailingMeasure(trailingFont);
        trailingWidth += trailingMeasure.horizontalAdvance(item.trailingSupportingText())
            + tokens.itemIconTextSpacing;
    }

    const QColor labelColour = resolveSlot(tokens.labelFor(selection, state));
    const QRectF labelRect = mirror(
        QRectF(x, item.supportingText().isEmpty() ? 0.0 : -2.0,
               item.width() - trailingWidth - x,
               item.supportingText().isEmpty() ? item.height() : item.height() * 0.55));
    if (labelColour.isValid() && !item.text().isEmpty()) {
        painter.setPen(labelColour);
        painter.setFont(MdTypeScale::font(tokens.labelTextType, TypeEmphasis::Baseline,
                                          MdTheme::instance().scriptCategory()));
        painter.drawText(labelRect, int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item)),
                         item.text());
    }

    if (!item.supportingText().isEmpty()) {
        const QColor supportingColour = resolveSlot(tokens.supportingTextFor(selection, state));
        if (supportingColour.isValid()) {
            painter.setPen(supportingColour);
            painter.setFont(MdTypeScale::font(tokens.supportingTextType, TypeEmphasis::Baseline,
                                              MdTheme::instance().scriptCategory()));
            painter.drawText(
                mirror(QRectF(x, item.height() * 0.5, item.width() - trailingWidth - x,
                              item.height() * 0.5)),
                int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item)),
                item.supportingText());
        }
    }

    if (!item.trailingSupportingText().isEmpty()) {
        const QColor trailingColour =
            resolveSlot(tokens.trailingSupportingTextFor(selection, state));
        if (trailingColour.isValid()) {
            const QFont trailingFont = MdTypeScale::font(tokens.trailingSupportingTextType,
                                                         TypeEmphasis::Baseline,
                                                         MdTheme::instance().scriptCategory());
            const QFontMetricsF trailingMeasure(trailingFont);
            const qreal w = trailingMeasure.horizontalAdvance(item.trailingSupportingText());
            painter.setPen(trailingColour);
            painter.setFont(trailingFont);
            painter.drawText(mirror(QRectF(item.width() - tokens.itemTrailingSpace - w, 0, w,
                                           item.height())),
                             int(Qt::AlignVCenter), item.trailingSupportingText());
        }
    }

    if (!item.trailingIconName().isEmpty()) {
        MdIcon::paint(&painter,
                      mirror(QRectF(item.width() - tokens.itemTrailingSpace - tokens.trailingIconSize,
                                    (item.height() - tokens.trailingIconSize) / 2.0,
                                    tokens.trailingIconSize, tokens.trailingIconSize)),
                      item.trailingIconName(), iconColour, item.iconSet(), item.iconFamily());
    }

    // 4. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, *inward* around the row's own shape.
    if (interactive && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, itemRect, {radius, radius, radius, radius},
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdExpressiveMenuStyle::paintMenu(QPainter &painter, MdExpressiveMenu &menu,
                                      const MdExpressiveMenuTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF surfaceRect(QPointF(0, 0), QSizeF(menu.width(), menu.height()));
    const qreal radius = MdShape::resolvedRadius(tokens.containerShape, surfaceRect.size());

    // The level-2 shadow under the surface.
    MdElevation::drawShadowDp(&painter, surfaceRect, radius, int(tokens.containerElevation),
                              MdTheme::instance().color(tokens.containerShadowColor));

    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawRoundedRect(surfaceRect, radius, radius);

    painter.restore();
}

void MdExpressiveMenuStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    if (auto *item = qobject_cast<MdExpressiveMenuItem *>(widget)) {
        paintMenuItem(*painter, *item, item->menuTokens());
    }
}

} // namespace md
