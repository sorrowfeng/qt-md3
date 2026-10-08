#include "MdButtonStyle.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdButton.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// Stroke centre line sits `offset + width / 2` outside the component, and the
/// stroke reaches half of whatever width it currently has. At the peak of the
/// focus grow animation that width is `activeWidth`, so this is the furthest
/// the indicator ever reaches.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

/// A colour role + opacity multiplier, resolved against the theme.
QColor resolveSlot(const MdButtonColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (qFuzzyCompare(slot.opacity, 1.0)) {
        return colour;
    }
    // Fold the multiplier into alpha so the colour keeps compositing against
    // whatever the button sits on — which is what M3's disabled opacities mean,
    // rather than a pre-blend against an assumed surface.
    colour.setAlphaF(colour.alphaF() * slot.opacity);
    return colour;
}

} // namespace

MdButtonStyle::MdButtonStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdButtonStyle *MdButtonStyle::shared()
{
    static QMutex mutex;
    static MdButtonStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints, and destroying it at static-destruction time would
        // race whatever widgets are still alive.
        instance = new MdButtonStyle;
        installPaintFilter<MdButton>(instance);
    }
    return instance;
}

bool MdButtonStyle::isInstalled()
{
    return hasPaintFilter(&MdButton::staticMetaObject);
}

MdFocusRingSpec MdButtonStyle::focusRingSpec(const MdButtonTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOffset;
    spec.inward = false;
    // The button token set publishes `thickness` only. `active-width`,
    // `duration` and the easing are md.comp.focus-ring's, and the module's
    // defaults already carry them, so they are deliberately left alone rather
    // than restated here where they could drift.
    spec.color = QColor();
    return spec;
}

qreal MdButtonStyle::focusRingInset(const MdFocusRingSpec &spec)
{
    return ringInsetFor(spec);
}

MdButtonState MdButtonStyle::stateFor(const MdButton &button)
{
    if (button.isEffectivelyDisabled()) {
        return MdButtonState::Disabled;
    }
    if (button.isDown()) {
        return MdButtonState::Pressed;
    }
    if (button.isHovered()) {
        return MdButtonState::Hovered;
    }
    if (button.hasFocus()) {
        return MdButtonState::Focused;
    }
    return MdButtonState::Enabled;
}

QColor MdButtonStyle::containerColor(const MdButtonTokens &tokens, MdButtonState state)
{
    return resolveSlot(tokens.state(state).container);
}

QColor MdButtonStyle::labelColor(const MdButtonTokens &tokens, MdButtonState state)
{
    return resolveSlot(tokens.state(state).labelText);
}

QColor MdButtonStyle::iconColor(const MdButtonTokens &tokens, MdButtonState state)
{
    return resolveSlot(tokens.state(state).icon);
}

QColor MdButtonStyle::outlineColor(const MdButtonTokens &tokens, MdButtonState state)
{
    const ColorRole role = tokens.state(state).outline;
    if (role == ColorRole::Count) {
        return QColor();
    }
    return MdTheme::instance().color(role);
}

