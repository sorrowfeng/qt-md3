#include "MdNavigationDrawerStyle.h"

#include "widgets/MdNavigationDrawer.h"
#include "widgets/MdNavigationDrawerItem.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"

#include <cmath>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
#include <QtGui/QPainter>

namespace md {

MdNavigationDrawerStyle::MdNavigationDrawerStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdNavigationDrawerStyle *MdNavigationDrawerStyle::shared()
{
    static QMutex mutex;
    static MdNavigationDrawerStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdNavigationDrawerStyle;
        installPaintFilter<MdNavigationDrawer>(instance);
    }
    return instance;
}

bool MdNavigationDrawerStyle::isInstalled()
{
    return hasPaintFilter(&MdNavigationDrawer::staticMetaObject);
}

MdNavigationDrawerStyle::Layout
MdNavigationDrawerStyle::layoutFor(const MdNavigationDrawer &drawer,
                                   const MdNavigationDrawerTokens &tokens)
{
    Layout layout;

    const QRectF rect(drawer.rect());
    layout.container = rect;

    const MdNavigationDrawerVariantTokens &v = tokens.forVariant(drawer.variant());

    // `corner-large-end`: the base Large radius on the **end** pair only —
    // a start-anchored sheet's leading edge stays square — mirrored when the
    // layout direction is. Radii order is TL, TR, BR, BL.
    const qreal radius = MdShape::resolvedRadius(v.containerShape, rect.size());
    const bool rtl = drawer.layoutDirection() == Qt::RightToLeft;
    layout.radii = rtl ? QList<qreal>{radius, 0.0, 0.0, radius}
                       : QList<qreal>{0.0, radius, radius, 0.0};

    const MdNavigationDrawerItemTokens &item = v.item;

    // The headline, if there is one: the behaviour's content inset (16) from
    // the start, 16 down from the top, in its own type's height. The 12 px
    // gap to what follows is Compose's sample spacing (`Spacer(12)` after the
    // headline) — the export publishes no headline-to-content row, and the
    // choice is recorded here rather than invented silently.
    qreal y = 16.0;
    if (!drawer.headline().isEmpty()) {
        const QFontMetricsF measure(MdTypeScale::font(v.headlineType, TypeEmphasis::Baseline,
                                                      MdTheme::instance().scriptCategory()));
        const qreal headlineHeight = std::ceil(measure.height());
        const qreal textLeft =
            drawer.layoutDirection() == Qt::RightToLeft ? item.contentTrailingSpace
                                                        : item.contentLeadingSpace;
        layout.headlineBox = QRectF(textLeft, y, rect.width() - textLeft - item.contentTrailingSpace,
                                    headlineHeight);
        y += headlineHeight + 12.0;

        // The divider sits under the headline when shown: a 1 px line at the
        // content inset, 4 px of air above and below (not published rows —
        // recorded with the same honesty as the 12 above).
        if (drawer.isDividerVisible()) {
            const qreal dividerLeft =
                drawer.layoutDirection() == Qt::RightToLeft ? item.contentTrailingSpace
                                                            : item.contentLeadingSpace;
            layout.dividerBox = QRectF(dividerLeft, y + 4.0,
                                       rect.width() - dividerLeft - item.contentTrailingSpace, 1.0);
            y += 4.0 + 1.0 + 12.0;
        }
    }

    // The items: full-width rows at the behaviour's `itemPadding` (12) inset,
    // stacked with no gap — the drawer's rows sit at the 56 px pitch the
    // indicator height gives them. Compose's `fillMaxWidth` items stack
    // directly in the sample column.
    const qreal width = qMax<qreal>(0.0, rect.width() - 2.0 * item.itemPadding);
    for (MdNavigationDrawerItem *drawerItem : drawer.items()) {
        // Read through `sizeHint()`: this function runs from the paint path
        // too, and measuring would resize the widgets mid-paint (the rail's
        // recorded rule).
        const qreal height = qreal(drawerItem->sizeHint().height());
        layout.itemBoxes.append(QRectF(item.itemPadding, y, width, height));
        y += height;
    }

    return layout;
}

void MdNavigationDrawerStyle::paintNavigationDrawer(QPainter &painter,
                                                    const MdNavigationDrawer &drawer,
                                                    const MdNavigationDrawerTokens &tokens,
                                                    const Layout &layout)
{
    const MdNavigationDrawerVariantTokens &v = tokens.forVariant(drawer.variant());

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(v.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));

    // The divider, when shown.
    if (!layout.dividerBox.isEmpty()) {
        painter.setBrush(MdTheme::instance().color(v.dividerColor));
        painter.drawRect(layout.dividerBox);
    }

    // The headline text.
    if (!layout.headlineBox.isEmpty() && !drawer.headline().isEmpty()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(MdTheme::instance().color(v.headlineColor));
        painter.setFont(MdTypeScale::font(v.headlineType, TypeEmphasis::Baseline,
                                          MdTheme::instance().scriptCategory()));
        painter.drawText(layout.headlineBox,
                         int(Qt::AlignVCenter | (drawer.layoutDirection() == Qt::RightToLeft
                                                     ? Qt::AlignRight
                                                     : Qt::AlignLeft)),
                         drawer.headline());
    }
    painter.restore();

    // The scrim and the modal elevation are carried and not painted — see the
    // header note and docs/porting-todo.md.
}

void MdNavigationDrawerStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *drawer = qobject_cast<MdNavigationDrawer *>(widget);
    if (painter == nullptr || drawer == nullptr) {
        return;
    }
    const MdNavigationDrawerTokens &tokens = drawer->tokens();
    paintNavigationDrawer(*painter, *drawer, tokens, layoutFor(*drawer, tokens));
}

} // namespace md
