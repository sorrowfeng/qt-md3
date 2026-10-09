#include "MdCheckBoxStyle.h"

#include "core/MdColorMath.h"
#include "core/MdFocusRing.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdCheckBox.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <cmath>

namespace md {

MdCheckBoxStyle::MdCheckBoxStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdCheckBoxStyle *MdCheckBoxStyle::shared()
{
    static QMutex mutex;
    static MdCheckBoxStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdCheckBoxStyle;
        installPaintFilter<MdCheckBox>(instance);
    }
    return instance;
}

bool MdCheckBoxStyle::isInstalled()
{
    return hasPaintFilter(&MdCheckBox::staticMetaObject);
}

MdFocusRingSpec MdCheckBoxStyle::focusRingSpec(const MdCheckBoxTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    // The export publishes no `focus-indicator.shape` row, so the ring follows
    // the box's own radii (material-web's rule) — component radii are passed at
    // paint time. This shape is only the fallback.
    spec.shape = ShapeCorner::Full;
    return spec;
}

MdNavigationItemState MdCheckBoxStyle::stateFor(const MdCheckBox &box)
{
    if (box.isEffectivelyDisabled()) {
        return MdNavigationItemState::Disabled;
    }
    if (box.isDown()) {
        return MdNavigationItemState::Pressed;
    }
    if (box.hasKeyboardFocus()) {
        return MdNavigationItemState::Focused;
    }
    if (box.isHovered()) {
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

qreal stateLayerOpacityFor(const MdCheckBoxTokens &tokens, MdNavigationItemState state)
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

/// The two sides of a colour table faded by the checkbox's progress. Where one
/// side is absent (an unchecked box's transparent fill, a checked box's absent
/// border) the fade runs against transparent — Compose's
/// `animateColorAsState` crosses `Color.Transparent` the same way. Both halves
/// are resolved for the *current* interaction state, so a hover on a checked
/// box reads the checked-hover row on both sides.
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

/// The `drawBox` port. Compose's two branches:
///
///   * fill == border → one filled round rect;
///   * otherwise      → the fill insets by the stroke width (its radius drops
///     by the same), and the border strokes around the inset-by-half rect.
void drawBox(QPainter &painter, const QRectF &boxRect, qreal radius, qreal strokeWidth,
             const QColor &boxColour, const QColor &borderColour)
{
    const bool hasFill = boxColour.isValid() && boxColour.alpha() > 0;
    const bool hasBorder = borderColour.isValid() && borderColour.alpha() > 0 && strokeWidth > 0.0;

    if (!hasFill && !hasBorder) {
        return;
    }

    const bool sameColour = hasFill && hasBorder && boxColour.rgba() == borderColour.rgba();
    if (sameColour || (hasFill && !hasBorder)) {
        // One filled round rect. (Compose takes this branch when the colours
        // match; a selected box has no separate border row to paint.)
        painter.setPen(Qt::NoPen);
        painter.setBrush(boxColour);
        painter.drawRoundedRect(boxRect, radius, radius);
        return;
    }

    if (hasFill) {
        // The fill insets by the stroke width on every side; Compose drops the
        // radius by the same amount and floors it at zero.
        QRectF fillRect = boxRect.adjusted(strokeWidth, strokeWidth, -strokeWidth, -strokeWidth);
        painter.setPen(Qt::NoPen);
        painter.setBrush(boxColour);
        painter.drawRoundedRect(fillRect, qMax(0.0, radius - strokeWidth),
                                qMax(0.0, radius - strokeWidth));
    }
    if (hasBorder) {
        // The border strokes centred on the rect inset by half the stroke.
        const qreal half = strokeWidth / 2.0;
        QRectF borderRect = boxRect.adjusted(half, half, -half, -half);
        QPen pen(borderColour, strokeWidth);
        pen.setCapStyle(Qt::FlatCap);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(borderRect, qMax(0.0, radius - half), qMax(0.0, radius - half));
    }
}

/// The `drawCheck` port. The path is two line segments through the fractions
/// of the box size; `gravitation` lerps the three points towards the centre
/// line (the indeterminate dash), and `fraction` reveals the path along its
/// length (Compose's `pathMeasure.getSegment(0, length * fraction)`).
void drawCheck(QPainter &painter, const QRectF &boxRect, const MdCheckBoxTokens &tokens,
               const QColor &checkColour, qreal fraction, qreal gravitation)
{
    if (fraction <= 0.0 || !checkColour.isValid() || checkColour.alpha() == 0) {
        return;
    }

    const qreal w = boxRect.width();
    const QPointF left(boxRect.left() + tokens.checkLeftX * w,
                       boxRect.top() + (tokens.checkLeftY + (0.5 - tokens.checkLeftY) * gravitation) * w);
    const QPointF cross(boxRect.left()
                            + (tokens.checkCrossX + (0.5 - tokens.checkCrossX) * gravitation) * w,
                        boxRect.top()
                            + (tokens.checkCrossY + (0.5 - tokens.checkCrossY) * gravitation) * w);
    const QPointF right(boxRect.left() + tokens.checkRightX * w,
                        boxRect.top()
                            + (tokens.checkRightY + (0.5 - tokens.checkRightY) * gravitation) * w);

    const qreal firstLeg = QLineF(left, cross).length();
    const qreal secondLeg = QLineF(cross, right).length();
    const qreal total = firstLeg + secondLeg;
    const qreal drawn = total * qBound(0.0, fraction, 1.0);

    QPainterPath path;
    path.moveTo(left);
    if (drawn <= firstLeg) {
        const QLineF leg(left, cross);
        QLineF partial = leg;
        partial.setLength(drawn);
        path.lineTo(partial.p2());
    } else {
        path.lineTo(cross);
        const QLineF leg(cross, right);
        QLineF partial = leg;
        partial.setLength(qMin(drawn - firstLeg, secondLeg));
        path.lineTo(partial.p2());
    }

    QPen pen(checkColour, tokens.strokeWidth);
    // Compose's check stroke: `StrokeCap.Square`, miter join.
    pen.setCapStyle(Qt::SquareCap);
    pen.setJoinStyle(Qt::MiterJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

} // namespace

void MdCheckBoxStyle::paintCheckBox(QPainter &painter,
                                    const MdCheckBox &box,
                                    const MdCheckBoxTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdNavigationItemState state = stateFor(box);
    const bool interactive = !box.isEffectivelyDisabled();
    const bool error = box.hasError();
    // An indeterminate box resolves the selected side.
    const MdCheckBoxSelection selection =
        box.checkState() == Qt::Unchecked ? MdCheckBoxSelection::Unselected
                                          : MdCheckBoxSelection::Selected;
    const qreal progress = qBound(0.0, box.colourProgress(), 1.0);

    const QRectF boxRect = box.boxRect();
    const QRectF layerRect = box.stateLayerRect();

    // 1. The circular state layer — hover and keyboard focus only. The press
    //    rides on the ripple.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const MdNavigationColourSlot &slot =
            tokens.stateLayerFor(error, selection, state);
        const QColor overlay = stateLayerOverlay(slot, kind, stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawEllipse(layerRect);
        }
    }

    // 2. The ripple — an unbounded circle of the state layer's diameter (so
    //    the bounds are the 40 px layer and the clip its circle), coloured
    //    with the pressed row.
    if (MdRippleController *ripple = box.rippleController()) {
        ripple->setBounds(layerRect.size());
        const MdNavigationColourSlot &pressSlot =
            tokens.stateLayerFor(error, selection, MdNavigationItemState::Pressed);
        ripple->setContentColor(resolveSlot(pressSlot));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            QPainterPath clip;
            clip.addEllipse(layerRect);
            MdRipple::paint(&painter, frame, clip, ripple->contentColor());
        }
    }

    // 3. The box and its check, faded between the two selection tables. Both
    //    sides resolve at the current interaction state, then the Oklab fade
    //    runs between them.
    const QColor boxColour = fadedColour(resolveSlot(tokens.boxFor(error, MdCheckBoxSelection::Unselected, state)),
                                         resolveSlot(tokens.boxFor(error, MdCheckBoxSelection::Selected, state)),
                                         progress);
    const QColor borderColour =
        fadedColour(resolveSlot(tokens.outlineFor(error, MdCheckBoxSelection::Unselected, state)),
                    resolveSlot(tokens.outlineFor(error, MdCheckBoxSelection::Selected, state)),
                    progress);
    const QColor checkColour =
        fadedColour(resolveSlot(tokens.checkmarkFor(error, MdCheckBoxSelection::Unselected, state)),
                    resolveSlot(tokens.checkmarkFor(error, MdCheckBoxSelection::Selected, state)),
                    progress);

    drawBox(painter, boxRect, tokens.containerShapeRadius, tokens.strokeWidth, boxColour,
            borderColour);
    drawCheck(painter, boxRect, tokens, checkColour, box.checkFraction(), box.crossGravitation());

    // 4. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, outward around the box, following its corner radii.
    if (interactive && box.hasKeyboardFocus()) {
        MdFocusRingController *ring = box.focusRingController();
        const qreal r = tokens.containerShapeRadius;
        MdFocusRing::paint(&painter, boxRect, {r, r, r, r},
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdCheckBoxStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *box = qobject_cast<MdCheckBox *>(widget);
    if (painter == nullptr || box == nullptr) {
        return;
    }
    paintCheckBox(*painter, *box, box->checkBoxTokens());
}

} // namespace md
