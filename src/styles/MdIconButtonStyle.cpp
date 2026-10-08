#include "MdIconButtonStyle.h"

#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdIconButton.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// Same derivation as the common button's: the stroke centre line sits
/// `offset + activeWidth / 2 + width / 2` outside the component.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

/// A colour role + opacity multiplier, resolved against the theme.
QColor resolveSlot(const MdIconButtonColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (qFuzzyCompare(slot.opacity, 1.0)) {
        return colour;
    }
    // Fold the multiplier into alpha so the colour keeps compositing against
    // whatever the button sits on — the disabled opacities are alpha facts,
    // not pre-blends against an assumed surface.
    colour.setAlphaF(colour.alphaF() * slot.opacity);
    return colour;
}

} // namespace

MdIconButtonStyle::MdIconButtonStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdIconButtonStyle *MdIconButtonStyle::shared()
{
    static QMutex mutex;
    static MdIconButtonStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdIconButtonStyle;
        installPaintFilter<MdIconButton>(instance);
    }
    return instance;
}

bool MdIconButtonStyle::isInstalled()
{
    return hasPaintFilter(&MdIconButton::staticMetaObject);
}

MdFocusRingSpec MdIconButtonStyle::focusRingSpec(const MdIconButtonTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOffset;
    spec.inward = false;
    // `active-width`, `duration` and the easing belong to the
    // md.comp.focus-ring module, whose defaults already carry them; restating
    // them here would be a second place for the same numbers to drift.
    spec.color = QColor();
    return spec;
}

