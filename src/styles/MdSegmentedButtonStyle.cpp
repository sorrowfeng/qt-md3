#include "MdSegmentedButtonStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdSegmentedButton.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

qreal ringInsetFor(const MdFocusRingSpec &spec)
{
    return spec.offset() + spec.activeWidth / 2.0 + spec.width / 2.0;
}

/// One segment's position-dependent radii, TL / TR / BR / BL. The base corner
/// is corner-full; the first segment rounds its inline-start corners, the
/// last its inline-end corners, middle segments are rectangles [compose
/// itemShape].
QList<qreal> segmentRadii(qreal base, int index, int count, bool rtl)
{
    if (count <= 1) {
        return QList<qreal>{base, base, base, base};
    }
    const bool isFirst = index == 0;
    const bool isLast = index == count - 1;
    // LTR: inline-start is the left edge. RTL swaps.
    const bool startOnLeft = !rtl;
    const bool roundsLeft = isFirst ? startOnLeft : (isLast ? !startOnLeft : false);
    const bool roundsRight = isFirst ? !startOnLeft : (isLast ? startOnLeft : false);
    return QList<qreal>{roundsLeft ? base : 0.0, roundsRight ? base : 0.0,
                        roundsRight ? base : 0.0, roundsLeft ? base : 0.0};
}

QColor themed(ColorRole role, qreal opacity)
{
    if (role == ColorRole::Count) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(role);
    if (!qFuzzyCompare(opacity, 1.0)) {
        colour.setAlphaF(colour.alphaF() * opacity);
    }
    return colour;
}

} // namespace

MdSegmentedButtonStyle::MdSegmentedButtonStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSegmentedButtonStyle *MdSegmentedButtonStyle::shared()
{
    static QMutex mutex;
    static MdSegmentedButtonStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        instance = new MdSegmentedButtonStyle;
        installPaintFilter<MdSegmentedButton>(instance);
    }
    return instance;
}

bool MdSegmentedButtonStyle::isInstalled()
{
    return hasPaintFilter(&MdSegmentedButton::staticMetaObject);
}

MdFocusRingSpec MdSegmentedButtonStyle::focusRingSpec(const MdSegmentedButtonTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.outwardOffset = tokens.focusIndicatorOffset;
    spec.inward = false;
    spec.color = QColor();
    return spec;
}

qreal MdSegmentedButtonStyle::focusRingInset(const MdSegmentedButtonTokens &tokens)
{
    return ringInsetFor(focusRingSpec(tokens));
}

