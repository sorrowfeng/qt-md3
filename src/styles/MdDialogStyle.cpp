#include "MdDialogStyle.h"

#include "core/MdElevation.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdDialog.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

/// The action row's height: Compose's `ButtonDefaults.MinHeight` (40 dp).
/// The dialog export publishes no height of its own — its buttons are
/// ordinary text buttons and inherit the button family's metrics.
constexpr qreal kActionRowHeight = 40.0;

QFontMetricsF metricsFor(TypeStyle style)
{
    return QFontMetricsF(MdTypeScale::font(style));
}

qreal wrappedTextHeight(const QFontMetricsF &metrics, const QString &text, qreal width)
{
    if (text.isEmpty()) {
        return 0.0;
    }
    return metrics.boundingRect(QRectF(0.0, 0.0, width, 0.0), Qt::TextWordWrap, text).height();
}

} // namespace

MdDialogStyle::MdDialogStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdDialogStyle *MdDialogStyle::shared()
{
    static QMutex mutex;
    static MdDialogStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdDialogStyle;
        installPaintFilter<MdDialog>(instance);
    }
    return instance;
}

bool MdDialogStyle::isInstalled()
{
    return hasPaintFilter(&MdDialog::staticMetaObject);
}

MdDialogStyle::Layout MdDialogStyle::layoutFor(qreal width, const ContentSpec &content,
                                               const MdDialogTokens &tokens)
{
    Layout layout;

    // The widget rect carries the shadow margin around the container (the
    // snackbar idiom): a child widget clips its own rect, so the level-3
    // shadow needs headroom the container does not own.
    layout.container = QRectF(0.0, 0.0, width, 0.0)
                          .adjusted(MdDialogTokens::kShadowMargin,
                                    MdDialogTokens::kShadowMargin,
                                    -MdDialogTokens::kShadowMargin,
                                    -MdDialogTokens::kShadowMargin);
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, layout.container.size());

    const qreal contentWidth = layout.container.width() - MdDialogTokens::kContainerPadding * 2.0;
    const QFontMetricsF headline = metricsFor(tokens.headlineStyle);
    const QFontMetricsF supporting = metricsFor(tokens.supportingTextStyle);

    // Vertical flow inside the container — Compose AlertDialogContent's
    // Column: icon (bottom 16), title (bottom 16), text (bottom 24), then
    // the end-aligned action rows at the bottom padding. `y` is in widget
    // coordinates, starting at the container's top padding.
    qreal y = layout.container.top() + MdDialogTokens::kContainerPadding;
    if (content.hasIcon) {
        layout.iconRect = QRectF(layout.container.left()
                                     + (layout.container.width() - tokens.iconSize) / 2.0,
                                 y, tokens.iconSize, tokens.iconSize);
        y += tokens.iconSize + MdDialogTokens::kIconBottomPadding;
    }
    if (!content.title.isEmpty()) {
        const qreal titleHeight = headline.height();
        layout.titleRect = QRectF(layout.container.left() + MdDialogTokens::kContainerPadding, y,
                                  contentWidth, titleHeight);
        y += titleHeight + MdDialogTokens::kTitleBottomPadding;
    }
    if (!content.text.isEmpty()) {
        const qreal textHeight = wrappedTextHeight(supporting, content.text, contentWidth);
        layout.textRect = QRectF(layout.container.left() + MdDialogTokens::kContainerPadding, y,
                                 contentWidth, textHeight);
        y += textHeight + MdDialogTokens::kTextBottomPadding;
    }

    // The action rows: Compose's AlertDialogFlowRow lays the confirm button
    // first child in a flipped (RTL) FlowRow — horizontally that puts the
    // confirm rightmost with the dismiss to its left; wrapped, the confirm
    // takes the first row and the dismiss the second, both end-aligned.
    const qreal availableRowWidth = contentWidth;
    QList<qreal> widths = content.actionWidths;
    if (!widths.isEmpty()) {
        qreal total = 0.0;
        for (const qreal w : widths) {
            total += w;
        }
        total += MdDialogTokens::kActionsSpacing * qreal(widths.size() - 1);

        if (widths.size() == 2 && total > availableRowWidth) {
            // Wrap: confirm (widths[0]) alone on the first row, dismiss on
            // the second — both flush to the end edge.
            const qreal confirmWidth = widths.first();
            const qreal dismissWidth = widths.last();
            layout.actionRows.append(
                QList<QRectF>{QRectF(layout.container.right() - MdDialogTokens::kContainerPadding
                                         - confirmWidth,
                                     y, confirmWidth, kActionRowHeight)});
            layout.actionRows.append(
                QList<QRectF>{QRectF(layout.container.right() - MdDialogTokens::kContainerPadding
                                         - dismissWidth,
                                     y + kActionRowHeight + MdDialogTokens::kActionsSpacing,
                                     dismissWidth, kActionRowHeight)});
            y += kActionRowHeight * 2.0 + MdDialogTokens::kActionsSpacing;
        } else {
            // One row, end-aligned, dismiss left of confirm.
            QList<QRectF> row;
            qreal x = layout.container.right() - MdDialogTokens::kContainerPadding;
            for (int i = widths.size() - 1; i >= 0; --i) {
                x -= widths.at(i);
                row.prepend(QRectF(x, y, widths.at(i), kActionRowHeight));
                x -= MdDialogTokens::kActionsSpacing;
            }
            layout.actionRows.append(row);
            y += kActionRowHeight;
        }
    }

    // The Column's own 24 px bottom padding closes the container.
    const qreal containerHeight = y + MdDialogTokens::kContainerPadding
        - layout.container.top();
    layout.container.setHeight(containerHeight);
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, layout.container.size());
    return layout;
}

