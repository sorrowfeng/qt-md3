#include "MdDividerStyle.h"

#include "core/MdTheme.h"
#include "widgets/MdDivider.h"

#include <cmath>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

MdDividerStyle::MdDividerStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdDividerStyle *MdDividerStyle::shared()
{
    static QMutex mutex;
    static MdDividerStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdDividerStyle;
        installPaintFilter<MdDivider>(instance);
    }
    return instance;
}

bool MdDividerStyle::isInstalled()
{
    return hasPaintFilter(&MdDivider::staticMetaObject);
}

MdDividerStyle::Layout MdDividerStyle::layoutFor(const MdDivider &divider,
                                                 const MdDividerTokens &tokens)
{
    Layout layout;

    const QRectF widgetRect(divider.rect());
    const qreal inset = tokens.inset;

    // The inset pads *inline* edges. Which physical edges those are depends
    // on the orientation (inline = left/right when horizontal, top/bottom
    // when vertical) and on the layout direction (an RTL layout swaps start
    // and end) — material-web spells the padding `padding-inline-*`, so the
    // logical reading is the source behaviour.
    const bool rtl = divider.layoutDirection() == Qt::RightToLeft;

    const bool padStart = divider.insetMode() == MdDivider::InsetMode::Start
                          || divider.insetMode() == MdDivider::InsetMode::Both;
    const bool padEnd = divider.insetMode() == MdDivider::InsetMode::End
                        || divider.insetMode() == MdDivider::InsetMode::Both;

    // In RTL the physical sides swap.
    const bool padLeading = rtl ? padEnd : padStart;
    const bool padTrailing = rtl ? padStart : padEnd;

    if (divider.orientation() == Qt::Horizontal) {
        // QRectF edges have no integer-style -1 offset: width() is the
        // straight subtraction of the padded insets.
        const qreal leading = padLeading ? inset : 0.0;
        const qreal trailing = padTrailing ? inset : 0.0;
        layout.line = QRectF(widgetRect.left() + leading, widgetRect.top(),
                             qMax<qreal>(widgetRect.width() - leading - trailing, 0.0),
                             widgetRect.height());
    } else {
        const qreal leading = padLeading ? inset : 0.0;
        const qreal trailing = padTrailing ? inset : 0.0;
        layout.line = QRectF(widgetRect.left(), widgetRect.top() + leading,
                             widgetRect.width(),
                             qMax<qreal>(widgetRect.height() - leading - trailing, 0.0));
    }
    return layout;
}

QRectF MdDividerStyle::snappedToDevicePixels(const QRectF &rect, qreal devicePixelRatio)
{
    if (devicePixelRatio <= 0.0) {
        return rect;
    }
    // Snap each edge outward onto the device-pixel grid: a hairline claims
    // whole pixels instead of straddling two of them at half coverage.
    const qreal scale = devicePixelRatio;
    const qreal left = std::floor(rect.left() * scale) / scale;
    const qreal top = std::floor(rect.top() * scale) / scale;
    const qreal right = std::ceil(rect.right() * scale) / scale;
    const qreal bottom = std::ceil(rect.bottom() * scale) / scale;
    return QRectF(left, top, qMax<qreal>(right - left, 0.0),
                  qMax<qreal>(bottom - top, 0.0));
}

void MdDividerStyle::paintDivider(QPainter &painter, const MdDivider &divider,
                                  const MdDividerTokens &tokens, const Layout &layout)
{
    painter.save();
    // No antialiasing: the hairline is a solid rectangle snapped onto the
    // device-pixel grid, never a half-covered blur.
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(Qt::NoPen);
    painter.setBrush(divider.effectiveColor());

    const qreal dpr = painter.device() ? painter.device()->devicePixelRatioF() : 1.0;
    painter.drawRect(snappedToDevicePixels(layout.line, dpr));

    painter.restore();
}

void MdDividerStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *divider = qobject_cast<MdDivider *>(widget);
    if (painter == nullptr || divider == nullptr) {
        return;
    }
    const MdDividerTokens &tokens = divider->tokens();
    paintDivider(*painter, *divider, tokens, layoutFor(*divider, tokens));
}

} // namespace md