MdSegmentedButtonStyle::Layout MdSegmentedButtonStyle::layoutFor(
    const MdSegmentedButton &button, const MdSegmentedButtonTokens &tokens)
{
    Layout layout;
    const MdTheme &theme = MdTheme::instance();
    const bool rtl = theme.isRightToLeft();

    const MdFocusRingSpec ringSpec = focusRingSpec(tokens);
    const qreal inset = ringInsetFor(ringSpec);
    layout.inset = inset;

    const QRectF inner = QRectF(button.rect()).adjusted(inset, inset, -inset, -inset);
    const int count = button.segmentCount();
    if (count == 0) {
        return layout;
    }

    const QFont font =
        MdTypeScale::font(tokens.labelStyle, TypeEmphasis::Baseline, theme.scriptCategory());
    const QFontMetricsF metrics(font);

    // Per-segment width: padding, [always-reserved icon slot, gap], label,
    // padding. The slot is reserved whether or not a segment shows a custom
    // icon, so the label never moves when the check scales in [compose
    // measure policy].
    const qreal slotWidth = tokens.iconSlotWidth() + tokens.iconSpacing;
    QVector<qreal> widths(count);
    for (int i = 0; i < count; ++i) {
        const QString label = button.segments().value(i);
        widths[i] = 2.0 * tokens.contentPadding + slotWidth
                    + (label.isEmpty() ? 0.0 : metrics.horizontalAdvance(label));
    }

    // The row overlaps neighbours by exactly the outline width [compose
    // spacedBy(-BorderWidth)], so the shared edges stack into one stroke.
    qreal total = widths.first();
    for (int i = 1; i < count; ++i) {
        total += widths.at(i) - tokens.outlineWidth;
    }
    layout.contentWidth = qMax<qreal>(inner.width(), total);

    const qreal base = tokens.baseCornerRadius();
    qreal cursor = rtl ? inner.left() + (inner.width() - total) : inner.left();
    for (int i = 0; i < count; ++i) {
        // append(SegmentLayout{}) + last() works on both Qt 5's QVector and
        // Qt 6's QList; emplaceBack() is Qt 6-only.
        layout.segments.append(SegmentLayout{});
        SegmentLayout &segment = layout.segments.last();
        segment.rect = QRectF(cursor, inner.top(), widths.at(i), tokens.containerHeight);
        // In RTL the enumeration still goes leading -> trailing, which draws
        // right-to-left; the radii helper mirrors accordingly.
        segment.radii = segmentRadii(base, i, count, rtl);
        cursor += widths.at(i) - tokens.outlineWidth;

        // Content boxes: slot on the inline-start side of the padding.
        const qreal slotLeft = rtl ? segment.rect.right() - tokens.contentPadding
                                         - tokens.iconSlotWidth()
                                   : segment.rect.left() + tokens.contentPadding;
        segment.iconSlot = QRectF(slotLeft, segment.rect.top(),
                                  tokens.iconSlotWidth(), tokens.containerHeight);
        const qreal labelLeft = rtl ? segment.iconSlot.left() - tokens.iconSpacing
                                    : segment.iconSlot.right() + tokens.iconSpacing;
        const QString label = button.segments().value(i);
        segment.hasLabel = !label.isEmpty();
        if (segment.hasLabel) {
            const qreal labelWidth = metrics.horizontalAdvance(label);
            segment.label = QRectF(rtl ? labelLeft - labelWidth : labelLeft, segment.rect.top(),
                                   labelWidth, tokens.containerHeight);
        }
    }
    return layout;
}

void MdSegmentedButtonStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *button = qobject_cast<MdSegmentedButton *>(widget);
    if (painter == nullptr || button == nullptr) {
        return;
    }
    const MdSegmentedButtonTokens &tokens = button->tokens();
    paintSegmentedButton(*painter, *button, tokens, layoutFor(*button, tokens));
}

