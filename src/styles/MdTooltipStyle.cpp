#include "MdTooltipStyle.h"

#include "widgets/MdTooltip.h"

#include "core/MdElevation.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtCore/QPoint>
#include <QtCore/QSize>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QTextLayout>
#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

namespace {

/// Lay the text out with wrapping, returning the block rect sized to the
/// text (position not yet applied) — the same helper shape the snackbar
/// style uses.
QRectF layoutText(const QString &text, const QFont &font, qreal maxWidth, int *lineCount)
{
    QTextLayout layout(text, font);
    QTextOption option;
    option.setWrapMode(QTextOption::WordWrap);
    layout.setTextOption(option);
    layout.beginLayout();
    qreal y = 0.0;
    qreal width = 0.0;
    int lines = 0;
    forever {
        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(maxWidth);
        line.setPosition(QPointF(0.0, y));
        y += line.height();
        width = qMax(width, line.naturalTextWidth());
        ++lines;
    }
    layout.endLayout();
    if (lineCount) {
        *lineCount = lines;
    }
    return QRectF(0.0, 0.0, width, y);
}

/// The state-layer overlay for the rich action's row.
QColor rowOverlay(const MdTheme &theme, const MdTooltipActionRow &row)
{
    if (row.stateLayer == ColorRole::Count || row.stateLayerOpacity <= 0.0) {
        return QColor();
    }
    QColor colour = theme.color(row.stateLayer);
    colour.setAlphaF(row.stateLayerOpacity);
    return colour;
}

/// The caret triangle for `caretRect`: apex on the anchor-facing tip.
QPainterPath caretPath(const QRectF &caretRect, MdTooltipCaretSide side)
{
    QPainterPath path;
    if (side == MdTooltipCaretSide::Bottom) {
        // Protrudes below the container: flat edge on top, tip at the bottom.
        path.moveTo(caretRect.left(), caretRect.top());
        path.lineTo(caretRect.right(), caretRect.top());
        path.lineTo(caretRect.center().x(), caretRect.bottom());
        path.closeSubpath();
    } else if (side == MdTooltipCaretSide::Top) {
        path.moveTo(caretRect.left(), caretRect.bottom());
        path.lineTo(caretRect.right(), caretRect.bottom());
        path.lineTo(caretRect.center().x(), caretRect.top());
        path.closeSubpath();
    }
    return path;
}

} // namespace

MdTooltipStyle::MdTooltipStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTooltipStyle *MdTooltipStyle::shared()
{
    static QMutex mutex;
    static MdTooltipStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTooltipStyle;
        installPaintFilter<MdTooltip>(instance);
    }
    return instance;
}

bool MdTooltipStyle::isInstalled()
{
    return hasPaintFilter(&MdTooltip::staticMetaObject);
}

MdTooltipStyle::PlainLayout MdTooltipStyle::plainLayout(const QRectF &bounds,
                                                        const QString &text,
                                                        const QFont &font,
                                                        const MdTooltipTokens &tokens)
{
    // Compose PlainTooltip: a Box padded 8/4 inside the surface; the text
    // wraps at the surface width minus the padding.
    const qreal textMaxWidth =
        qMax<qreal>(0.0, bounds.width() - tokens.kPlainHorizontalPadding * 2.0);
    const QRectF textSize = layoutText(text, font, textMaxWidth, nullptr);
    PlainLayout layout;
    layout.textRect = QRectF(bounds.left() + tokens.kPlainHorizontalPadding,
                             bounds.top() + tokens.kPlainVerticalPadding, textSize.width(),
                             textSize.height());
    return layout;
}

