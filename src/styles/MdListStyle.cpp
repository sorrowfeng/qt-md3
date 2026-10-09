#include "MdListStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdList.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

MdListStyle::MdListStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdListStyle *MdListStyle::shared()
{
    static QMutex mutex;
    static MdListStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdListStyle;
        installPaintFilter<MdList>(instance);
    }
    return instance;
}

bool MdListStyle::isInstalled()
{
    return hasPaintFilter(&MdList::staticMetaObject);
}

MdListStyle::Layout MdListStyle::layoutFor(const MdList &list, const MdListContainerTokens &tokens)
{
    Layout layout;
    layout.container = QRectF(list.rect());
    layout.radii = MdShape::resolvedRadii(tokens.shape, layout.container.size());
    return layout;
}

void MdListStyle::paintList(QPainter &painter, const MdList &list,
                            const MdListContainerTokens &tokens, const Layout &layout)
{
    Q_UNUSED(list);
    // material-web's `:host { background: container-color }` — one rounded
    // container, no state, no interaction. The items paint themselves on top.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
    painter.restore();
}

void MdListStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *list = qobject_cast<MdList *>(widget);
    if (painter == nullptr || list == nullptr) {
        return;
    }
    const MdListContainerTokens &tokens = list->tokens();
    paintList(*painter, *list, tokens, layoutFor(*list, tokens));
}

} // namespace md
