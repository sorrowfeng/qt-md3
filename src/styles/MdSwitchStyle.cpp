#include "MdSwitchStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdSwitch.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

MdSwitchStyle::MdSwitchStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSwitchStyle *MdSwitchStyle::shared()
{
    static QMutex mutex;
    static MdSwitchStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdSwitchStyle;
        installPaintFilter<MdSwitch>(instance);
    }
    return instance;
}

bool MdSwitchStyle::isInstalled()
{
    return hasPaintFilter(&MdSwitch::staticMetaObject);
}

MdFocusRingSpec MdSwitchStyle::focusRingSpec(const MdSwitchTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    // Around the track (`track.shape: corner-full`). Compose's inset-ring
    // variant is an opt-in upstream and not ported — recorded in the token
    // header.
    spec.shape = ShapeCorner::Full;
    return spec;
}

MdNavigationItemState MdSwitchStyle::stateFor(const MdSwitch &sw)
{
    if (sw.isEffectivelyDisabled()) {
        return MdNavigationItemState::Disabled;
    }
    if (sw.isDown()) {
        return MdNavigationItemState::Pressed;
    }
    if (sw.hasKeyboardFocus()) {
        return MdNavigationItemState::Focused;
    }
    if (sw.isHovered()) {
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

qreal stateLayerOpacityFor(const MdSwitchTokens &tokens, MdNavigationItemState state)
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

void MdSwitchStyle::paintSwitch(QPainter &painter, const MdSwitch &sw, const MdSwitchTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationItemState state = stateFor(sw);
    const bool interactive = !sw.isEffectivelyDisabled();
    const MdSwitchSelection selection =
        sw.isChecked() ? MdSwitchSelection::Selected : MdSwitchSelection::Unselected;
    const int s = int(selection);

    const QRectF trackRect = sw.trackRect();
    const QRectF thumbRect = sw.thumbRect();
    const QRectF layerRect = sw.stateLayerRect();
    const QList<qreal> trackRadii{trackRect.height() / 2.0, trackRect.height() / 2.0,
                                  trackRect.height() / 2.0, trackRect.height() / 2.0};

    // 1. The track — fill and 2 px outline. The checked side has no outline
    //    rows (Compose's default checked border is transparent).
    const QColor trackColour = resolveSlot(tokens.track[s][int(state)]);
    const QColor outlineColour = resolveSlot(tokens.trackOutline[s][int(state)]);
    if (trackColour.isValid()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(trackColour);
        painter.drawPath(MdShape::roundedRect(trackRect, trackRadii));
    }
    if (outlineColour.isValid() && tokens.trackOutlineWidth > 0.0) {
        QPen pen(outlineColour, tokens.trackOutlineWidth);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        const qreal half = tokens.trackOutlineWidth / 2.0;
        painter.drawPath(MdShape::roundedRect(trackRect.adjusted(half, half, -half, -half),
                                              {qMax(0.0, trackRect.height() / 2.0 - half),
                                               qMax(0.0, trackRect.height() / 2.0 - half),
                                               qMax(0.0, trackRect.height() / 2.0 - half),
                                               qMax(0.0, trackRect.height() / 2.0 - half)}));
    }

    // 2. The state layer riding the thumb — hover and keyboard focus only.
    //    The press is the ripple's.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const QColor overlay = stateLayerOverlay(tokens.stateLayer[s][int(state)], kind,
                                                 stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawEllipse(layerRect);
        }
    }

    // 3. The ripple — the pressed row's colour inside the 40 px circle at the
    //    thumb's centre.
    if (MdRippleController *ripple = sw.rippleController()) {
        ripple->setBounds(layerRect.size());
        ripple->setContentColor(resolveSlot(tokens.stateLayer[s][int(MdNavigationItemState::Pressed)]));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            QPainterPath clip;
            clip.addEllipse(layerRect);
            MdRipple::paint(&painter, frame, clip, ripple->contentColor());
        }
    }

    // 4. The handle — the animated-diameter circle with its elevation shadow
    //    (level 1; level 0 disabled), then the thumb icon.
    if (interactive) {
        MdElevation::drawShadowDp(&painter, thumbRect, thumbRect.height() / 2.0,
                                  MdElevation::shadowDp(tokens.handleElevation),
                                  MdTheme::instance().color(tokens.handleShadowColor));
    }
    const QColor handleColour = resolveSlot(tokens.handle[s][int(state)]);
    if (handleColour.isValid() && handleColour.alpha() > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(handleColour);
        painter.drawEllipse(thumbRect);
    }
    if (!sw.iconName().isEmpty()) {
        const QColor iconColour = resolveSlot(tokens.icon[s][int(state)]);
        if (iconColour.isValid() && iconColour.alpha() > 0) {
            const qreal inset = (thumbRect.width() - tokens.iconSize) / 2.0;
            MdIcon::paint(&painter,
                          thumbRect.adjusted(inset, inset, -inset, -inset), sw.iconName(),
                          iconColour, sw.iconSet(), sw.iconFamily());
        }
    }

    // 5. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, outward around the track.
    if (interactive && sw.hasKeyboardFocus()) {
        MdFocusRingController *ring = sw.focusRingController();
        MdFocusRing::paint(&painter, trackRect, trackRadii,
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdSwitchStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *sw = qobject_cast<MdSwitch *>(widget);
    if (painter == nullptr || sw == nullptr) {
        return;
    }
    paintSwitch(*painter, *sw, sw->switchTokens());
}

} // namespace md