MdTooltipStyle::RichLayout MdTooltipStyle::richLayout(const QRectF &bounds,
                                                      const QString &title,
                                                      const QString &text,
                                                      const QString &actionLabel,
                                                      const QFont &subheadFont,
                                                      const QFont &textFont,
                                                      const QFont &actionFont,
                                                      const MdTooltipTokens &tokens)
{
    // Compose RichTooltip: a Column with RichTooltipHorizontalPadding on both
    // sides; subhead via paddingFromBaseline(top = 28), text via
    // paddingFromBaseline(top = 24) + bottom 16, action min height 36 over a
    // bottom padding of 8. Compose's paddingFromBaseline puts the FIRST
    // BASELINE at the given distance; we subtract the first line's ascent,
    // the same visual intent the snackbar's first-line port used.
    const bool hasSubhead = !title.isEmpty();
    const bool hasAction = !actionLabel.isEmpty();

    const qreal textMaxWidth =
        qMax<qreal>(0.0, bounds.width() - tokens.kRichHorizontalPadding * 2.0);

    RichLayout layout;
    qreal y = bounds.top();

    if (hasSubhead) {
        const QFontMetricsF subheadMetrics(subheadFont);
        const QRectF subheadSize = layoutText(title, subheadFont, textMaxWidth, nullptr);
        const qreal placeY = y + tokens.kHeightToSubheadFirstLine - subheadMetrics.ascent();
        layout.subheadRect =
            QRectF(bounds.left() + tokens.kRichHorizontalPadding, placeY, subheadSize.width(),
                   subheadSize.height());
        y = layout.subheadRect.top() + subheadSize.height();
    }

    {
        const QFontMetricsF textMetrics(textFont);
        const QRectF textSize = layoutText(text, textFont, textMaxWidth, nullptr);
        if (!hasSubhead && !hasAction) {
            // textVerticalPadding(false, false): the plain 4 px vertical
            // padding — a text-only rich tooltip reads like a plain one.
            const qreal placeY = y + tokens.kPlainVerticalPadding;
            layout.textRect = QRectF(bounds.left() + tokens.kRichHorizontalPadding, placeY,
                                     textSize.width(), textSize.height());
            layout.containerHeight =
                placeY + textSize.height() + tokens.kPlainVerticalPadding - bounds.top();
            return layout;
        }
        const qreal placeY = y + tokens.kHeightFromSubheadToTextFirstLine - textMetrics.ascent();
        layout.textRect = QRectF(bounds.left() + tokens.kRichHorizontalPadding, placeY,
                                 textSize.width(), textSize.height());
        y = layout.textRect.top() + textSize.height() + tokens.kTextBottomPadding;
    }

    if (hasAction) {
        const QFontMetricsF actionMetrics(actionFont);
        const qreal labelWidth = actionMetrics.horizontalAdvance(actionLabel);
        // The hit region plays the TextButton's role; the label sits centred
        // inside it (the project's hit-region-first model).
        const qreal hitWidth = labelWidth + tokens.kActionHitPadding * 2.0;
        const qreal boxHeight = qMax<qreal>(tokens.kActionLabelMinHeight,
                                            actionMetrics.height() + tokens.kActionHitPadding * 2.0);
        const qreal boxTop = y;
        const qreal centerY = boxTop + boxHeight / 2.0;
        // The hit region is start-aligned into the padded column: a rich
        // tooltip's action reads as the spec's leading button.
        layout.actionHitRect = QRectF(bounds.left() + tokens.kRichHorizontalPadding,
                                      centerY - actionMetrics.height() / 2.0
                                          - tokens.kActionHitPadding,
                                      hitWidth,
                                      actionMetrics.height() + tokens.kActionHitPadding * 2.0);
        layout.actionRect = QRectF(layout.actionHitRect.left() + tokens.kActionHitPadding,
                                   centerY - actionMetrics.height() / 2.0, labelWidth,
                                   actionMetrics.height());
        layout.containerHeight = boxTop + boxHeight + tokens.kActionLabelBottomPadding
            - bounds.top();
        return layout;
    }

    layout.containerHeight = y - bounds.top();
    return layout;
}

qreal MdTooltipStyle::plainHeightForWidth(qreal width, const QString &text,
                                          const MdTooltipTokens &tokens)
{
    const QRectF bounds(0.0, 0.0, width, 0.0);
    const PlainLayout layout =
        plainLayout(bounds, text, MdTypeScale::font(tokens.plainTextStyle), tokens);
    return qMax<qreal>(tokens.kMinHeight, layout.textRect.bottom() + tokens.kPlainVerticalPadding);
}

