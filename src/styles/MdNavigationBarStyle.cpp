#include "MdNavigationBarStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationBarItem.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>

namespace md {

MdNavigationBarStyle::MdNavigationBarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdNavigationBarStyle *MdNavigationBarStyle::shared()
{
    static QMutex mutex;
    static MdNavigationBarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdNavigationBarStyle;
        installPaintFilter<MdNavigationBar>(instance);
    }
    return instance;
}

bool MdNavigationBarStyle::isInstalled()
{
    return hasPaintFilter(&MdNavigationBar::staticMetaObject);
}

MdNavigationBarStyle::Layout MdNavigationBarStyle::layoutFor(const MdNavigationBar &bar,
                                                             const MdNavigationBarTokens &tokens)
{
    Layout layout;

    const QRectF rect(bar.rect());
    layout.container = rect;
    layout.radii = MdShape::resolvedRadii(
        tokens.forVariant(bar.variant()).containerShape, rect.size());

    const int count = int(bar.items().size());
    if (count <= 0) {
        return layout;
    }

    const qreal gap = count > 1 ? tokens.itemBetweenSpace : 0.0;
    const qreal totalGap = gap * qreal(count - 1);

    // `Centered`: the run takes a fixed *fraction* of the bar and is centred in
    // it. Compose's `((100 - 10 * (itemsCount + 3)) / 2) / 100` is 20 % a side
    // at three items, 15 % at four, 10 % at five, and 0 from seven on — it is
    // clamped rather than allowed to go negative and invert the row.
    qreal bandX = rect.left();
    qreal bandWidth = rect.width();
    if (bar.arrangement() == MdNavigationBarArrangement::Centered) {
        const qreal sideFraction = qMax<qreal>(0.0, (70.0 - 10.0 * qreal(count)) / 200.0);
        const qreal side = sideFraction * rect.width();
        bandX += side;
        bandWidth = qMax<qreal>(0.0, rect.width() - 2.0 * side);
    }

    // Items share the band equally in **both** arrangements; what changes is the
    // band. The gaps come out of the band before it is divided, so the run ends
    // exactly on the band's trailing edge instead of overshooting it.
    const qreal each = qMax<qreal>(0.0, (bandWidth - totalGap) / qreal(count));

    layout.itemBoxes.reserve(count);
    qreal x = bandX;
    for (int i = 0; i < count; ++i) {
        layout.itemBoxes.append(QRectF(x, rect.top(), each, rect.height()));
        x += each + gap;
    }

    return layout;
}

void MdNavigationBarStyle::paintNavigationBar(QPainter &painter, const MdNavigationBar &bar,
                                              const MdNavigationBarTokens &tokens,
                                              const Layout &layout)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.forVariant(bar.variant()).containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
    painter.restore();

    // `container.elevation` (level 2 in both families) is carried and not
    // painted — see the header note. Unlike the docked toolbar, which publishes
    // no elevation row at all, this is a deliberate omission of a published row,
    // so it is recorded in docs/porting-todo.md rather than left implicit.
}

void MdNavigationBarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *bar = qobject_cast<MdNavigationBar *>(widget);
    if (painter == nullptr || bar == nullptr) {
        return;
    }
    const MdNavigationBarTokens &tokens = bar->tokens();
    paintNavigationBar(*painter, *bar, tokens, layoutFor(*bar, tokens));
}

} // namespace md
