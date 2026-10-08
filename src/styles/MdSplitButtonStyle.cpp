#include "MdSplitButtonStyle.h"

#include "MdButtonStyle.h"
#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdSplitButton.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// Stroke centre line sits `offset + activeWidth / 2 + width / 2` outside the
/// component — the same derivation MdButtonStyle uses.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

/// A colour slot of the borrowed button rows, resolved against the theme.
QColor resolveSlot(const MdButtonColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (!qFuzzyCompare(slot.opacity, 1.0)) {
        colour.setAlphaF(colour.alphaF() * slot.opacity);
    }
    return colour;
}

/// One half's painted shape: the outer edge is the full-pill corner, the
/// facing edge carries the animated inner radius.
QList<qreal> halfRadii(qreal outer, qreal inner, bool outerOnLeft)
{
    // TL, TR, BR, BL.
    return outerOnLeft ? QList<qreal>{outer, inner, inner, outer}
                       : QList<qreal>{inner, outer, outer, inner};
}

} // namespace

MdSplitButtonStyle::MdSplitButtonStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSplitButtonStyle *MdSplitButtonStyle::shared()
{
    static QMutex mutex;
    static MdSplitButtonStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints (same contract as the other families).
        instance = new MdSplitButtonStyle;
        installPaintFilter<MdSplitButton>(instance);
    }
    return instance;
}

bool MdSplitButtonStyle::isInstalled()
{
    return hasPaintFilter(&MdSplitButton::staticMetaObject);
}

MdFocusRingSpec MdSplitButtonStyle::focusRingSpec(const MdButtonTokens &tokens)
{
    // The split-button export publishes no focus-indicator rows; the button
    // family's come across with the colour rows (recorded in
    // docs/porting-todo.md).
    return MdButtonStyle::focusRingSpec(tokens);
}