MdButtonStyle::Layout MdButtonStyle::layoutFor(const MdButton &button, const MdButtonTokens &tokens)
{
    Layout layout;

    const MdFocusRingSpec ringSpec = focusRingSpec(tokens);
    const qreal inset = ringInsetFor(ringSpec);
    const QRectF widgetRect(button.rect());
    const QRectF inner = widgetRect.adjusted(inset, inset, -inset, -inset);

    // The label as painted, not QPushButton::text() — see displayText().
    const QString label = button.displayText();
    const QFont font = MdTypeScale::font(tokens.labelStyle, TypeEmphasis::Baseline,
                                         MdTheme::instance().scriptCategory());
    const QFontMetricsF metrics(font);
    const qreal labelWidth = label.isEmpty() ? 0.0 : metrics.horizontalAdvance(label);

    // "Leading" is the inline-start side, so it swaps under RTL.
    const bool rtl = MdTheme::instance().isRightToLeft();
    const QString startIcon = rtl ? button.trailingIcon() : button.leadingIcon();
    const QString endIcon = rtl ? button.leadingIcon() : button.trailingIcon();
    layout.startIconName = startIcon;
    layout.endIconName = endIcon;
    layout.hasLeadingIcon = !startIcon.isEmpty();
    layout.hasTrailingIcon = !endIcon.isEmpty();

    // md.comp.button.<size> arithmetic: padding, icon, gap, label, gap, icon,
    // padding — with the gaps only present when there is a label to separate.
    const bool hasLabel = !label.isEmpty();
    qreal contentWidth = tokens.leadingSpace + tokens.trailingSpace + labelWidth;
    if (layout.hasLeadingIcon) {
        contentWidth += tokens.iconSize;
        if (hasLabel) {
            contentWidth += tokens.iconLabelSpace;
        }
    }
    if (layout.hasTrailingIcon) {
        contentWidth += tokens.iconSize;
        if (hasLabel) {
            contentWidth += tokens.iconLabelSpace;
        }
    }

    layout.preferredContainerSize = QSizeF(contentWidth, tokens.containerHeight);

    // Horizontally the container fills whatever the layout offers (a stretched
    // button is a legitimate M3 shape); vertically it is the token height,
    // centred, so a taller cell does not stretch the pill.
    const qreal availableWidth = qMax(inner.width(), 0.0);
    const qreal containerWidth = qMax(availableWidth, contentWidth);
    const qreal containerHeight = tokens.containerHeight;
    layout.container = QRectF(inner.left(), inner.top() + (inner.height() - containerHeight) / 2.0,
                              containerWidth, containerHeight);
    if (inner.height() <= 0.0) {
        layout.container = QRectF(inner.left(), inner.top(), containerWidth, containerHeight);
    }

    // Corner radii, morphed for the press. Both ends are resolved through the
    // shape scale so `corner-full` becomes a real pill for this box rather than
    // the -1 sentinel — unless the button carries a per-corner override, which
    // a button group sets so an item can show a different corner towards each
    // neighbour. An override is already a list of concrete radii in the same
    // order, so it bypasses the shape tokens entirely rather than blending with
    // them: the group computed it from the group's own tokens.
    const QSizeF containerSize = layout.container.size();
    const QList<qreal> restingOverride = button.restingCornerRadii();
    const QList<qreal> pressedOverride = button.pressedCornerRadii();
    const QList<qreal> resting = restingOverride.size() == 4
                                     ? restingOverride
                                     : MdShape::resolvedRadii(tokens.restingShape, containerSize);
    const QList<qreal> pressed = pressedOverride.size() == 4
                                     ? pressedOverride
                                     : MdShape::resolvedRadii(tokens.pressedShape, containerSize);
    layout.radii = MdShape::lerpRadii(resting, pressed, button.pressMorph());

    // The content group is centred in the container, so a stretched button
    // keeps its icon and label together in the middle.
    const qreal slack = qMax(containerWidth - contentWidth, 0.0);
    qreal cursor = rtl ? layout.container.right() - slack / 2.0 - tokens.leadingSpace
                       : layout.container.left() + slack / 2.0 + tokens.leadingSpace;
    const qreal dir = rtl ? -1.0 : 1.0;

    const auto place = [&](qreal width) {
        const QRectF box = rtl ? QRectF(cursor - width, layout.container.top(), width,
                                        layout.container.height())
                               : QRectF(cursor, layout.container.top(), width,
                                        layout.container.height());
        cursor += dir * width;
        return box;
    };

    if (layout.hasLeadingIcon) {
        layout.leadingIcon = place(tokens.iconSize);
        if (hasLabel) {
            cursor += dir * tokens.iconLabelSpace;
        }
    }
    if (hasLabel) {
        layout.label = place(labelWidth);
        if (layout.hasTrailingIcon) {
            cursor += dir * tokens.iconLabelSpace;
        }
    }
    if (layout.hasTrailingIcon) {
        layout.trailingIcon = place(tokens.iconSize);
    }

    return layout;
}

void MdButtonStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *button = qobject_cast<MdButton *>(widget);
    if (painter == nullptr || button == nullptr) {
        return;
    }
    const MdButtonTokens &tokens = button->tokens();
    paintButton(*painter, *button, tokens, layoutFor(*button, tokens));
}

