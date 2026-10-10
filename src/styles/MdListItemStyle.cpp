#include "MdListItemStyle.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdListItem.h"

#include <cmath>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// [compose] `isSupportingMultilineHeuristic`: a supporting text taller than
/// 30sp is treated as multiline before it is measured for real. The paint
/// path measures the wrapped text instead; this estimate only has to agree
/// for the intrinsic pass.
constexpr qreal kMultilineThreshold = 30.0;

qreal lineHeight(TypeStyle style)
{
    return MdTypeScale::lineHeight(style, TypeEmphasis::Baseline,
                                   MdTheme::instance().scriptCategory());
}

QFont fontFor(TypeStyle style)
{
    return MdTypeScale::font(style, TypeEmphasis::Baseline, MdTheme::instance().scriptCategory());
}

/// The wrapped height of `text` in `style` within `width` — the supporting
/// text may be a paragraph, and three-line items are exactly the ones whose
/// supporting text wraps [compose].
qreal wrappedHeight(const QString &text, TypeStyle style, qreal width)
{
    if (text.isEmpty()) {
        return 0.0;
    }
    const QFontMetrics metrics(fontFor(style));
    const QRect bounds = metrics.boundingRect(QRect(0, 0, int(width), 10000),
                                              int(MdStyleBase::leadingAlignment(nullptr) | Qt::TextWordWrap), text);
    return qMax<qreal>(bounds.height(), qreal(metrics.height()));
}

/// Source-over composite of `top` at `opacity` onto an opaque `base` — the
/// arithmetic Compose performs with `DisabledAlpha` and the export publishes
/// as per-element opacities.
QColor compositeOver(const QColor &base, const QColor &top, qreal opacity)
{
    QColor layer = top;
    const qreal alpha = qBound(0.0, layer.alphaF() * opacity, 1.0);
    const auto mix = [alpha](int a, int b) {
        return int(std::lround(b * alpha + a * (1.0 - alpha)));
    };
    return QColor(mix(base.red(), layer.red()), mix(base.green(), layer.green()),
                  mix(base.blue(), layer.blue()), base.alpha());
}

bool supportingIsMultiline(const MdListItem &item, const MdListTokens &tokens, qreal contentWidth)
{
    if (item.supporting().isEmpty()) {
        return false;
    }
    const qreal single = lineHeight(tokens.supportingTextType);
    return wrappedHeight(item.supporting(), tokens.supportingTextType, contentWidth)
        > single + 0.5;
}

/// The leading slot's width and height for the current content.
QSizeF leadingExtent(const MdListItem &item, const MdListTokens &tokens)
{
    switch (item.leadingKind()) {
    case MdListItem::LeadingKind::Icon:
        return QSizeF(tokens.leadingIconSize, tokens.leadingIconSize);
    case MdListItem::LeadingKind::Avatar:
        return QSizeF(tokens.leadingAvatarSize, tokens.leadingAvatarSize);
    case MdListItem::LeadingKind::Image:
        return QSizeF(tokens.leadingImageWidth, tokens.leadingImageHeight);
    case MdListItem::LeadingKind::Video:
        return QSizeF(tokens.leadingVideoWidth, tokens.leadingVideoHeight);
    case MdListItem::LeadingKind::None:
        break;
    }
    if (item.leadingWidget() != nullptr) {
        return item.leadingWidget()->sizeHint();
    }
    return QSizeF();
}

QSizeF trailingExtent(const MdListItem &item, const MdListTokens &tokens)
{
    if (!item.trailingIcon().isEmpty()) {
        return QSizeF(tokens.trailingIconSize, tokens.trailingIconSize);
    }
    if (!item.trailingSupporting().isEmpty()) {
        const QFontMetrics metrics(fontFor(tokens.trailingSupportingTextType));
        return QSizeF(metrics.horizontalAdvance(item.trailingSupporting()),
                      lineHeight(tokens.trailingSupportingTextType));
    }
    if (item.trailingWidget() != nullptr) {
        return item.trailingWidget()->sizeHint();
    }
    return QSizeF();
}

} // namespace

MdListItemStyle::MdListItemStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdListItemStyle *MdListItemStyle::shared()
{
    static QMutex mutex;
    static MdListItemStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdListItemStyle;
        installPaintFilter<MdListItem>(instance);
    }
    return instance;
}

bool MdListItemStyle::isInstalled()
{
    return hasPaintFilter(&MdListItem::staticMetaObject);
}

