#include "MdFabStyle.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "widgets/MdFab.h"

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

/// A colour role + opacity multiplier, resolved against the theme.
QColor resolveSlot(const MdFabColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (qFuzzyCompare(slot.opacity, 1.0)) {
        return colour;
    }
    // Fold the multiplier into alpha so the colour keeps compositing against
    // whatever the FAB sits on — the disabled opacities are alpha facts, not
    // pre-blends against an assumed surface.
    colour.setAlphaF(colour.alphaF() * slot.opacity);
    return colour;
}

} // namespace

MdFabStyle::MdFabStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdFabStyle *MdFabStyle::shared()
{
    static QMutex mutex;
    static MdFabStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdFabStyle;
        installPaintFilter<MdFab>(instance);
    }
    return instance;
}

bool MdFabStyle::isInstalled()
{
    return hasPaintFilter(&MdFab::staticMetaObject);
}

MdFocusRingSpec MdFabStyle::focusRingSpec(const MdFabTokens &tokens)
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

qreal MdFabStyle::focusRingInset(const MdFabTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdFabState MdFabStyle::stateFor(const MdFab &fab)
{
    if (fab.isEffectivelyDisabled()) {
        return MdFabState::Disabled;
    }
    if (fab.isDown()) {
        return MdFabState::Pressed;
    }
    if (fab.isHovered()) {
        return MdFabState::Hovered;
    }
    if (fab.hasKeyboardFocus()) {
        return MdFabState::Focused;
    }
    return MdFabState::Enabled;
}

MdFabStyle::Layout MdFabStyle::layoutFor(const MdFab &fab, const MdFabTokens &tokens)
{
    Layout layout;

    const qreal inset = focusRingInset(tokens);
    const QRectF widgetRect(fab.rect());
    const QRectF inner = widgetRect.adjusted(inset, inset, -inset, -inset);

    // md.comp.fab arithmetic: the container is the published square — height
    // and width are separate tokens and both are honoured.
    const QSizeF containerSize(tokens.containerWidth, tokens.containerHeight);

    // The container is centred in the inner rect: horizontally a squeezed
    // widget keeps a centred square rather than a squashed one, vertically the
    // token height wins over whatever the widget ended up being.
    const QPointF origin(
        inner.left() + qMax<qreal>((inner.width() - containerSize.width()) / 2.0, 0.0),
        inner.top() + qMax<qreal>((inner.height() - containerSize.height()) / 2.0, 0.0));
    layout.container = QRectF(origin, containerSize);
    layout.preferredSize = QSizeF(containerSize.width() + 2.0 * inset,
                                  containerSize.height() + 2.0 * inset);

    // Corner radii, static: this family publishes no pressed shape, so unlike
    // the button/icon-button layouts there is no morph here.
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, containerSize);

    layout.icon = QRectF(layout.container.center().x() - tokens.iconSize / 2.0,
                         layout.container.center().y() - tokens.iconSize / 2.0,
                         tokens.iconSize, tokens.iconSize);

    return layout;
}

void MdFabStyle::paintFab(QPainter &painter, const MdFab &fab, const MdFabTokens &tokens,
                          const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const MdFabState state = stateFor(fab);
    const MdFabStateColours &colours = tokens.family.state(state);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow, then the container. A FAB is *the* shadowed component of the
    //    system: every colour set publishes a container.elevation (level3
    //    resting, level4 hovered; level1/2 lowered), so the shadow follows the
    //    state's elevation row. A disabled FAB sits at level0 and casts
    //    nothing.
    const ElevationLevel elevation = tokens.family.elevation(state);
    if (elevation != ElevationLevel::Level0) {
        const qreal radius = layout.radii.isEmpty() ? 0.0 : layout.radii.first();
        MdElevation::drawShadow(&painter, layout.container, radius, elevation,
                                theme.color(ColorRole::Shadow));
    }

    if (colours.container.isPresent()) {
        const QColor fill = resolveSlot(colours.container);
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(path);
        }
    }

    // 2. State layer, one composited overlay. Hover and keyboard focus only —
    //    press rides on the ripple, as in MdButtonStyle (see the longer note
    //    there).
    const bool interactive = !fab.isEffectivelyDisabled();
    StateLayerKind kind = StateLayerKind::Hover;
    if (interactive && colours.stateLayer != ColorRole::Count
        && MdStateLayer::strongestActive(&kind, fab.isHovered(), fab.hasKeyboardFocus(), false,
                                         false)) {
        const QColor overlay = MdStateLayer::overlay(theme.color(colours.stateLayer), kind);
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(path);
        }
    }

    // 3. Ripple, clipped to the container shape.
    if (MdRippleController *ripple = fab.rippleController()) {
        const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, layout.radii);
        ripple->setBounds(layout.container.size());
        ripple->setClipPath(localPath);
        // The ripple is the pressed state layer, so it takes the pressed row's
        // published colour — not a global on-surface.
        const ColorRole pressLayerRole = tokens.family.state(MdFabState::Pressed).stateLayer;
        ripple->setContentColor(theme.color(pressLayerRole != ColorRole::Count
                                                ? pressLayerRole
                                                : ColorRole::OnSurface));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(layout.container.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 4. The icon — the whole content of this component.
    const QColor iconColour = resolveSlot(colours.icon);
    if (iconColour.isValid()) {
        MdIcon::paint(&painter, layout.icon, fab.iconName(), iconColour, fab.iconSet(),
                      fab.iconFamily());
    }

    // 5. Focus indicator, last so nothing paints over it. Keyboard focus only.
    if (interactive && fab.hasKeyboardFocus()) {
        MdFocusRingController *ring = fab.focusRingController();
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(tokens.focusIndicator), focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdFabStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *fab = qobject_cast<MdFab *>(widget);
    if (painter == nullptr || fab == nullptr) {
        return;
    }
    const MdFabTokens &tokens = fab->tokens();
    paintFab(*painter, *fab, tokens, layoutFor(*fab, tokens));
}

} // namespace md