void MdButtonStyle::paintButton(QPainter &painter,
                                const MdButton &button,
                                const MdButtonTokens &tokens,
                                const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const MdButtonState state = stateFor(button);
    const MdButtonStateColours &colours = tokens.state(state);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // The outlined variant's stroke lives inside the container box (CSS
    // `border` with border-box sizing), so everything that must sit *inside*
    // the stroke — the state layer and the ripple — uses this inset path.
    const qreal strokeInset = colours.paintsOutline() ? tokens.outlineWidth / 2.0 : 0.0;
    QList<qreal> innerRadii = layout.radii;
    for (qreal &radius : innerRadii) {
        radius = qMax(radius - strokeInset, 0.0);
    }
    const QRectF innerRect = layout.container.adjusted(strokeInset, strokeInset, -strokeInset,
                                                       -strokeInset);
    const QPainterPath innerPath = MdShape::roundedRect(innerRect, innerRadii);

    // 1. Shadow, then the container.
    //
    // Only the variants whose *enabled* elevation is non-zero draw one, which
    // in this token set is the elevated variant alone. MD3 expresses height
    // with tonal surfaces, so a filled or tonal button gets its container
    // colour and nothing else — even though `md.comp.button.filled.hovered.
    // container.elevation` publishes level1 and would otherwise put a shadow on
    // every hover. That token is marked `@deprecated No longer part of the
    // design spec` upstream; it is preserved in the table and deliberately not
    // painted. See docs/porting-todo.md.
    const bool paintsShadow = tokens.enabled.elevation != ElevationLevel::Level0;
    if (paintsShadow && colours.elevation != ElevationLevel::Level0) {
        const qreal radius = layout.radii.isEmpty() ? 0.0 : layout.radii.first();
        MdElevation::drawShadow(&painter, layout.container, radius, colours.elevation,
                                theme.color(ColorRole::Shadow));
    }

    if (colours.paintsContainer()) {
        const QColor fill = containerColor(tokens, state);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(path);
        }
    }

    // 2. State layer. One composited overlay, not a stack — M3 does not add
    //    hover + focus + pressed together. Painted as a translucent tint so it
    //    is correct over a container *and* over whatever a container-less
    //    variant happens to be sitting on.
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

    // 3. Ripple, clipped to the morphing inner shape. The controller works in
    //    container-local coordinates, so the painter moves to the container
    //    origin first and the clip path is built in that space too. The radii
    //    come straight from the layout, so a per-corner override is followed
    //    rather than recomputed from the shape tokens.
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

    // 4. Outline, on top of the state layer so a hover does not tint the stroke.
    if (colours.paintsOutline()) {
        const QColor stroke = outlineColor(tokens, state);
        if (stroke.isValid() && tokens.outlineWidth > 0.0) {
            QPen pen(stroke);
            pen.setWidthF(tokens.outlineWidth);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(innerPath);
        }
    }

    // 5. Icon and label.
    const QColor iconColour = iconColor(tokens, state);
    const QColor textColour = labelColor(tokens, state);

    if (layout.hasLeadingIcon && iconColour.isValid()) {
        MdIcon::paint(&painter, layout.leadingIcon, layout.startIconName, iconColour);
    }
    if (layout.hasTrailingIcon && iconColour.isValid()) {
        MdIcon::paint(&painter, layout.trailingIcon, layout.endIconName, iconColour);
    }

    if (!layout.label.isEmpty() && textColour.isValid()) {
        const QFont labelFont = MdTypeScale::font(tokens.labelStyle, TypeEmphasis::Baseline,
                                                  theme.scriptCategory());
        const QFontMetricsF metrics(labelFont);
        // Baseline placement rather than drawText(rect, AlignCenter): it needs
        // no slack in the rect, so the label is never clipped and the group
        // stays exactly the width the token arithmetic computed.
        const qreal baseline = layout.label.top()
                               + (layout.label.height() + metrics.ascent() - metrics.descent()) / 2.0;
        painter.setFont(labelFont);
        painter.setPen(textColour);
        const QString label = button.displayText();
        const qreal x = theme.isRightToLeft()
                            ? layout.label.right() - metrics.horizontalAdvance(label)
                            : layout.label.left();
        painter.drawText(QPointF(x, baseline), label);
    }

    // 6. Focus indicator, last so nothing paints over it, and only while the
    //    button can actually be activated.
    if (interactive && button.hasFocus()) {
        MdFocusRingController *ring = button.focusRingController();
        const MdFocusRingSpec spec = focusRingSpec(tokens);
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(tokens.focusIndicator), spec,
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

} // namespace md