void MdSegmentedButtonStyle::paintSegmentedButton(QPainter &painter,
                                                  const MdSegmentedButton &button,
                                                  const MdSegmentedButtonTokens &tokens,
                                                  const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const bool rtl = theme.isRightToLeft();
    const bool disabled = button.isEffectivelyDisabled();
    const int count = layout.segments.size();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. Container fills (selected segments only — the unselected family has
    //    no container row at all), then 2. full outlines per segment; the
    //    overlap makes shared edges one 1 px stroke.
    for (int i = 0; i < count; ++i) {
        const SegmentLayout &segment = layout.segments.at(i);
        const QPainterPath path = MdShape::roundedRect(segment.rect, segment.radii);
        if (button.isChecked(i)) {
            // The disabled selected segment keeps its container fill (the
            // export has no disabled-container row; Compose uses
            // SelectedContainerColor for disabledActive too) — the content
            // opacities carry the disabled look.
            const QColor fill = themed(tokens.selected.container, 1.0);
            if (fill.isValid()) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(fill);
                painter.drawPath(path);
            }
        }
        const QColor stroke =
            disabled ? themed(tokens.disabledOutline, tokens.disabledOutlineOpacity)
                     : themed(tokens.unselected.outline, 1.0);
        if (stroke.isValid() && tokens.outlineWidth > 0.0) {
            QPen pen(stroke);
            pen.setWidthF(tokens.outlineWidth);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(path);
        }
    }

    // 3. Flat state layer per segment — hover and keyboard focus only; press
    //    is the ripple (the interaction-fidelity rule every family shares).
    for (int i = 0; i < count; ++i) {
        if (disabled) {
            break;
        }
        const bool hovered = button.hoveredSegment() == i;
        const bool keyboardFocused = button.hasKeyboardFocus() && button.focusedSegment() == i;
        StateLayerKind kind = StateLayerKind::Hover;
        if (!MdStateLayer::strongestActive(&kind, hovered, keyboardFocused, false, false)) {
            continue;
        }
        const ColorRole layerRole =
            button.isChecked(i) ? tokens.selected.stateLayer : tokens.unselected.stateLayer;
        const QColor overlay = MdStateLayer::overlay(theme.color(layerRole), kind);
        if (overlay.isValid()) {
            const SegmentLayout &segment = layout.segments.at(i);
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(MdShape::roundedRect(segment.rect, segment.radii));
        }
    }

    // 4. Per-segment ripple, clipped to the segment shape. The ripple colour
    //    is the selection family's pressed state-layer colour.
    for (int i = 0; i < count; ++i) {
        MdRippleController *ripple = button.rippleController(i);
        if (ripple == nullptr) {
            continue;
        }
        const SegmentLayout &segment = layout.segments.at(i);
        const QRectF localRect(QPointF(0.0, 0.0), segment.rect.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, segment.radii);
        ripple->setBounds(segment.rect.size());
        ripple->setClipPath(localPath);
        const ColorRole pressRole =
            button.isChecked(i) ? tokens.selected.stateLayer : tokens.unselected.stateLayer;
        ripple->setContentColor(theme.color(pressRole));
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(segment.rect.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 5. Content: the reserved icon slot (check crossfaded over the custom
    //    icon) and the label.
    const QFont font =
        MdTypeScale::font(tokens.labelStyle, TypeEmphasis::Baseline, theme.scriptCategory());
    for (int i = 0; i < count; ++i) {
        const SegmentLayout &segment = layout.segments.at(i);
        const bool selected = button.isChecked(i);
        const qreal morph = button.checkMorph(i);

        const QColor contentColour =
            disabled ? themed(tokens.disabledContent, tokens.disabledLabelOpacity)
                     : themed(selected ? tokens.selected.labelText : tokens.unselected.labelText,
                              1.0);
        const QColor iconColour =
            disabled ? themed(tokens.disabledContent, tokens.disabledIconOpacity)
                     : themed(selected ? tokens.selected.icon : tokens.unselected.icon, 1.0);

        // The custom icon fades out as the check scales in [compose
        // crossfade]; the check scales from 0 around its centre.
        const QString customIcon = button.leadingIcons().value(i);
        if (!customIcon.isEmpty() && morph < 1.0 && iconColour.isValid()) {
            painter.setOpacity(1.0 - morph);
            MdIcon::paint(&painter, segment.iconSlot, customIcon, iconColour);
            painter.setOpacity(1.0);
        }
        if (morph > 0.0 && iconColour.isValid()) {
            painter.setOpacity(morph);
            painter.save();
            const QPointF center = segment.iconSlot.center();
            painter.translate(center);
            painter.scale(morph, morph);
            painter.translate(-center);
            MdIcon::paint(&painter, segment.iconSlot, QStringLiteral("check"), iconColour);
            painter.restore();
            painter.setOpacity(1.0);
        }

        if (segment.hasLabel && contentColour.isValid()) {
            const QFontMetricsF fontMetrics(font);
            const qreal baseline = segment.label.top()
                                   + (segment.label.height() + fontMetrics.ascent()
                                      - fontMetrics.descent()) / 2.0;
            painter.setFont(font);
            painter.setPen(contentColour);
            const QString label = button.segments().value(i);
            const qreal x = rtl ? segment.label.right() - fontMetrics.horizontalAdvance(label)
                                : segment.label.left();
            painter.drawText(QPointF(x, baseline), label);
        }
    }

    // 6. Focus indicator around the focused segment, keyboard focus only.
    if (!disabled && button.hasKeyboardFocus()) {
        const int focused = button.focusedSegment();
        if (focused >= 0 && focused < count) {
            const SegmentLayout &segment = layout.segments.at(focused);
            MdFocusRingController *ring = button.focusRingController();
            MdFocusRing::paint(
                &painter, segment.rect, segment.radii,
                theme.color(tokens.focusIndicator), focusRingSpec(tokens),
                (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
        }
    }

    painter.restore();
}

} // namespace md