int MdListItemStyle::lineCount(const MdListItem &item)
{
    // [compose] ListItemType: three-line when there is both an overline and
    // supporting content, or when the supporting text is multiline; two-line
    // when there is either; else one line.
    const bool hasOverline = !item.overline().isEmpty();
    const bool hasSupporting = !item.supporting().isEmpty();
    // The width available to the supporting text, which decides whether it
    // wraps — and a wrapped supporting text is a three-line item.
    const qreal width = qMax<qreal>(item.width(), 120.0);
    const bool multiline = supportingIsMultiline(item, item.tokens(), width * 0.7);

    if ((hasOverline && hasSupporting) || multiline) {
        return 3;
    }
    if (hasOverline || hasSupporting) {
        return 2;
    }
    return 1;
}

qreal MdListItemStyle::measuredHeight(const MdListItem &item, const MdListTokens &tokens,
                                      qreal contentWidth)
{
    const int lines = lineCount(item);

    const qreal padding = tokens.topSpace + tokens.bottomSpace;
    const qreal textWidth =
        qMax<qreal>(contentWidth - tokens.leadingSpace - tokens.trailingSpace, 40.0);

    qreal content = 0.0;
    if (!item.overline().isEmpty()) {
        content += lineHeight(tokens.overlineType);
    }
    content += lineHeight(tokens.labelTextType);
    if (!item.supporting().isEmpty()) {
        content += wrappedHeight(item.supporting(), tokens.supportingTextType, textWidth);
    }

    const qreal minimum = lines == 3 ? tokens.threeLineHeight
                                     : (lines == 2 ? tokens.twoLineHeight : tokens.oneLineHeight);
    return qMax(minimum, content + padding);
}

ShapeCorner MdListItemStyle::shapeFor(const MdListItem &item, const MdListTokens &tokens)
{
    if (item.variant() == MdListVariant::Standard) {
        return tokens.containerShape;
    }

    // [compose] `shapeForInteraction`, highest priority first:
    // pressed > dragged > selected > focused > hovered > base.
    if (item.isPressed() && item.isInteractive()) {
        return tokens.pressedShape;
    }
    if (item.isDragged()) {
        return tokens.draggedShape;
    }
    if (!item.isEnabled()) {
        // The export publishes two disabled shape rows: an unselected disabled
        // item drops to corner-extra-small and a *selected* disabled one keeps
        // corner-large. Compose has no disabled branch at all — there the
        // selection would win — which lands on the same two values.
        return item.isSelected() ? tokens.selectedDisabledShape : tokens.disabledShape;
    }
    if (item.isSelected()) {
        return tokens.selectedShape;
    }
    if (item.hasKeyboardFocus()) {
        return tokens.focusedShape;
    }
    if (item.isHovered() && item.isInteractive()) {
        return tokens.hoveredShape;
    }
    return tokens.expressiveShape;
}

QList<qreal> MdListItemStyle::radiiFor(const MdListItem &item, const MdListTokens &tokens,
                                       const QSizeF &size)
{
    const ShapeCorner shape = shapeFor(item, tokens);
    QList<qreal> radii = size.isEmpty() ? MdShape::radii(shape) : MdShape::resolvedRadii(shape, size);

    // [compose] `segmentedShapes(index, count)`: the first item's *top* pair
    // and the last item's *bottom* pair take the list's own
    // `container.shape`; a single item takes all four. `index < 0` — or a
    // count of zero — is how "this list is not segmented" is expressed.
    const int index = item.segmentedIndex();
    const int count = item.segmentedCount();
    if (index < 0 || count <= 0) {
        return radii;
    }

    const qreal outer = size.isEmpty() ? MdShape::radius(tokens.listShape)
                                       : MdShape::resolvedRadius(tokens.listShape, size);
    // Radii are ordered top-left, top-right, bottom-right, bottom-left.
    if (count == 1) {
        for (int i = 0; i < radii.size(); ++i) {
            radii[i] = outer;
        }
    } else if (index == 0) {
        radii[0] = outer;
        radii[1] = outer;
    } else if (index == count - 1) {
        radii[2] = outer;
        radii[3] = outer;
    }
    return radii;
}

MdListState MdListItemStyle::stateFor(const MdListItem &item)
{
    // The colour rows resolve in a different order from the shape rows:
    // disabled first, which is `ListItemColors.containerColor`'s own ladder
    // (`!enabled -> …, dragged -> …, selected -> …`). See the header note.
    if (!item.isEnabled()) {
        return MdListState::Disabled;
    }
    if (item.isPressed() && item.isInteractive()) {
        return MdListState::Pressed;
    }
    if (item.isDragged()) {
        return MdListState::Dragged;
    }
    if (item.hasKeyboardFocus()) {
        return MdListState::Focused;
    }
    if (item.isHovered() && item.isInteractive()) {
        return MdListState::Hovered;
    }
    return MdListState::Enabled;
}

