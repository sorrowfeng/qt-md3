#include "MdFabMenuStyle.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdFabMenuItem.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// Same derivation as the other families': the stroke centre line sits
/// `offset + activeWidth / 2 + width / 2` outside the component.
qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

} // namespace

MdFabMenuStyle::MdFabMenuStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdFabMenuStyle *MdFabMenuStyle::shared()
{
    static QMutex mutex;
    static MdFabMenuStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdFabMenuStyle;
        installPaintFilter<MdFabMenuItem>(instance);
    }
    return instance;
}

bool MdFabMenuStyle::isInstalled()
{
    return hasPaintFilter(&MdFabMenuItem::staticMetaObject);
}

MdFocusRingSpec MdFabMenuStyle::focusRingSpec(const MdFabMenuTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOffset;
    spec.inward = false;
    spec.color = QColor();
    return spec;
}

qreal MdFabMenuStyle::focusRingInset(const MdFabMenuTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdFabState MdFabMenuStyle::stateFor(const MdFabMenuItem &item)
{
    if (item.isEffectivelyDisabled()) {
        return MdFabState::Disabled;
    }
    if (item.isDown()) {
        return MdFabState::Pressed;
    }
    if (item.isHovered()) {
        return MdFabState::Hovered;
    }
    if (item.hasKeyboardFocus()) {
        return MdFabState::Focused;
    }
    return MdFabState::Enabled;
}

MdFabMenuStyle::Layout MdFabMenuStyle::layoutFor(const MdFabMenuItem &item,
                                                 const MdFabMenuElementTokens &element)
{
    Layout layout;

    const MdFabMenuTokens &tokens = item.menuTokens();
    const qreal inset = ringInsetFor(focusRingSpec(tokens));
    const QRectF widgetRect(item.rect());
    const QRectF inner = widgetRect.adjusted(inset, inset, -inset, -inset);

    // The label font comes from the element's type scale row.
    layout.labelFont = MdTypeScale::font(element.labelStyle);
    const QFontMetricsF metrics(layout.labelFont);
    const QString label = item.text();
    const qreal labelWidth = label.isEmpty() ? 0.0 : metrics.horizontalAdvance(label);

    // md.comp.fab-menu arithmetic: the close button is a published 56 px
    // square; the list item width is derived — leading + icon + gap + label
    // + trailing.
    qreal containerWidth = element.containerWidth;
    if (containerWidth <= 0.0) {
        qreal contentWidth = 0.0;
        const bool hasIcon = !item.iconName().isEmpty();
        if (hasIcon) {
            contentWidth += element.iconSize;
        }
        if (!label.isEmpty()) {
            if (hasIcon) {
                contentWidth += element.iconLabelSpace;
            }
            contentWidth += labelWidth;
        }
        containerWidth = element.leadingSpace + contentWidth + element.trailingSpace;
    }
    const QSizeF containerSize(containerWidth, element.containerHeight);

    const QPointF origin(
        inner.left() + qMax<qreal>((inner.width() - containerSize.width()) / 2.0, 0.0),
        inner.top() + qMax<qreal>((inner.height() - containerSize.height()) / 2.0, 0.0));
    layout.container = QRectF(origin, containerSize);
    layout.preferredSize = QSizeF(containerSize.width() + 2.0 * inset,
                                  containerSize.height() + 2.0 * inset);

    layout.radii = MdShape::resolvedRadii(element.containerShape, containerSize);

    const qreal centerY = layout.container.center().y();
    qreal cursor = layout.container.left() + element.leadingSpace;
    const bool hasIcon = !item.iconName().isEmpty();
    if (hasIcon) {
        layout.icon = QRectF(cursor, centerY - element.iconSize / 2.0, element.iconSize,
                             element.iconSize);
        cursor += element.iconSize;
    }
    if (!label.isEmpty()) {
        if (hasIcon) {
            cursor += element.iconLabelSpace;
        }
        layout.label = QRectF(cursor, centerY - element.containerHeight / 2.0, labelWidth,
                              element.containerHeight);
    }

    return layout;
}

void MdFabMenuStyle::paintItem(QPainter &painter, const MdFabMenuItem &item,
                               const MdFabMenuElementTokens &element, const Layout &layout,
                               qreal reveal, qreal revealOffsetY)
{
    const MdTheme &theme = MdTheme::instance();
    const MdFabState state = stateFor(item);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setOpacity(painter.opacity() * reveal);
    if (revealOffsetY != 0.0) {
        painter.translate(0.0, revealOffsetY);
    }

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow, then the container. The close button carries the raised-FAB
    //    ladder (level3 resting, level4 hovered); list items stay at level0
    //    and cast nothing.
    ElevationLevel elevation = element.enabledElevation;
    switch (state) {
    case MdFabState::Enabled: elevation = element.enabledElevation; break;
    case MdFabState::Hovered: elevation = element.hoveredElevation; break;
    case MdFabState::Focused: elevation = element.focusedElevation; break;
    case MdFabState::Pressed: elevation = element.pressedElevation; break;
    case MdFabState::Disabled: elevation = element.disabledElevation; break;
    case MdFabState::Count: break;
    }
    if (elevation != ElevationLevel::Level0) {
        const qreal radius = layout.radii.isEmpty() ? 0.0 : layout.radii.first();
        MdElevation::drawShadow(&painter, layout.container, radius, elevation,
                                theme.color(ColorRole::Shadow));
    }

    // 2. The container. The disabled row folds the spec-table opacities into
    //    alpha, same as the other families.
    if (element.container != ColorRole::Count) {
        QColor fill = theme.color(element.container);
        if (state == MdFabState::Disabled) {
            fill.setAlphaF(fill.alphaF() * fabMenuDisabledContainerOpacity());
        }
        if (fill.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(path);
        }
    }

    // 3. State layer, hover and keyboard focus only — press rides on the
    //    ripple.
    const bool interactive = !item.isEffectivelyDisabled();
    StateLayerKind kind = StateLayerKind::Hover;
    if (interactive && element.stateLayer != ColorRole::Count
        && MdStateLayer::strongestActive(&kind, item.isHovered(), item.hasKeyboardFocus(), false,
                                         false)) {
        const QColor overlay = MdStateLayer::overlay(theme.color(element.stateLayer), kind);
        if (overlay.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(path);
        }
    }

    // 4. Ripple, clipped to the container shape, taking the pressed row's
    //    published state-layer colour.
    if (MdRippleController *ripple = item.rippleController()) {
        const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, layout.radii);
        ripple->setBounds(layout.container.size());
        ripple->setClipPath(localPath);
        ripple->setContentColor(theme.color(element.stateLayer != ColorRole::Count
                                                ? element.stateLayer
                                                : ColorRole::OnSurface));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(layout.container.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 5. The content: icon and label share one published role per set.
    QColor contentColour;
    if (element.content != ColorRole::Count) {
        contentColour = theme.color(element.content);
        if (state == MdFabState::Disabled) {
            contentColour.setAlphaF(contentColour.alphaF() * fabMenuDisabledContentOpacity());
        }
    }
    if (contentColour.isValid()) {
        if (layout.icon.isValid()) {
            MdIcon::paint(&painter, layout.icon, item.iconName(), contentColour, item.iconSet(),
                          item.iconFamily());
        }
        if (layout.label.isValid() && !item.text().isEmpty()) {
            painter.setFont(layout.labelFont);
            painter.setPen(contentColour);
            painter.setBrush(Qt::NoBrush);
            painter.drawText(layout.label, Qt::AlignVCenter | MdStyleBase::leadingAlignment(&item), item.text());
        }
    }

    // 6. Focus indicator, last so nothing paints over it. Keyboard focus only.
    if (interactive && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(item.menuTokens().focusIndicator),
                           focusRingSpec(item.menuTokens()),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdFabMenuStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *item = qobject_cast<MdFabMenuItem *>(widget);
    if (painter == nullptr || item == nullptr) {
        return;
    }
    const MdFabMenuTokens &tokens = item->menuTokens();
    const MdFabMenuElementTokens &element = item->elementRole() == FabMenuElement::CloseButton
                                                ? tokens.closeButton
                                                : tokens.listItem;
    paintItem(*painter, *item, element, layoutFor(*item, element), item->reveal(), 0.0);
}

} // namespace md
