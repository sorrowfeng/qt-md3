#include "MdRadioButtonStyle.h"

#include "core/MdColorMath.h"
#include "core/MdFocusRing.h"
#include "core/MdRipple.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdRadioButton.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

MdRadioButtonStyle::MdRadioButtonStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdRadioButtonStyle *MdRadioButtonStyle::shared()
{
    static QMutex mutex;
    static MdRadioButtonStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdRadioButtonStyle;
        installPaintFilter<MdRadioButton>(instance);
    }
    return instance;
}

bool MdRadioButtonStyle::isInstalled()
{
    return hasPaintFilter(&MdRadioButton::staticMetaObject);
}

MdFocusRingSpec MdRadioButtonStyle::focusRingSpec(const MdRadioButtonTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    // Compose's `focusRingShape = CircleShape`; the export publishes no shape
    // row. The paint passes the icon's circular radii explicitly.
    spec.shape = ShapeCorner::Full;
    return spec;
}

MdNavigationItemState MdRadioButtonStyle::stateFor(const MdRadioButton &button)
{
    if (button.isEffectivelyDisabled()) {
        return MdNavigationItemState::Disabled;
    }
    if (button.isDown()) {
        return MdNavigationItemState::Pressed;
    }
    if (button.hasKeyboardFocus()) {
        return MdNavigationItemState::Focused;
    }
    if (button.isHovered()) {
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

qreal stateLayerOpacityFor(const MdRadioButtonTokens &tokens, MdNavigationItemState state)
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

/// The two sides of a colour table faded by the button's progress. Where one
/// side is absent the fade runs against transparent — Compose's
/// `animateColorAsState` crosses `Color.Transparent` the same way. Both halves
/// resolve at the *current* interaction state, so a hover on a selected button
/// reads the selected-hover row on both sides.
QColor fadedColour(const QColor &from, const QColor &to, qreal progress)
{
    QColor a = from.isValid() ? from : QColor(Qt::transparent);
    const QColor b = to.isValid() ? to : QColor(Qt::transparent);
    const qreal clamped = qBound(0.0, progress, 1.0);
    if (clamped <= 0.0) {
        return a.alpha() == 0 ? QColor() : a;
    }
    QColor out = QColor::fromRgba(MdColorMath::lerpOklab(a.rgba(), b.rgba(), clamped));
    out.setAlphaF(qBound(0.0, a.alphaF() + (b.alphaF() - a.alphaF()) * clamped, 1.0));
    return out;
}

} // namespace

void MdRadioButtonStyle::paintRadioButton(QPainter &painter,
                                          const MdRadioButton &button,
                                          const MdRadioButtonTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationItemState state = stateFor(button);
    const bool interactive = !button.isEffectivelyDisabled();
    const MdRadioButtonSelection selection =
        button.isChecked() ? MdRadioButtonSelection::Selected : MdRadioButtonSelection::Unselected;
    const qreal progress = qBound(0.0, button.colourProgress(), 1.0);

    const QRectF iconRect = button.iconRect();
    const QRectF layerRect = button.stateLayerRect();

    // 1. The circular state layer — hover and keyboard focus only. The press
    //    rides on the ripple.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const MdNavigationColourSlot &slot = tokens.stateLayerFor(selection, state);
        const QColor overlay = stateLayerOverlay(slot, kind, stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawEllipse(layerRect);
        }
    }

    // 2. The ripple — the pressed row's colour inside the 40 px layer's circle
    //    (the bounded stand-in for Compose's fixed-radius unbounded circle;
    //    recorded in the token header).
    if (MdRippleController *ripple = button.rippleController()) {
        ripple->setBounds(layerRect.size());
        const MdNavigationColourSlot &pressSlot =
            tokens.stateLayerFor(selection, MdNavigationItemState::Pressed);
        ripple->setContentColor(resolveSlot(pressSlot));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            QPainterPath clip;
            clip.addEllipse(layerRect);
            MdRipple::paint(&painter, frame, clip, ripple->contentColor());
        }
    }

    // 3. The icon — one colour for the stroke circle and the dot, faded
    //    between the two selection tables at the current interaction state.
    const QColor iconColour =
        fadedColour(resolveSlot(tokens.iconFor(MdRadioButtonSelection::Unselected, state)),
                    resolveSlot(tokens.iconFor(MdRadioButtonSelection::Selected, state)),
                    progress);
    if (iconColour.isValid() && iconColour.alpha() > 0) {
        // The stroke circle: radius `iconSize/2 - strokeWidth/2`, stroked
        // `strokeWidth` — the stroke's outer edge lands on the icon canvas.
        const qreal strokeRadius = tokens.iconSize / 2.0 - tokens.strokeWidth / 2.0;
        QPen pen(iconColour, tokens.strokeWidth);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawEllipse(QRectF(iconRect.center().x() - strokeRadius,
                                   iconRect.center().y() - strokeRadius, strokeRadius * 2.0,
                                   strokeRadius * 2.0));

        // The dot: radius `animatedDotDiameter/2 - strokeWidth/2`, filled.
        const qreal dotRadius = button.animatedDotDiameter() / 2.0 - tokens.strokeWidth / 2.0;
        if (dotRadius > 0.0) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(iconColour);
            painter.drawEllipse(QRectF(iconRect.center().x() - dotRadius,
                                       iconRect.center().y() - dotRadius, dotRadius * 2.0,
                                       dotRadius * 2.0));
        }
    }

    // 4. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, a circle around the icon (Compose's CircleShape).
    if (interactive && button.hasKeyboardFocus()) {
        MdFocusRingController *ring = button.focusRingController();
        const qreal r = iconRect.height() / 2.0;
        MdFocusRing::paint(&painter, iconRect, {r, r, r, r},
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdRadioButtonStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *button = qobject_cast<MdRadioButton *>(widget);
    if (painter == nullptr || button == nullptr) {
        return;
    }
    paintRadioButton(*painter, *button, button->radioButtonTokens());
}

} // namespace md