qreal MdTooltipStyle::richHeightForWidth(qreal width,
                                         const QString &title,
                                         const QString &text,
                                         const QString &actionLabel,
                                         const MdTooltipTokens &tokens)
{
    const QRectF bounds(0.0, 0.0, width, 0.0);
    const RichLayout layout = richLayout(
        bounds, title, text, actionLabel, MdTypeScale::font(tokens.richSubheadStyle),
        MdTypeScale::font(tokens.richTextStyle), MdTypeScale::font(tokens.richActionLabelStyle),
        tokens);
    const qreal contentHeight = qMax<qreal>(tokens.kMinHeight, layout.containerHeight);
    return qMax<qreal>(tokens.kMinHeight, contentHeight);
}

qreal MdTooltipStyle::plainWidthFor(const QString &text, const MdTooltipTokens &tokens)
{
    const QFontMetricsF metrics(MdTypeScale::font(tokens.plainTextStyle));
    qreal width = metrics.horizontalAdvance(text) + tokens.kPlainHorizontalPadding * 2.0;
    return qBound(tokens.kMinWidth, width, tokens.kMaxPlainWidth);
}

qreal MdTooltipStyle::richWidthFor(const QString &title,
                                   const QString &text,
                                   const QString &actionLabel,
                                   const MdTooltipTokens &tokens)
{
    const QFontMetricsF subheadMetrics(MdTypeScale::font(tokens.richSubheadStyle));
    const QFontMetricsF textMetrics(MdTypeScale::font(tokens.richTextStyle));
    const QFontMetricsF actionMetrics(MdTypeScale::font(tokens.richActionLabelStyle));
    qreal content = textMetrics.horizontalAdvance(text);
    if (!title.isEmpty()) {
        content = qMax(content, subheadMetrics.horizontalAdvance(title));
    }
    if (!actionLabel.isEmpty()) {
        content = qMax(content, actionMetrics.horizontalAdvance(actionLabel)
                + tokens.kActionHitPadding * 2.0);
    }
    const qreal width = content + tokens.kRichHorizontalPadding * 2.0;
    return qBound(tokens.kMinWidth, width, tokens.kMaxRichWidth);
}

QPoint MdTooltipStyle::abovePopupPosition(const QRect &anchorRect,
                                          const QSize &size,
                                          const QSize &windowSize,
                                          qreal spacing)
{
    // Horizontal alignment preference: middle -> start -> end; vertical:
    // above -> below; always coerced into the window.
    int x = anchorRect.left() + (anchorRect.width() - size.width()) / 2;
    x = qBound(0, x, qMax(0, windowSize.width() - size.width()));

    int y = anchorRect.top() - size.height() - int(spacing);
    if (y < 0) {
        y = anchorRect.bottom() + 1 + int(spacing);
    }
    y = qBound(0, y, qMax(0, windowSize.height() - size.height()));
    return QPoint(x, y);
}

QPoint MdTooltipStyle::richPopupPosition(const QRect &anchorRect,
                                         const QSize &size,
                                         const QSize &windowSize,
                                         qreal spacing)
{
    // Start-aligned, shifting left when the right edge clips and centring
    // when both would; above preferred, below otherwise.
    int x = anchorRect.left();
    if (x + size.width() > windowSize.width()) {
        x = anchorRect.right() + 1 - size.width();
        if (x < 0) {
            x = anchorRect.left() + (anchorRect.width() - size.width()) / 2;
        }
    }

    int y = anchorRect.top() - size.height() - int(spacing);
    if (y < 0) {
        y = anchorRect.bottom() + 1 + int(spacing);
    }
    return QPoint(x, y);
}

void MdTooltipStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *tooltip = qobject_cast<MdTooltip *>(widget);
    if (!tooltip) {
        return;
    }
    paintTooltip(painter, tooltip->containerRect(), tooltip->variant(), tooltip->title(),
                 tooltip->text(), tooltip->actionLabel(), tooltip->actionState(),
                 tooltip->actionRippleFrame(), tooltip->actionHasKeyboardFocus(),
                 tooltip->caretSide(), tooltip->caretRect(), tooltip->paintOpacity());
}

