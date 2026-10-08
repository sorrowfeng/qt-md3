#include "MdSnackbarStyle.h"

#include "widgets/MdSnackbar.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdRipple.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtGui/QTextLayout>
#include <QtWidgets/QWidget>

#include <cmath>

namespace md {

namespace {

/// The action label's interactive region grows by this much around the text —
/// the text-button chrome Compose composes the label into, not a published
/// row (label height + 24 is the 40 px text-button container for label-large
/// anyway).
constexpr qreal kActionHitPadding = 12.0;

/// The dismiss icon's interactive region: the 40 px icon-button container
/// Compose places the 24 px glyph in (md.comp.icon-button.medium).
constexpr qreal kDismissHitSize = 40.0;

/// Lay the message out with wrapping, returning the block rect sized to the
/// text (position not yet applied) and the line count.
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

/// The state-layer overlay for one element row: the row's colour at the row's
/// opacity (the export publishes the opacity per state; Enabled carries none).
QColor rowOverlay(const MdTheme &theme, const MdSnackbarElementRow &row)
{
    if (row.stateLayer == ColorRole::Count || row.stateLayerOpacity <= 0.0) {
        return QColor();
    }
    QColor colour = theme.color(row.stateLayer);
    colour.setAlphaF(row.stateLayerOpacity);
    return colour;
}

} // namespace

MdSnackbarStyle::MdSnackbarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSnackbarStyle *MdSnackbarStyle::shared()
{
    static QMutex mutex;
    static MdSnackbarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdSnackbarStyle;
        installPaintFilter<MdSnackbar>(instance);
    }
    return instance;
}

bool MdSnackbarStyle::isInstalled()
{
    return hasPaintFilter(&MdSnackbar::staticMetaObject);
}

MdSnackbarStyle::OneRowLayout MdSnackbarStyle::oneRowLayout(
    const QRectF &bounds,
    const QString &message,
    const QString &actionLabel,
    bool withDismissAction,
    const QFont &supportingFont,
    const QFont &actionFont,
    qreal iconSize,
    const MdSnackbarTokens &tokens)
{
    OneRowLayout layout;

    // Compose OneRowSnackbar measure policy, in order:
    //   container width = min(maxWidth, 600)            (the widget's bounds)
    //   dismiss at the right edge, action left of it
    //   extra text-end spacing 8 when no dismiss follows the text
    //   text max width = width - action - dismiss - extra
    //   height = max(48, content) — or the wrapped rules below
    const QFontMetricsF actionMetrics(actionFont);
    const qreal labelWidth =
        actionLabel.isEmpty() ? 0.0 : actionMetrics.horizontalAdvance(actionLabel);
    // The hit region plays the role of Compose's TextButton bounds: the
    // label sits centred inside it.
    const qreal actionWidth =
        labelWidth > 0.0 ? labelWidth + kActionHitPadding * 2.0 : 0.0;
    const qreal dismissWidth = withDismissAction ? kDismissHitSize : 0.0;
    // The container's end padding: nothing with a dismiss icon (the icon
    // button chrome provides the inset), the extra text-end spacing without.
    const qreal rowRight = bounds.right() - (withDismissAction ? 0.0
                                                               : tokens.kTextEndExtraSpacing);
    const qreal extraSpacing = withDismissAction ? 0.0 : tokens.kTextEndExtraSpacing;

    // The dismiss icon: 24 px glyph centred in its 40 px chrome, flush right.
    const qreal centerY = bounds.top() + bounds.height() / 2.0;
    if (withDismissAction) {
        layout.dismissRect = QRectF(rowRight - kDismissHitSize + (kDismissHitSize - iconSize) / 2.0,
                                    centerY - iconSize / 2.0, iconSize, iconSize);
        layout.dismissHitRect =
            QRectF(rowRight - kDismissHitSize, centerY - kDismissHitSize / 2.0, kDismissHitSize,
                   kDismissHitSize);
    }

    // The action label, left of the dismiss.
    if (!actionLabel.isEmpty()) {
        const qreal labelHeight = actionMetrics.height();
        const qreal hitRight = rowRight - dismissWidth;
        layout.actionHitRect =
            QRectF(hitRight - actionWidth, centerY - labelHeight / 2.0 - kActionHitPadding,
                   actionWidth, labelHeight + kActionHitPadding * 2.0);
        layout.actionRect = QRectF(hitRight - kActionHitPadding - labelWidth,
                                   centerY - labelHeight / 2.0, labelWidth, labelHeight);
    }

    // The message.
    int lines = 1;
    const qreal textMaxWidth =
        qMax<qreal>(0.0, bounds.width() - tokens.kHorizontalSpacing - actionWidth - dismissWidth
                             - extraSpacing);
    const QRectF textSize = layoutText(message, supportingFont, textMaxWidth, &lines);
    layout.oneLine = lines <= 1;

    qreal textPlaceY;
    if (layout.oneLine) {
        const qreal contentHeight = qMax(textSize.height(),
                                         qMax(layout.actionHitRect.height(),
                                              layout.dismissHitRect.height()));
        layout.containerHeight = qMax<qreal>(tokens.singleLineHeight, contentHeight);
        textPlaceY = (layout.containerHeight - textSize.height()) / 2.0;
    } else {
        // HeightToFirstLine: a wrapped message's first line sits 30 px from
        // the top; Compose subtracts the first baseline, we subtract the
        // first line's ascent, the same visual intent.
        const QFontMetricsF supportingMetrics(supportingFont);
        textPlaceY = tokens.kHeightToFirstLine - supportingMetrics.ascent();
        const qreal contentHeight = textPlaceY + textSize.height();
        layout.containerHeight = qMax<qreal>(tokens.twoLinesHeight, contentHeight);
    }

    layout.textRect = QRectF(bounds.left() + tokens.kHorizontalSpacing, bounds.top() + textPlaceY,
                             textSize.width(), textSize.height());
    return layout;
}