MdListItemStyle::Layout MdListItemStyle::layoutFor(const MdListItem &item,
                                                   const MdListTokens &tokens)
{
    Layout layout;

    const QRectF widgetRect(item.rect());
    const qreal height = widgetRect.height();
    const qreal width = widgetRect.width();

    layout.lines = lineCount(item);
    layout.container = QRectF(widgetRect.left(), widgetRect.top(), width, height);
    layout.radii = radiiFor(item, tokens, layout.container.size());

    // [spec] content is centred until the item is 88dp or taller, top-aligned
    // from there. Compose derives a breakpoint from the two- and three-line
    // heights minus its interactive paddings; the spec's 88dp is used here and
    // the difference recorded in docs/porting-todo.md.
    layout.topAligned = height >= tokens.verticalAlignmentBreakpoint;

    // --- the leading slot ---------------------------------------------------
    const QSizeF leading = leadingExtent(item, tokens);
    qreal contentLeft = widgetRect.left() + tokens.leadingSpace;
    if (!leading.isEmpty()) {
        // [spec] The leading *icon* is always top-aligned with an 8dp top
        // padding (12dp once the item reaches the 88dp breakpoint) — a
        // 56dp-tall icon item puts its glyph at y = 8, not at the centre.
        // Every other leading element follows the content's alignment.
        qreal top = 0.0;
        if (item.leadingKind() == MdListItem::LeadingKind::Icon) {
            top = widgetRect.top() + (height >= tokens.verticalAlignmentBreakpoint
                                          ? tokens.leadingIconTopPaddingTall
                                          : tokens.leadingIconTopPadding);
        } else if (layout.topAligned) {
            top = widgetRect.top() + tokens.topSpace;
        } else {
            top = widgetRect.top() + (height - leading.height()) / 2.0;
        }
        layout.leading = QRectF(contentLeft, top, leading.width(), leading.height());
        contentLeft += leading.width() + tokens.betweenSpace;
    }

    // --- the trailing slot --------------------------------------------------
    const QSizeF trailing = trailingExtent(item, tokens);
    qreal contentRight = widgetRect.left() + width - tokens.trailingSpace;
    if (!trailing.isEmpty()) {
        layout.trailing = QRectF(contentRight - trailing.width(),
                                 widgetRect.top() + (height - trailing.height()) / 2.0,
                                 trailing.width(), trailing.height());
        if (layout.topAligned) {
            layout.trailing.moveTop(widgetRect.top() + tokens.topSpace);
        }
        contentRight -= trailing.width() + tokens.betweenSpace;
    }

    const qreal contentWidth = qMax<qreal>(contentRight - contentLeft, 0.0);

    // --- the text column ----------------------------------------------------
    qreal textTop = 0.0;
    qreal textHeight = 0.0;
    const bool hasOverline = !item.overline().isEmpty();
    const bool hasSupporting = !item.supporting().isEmpty();
    if (hasOverline) {
        textHeight += lineHeight(tokens.overlineType);
    }
    textHeight += lineHeight(tokens.labelTextType);
    if (hasSupporting) {
        textHeight += wrappedHeight(item.supporting(), tokens.supportingTextType, contentWidth);
    }

    if (layout.topAligned) {
        textTop = widgetRect.top() + tokens.topSpace;
    } else {
        textTop = widgetRect.top() + (height - textHeight) / 2.0;
    }

    if (hasOverline) {
        layout.overline = QRectF(contentLeft, textTop, contentWidth, lineHeight(tokens.overlineType));
        textTop += layout.overline.height();
    }
    layout.headline = QRectF(contentLeft, textTop, contentWidth, lineHeight(tokens.labelTextType));
    textTop += layout.headline.height();
    if (hasSupporting) {
        layout.supporting = QRectF(contentLeft, textTop, contentWidth,
                                   wrappedHeight(item.supporting(), tokens.supportingTextType,
                                                 contentWidth));
    }

    if (!item.trailingSupporting().isEmpty()) {
        layout.trailingSupporting =
            QRectF(contentLeft, widgetRect.top() + (height - lineHeight(tokens.trailingSupportingTextType)) / 2.0,
                   contentWidth, lineHeight(tokens.trailingSupportingTextType));
        if (layout.topAligned) {
            layout.trailingSupporting.moveTop(widgetRect.top() + tokens.topSpace);
        }
    }

    return layout;
}