void MdTooltipStyle::paintTooltip(QPainter *painter,
                                  const QRectF &containerRect,
                                  MdTooltipVariant variant,
                                  const QString &title,
                                  const QString &text,
                                  const QString &actionLabel,
                                  MdTooltipActionState actionState,
                                  const MdRippleFrame &actionRipple,
                                  bool actionHasKeyboardFocus,
                                  MdTooltipCaretSide caretSide,
                                  const QRectF &caretRect,
                                  qreal opacity) const
{
    const MdTooltipTokens tokens = MdTooltipTokens::resolve(variant);
    const MdTheme &theme = MdTheme::instance();

    if (opacity < 1.0) {
        painter->setOpacity(opacity);
    }

    // 1. The caret first: it fuses with the container behind it (Compose
    //    unions the paths; same colour, same effect).
    if (caretSide != MdTooltipCaretSide::None) {
        const QPainterPath triangle = caretPath(caretRect, caretSide);
        const QColor containerColour = theme.color(variant == MdTooltipVariant::Plain
                                                       ? tokens.plainContainerColor
                                                       : tokens.richContainerColor);
        painter->fillPath(triangle, containerColour);
    }

    // 2. Shadow (rich: level 2) then the container.
    if (variant == MdTooltipVariant::Rich) {
        MdElevation::drawShadow(painter, containerRect, MdShape::radius(tokens.richContainerShape),
                                tokens.richContainerElevation,
                                theme.color(tokens.richContainerShadowColor));
    }
    const QPainterPath container = MdShape::roundedRect(
        containerRect, variant == MdTooltipVariant::Plain ? tokens.plainContainerShape
                                                          : tokens.richContainerShape);
    painter->save();
    painter->setClipPath(container);
    painter->fillPath(container, theme.color(variant == MdTooltipVariant::Plain
                                                 ? tokens.plainContainerColor
                                                 : tokens.richContainerColor));

    if (variant == MdTooltipVariant::Plain) {
        const PlainLayout layout =
            plainLayout(containerRect, text, MdTypeScale::font(tokens.plainTextStyle), tokens);
        painter->setFont(MdTypeScale::font(tokens.plainTextStyle));
        painter->setPen(theme.color(tokens.plainTextColor));
        painter->drawText(layout.textRect, MdStyleBase::leadingAlignment(nullptr) | Qt::AlignTop, text);
    } else {
        const RichLayout layout = richLayout(
            containerRect, title, text, actionLabel, MdTypeScale::font(tokens.richSubheadStyle),
            MdTypeScale::font(tokens.richTextStyle),
            MdTypeScale::font(tokens.richActionLabelStyle), tokens);
        if (layout.subheadRect.isValid()) {
            painter->setFont(MdTypeScale::font(tokens.richSubheadStyle));
            painter->setPen(theme.color(tokens.richSubheadColor));
            painter->drawText(layout.subheadRect, MdStyleBase::leadingAlignment(nullptr) | Qt::AlignTop, title);
        }
        painter->setFont(MdTypeScale::font(tokens.richTextStyle));
        painter->setPen(theme.color(tokens.richTextColor));
        painter->drawText(layout.textRect, MdStyleBase::leadingAlignment(nullptr) | Qt::AlignTop, text);

        if (layout.actionRect.isValid()) {
            const QPainterPath actionClip = MdShape::roundedRect(
                layout.actionHitRect.isValid() ? layout.actionHitRect
                                               : layout.actionRect.adjusted(-12.0, -12.0, 12.0, 12.0),
                ShapeCorner::Full);
            const MdTooltipActionRow row = tokens.actionRow(actionState);
            const QColor overlay = rowOverlay(theme, row);
            if (overlay.isValid()) {
                painter->fillPath(actionClip, overlay);
            }
            if (actionRipple.valid) {
                MdRipple::paint(painter, actionRipple, actionClip, theme.color(row.stateLayer));
            }
            painter->setFont(MdTypeScale::font(tokens.richActionLabelStyle));
            painter->setPen(theme.color(row.content));
            painter->drawText(layout.actionRect, Qt::AlignCenter, actionLabel);
        }
    }

    Q_UNUSED(actionHasKeyboardFocus);
    painter->restore();
}

void MdTooltipStyle::onThemeUpdate() {}

} // namespace md