qreal MdIconButtonStyle::focusRingInset(const MdIconButtonTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdIconButtonState MdIconButtonStyle::stateFor(const MdIconButton &button)
{
    if (button.isEffectivelyDisabled()) {
        return MdIconButtonState::Disabled;
    }
    if (button.isDown()) {
        return MdIconButtonState::Pressed;
    }
    if (button.isHovered()) {
        return MdIconButtonState::Hovered;
    }
    if (button.hasFocus()) {
        return MdIconButtonState::Focused;
    }
    return MdIconButtonState::Enabled;
}

MdIconButtonStyle::Layout MdIconButtonStyle::layoutFor(const MdIconButton &button,
                                                       const MdIconButtonTokens &tokens)
{
    Layout layout;

    const qreal inset = focusRingInset(tokens);
    const QRectF widgetRect(button.rect());
    const QRectF inner = widgetRect.adjusted(inset, inset, -inset, -inset);

    // md.comp.icon-button arithmetic: leading space, icon, trailing space —
    // symmetric on the default track, which makes the container a square of
    // the token height.
    const qreal leading = button.spaceTrack() == IconButtonSpaceTrack::Narrow
                              ? tokens.narrowLeadingSpace
                              : button.spaceTrack() == IconButtonSpaceTrack::Wide
                                    ? tokens.wideLeadingSpace
                                    : tokens.defaultLeadingSpace;
    const qreal trailing = button.spaceTrack() == IconButtonSpaceTrack::Narrow
                               ? tokens.narrowTrailingSpace
                               : button.spaceTrack() == IconButtonSpaceTrack::Wide
                                     ? tokens.wideTrailingSpace
                                     : tokens.defaultTrailingSpace;
    const QSizeF containerSize(leading + tokens.iconSize + trailing, tokens.containerHeight);

    // The container is centred in the inner rect: horizontally a squeezed
    // widget keeps a centred square rather than a squashed one, vertically the
    // token height wins over whatever the widget ended up being.
    const QPointF origin(inner.left() + qMax<qreal>((inner.width() - containerSize.width()) / 2.0, 0.0),
                         inner.top() + qMax<qreal>((inner.height() - containerSize.height()) / 2.0, 0.0));
    layout.container = QRectF(origin, containerSize);
    layout.preferredSize = QSizeF(containerSize.width() + 2.0 * inset,
                                  containerSize.height() + 2.0 * inset);

    // Corner radii, selected-aware and morphed for the press. The selected
    // shapes are the published *other* knob — selected round is the square-ish
    // corner and selected square is full — which is the token form of the
    // spec's "selected shape changes between square and round".
    const bool selectedNow = button.isToggleable() && button.isChecked();
    const ShapeCorner resting = selectedNow ? (button.buttonShape() == ButtonShape::Square
                                                   ? tokens.selectedSquareShape
                                                   : tokens.selectedRoundShape)
                                            : (button.buttonShape() == ButtonShape::Square
                                                   ? tokens.squareShape
                                                   : tokens.roundShape);
    const QList<qreal> restingRadii = MdShape::resolvedRadii(resting, containerSize);
    const QList<qreal> pressedRadii = MdShape::resolvedRadii(tokens.pressedShape, containerSize);
    layout.radii = MdShape::lerpRadii(restingRadii, pressedRadii, button.pressMorph());

    layout.icon = QRectF(layout.container.center().x() - tokens.iconSize / 2.0,
                         layout.container.center().y() - tokens.iconSize / 2.0,
                         tokens.iconSize, tokens.iconSize);

    return layout;
}

void MdIconButtonStyle::paintIconButton(QPainter &painter,
                                        const MdIconButton &button,
                                        const MdIconButtonTokens &tokens,
                                        const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const bool selectedNow = button.isToggleable() && button.isChecked();
    const MdIconButtonStateColours &colours =
        tokens.familyFor(button.isToggleable(), selectedNow)
            .state(stateFor(button));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // The outlined stroke lives inside the container box, so the state layer
    // and the ripple use the inset path — the same arrangement the common
    // button's outlined variant makes.
    const qreal strokeInset = colours.paintsOutline() ? tokens.outlineWidth / 2.0 : 0.0;
    QList<qreal> innerRadii = layout.radii;
    for (qreal &radius : innerRadii) {
        radius = qMax(radius - strokeInset, 0.0);
    }
    const QRectF innerRect = layout.container.adjusted(strokeInset, strokeInset, -strokeInset,
                                                       -strokeInset);
    const QPainterPath innerPath = MdShape::roundedRect(innerRect, innerRadii);

    // 1. Container. No elevation anywhere in this family: no icon-button
    //    variant publishes a container.elevation, so there is no shadow to
    //    consider — height is carried by the container colour alone.
    if (colours.paintsContainer()) {
        const QColor fill = resolveSlot(colours.container);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(path);
        }
    }

    // 2. State layer, one composited overlay.
    const bool interactive = !button.isEffectivelyDisabled();
    StateLayerKind kind = StateLayerKind::Hover;
    if (interactive
        && MdStateLayer::strongestActive(&kind, button.isHovered(), button.hasFocus(),
                                         button.isDown(), false)
        && colours.stateLayer != ColorRole::Count) {
        const QColor overlay = MdStateLayer::overlay(theme.color(colours.stateLayer), kind);
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(innerPath);
        }
    }

    // 3. Ripple, clipped to the morphing inner shape.
    if (MdRippleController *ripple = button.rippleController()) {
        const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, layout.radii);
        ripple->setBounds(layout.container.size());
        ripple->setClipPath(localPath);
        ripple->setContentColor(theme.color(ColorRole::OnSurface));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(layout.container.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 4. Outline, above the state layer so a hover does not tint the stroke.
    if (colours.paintsOutline() && tokens.outlineWidth > 0.0) {
        const QColor stroke = theme.color(colours.outline);
        if (stroke.isValid()) {
            QPen pen(stroke);
            pen.setWidthF(tokens.outlineWidth);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(innerPath);
        }
    }

    // 5. The icon — the whole content of this component.
    const QColor iconColour = resolveSlot(colours.icon);
    if (iconColour.isValid()) {
        MdIcon::paint(&painter, layout.icon, button.iconName(), iconColour,
                      button.iconSet(), button.iconFamily());
    }

    // 6. Focus indicator, last so nothing paints over it.
    if (interactive && button.hasFocus()) {
        MdFocusRingController *ring = button.focusRingController();
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(tokens.focusIndicator), focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdIconButtonStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *button = qobject_cast<MdIconButton *>(widget);
    if (painter == nullptr || button == nullptr) {
        return;
    }
    const MdIconButtonTokens &tokens = button->tokens();
    paintIconButton(*painter, *button, tokens, layoutFor(*button, tokens));
}

} // namespace md