MdFocusRingSpec MdListItemStyle::focusRingSpec(const MdListTokens &tokens)
{
    // The list item's ring is the *inward* variant — material-web configures
    // `md-focus-ring` inward for list items so the ring never collides with a
    // neighbouring item, unlike the card's outward ring. Every number comes
    // from the component's own `md.comp.list.focus.indicator.*` rows; the
    // colour is left to the paint, which reads it from these same tokens.
    MdFocusRingSpec spec;
    spec.inward = true;
    spec.width = tokens.focusIndicatorThickness;
    spec.inwardOffset = tokens.focusRingGap();
    spec.color = QColor();
    return spec;
}

qreal MdListItemStyle::focusRingInset(const MdListTokens &tokens)
{
    // Because the ring is drawn inside the container it reserves no outside
    // margin at all — `focusRingGap()` is the ring's own inward offset, not a
    // margin the parent has to make room for.
    return focusRingSpec(tokens).offset();
}

void MdListItemStyle::paintListItem(QPainter &painter, const MdListItem &item,
                                    const MdListTokens &tokens, const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();
    const bool disabled = !item.isEnabled();
    const bool selected = item.isSelected();
    const MdListFamily &family = selected ? tokens.selectedFamily : tokens.family;
    const MdListDisabledRow &disabledRow = tokens.disabledRow(selected);
    const MdListState state = stateFor(item);
    // The colour row paint reads: the enabled row for Enabled, the state's own
    // row otherwise (rows the export does not publish keep the enabled value —
    // see MdListTokens).
    const MdListStateRow &row = state == MdListState::Enabled ? family.enabled : family.state(state);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. The container. The state's row supplies the resting colour; the
    //    disabled row composites over it the way Compose does (the disabled
    //    colour at its published opacity over the enabled container).
    //
    //    No shadow here: `container.elevation` is level0 and the dragged row
    //    is level4, but a list item's container *is* its whole widget rect, so
    //    a shadow drawn around it would be clipped away by Qt. Compose paints
    //    the dragged item in an overlay above the list; MdList lays items out
    //    in place, so the row is carried and not painted — see
    //    docs/porting-todo.md and the note in MdListTokens.h.
    QColor containerColor = theme.color(family.container);
    if (disabled) {
        if (disabledRow.containerOpacity > 0.0) {
            containerColor = compositeOver(containerColor, theme.color(disabledRow.container),
                                           disabledRow.containerOpacity);
        }
    } else if (state == MdListState::Enabled) {
        containerColor = theme.color(family.container);
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(containerColor);
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));

    const auto contentColour = [&](ColorRole role, qreal opacity) {
        // Compose composites a disabled colour over the container at the
        // published opacity; the text and icons then paint that colour.
        return compositeOver(containerColor, theme.color(role), opacity);
    };

    // 2. The hover / keyboard-focus / dragged state layer — the strongest
    //    active one wins, and the press never paints a flat layer (the ripple
    //    carries it, the same rule as the button families). The colour comes
    //    from the state's own `state-layer.color` row and its published
    //    opacity.
    if (!disabled && item.isInteractive()) {
        StateLayerKind kind = StateLayerKind::Hover;
        if (MdStateLayer::strongestActive(&kind, item.isHovered(), item.hasKeyboardFocus(), false,
                                          item.isDragged())
            && row.stateLayerOpacity > 0.0) {
            QColor overlay = theme.color(row.stateLayer);
            overlay.setAlphaF(qBound(0.0, overlay.alphaF() * row.stateLayerOpacity, 1.0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(overlay);
            painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
        }
    }

    // 3. The press ripple.
    if (MdRippleController *ripple = item.rippleController()) {
        const QRectF localRect(QPointF(0.0, 0.0), layout.container.size());
        const QPainterPath localPath = MdShape::roundedRect(localRect, layout.radii);
        const MdRippleFrame frame = ripple->currentFrame();
        if (frame.valid) {
            painter.save();
            painter.translate(layout.container.topLeft());
            MdRipple::paint(&painter, frame, localPath, ripple->contentColor());
            painter.restore();
        }
    }

    // 4. The leading slot.
    if (layout.leading.isValid()) {
        switch (item.leadingKind()) {
        case MdListItem::LeadingKind::Icon: {
            const QColor iconColour =
                disabled ? contentColour(disabledRow.leadingIcon, disabledRow.leadingIconOpacity)
                         : theme.color(row.leadingIcon);
            MdIcon::paint(&painter, layout.leading, item.leadingIcon(), iconColour);
            break;
        }
        case MdListItem::LeadingKind::Avatar: {
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme.color(tokens.leadingAvatarColor));
            painter.drawPath(MdShape::roundedRect(layout.leading, tokens.leadingAvatarShape));
            if (!item.avatarLabel().isEmpty()) {
                painter.setPen(QPen(theme.color(tokens.leadingAvatarLabelColor), 0.0));
                painter.setFont(fontFor(tokens.leadingAvatarLabelType));
                painter.drawText(layout.leading, Qt::AlignCenter, item.avatarLabel());
            }
            break;
        }
        case MdListItem::LeadingKind::Image:
        case MdListItem::LeadingKind::Video: {
            // The media itself is the caller's widget, placed in this slot.
            // The item paints the token surface behind it so an empty slot is
            // still visible as geometry rather than a hole.
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme.color(ColorRole::SurfaceVariant));
            painter.drawPath(MdShape::roundedRect(layout.leading, tokens.leadingImageShape));
            break;
        }
        case MdListItem::LeadingKind::None:
            break;
        }
    }

    // 5. The text column.
    const QFont headlineFont = fontFor(tokens.labelTextType);
    painter.setFont(headlineFont);
    painter.setPen(QPen(disabled ? contentColour(disabledRow.labelText, disabledRow.labelTextOpacity)
                                 : theme.color(row.labelText),
                        0.0));
    const int headlineFlags = int(MdStyleBase::leadingAlignment(nullptr) | Qt::AlignVCenter | Qt::TextWordWrap);
    painter.drawText(layout.headline, headlineFlags, item.headline());

    if (layout.overline.isValid()) {
        painter.setFont(fontFor(tokens.overlineType));
        painter.setPen(QPen(disabled ? contentColour(disabledRow.overline, disabledRow.overlineOpacity)
                                     : theme.color(row.overline),
                            0.0));
        painter.drawText(layout.overline, int(MdStyleBase::leadingAlignment(nullptr) | Qt::AlignVCenter), item.overline());
    }

    if (layout.supporting.isValid()) {
        painter.setFont(fontFor(tokens.supportingTextType));
        painter.setPen(QPen(disabled ? contentColour(disabledRow.supportingText,
                                                     disabledRow.supportingTextOpacity)
                                     : theme.color(row.supportingText),
                            0.0));
        painter.drawText(layout.supporting, int(MdStyleBase::leadingAlignment(nullptr) | Qt::AlignTop | Qt::TextWordWrap),
                         item.supporting());
    }

    if (layout.trailingSupporting.isValid()) {
        painter.setFont(fontFor(tokens.trailingSupportingTextType));
        painter.setPen(QPen(
            disabled ? contentColour(disabledRow.trailingSupportingText,
                                     disabledRow.trailingSupportingTextOpacity)
                     : theme.color(row.trailingSupportingText),
            0.0));
        painter.drawText(layout.trailingSupporting, int(MdStyleBase::leadingAlignment(nullptr) | Qt::AlignVCenter),
                         item.trailingSupporting());
    }

    // 6. The trailing slot.
    if (layout.trailing.isValid() && !item.trailingIcon().isEmpty()) {
        const QColor iconColour = disabled
            ? contentColour(disabledRow.trailingIcon, disabledRow.trailingIconOpacity)
            : theme.color(row.trailingIcon);
        MdIcon::paint(&painter, layout.trailing, item.trailingIcon(), iconColour);
    }

    // 7. The focus indicator, last so nothing paints over it. material-web
    //    configures the list item's ring `inward`; keyboard focus only.
    if (!disabled && item.isInteractive() && item.hasKeyboardFocus()) {
        MdFocusRingController *ring = item.focusRingController();
        MdFocusRing::paint(&painter, layout.container, layout.radii,
                           theme.color(tokens.focusIndicatorColor), focusRingSpec(tokens),
                           (ring && ring->isAnimating()) ? ring->elapsedMs() : -1);
    }

    painter.restore();
}

void MdListItemStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *item = qobject_cast<MdListItem *>(widget);
    if (painter == nullptr || item == nullptr) {
        return;
    }
    const MdListTokens &tokens = item->tokens();
    paintListItem(*painter, *item, tokens, layoutFor(*item, tokens));
}

} // namespace md