qreal MdSplitButtonStyle::focusRingInset(const MdButtonTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdButtonState MdSplitButtonStyle::stateFor(const MdSplitButton &button, MdSplitButton::Zone zone)
{
    if (button.isEffectivelyDisabled()) {
        return MdButtonState::Disabled;
    }
    if (button.pressedZone() == zone) {
        return MdButtonState::Pressed;
    }
    if (button.hoveredZone() == zone) {
        return MdButtonState::Hovered;
    }
    // The Focused colour row applies to the half keyboard interaction would
    // act on, under the same `:focus-visible` rule as everywhere else.
    if (button.hasKeyboardFocus() && button.focusedZone() == zone) {
        return MdButtonState::Focused;
    }
    return MdButtonState::Enabled;
}

MdSplitButtonStyle::Layout MdSplitButtonStyle::layoutFor(const MdSplitButton &button,
                                                         const MdSplitButtonTokens &tokens)
{
    Layout layout;

    const MdSplitButtonMetrics &metrics = tokens.metrics;
    const MdButtonTokens &buttonTokens = tokens.button;

    const MdFocusRingSpec ringSpec = focusRingSpec(buttonTokens);
    const qreal inset = ringInsetFor(ringSpec);
    layout.inset = inset;

    const QRectF widgetRect(button.rect());
    const QRectF inner = widgetRect.adjusted(inset, inset, -inset, -inset);

    // Leading content: padding, [icon, gap], label, padding — the gaps only
    // present when there is a label to separate (the button family's rule).
    const MdTheme &theme = MdTheme::instance();
    const bool rtl = theme.isRightToLeft();
    const bool hasLabel = !button.text().isEmpty();
    const bool hasLeadingIcon = !button.leadingIcon().isEmpty();

    const QFont font = MdTypeScale::font(buttonTokens.labelStyle, TypeEmphasis::Baseline,
                                         theme.scriptCategory());
    const QFontMetricsF fontMetrics(font);
    const qreal labelWidth = hasLabel ? fontMetrics.horizontalAdvance(button.text()) : 0.0;

    const qreal leadingHeight = metrics.containerHeight;
    qreal leadingWidth = metrics.leadingLeadingSpace + metrics.leadingTrailingSpace + labelWidth;
    if (hasLeadingIcon) {
        leadingWidth += buttonTokens.iconSize;
        if (hasLabel) {
            leadingWidth += buttonTokens.iconLabelSpace;
        }
    }
    const qreal trailingWidth =
        metrics.trailingLeadingSpace + metrics.trailingIconSize + metrics.trailingTrailingSpace;

    const qreal splitWidth = leadingWidth + metrics.betweenSpace + trailingWidth;
    const qreal containerWidth = qMax<qreal>(inner.width(), splitWidth);
    layout.container = QRectF(inner.left(),
                              inner.top() + qMax<qreal>(0.0, (inner.height() - leadingHeight) / 2.0),
                              containerWidth, leadingHeight);

    // The split hugs the inline-start edge of the container; the slack a
    // stretched control leaves goes to the inline-end side.
    const qreal slack = qMax<qreal>(0.0, containerWidth - splitWidth);
    const qreal leadingLeft = rtl ? layout.container.right() - slack - leadingWidth
                                  : layout.container.left();
    const qreal trailingLeft = rtl ? layout.container.left()
                                   : leadingLeft + leadingWidth + metrics.betweenSpace;
    layout.leading = QRectF(leadingLeft, layout.container.top(), leadingWidth, leadingHeight);
    layout.trailing = QRectF(trailingLeft, layout.container.top(), trailingWidth, leadingHeight);

    // Corner radii. The outer corner is corner-full at every size; the facing
    // corners carry the animated inner radius from the widget.
    const qreal outer = metrics.outerCornerRadius();
    const qreal leadingInner = button.innerCornerRadius(MdSplitButton::Zone::Leading);
    const qreal trailingInner = button.innerCornerRadius(MdSplitButton::Zone::Trailing);
    // In LTR the leading half's outer edge is on the left; in RTL it swaps.
    layout.leadingRadii = halfRadii(outer, leadingInner, !rtl);
    layout.trailingRadii = halfRadii(outer, trailingInner, rtl);

    // Content boxes inside the leading half.
    const qreal dir = rtl ? -1.0 : 1.0;
    qreal cursor = rtl ? layout.leading.right() - metrics.leadingLeadingSpace
                       : layout.leading.left() + metrics.leadingLeadingSpace;
    const auto place = [&](qreal width) {
        const QRectF box = rtl ? QRectF(cursor - width, layout.leading.top(), width, leadingHeight)
                               : QRectF(cursor, layout.leading.top(), width, leadingHeight);
        cursor += dir * width;
        return box;
    };

    const QString startIconName = rtl ? button.trailingIcon() : button.leadingIcon();
    layout.startIconName = startIconName;
    layout.hasLeadingIcon = !startIconName.isEmpty();
    if (layout.hasLeadingIcon) {
        layout.leadingIcon = place(buttonTokens.iconSize);
        if (hasLabel) {
            cursor += dir * buttonTokens.iconLabelSpace;
        }
    }
    layout.hasLabel = hasLabel;
    layout.label = hasLabel ? place(labelWidth) : QRectF();

    // The dropdown icon sits centred in the trailing half's content box.
    layout.endIconName = button.trailingIcon();
    layout.trailingIcon = QRectF(
        rtl ? layout.trailing.left() + metrics.trailingTrailingSpace
            : layout.trailing.left() + metrics.trailingLeadingSpace,
        layout.trailing.top(), metrics.trailingIconSize, leadingHeight);

    return layout;
}

void MdSplitButtonStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *button = qobject_cast<MdSplitButton *>(widget);
    if (painter == nullptr || button == nullptr) {
        return;
    }
    const MdSplitButtonTokens &tokens = button->tokens();
    paintSplitButton(*painter, *button, tokens, layoutFor(*button, tokens));
}

