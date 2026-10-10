#include "MdChipStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdChip.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

MdChipStyle::MdChipStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdChipStyle *MdChipStyle::shared()
{
    static QMutex mutex;
    static MdChipStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdChipStyle;
        installPaintFilter<MdChip>(instance);
    }
    return instance;
}

bool MdChipStyle::isInstalled()
{
    return hasPaintFilter(&MdChip::staticMetaObject);
}

MdFocusRingSpec MdChipStyle::focusRingSpec(const MdChipVariantTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    spec.shape = tokens.containerShape;
    return spec;
}

MdChipInteraction MdChipStyle::stateFor(const MdChip &chip)
{
    if (chip.isEffectivelyDisabled()) {
        return MdChipInteraction::Disabled;
    }
    if (chip.isDragged()) {
        return MdChipInteraction::Dragged;
    }
    if (chip.isDown()) {
        return MdChipInteraction::Pressed;
    }
    if (chip.hasKeyboardFocus()) {
        return MdChipInteraction::Focused;
    }
    if (chip.isHovered()) {
        return MdChipInteraction::Hovered;
    }
    return MdChipInteraction::Enabled;
}

qreal MdChipStyle::cornerRadius(const MdChipVariantTokens &tokens)
{
    return MdShape::radius(tokens.containerShape);
}

namespace {

/// A `MdNavigationColourSlot` as a paintable colour.
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

qreal stateLayerOpacityFor(const MdChipVariantTokens &tokens, MdChipInteraction state)
{
    switch (state) {
    case MdChipInteraction::Hovered:
        return tokens.hoverStateLayerOpacity;
    case MdChipInteraction::Focused:
        return tokens.focusStateLayerOpacity;
    case MdChipInteraction::Pressed:
        return tokens.pressedStateLayerOpacity;
    case MdChipInteraction::Dragged:
        return tokens.draggedStateLayerOpacity;
    default:
        return 0.0;
    }
}

StateLayerKind stateLayerKindFor(MdChipInteraction state)
{
    switch (state) {
    case MdChipInteraction::Hovered:
        return StateLayerKind::Hover;
    case MdChipInteraction::Focused:
        return StateLayerKind::Focus;
    case MdChipInteraction::Pressed:
        return StateLayerKind::Pressed;
    case MdChipInteraction::Dragged:
        return StateLayerKind::Dragged;
    default:
        return StateLayerKind::Hover;
    }
}

} // namespace

void MdChipStyle::paintChip(QPainter &painter,
                            const MdChip &chip,
                            const MdChipVariantTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdChipInteraction state = stateFor(chip);
    const int selection = chip.isSelected() ? 1 : 0;
    const MdChipKind kind = chip.kind();
    const MdChipSurfaceTokens &surface = tokens.surface[int(kind)];
    const qreal radius = cornerRadius(tokens);
    const QRectF chipRect(QPointF(0.0, 0.0), QSizeF(qreal(chip.width()), qreal(chip.height())));
    const QList<qreal> radii{radius, radius, radius, radius};
    const QPainterPath chipPath = MdShape::roundedRect(chipRect, radii);
    const MdChip::Boxes boxes = chip.boxes();

    // 1. The container. Elevated chips draw their shadow first, then the fill.
    const QColor containerColour = resolveSlot(surface.container[selection][int(state)]);
    const bool disabled = state == MdChipInteraction::Disabled;
    if (kind == MdChipKind::Elevated && !disabled) {
        MdElevation::drawShadowDp(
            &painter, chipRect, radius,
            MdElevation::shadowDp(surface.containerElevation[int(state)]),
            MdTheme::instance().color(surface.containerShadowColor));
    }
    if (containerColour.isValid()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(containerColour);
        painter.drawPath(chipPath);
    }
    // The flat outline — one stroke, its own width and colour.
    const QColor outlineColour = resolveSlot(surface.outline[selection][int(state)]);
    if (outlineColour.isValid() && surface.outlineWidth[selection] > 0.0) {
        QPen pen(outlineColour, surface.outlineWidth[selection]);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        // Stroke centred on the rect inset by half the width.
        const qreal half = surface.outlineWidth[selection] / 2.0;
        painter.drawPath(MdShape::roundedRect(
            chipRect.adjusted(half, half, -half, -half),
            {qMax(0.0, radius - half), qMax(0.0, radius - half), qMax(0.0, radius - half),
             qMax(0.0, radius - half)}));
    }

    // 2. The state layer, clipped to the chip's shape. Hover / focus / drag
    //    are flat layers; the press is the ripple's.
    const bool interactive = !disabled;
    if (interactive
        && (state == MdChipInteraction::Hovered || state == MdChipInteraction::Focused)) {
        const QColor overlay =
            stateLayerOverlay(surface.stateLayer[selection][int(state)],
                              stateLayerKindFor(state),
                              stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(chipPath);
        }
    }

    // 3. The ripple, bounded to the chip and coloured with the pressed row —
    //    the families' swap lives in that row.
    if (MdRippleController *ripple = chip.rippleController()) {
        ripple->setBounds(chipRect.size());
        ripple->setContentColor(resolveSlot(
            surface.stateLayer[selection][int(MdChipInteraction::Pressed)]));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            MdRipple::paint(&painter, frame, chipPath, ripple->contentColor());
        }
    }

    // 4. The content.
    if (boxes.hasAvatar && boxes.avatar.isValid()) {
        // The avatar: a corner-full circle carrying the fallback glyph.
        const QColor avatarColour = resolveSlot(surface.leadingIcon[selection][int(state)]);
        painter.setPen(Qt::NoPen);
        painter.setBrush(avatarColour);
        painter.drawEllipse(boxes.avatar);
        if (!chip.avatarIconName().isEmpty() && avatarColour.isValid()) {
            MdIcon::paint(&painter, boxes.avatar, chip.avatarIconName(),
                          MdTheme::instance().color(ColorRole::OnSecondaryContainer),
                          chip.iconSet(), chip.iconFamily());
        }
    } else if (boxes.hasLeadingIcon && boxes.leadingIcon.isValid()) {
        const QColor iconColour = resolveSlot(surface.leadingIcon[selection][int(state)]);
        if (iconColour.isValid()) {
            MdIcon::paint(&painter, boxes.leadingIcon, chip.iconName(), iconColour,
                          chip.iconSet(), chip.iconFamily());
        }
    }

    if (boxes.label.isValid() && !chip.text().isEmpty()) {
        const QColor labelColour = resolveSlot(surface.label[selection][int(state)]);
        if (labelColour.isValid()) {
            painter.setPen(labelColour);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(MdTypeScale::font(tokens.labelTextType, TypeEmphasis::Baseline,
                                              MdTheme::instance().scriptCategory()));
            painter.drawText(boxes.label, int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&chip)), chip.text());
        }
    }

    if (boxes.hasTrailingIcon && boxes.trailingIcon.isValid()) {
        const QColor trailingColour = resolveSlot(surface.trailingIcon[selection][int(state)]);
        if (trailingColour.isValid()) {
            MdIcon::paint(&painter, boxes.trailingIcon, chip.trailingIconName(), trailingColour,
                          chip.iconSet(), chip.iconFamily());
        }
    }

    // 5. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, outward around the chip.
    if (interactive && chip.hasKeyboardFocus()) {
        MdFocusRingController *ring = chip.focusRingController();
        MdFocusRing::paint(&painter, chipRect, radii,
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdChipStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *chip = qobject_cast<MdChip *>(widget);
    if (painter == nullptr || chip == nullptr) {
        return;
    }
    paintChip(*painter, *chip, chip->chipTokens());
}

} // namespace md