qreal MdDialogStyle::heightForWidth(qreal width, const ContentSpec &content,
                                    const MdDialogTokens &tokens)
{
    return layoutFor(width, content, tokens).container.height()
        + MdDialogTokens::kShadowMargin * 2.0;
}

void MdDialogStyle::paintDialog(QPainter &painter, const MdDialog &dialog,
                                const MdDialogTokens &tokens, const Layout &layout,
                                qreal opacity)
{
    const MdTheme &theme = MdTheme::instance();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (opacity < 1.0) {
        painter.setOpacity(painter.opacity() * qBound(0.0, opacity, 1.0));
    }

    const QPainterPath path = MdShape::roundedRect(layout.container, layout.radii);

    // 1. Shadow — the container's level-3 elevation is static; a dialog has
    //    no interactive state rows to animate through.
    MdElevation::drawShadow(&painter, layout.container, layout.radii.isEmpty()
                                                       ? 0.0
                                                       : layout.radii.first(),
                            tokens.containerElevation, theme.color(tokens.containerShadowColor));

    // 2. Container.
    painter.setPen(Qt::NoPen);
    painter.setBrush(theme.color(tokens.containerColor));
    painter.drawPath(path);

    painter.restore();

    // Text, faded by the host's transition through the painter opacity.
    painter.save();
    if (opacity < 1.0) {
        painter.setOpacity(qBound(0.0, opacity, 1.0));
    }
    if (layout.titleRect.isValid()) {
        painter.setPen(theme.color(tokens.headlineColor));
        painter.setFont(MdTypeScale::font(tokens.headlineStyle));
        painter.drawText(layout.titleRect, Qt::AlignLeft | Qt::AlignVCenter, dialog.title());
    }
    if (layout.textRect.isValid()) {
        painter.setPen(theme.color(tokens.supportingTextColor));
        painter.setFont(MdTypeScale::font(tokens.supportingTextStyle));
        painter.drawText(layout.textRect, Qt::TextWordWrap, dialog.text());
    }
    painter.restore();
}

void MdDialogStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *dialog = qobject_cast<MdDialog *>(widget);
    if (painter == nullptr || dialog == nullptr) {
        return;
    }
    const MdDialogTokens &tokens = dialog->tokens();
    paintDialog(*painter, *dialog, tokens, dialog->currentLayout(), dialog->paintOpacity());
}

} // namespace md
