#include "MdTabsStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"

#include <cmath>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>

namespace md {

MdTabsStyle::MdTabsStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTabsStyle *MdTabsStyle::shared()
{
    static QMutex mutex;
    static MdTabsStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTabsStyle;
        installPaintFilter<MdTabs>(instance);
    }
    return instance;
}

bool MdTabsStyle::isInstalled()
{
    return hasPaintFilter(&MdTabs::staticMetaObject);
}

MdTabsStyle::Layout MdTabsStyle::layoutFor(const MdTabs &tabs, const MdTabsTokens &tokens)
{
    Layout out;
    const MdTabsVariantTokens &t = tokens.forVariant(tabs.variant());
    const int count = tabs.count();
    if (count == 0) {
        return out;
    }

    // The row height is the tallest tab's own hint — Compose folds every
    // tab's `maxIntrinsicHeight` the same way, and a row mixing text-only and
    // icon+label tabs takes the 64.
    qreal rowHeight = 0.0;
    for (int i = 0; i < count; ++i) {
        rowHeight = qMax(rowHeight, qreal(tabs.tabAt(i)->sizeHint().height()));
    }
    out.rowHeight = rowHeight;

    if (tabs.layout() == MdTabsLayout::Fixed) {
        // Compose's `TabRowImpl`: every tab measures to the equal share of the
        // row's width.
        const qreal rowWidth = qMax<qreal>(qreal(tabs.width()), 0.0);
        const qreal tabWidth = rowWidth / qreal(count);
        for (int i = 0; i < count; ++i) {
            out.tabRects.append(QRectF(qreal(i) * tabWidth, 0.0, tabWidth, rowHeight));
        }
        out.contentWidth = rowWidth;
        return out;
    }

    // Scrollable: each tab takes its own content width from the family's 90 px
    // minimum up, and the run sits `edgePadding` in from each end — Compose's
    // `ScrollableTabRowImpl`.
    qreal x = t.scrollableEdgePadding;
    for (int i = 0; i < count; ++i) {
        const qreal tabWidth =
            qMax<qreal>(t.scrollableMinTabWidth, qreal(tabs.tabAt(i)->sizeHint().width()));
        out.tabRects.append(QRectF(x, 0.0, tabWidth, rowHeight));
        x += tabWidth;
    }
    out.contentWidth = x + t.scrollableEdgePadding;
    return out;
}

QRectF MdTabsStyle::indicatorRectFor(const Layout &layout, int index,
                                     const MdTabsVariantTokens &tokens, qreal contentWidth)
{
    if (index < 0 || index >= layout.tabRects.size()) {
        return QRectF();
    }
    const QRectF rect = layout.tabRects.at(index);

    // Both families centre the indicator in the tab. Compose's scrollable
    // implementation does this explicitly; its fixed implementation omits the
    // step, where Flutter's M3 defaults — generated from the same token
    // database — centre both. The centring wins; see porting-todo.md.
    if (tokens.activeIndicatorTopRounded) {
        // The primary family: the indicator follows the tab's *content* width.
        const qreal width = qBound<qreal>(tokens.indicatorMinimumWidth, contentWidth,
                                          rect.width());
        return QRectF(rect.x() + (rect.width() - width) / 2.0,
                      layout.rowHeight - tokens.activeIndicatorHeight, width,
                      tokens.activeIndicatorHeight);
    }

    // The secondary family: the indicator spans the whole tab, square.
    return QRectF(rect.x(), layout.rowHeight - tokens.activeIndicatorHeight, rect.width(),
                  tokens.activeIndicatorHeight);
}

void MdTabsStyle::paintTabs(QPainter &painter, const MdTabs &tabs, const MdTabsTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const MdTabsVariantTokens &t = tokens.forVariant(tabs.variant());
    const qreal w = qreal(tabs.width());
    const qreal h = qreal(tabs.height());
    const QRectF container(0.0, 0.0, w, h);

    // 1. The container: flat `surface` at level0 with no shadow (the row's
    //    elevation row is level0 in both families).
    const QColor containerColour = MdTheme::instance().color(t.containerColor);
    if (containerColour.isValid()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(containerColour);
        painter.drawRect(container);
    }

    // 2. The divider, deprecated but published and still painted upstream.
    if (t.dividerHeight > 0.0 && h > t.dividerHeight) {
        const QColor dividerColour = MdTheme::instance().color(t.dividerColor);
        if (dividerColour.isValid()) {
            painter.setBrush(dividerColour);
            painter.drawRect(QRectF(0.0, h - t.dividerHeight, w, t.dividerHeight));
        }
    }

    // 3. The indicator, in content coordinates offset by the scroll. Primary
    //    is rounded on its top corners (`3px 3px 0px 0px`); secondary is
    //    square.
    const QRectF indicator(0.0, 0.0, tabs.indicatorWidth(), t.activeIndicatorHeight);
    if (indicator.width() > 0.0 && tabs.currentIndex() >= 0) {
        QRectF drawn = indicator;
        drawn.moveLeft(tabs.indicatorOffset() - tabs.scrollOffset());
        drawn.moveTop(h - t.activeIndicatorHeight - t.dividerHeight);

        const QColor indicatorColour = MdTheme::instance().color(t.activeIndicatorColor);
        if (indicatorColour.isValid()) {
            painter.setBrush(indicatorColour);
            if (t.activeIndicatorTopRounded) {
                const QList<qreal> radii = {t.activeIndicatorHeight, t.activeIndicatorHeight, 0.0,
                                            0.0};
                painter.drawPath(MdShape::roundedRect(drawn, radii));
            } else {
                painter.drawRect(drawn);
            }
        }
    }

    painter.restore();
}

void MdTabsStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *tabs = qobject_cast<MdTabs *>(widget);
    if (painter == nullptr || tabs == nullptr) {
        return;
    }
    paintTabs(*painter, *tabs, tabs->tokens());
}

} // namespace md
