#include "MdBottomAppBarStyle.h"

#include "MdChildBox.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdBottomAppBar.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSizeF>
#include <QtCore/QVector>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

MdBottomAppBarStyle::MdBottomAppBarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdBottomAppBarStyle *MdBottomAppBarStyle::shared()
{
    static QMutex mutex;
    static MdBottomAppBarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdBottomAppBarStyle;
        installPaintFilter<MdBottomAppBar>(instance);
    }
    return instance;
}

bool MdBottomAppBarStyle::isInstalled()
{
    return hasPaintFilter(&MdBottomAppBar::staticMetaObject);
}

MdBottomAppBarStyle::Layout MdBottomAppBarStyle::layoutFor(const MdBottomAppBar &bar,
                                                           const MdBottomAppBarTokens &tokens)
{
    Layout layout;
    const QRectF rect = QRectF(bar.rect());
    layout.container = rect;
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, rect.size());

    // `BottomAppBarDefaults.ContentPadding` is `PaddingValues(start = 4,
    // top = 4, end = 4)` — there is no bottom, which is why the band runs to
    // the container's bottom edge and the row's contents end up 2 px below the
    // true centre. See the header comment: reproduced, not corrected.
    layout.content = QRectF(rect.left() + tokens.contentLeadingSpace,
                            rect.top() + tokens.contentTopSpace,
                            qMax<qreal>(rect.width() - tokens.contentLeadingSpace
                                            - tokens.contentTrailingSpace,
                                        0.0),
                            qMax<qreal>(rect.height() - tokens.contentTopSpace, 0.0));

    QWidget *fab = bar.floatingActionButton();
    if (fab == nullptr) {
        layout.fabBox = QRectF();
        layout.actions = layout.content;
    } else {
        // The tokens place the FAB's *container*, so the strip is as wide as
        // the container plus its own leading padding — not as wide as the
        // widget, which carries the focus ring's margin on both sides.
        const QSizeF fabContainer = MdChildBox::measure(fab).containerSize();
        layout.fabBox = QRectF(layout.content.right() - tokens.fabLeadingSpace - fabContainer.width(),
                               layout.content.top() + tokens.fabTopSpace, fabContainer.width(),
                               fabContainer.height());
        layout.actions = QRectF(layout.content.left(), layout.content.top(),
                                qMax<qreal>(layout.fabBox.left() - layout.content.left(), 0.0),
                                layout.content.height());
    }

    // --- the row's own children ---------------------------------------------
    // The arrangement distributes *containers*. Measuring each child is what
    // produces the advance: a 40 px icon button is a 55 px widget, and laying
    // the row out by those 55 px would draw the icons 15 px further apart than
    // the tokens say.
    const QList<QWidget *> children = bar.widgets();
    QVector<QSizeF> sizes;
    sizes.reserve(children.size());
    qreal contentWidth = 0.0;
    for (QWidget *child : children) {
        const QSizeF size = MdChildBox::measure(child).containerSize();
        sizes.append(size);
        contentWidth += size.width();
    }

    qreal x = layout.actions.left();
    qreal gap = 0.0;
    switch (bar.arrangement()) {
    case MdBottomAppBar::Arrangement::Start:
        break;
    case MdBottomAppBar::Arrangement::Center:
        x = layout.actions.left() + (layout.actions.width() - contentWidth) / 2.0;
        break;
    case MdBottomAppBar::Arrangement::End:
        x = layout.actions.right() - contentWidth;
        break;
    case MdBottomAppBar::Arrangement::SpaceBetween:
        gap = sizes.size() > 1 ? (layout.actions.width() - contentWidth) / qreal(sizes.size() - 1)
                               : 0.0;
        break;
    case MdBottomAppBar::Arrangement::Count:
        break;
    }

    layout.childBoxes.reserve(sizes.size());
    for (int i = 0; i < sizes.size(); ++i) {
        const QSizeF size = sizes.at(i);
        // Vertically centred in the band: Compose's
        // `Row(verticalAlignment = CenterVertically)`. The band itself is 2 px
        // below the container's centre because it has no bottom padding — see
        // the header comment.
        const qreal top = layout.actions.top() + (layout.actions.height() - size.height()) / 2.0;
        layout.childBoxes.append(QRectF(x, top, size.width(), size.height()));
        x += size.width() + gap;
    }

    return layout;
}

void MdBottomAppBarStyle::paintBottomAppBar(QPainter &painter, const MdBottomAppBar &bar,
                                            const MdBottomAppBarTokens &tokens,
                                            const Layout &layout)
{
    Q_UNUSED(bar);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
    painter.restore();

    // `container.elevation` (level 2) is carried and not painted: a bottom app
    // bar's shadow falls *above* its own rect, which Qt clips away for a widget
    // laid out in place. Compose draws it because a Compose layout does not
    // clip. Recorded in docs/porting-todo.md — the same call the list family
    // made for its dragged elevation.
}

void MdBottomAppBarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *bar = qobject_cast<MdBottomAppBar *>(widget);
    if (painter == nullptr || bar == nullptr) {
        return;
    }
    const MdBottomAppBarTokens &tokens = bar->tokens();
    paintBottomAppBar(*painter, *bar, tokens, layoutFor(*bar, tokens));
}

} // namespace md