void MdSplitButtonStyle::paintSplitButton(QPainter &painter,
                                          const MdSplitButton &button,
                                          const MdSplitButtonTokens &tokens,
                                          const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const MdSplitButtonMetrics &metrics = tokens.metrics;
    const MdButtonTokens &buttonTokens = tokens.button;

    struct Half
    {
        QRectF rect;
        QList<qreal> radii;
        MdButtonState state;
        MdSplitButton::Zone zone;
    };
    const Half halves[2] = {
        {layout.leading, layout.leadingRadii, stateFor(button, MdSplitButton::Zone::Leading),
         MdSplitButton::Zone::Leading},
        {layout.trailing, layout.trailingRadii, stateFor(button, MdSplitButton::Zone::Trailing),
         MdSplitButton::Zone::Trailing},
    };

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. Shadow (elevated only), then the two container fills. Both shadows
    //    first, so neither fill is painted over by the other half's shadow.
    const bool paintsShadow = buttonTokens.enabled.elevation != ElevationLevel::Level0;
    if (paintsShadow) {
        for (const Half &half : halves) {
            const ElevationLevel level = buttonTokens.state(half.state).elevation;
            if (level != ElevationLevel::Level0) {
                MdElevation::drawShadow(&painter, half.rect, metrics.outerCornerRadius(), level,
                                        theme.color(ColorRole::Shadow));
            }
        }
    }
    for (const Half &half : halves) {
        const MdButtonStateColours &colours = buttonTokens.state(half.state);
        if (!colours.paintsContainer()) {
            continue;
        }
        const QColor fill = resolveSlot(colours.container);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(MdShape::roundedRect(half.rect, half.radii));
        }
    }

    // 2. Per-half flat state layer — hover and keyboard focus only. Press is
    //    not a flat layer (interaction-fidelity rule #3): the ripple below is
    //    the whole press response, exactly as in MdButtonStyle.
    for (const Half &half : halves) {
        if (button.isEffectivelyDisabled()) {
            break;
        }
        const bool halfHovered = button.hoveredZone() == half.zone;
        const bool halfKeyboardFocused =
            button.hasKeyboardFocus() && button.focusedZone() == half.zone;
        StateLayerKind kind = StateLayerKind::Hover;
        if (!MdStateLayer::strongestActive(&kind, halfHovered, halfKeyboardFocused, false, false)) {
            continue;
        }
        const ColorRole layerRole = buttonTokens.state(half.state).stateLayer;
        if (layerRole == ColorRole::Count) {
            continue;
        }
        const QColor overlay = MdStateLayer::overlay(theme.color(layerRole), kind);
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(MdShape::roundedRect(half.rect, half.radii));
        }
    }

    // 3. Per-half ripple, clipped to that half's shape and coloured by the
    //    pressed row's state-layer colour (interaction-fidelity rules #1-#2).
    for (const Half &half : halves) {
        MdRippleController *ripple = button.rippleController(half.zone);
        if (ripple == nullptr) {
            continue;
        }
        const QRectF localRect(QPointF(0.0, 0.0), half.rect.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, half.radii);
        ripple->setBounds(half.rect.size());
        ripple->setClipPath(localPath);
        const ColorRole pressLayerRole = buttonTokens.state(MdButtonState::Pressed).stateLayer;
        ripple->setContentColor(theme.color(pressLayerRole != ColorRole::Count
                                                ? pressLayerRole
                                                : ColorRole::OnSurface));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(half.rect.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 4. Outline (the borrowed outlined variant), on top of the state layer.
    for (const Half &half : halves) {
        const MdButtonStateColours &colours = buttonTokens.state(half.state);
        if (!colours.paintsOutline() || buttonTokens.outlineWidth <= 0.0) {
            continue;
        }
        const QColor stroke = theme.color(colours.outline);
        if (stroke.isValid()) {
            QPen pen(stroke);
            pen.setWidthF(buttonTokens.outlineWidth);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(MdShape::roundedRect(half.rect, half.radii));
        }
    }

    // 5. Leading content: icon and label.
    const MdButtonStateColours &leadingColours = buttonTokens.state(halves[0].state);
    const QColor iconColour = resolveSlot(leadingColours.icon);
    const QColor textColour = resolveSlot(leadingColours.labelText);

    if (layout.hasLeadingIcon && !layout.startIconName.isEmpty() && iconColour.isValid()) {
        MdIcon::paint(&painter, layout.leadingIcon, layout.startIconName, iconColour);
    }
    if (layout.hasLabel && textColour.isValid()) {
        const QFont font = MdTypeScale::font(buttonTokens.labelStyle, TypeEmphasis::Baseline,
                                             theme.scriptCategory());
        const QFontMetricsF fontMetrics(font);
        const qreal baseline =
            layout.label.top() + (layout.label.height() + fontMetrics.ascent()
                                  - fontMetrics.descent()) / 2.0;
        painter.setFont(font);
        painter.setPen(textColour);
        const qreal x = theme.isRightToLeft()
                            ? layout.label.right() - fontMetrics.horizontalAdvance(button.text())
                            : layout.label.left();
        painter.drawText(QPointF(x, baseline), button.text());
    }

    // 6. The trailing dropdown icon, coloured by the trailing half's state.
    if (!layout.endIconName.isEmpty()) {
        const QColor trailingIconColour =
            resolveSlot(buttonTokens.state(halves[1].state).icon);
        if (trailingIconColour.isValid()) {
            MdIcon::paint(&painter, layout.trailingIcon, layout.endIconName, trailingIconColour);
        }
    }

    // 7. Focus indicator around the *whole* split, keyboard focus only — one
    //    control, one ring, whatever half holds the keyboard caret.
    if (!button.isEffectivelyDisabled() && button.hasKeyboardFocus()) {
        MdFocusRingController *ring = button.focusRingController();
        const MdFocusRingSpec spec = focusRingSpec(buttonTokens);
        // The ring hugs the outer pill: corner-full on all four corners.
        const QList<qreal> containerRadii(4, metrics.outerCornerRadius());
        MdFocusRing::paint(&painter, layout.container, containerRadii,
                           theme.color(buttonTokens.focusIndicator), spec,
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

} // namespace md
