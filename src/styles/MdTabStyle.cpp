#include "MdTabStyle.h"

#include "core/MdColorMath.h"
#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTab.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>

namespace md {

MdTabStyle::MdTabStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTabStyle *MdTabStyle::shared()
{
    static QMutex mutex;
    static MdTabStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTabStyle;
        installPaintFilter<MdTab>(instance);
    }
    return instance;
}

bool MdTabStyle::isInstalled()
{
    return hasPaintFilter(&MdTab::staticMetaObject);
}

MdFocusRingSpec MdTabStyle::focusRingSpec(const MdTabsVariantTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    // The export's `focus-indicator-outline-offset` is the **inner** offset, so
    // the ring draws inside the tab and the tab needs no outward margin.
    spec.outwardOffset = 2.0;
    spec.inwardOffset = tokens.focusIndicatorOffset;
    spec.inward = true;
    spec.shape = ShapeCorner::None;
    return spec;
}

MdNavigationItemState MdTabStyle::stateFor(const MdTab &tab)
{
    if (tab.isEffectivelyDisabled()) {
        return MdNavigationItemState::Disabled;
    }
    if (tab.isDown()) {
        return MdNavigationItemState::Pressed;
    }
    if (tab.hasKeyboardFocus()) {
        return MdNavigationItemState::Focused;
    }
    if (tab.isHovered()) {
        return MdNavigationItemState::Hovered;
    }
    return MdNavigationItemState::Enabled;
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

qreal stateLayerOpacityFor(const MdTabsVariantTokens &tokens, MdNavigationItemState state)
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

/// One side of the colour table faded by the tab's progress. Compose's
/// `TabTransition` interpolates `LocalContentColor` between the two sides in
/// `Color.VectorConverter`'s space — Oklab.
QColor fadedColour(const MdNavigationColourSlot &inactive,
                   const MdNavigationColourSlot &active,
                   qreal progress)
{
    const QColor a = resolveSlot(inactive);
    const QColor b = resolveSlot(active);
    if (!a.isValid() && !b.isValid()) {
        return QColor();
    }
    if (!a.isValid() || !b.isValid()) {
        return a.isValid() ? a : b;
    }
    const qreal clamped = qBound(0.0, progress, 1.0);
    QColor out = QColor::fromRgba(MdColorMath::lerpOklab(a.rgba(), b.rgba(), clamped));
    out.setAlphaF(qBound(0.0, a.alphaF() + (b.alphaF() - a.alphaF()) * clamped, 1.0));
    return out;
}

} // namespace

void MdTabStyle::paintTab(QPainter &painter, const MdTab &tab, const MdTabsVariantTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTab::Boxes boxes = tab.boxes();
    const MdNavigationItemState state = stateFor(tab);
    const qreal progress = qBound(0.0, tab.colourProgress(), 1.0);
    const QRectF tabRect(QPointF(0.0, 0.0), QSizeF(qreal(tab.width()), qreal(tab.height())));

    const bool interactive = !tab.isEffectivelyDisabled();

    // 1. The state layer — hover and keyboard focus only, spanning the whole
    //    tab. The press rides on the ripple, as in the button families.
    if (interactive
        && (state == MdNavigationItemState::Hovered || state == MdNavigationItemState::Focused)) {
        const StateLayerKind kind =
            state == MdNavigationItemState::Focused ? StateLayerKind::Focus : StateLayerKind::Hover;
        const MdNavigationColourSlot &slot = tab.isSelected()
                                                 ? tokens.activeStateLayerFor(state)
                                                 : tokens.inactiveStateLayerFor(state);
        const QColor overlay = stateLayerOverlay(slot, kind, stateLayerOpacityFor(tokens, state));
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawRect(tabRect);
        }
    }

    // 2. The ripple, bounded to the tab and coloured with the *active* side's
    //    pressed colour — the colour the tab is about to earn.
    if (MdRippleController *ripple = tab.rippleController()) {
        ripple->setBounds(tabRect.size());
        const MdNavigationColourSlot &pressSlot =
            tab.isSelected() ? tokens.activeStateLayerFor(MdNavigationItemState::Pressed)
                             : tokens.inactiveStateLayerFor(MdNavigationItemState::Pressed);
        const QColor pressColour = pressSlot.isPresent()
                                       ? resolveSlot(pressSlot)
                                       : MdTheme::instance().color(tokens.activeIndicatorColor);
        ripple->setContentColor(pressColour);
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            MdRipple::paint(&painter, frame, ripple->clipPath(), ripple->contentColor());
        }
    }

    // 3. The icon, faded between the two tables by the selection progress.
    if (boxes.hasIcon && boxes.icon.isValid()) {
        const QColor iconColour =
            fadedColour(tokens.inactiveIconFor(state), tokens.activeIconFor(state), progress);
        if (iconColour.isValid()) {
            MdIcon::paint(&painter, boxes.icon, tab.iconName(), iconColour, tab.iconSet(),
                          tab.iconFamily());
        }
    }

    // 4. The label, faded the same way.
    if (boxes.hasLabel && boxes.label.isValid()) {
        const QColor labelColour =
            fadedColour(tokens.inactiveLabelFor(state), tokens.activeLabelFor(state), progress);
        if (labelColour.isValid()) {
            painter.setPen(labelColour);
            painter.setBrush(Qt::NoBrush);
            painter.setFont(tab.labelFont());
            painter.drawText(boxes.label, int(Qt::AlignCenter), tab.label());
        }
    }

    // 5. The focus indicator, last so nothing paints over it. Keyboard focus
    //    only, and inward at the tab's bounds.
    if (interactive && tab.hasKeyboardFocus()) {
        MdFocusRingController *ring = tab.focusRingController();
        MdFocusRing::paint(&painter, tabRect, {}, MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdTabStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *tab = qobject_cast<MdTab *>(widget);
    if (painter == nullptr || tab == nullptr) {
        return;
    }
    paintTab(*painter, *tab, tab->variantTokens());
}

} // namespace md
