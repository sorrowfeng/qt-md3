#include "MdCardStyle.h"

#include "core/MdElevation.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdCard.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// Same derivation as the button families': the stroke centre line sits
/// `offset + activeWidth / 2 + width / 2` outside the component.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

/// Compose composites a disabled colour (token role at
/// `DisabledContainerOpacity`) *over the enabled colour*, so the disabled
/// card keeps a ghost of its own tone rather than floating on an assumed
/// surface. `alpha` carries the opacity to fold in.
QColor compositedOver(const ColorRole source, qreal alpha, const QColor &base,
                      const MdTheme &theme)
{
    QColor colour = theme.color(source);
    colour.setAlphaF(colour.alphaF() * alpha);
    if (!base.isValid()) {
        return colour;
    }
    // src * a + dst * (1 - a), straight alpha.
    const qreal a = colour.alphaF();
    const int r = int(qRound(colour.redF() * a + base.redF() * (1.0 - a)));
    const int g = int(qRound(colour.greenF() * a + base.greenF() * (1.0 - a)));
    const int b = int(qRound(colour.blueF() * a + base.blueF() * (1.0 - a)));
    return QColor(r, g, b);
}

} // namespace

MdCardStyle::MdCardStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdCardStyle *MdCardStyle::shared()
{
    static QMutex mutex;
    static MdCardStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdCardStyle;
        installPaintFilter<MdCard>(instance);
    }
    return instance;
}

bool MdCardStyle::isInstalled()
{
    return hasPaintFilter(&MdCard::staticMetaObject);
}

MdFocusRingSpec MdCardStyle::focusRingSpec(const MdCardTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOffset;
    spec.inward = false;
    // `active-width`, `duration` and the easing belong to the
    // md.comp.focus-ring module, whose defaults already carry them.
    spec.color = QColor();
    return spec;
}

qreal MdCardStyle::focusRingInset(const MdCardTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdCardState MdCardStyle::stateFor(const MdCard &card)
{
    // A non-clickable card never paints an interactive state: Compose's
    // non-clickable overload has no interaction source at all, so the
    // enabled row is the whole story (even when the widget is disabled —
    // Compose only applies the disabled colours to the clickable overload).
    if (!card.isClickable()) {
        return MdCardState::Enabled;
    }
    if (!card.isEnabled()) {
        return MdCardState::Disabled;
    }
    if (card.isDragged()) {
        return MdCardState::Dragged;
    }
    if (card.isPressed()) {
        return MdCardState::Pressed;
    }
    if (card.isHovered()) {
        return MdCardState::Hovered;
    }
    if (card.hasKeyboardFocus()) {
        return MdCardState::Focused;
    }
    return MdCardState::Enabled;
}

MdCardStyle::Layout MdCardStyle::layoutFor(const MdCard &card, const MdCardTokens &tokens)
{
    Layout layout;

    // A clickable card reserves the focus-indicator margin; a non-clickable
    // one is the plain surface, edge to edge. The widget keeps its contents
    // margins in sync, so a layout installed on the card fills the container
    // exactly.
    const qreal inset = card.isClickable() ? focusRingInset(tokens) : 0.0;
    layout.container = QRectF(card.rect()).adjusted(inset, inset, -inset, -inset);
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, layout.container.size());

    return layout;
}

void MdCardStyle::paintCard(QPainter &painter, const MdCard &card, const MdCardTokens &tokens,
                            const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const MdCardState state = stateFor(card);
    const MdCardStateRow &row = tokens.family.state(state);
    const bool disabled = state == MdCardState::Disabled;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow. The elevation ladder is the family's visual signature; the
    //    widget animates between the rows' dp values (Compose animates the
    //    elevation through the theme's motion scheme), so the paint reads
    //    the animated value off the card, not the raw row.
    const qreal elevationDp = card.currentShadowDp();
    if (elevationDp > 0.0) {
        const qreal radius = layout.radii.isEmpty() ? 0.0 : layout.radii.first();
        MdElevation::drawShadowDp(&painter, layout.container, radius, elevationDp,
                                  theme.color(ColorRole::Shadow));
    }

    // 2. Container. Disabled composites the export's disabled container at
    //    0.38 over the enabled colour; the content colour fades to 0.38 the
    //    same way (both carried by the card, which exposes the content
    //    colour to its children through the palette).
    QColor container = theme.color(row.container);
    if (disabled) {
        container = compositedOver(tokens.family.disabled.container,
                                   kDisabledContainerOpacity,
                                   theme.color(tokens.family.enabled.container), theme);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(container);
    painter.drawPath(path);

    // 3. Outlined variant: the 1 px stroke, state-tinted. Disabled fades to
    //    0.12 composited over the card's own container colour (Compose
    //    composites over the *elevated* card's container — see the recorded
    //    divergence in docs/porting-todo.md).
    if (row.outline != ColorRole::Count) {
        QColor outline = theme.color(row.outline);
        if (disabled) {
            outline = compositedOver(row.outline, kDisabledOutlineOpacity,
                                     theme.color(tokens.family.enabled.container), theme);
        }
        QPen pen(outline, tokens.outlineWidth);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(path);
    }

    // 4. State layer, one composited overlay. Hover and keyboard focus only
    //    — press rides on the ripple; dragged paints its own flat layer at
    //    the dragged opacity.
    if (!disabled) {
        StateLayerKind kind = StateLayerKind::Hover;
        if (row.stateLayer != ColorRole::Count
            && MdStateLayer::strongestActive(&kind, card.isHovered(), card.hasKeyboardFocus(),
                                             false, card.isDragged())) {
            const QColor overlay = MdStateLayer::overlay(theme.color(row.stateLayer), kind);
            if (overlay.isValid()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(overlay);
                painter.drawPath(path);
            }
        }
    }

    // 5. Ripple, clipped to the container shape. The ripple is the pressed
    //    state layer, so it takes the pressed row's published colour.
    if (MdRippleController *ripple = card.rippleController()) {
        const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, layout.radii);
        ripple->setBounds(layout.container.size());
        ripple->setClipPath(localPath);
        ripple->setContentColor(theme.color(tokens.family.pressed.stateLayer != ColorRole::Count
                                                ? tokens.family.pressed.stateLayer
                                                : ColorRole::OnSurface));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(layout.container.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 6. Focus indicator, last so nothing paints over it. Keyboard focus
    //    only (:focus-visible semantics).
    if (!disabled && card.isClickable() && card.hasKeyboardFocus()) {
        MdFocusRingController *ring = card.focusRingController();
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(tokens.focusIndicator), focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdCardStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *card = qobject_cast<MdCard *>(widget);
    if (painter == nullptr || card == nullptr) {
        return;
    }
    const MdCardTokens &tokens = card->tokens();
    paintCard(*painter, *card, tokens, layoutFor(*card, tokens));
}

} // namespace md