MdSnackbarStyle::NewLineLayout MdSnackbarStyle::newLineLayout(
    const QRectF &bounds,
    const QString &message,
    const QString &actionLabel,
    bool withDismissAction,
    const QFont &supportingFont,
    const QFont &actionFont,
    qreal iconSize,
    const MdSnackbarTokens &tokens)
{
    NewLineLayout layout;

    // Compose NewLineButtonSnackbar: a column — text (start 16, vertical 14,
    // end 16) over an end-aligned action row (bottom 4, end 8 without a
    // dismiss icon).
    int lines = 1;
    const qreal textMaxWidth = qMax<qreal>(0.0, bounds.width() - tokens.kHorizontalSpacing * 2.0);
    const QRectF textSize = layoutText(message, supportingFont, textMaxWidth, &lines);

    const qreal textTop = tokens.kTextVerticalPadding;
    layout.textRect = QRectF(bounds.left() + tokens.kHorizontalSpacing, bounds.top() + textTop,
                             textSize.width(), textSize.height());

    const QFontMetricsF actionMetrics(actionFont);
    const qreal labelWidth =
        actionLabel.isEmpty() ? 0.0 : actionMetrics.horizontalAdvance(actionLabel);
    const qreal actionWidth =
        labelWidth > 0.0 ? labelWidth + kActionHitPadding * 2.0 : 0.0;
    const qreal dismissWidth = withDismissAction ? kDismissHitSize : 0.0;
    const qreal rowEndInset = withDismissAction ? 0.0 : tokens.kHorizontalSpacingButtonSide;
    const qreal rowHeight = qMax<qreal>(kActionHitPadding * 2.0 + actionMetrics.height(),
                                        withDismissAction ? kDismissHitSize : 0.0);
    const qreal rowTop = bounds.top() + textTop + textSize.height()
        + tokens.kActionButtonBottomPadding;
    const qreal rowCenterY = rowTop + rowHeight / 2.0;
    const qreal rowRight = bounds.right() - rowEndInset;

    if (withDismissAction) {
        layout.dismissRect =
            QRectF(rowRight - dismissWidth + (kDismissHitSize - iconSize) / 2.0,
                   rowCenterY - iconSize / 2.0, iconSize, iconSize);
        layout.dismissHitRect = QRectF(rowRight - dismissWidth, rowCenterY - kDismissHitSize / 2.0,
                                       kDismissHitSize, kDismissHitSize);
    }

    if (!actionLabel.isEmpty()) {
        const qreal hitRight = rowRight - dismissWidth;
        layout.actionHitRect =
            QRectF(hitRight - actionWidth, rowCenterY - actionMetrics.height() / 2.0
                                              - kActionHitPadding,
                   actionWidth, actionMetrics.height() + kActionHitPadding * 2.0);
        layout.actionRect =
            QRectF(hitRight - kActionHitPadding - labelWidth,
                   rowCenterY - actionMetrics.height() / 2.0, labelWidth,
                   actionMetrics.height());
    }

    layout.containerHeight =
        textTop + textSize.height() + tokens.kActionButtonBottomPadding + rowHeight;
    return layout;
}

qreal MdSnackbarStyle::heightForWidth(qreal width,
                                      const QString &message,
                                      bool actionOnNewLine,
                                      const QString &actionLabel,
                                      bool withDismissAction,
                                      const MdSnackbarTokens &tokens)
{
    const QFont supportingFont = MdTypeScale::font(tokens.supportingTextStyle);
    const QFont actionFont = MdTypeScale::font(tokens.actionLabelStyle);
    const QRectF bounds(0.0, 0.0, width, 0.0);
    if (actionOnNewLine && !actionLabel.isEmpty()) {
        return newLineLayout(bounds, message, actionLabel, withDismissAction, supportingFont,
                             actionFont, tokens.iconSize, tokens)
            .containerHeight;
    }
    return oneRowLayout(bounds, message, actionLabel, withDismissAction, supportingFont,
                        actionFont, tokens.iconSize, tokens)
        .containerHeight;
}

void MdSnackbarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *snackbar = qobject_cast<MdSnackbar *>(widget);
    if (!snackbar) {
        return;
    }

    if (snackbar->paintOpacity() < 1.0) {
        painter->setOpacity(snackbar->paintOpacity());
    }
    paintSnackbar(painter, snackbar->containerRect(), snackbar->message(), snackbar->actionLabel(),
                  snackbar->hasDismissAction(), snackbar->isActionOnNewLine(),
                  snackbar->actionState(), snackbar->iconState(), snackbar->actionRippleFrame(),
                  snackbar->iconRippleFrame(), snackbar->actionHasKeyboardFocus(),
                  snackbar->dismissHasKeyboardFocus());
}

void MdSnackbarStyle::paintSnackbar(QPainter *painter,
                                    const QRectF &rect,
                                    const QString &message,
                                    const QString &actionLabel,
                                    bool withDismissAction,
                                    bool actionOnNewLine,
                                    MdSnackbarState actionState,
                                    MdSnackbarState iconState,
                                    const MdRippleFrame &actionRipple,
                                    const MdRippleFrame &iconRipple,
                                    bool actionHasKeyboardFocus,
                                    bool iconHasKeyboardFocus) const
{
    const MdSnackbarTokens tokens = MdSnackbarTokens::resolve();
    const MdTheme &theme = MdTheme::instance();

    // 1. Shadow (level 3) then the inverse-surface container. The shape row is
    //    the extra-small corner — a snackbar is deliberately almost square.
    MdElevation::drawShadow(painter, rect, MdShape::radius(tokens.containerShape),
                            tokens.containerElevation, theme.color(tokens.containerShadowColor));
    const QPainterPath container = MdShape::roundedRect(rect, tokens.containerShape);
    painter->save();
    painter->setClipPath(container);
    painter->fillPath(container, theme.color(tokens.containerColor));

    // 2. Layout and the message.
    const QFont supportingFont = MdTypeScale::font(tokens.supportingTextStyle);
    const QFont actionFont = MdTypeScale::font(tokens.actionLabelStyle);
    QRectF actionRect;
    QRectF actionHitRect;
    QRectF dismissRect;
    QRectF dismissHitRect;
    if (actionOnNewLine && !actionLabel.isEmpty()) {
        const NewLineLayout layout =
            newLineLayout(rect, message, actionLabel, withDismissAction, supportingFont,
                          actionFont, tokens.iconSize, tokens);
        painter->setFont(supportingFont);
        painter->setPen(theme.color(tokens.supportingTextColor));
        painter->drawText(layout.textRect, Qt::AlignLeft | Qt::AlignTop, message);
        actionRect = layout.actionRect;
        actionHitRect = layout.actionHitRect;
        dismissRect = layout.dismissRect;
        dismissHitRect = layout.dismissHitRect;
    } else {
        const OneRowLayout layout =
            oneRowLayout(rect, message, actionLabel, withDismissAction, supportingFont, actionFont,
                         tokens.iconSize, tokens);
        painter->setFont(supportingFont);
        painter->setPen(theme.color(tokens.supportingTextColor));
        painter->drawText(layout.textRect, Qt::AlignLeft | Qt::AlignTop, message);
        actionRect = layout.actionRect;
        actionHitRect = layout.actionHitRect;
        dismissRect = layout.dismissRect;
        dismissHitRect = layout.dismissHitRect;
    }

    // 3. The action: state layer, ripple, then the label text.
    if (actionRect.isValid()) {
        const QPainterPath actionClip =
            MdShape::roundedRect(actionHitRect.isValid() ? actionHitRect
                                                         : actionRect.adjusted(-12.0, -12.0, 12.0, 12.0),
                                 ShapeCorner::Full);
        const MdSnackbarElementRow row = tokens.actionRow(actionState);
        const QColor overlay = rowOverlay(theme, row);
        if (overlay.isValid()) {
            painter->fillPath(actionClip, overlay);
        }
        if (actionRipple.valid) {
            MdRipple::paint(painter, actionRipple, actionClip, theme.color(row.stateLayer));
        }
        painter->setFont(actionFont);
        painter->setPen(theme.color(row.content));
        painter->drawText(actionRect, Qt::AlignCenter, actionLabel);
    }

    // 4. The dismiss icon: state layer, ripple, glyph.
    if (dismissRect.isValid()) {
        const QPainterPath dismissClip =
            MdShape::roundedRect(dismissHitRect.isValid() ? dismissHitRect
                                                          : dismissRect.adjusted(-8.0, -8.0, 8.0, 8.0),
                                 ShapeCorner::Full);
        const MdSnackbarElementRow row = tokens.iconRow(iconState);
        const QColor overlay = rowOverlay(theme, row);
        if (overlay.isValid()) {
            painter->fillPath(dismissClip, overlay);
        }
        if (iconRipple.valid) {
            MdRipple::paint(painter, iconRipple, dismissClip, theme.color(row.stateLayer));
        }
        MdIcon::paint(painter, dismissRect, QStringLiteral("close"), theme.color(row.content));
    }

    Q_UNUSED(actionHasKeyboardFocus);
    Q_UNUSED(iconHasKeyboardFocus);
    painter->restore();
}

void MdSnackbarStyle::onThemeUpdate() {}

} // namespace md
