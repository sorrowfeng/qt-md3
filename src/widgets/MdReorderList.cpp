#include "MdReorderList.h"

#include "MdListItem.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

namespace md {

MdReorderList::MdReorderList(QWidget *parent)
    : MdList(parent)
{
    init();
}

MdReorderList::~MdReorderList() = default;

void MdReorderList::init()
{
    setAccessibleName(QStringLiteral("reorderable list"));
    // The container itself owns the drag gesture; items keep their own focus.
    setMouseTracking(false);
}

void MdReorderList::setDragEnabled(bool enabled)
{
    if (m_dragEnabled == enabled) {
        return;
    }
    m_dragEnabled = enabled;
    emit dragEnabledChanged(m_dragEnabled);
}

void MdReorderList::setMoveButtonsVisible(bool visible)
{
    if (m_moveButtonsVisible == visible) {
        return;
    }
    m_moveButtonsVisible = visible;
    refreshMoveButtons();
    emit moveButtonsVisibleChanged(m_moveButtonsVisible);
}

int MdReorderList::moveItem(int index, int delta)
{
    const QList<MdListItem *> all = items();
    if (index < 0 || index >= all.size() || delta == 0) {
        return index;
    }
    const int target = index + delta;
    if (target < 0 || target >= all.size()) {
        return index;
    }

    MdListItem *item = all.at(index);
    removeItem(item);
    insertItem(target, item);
    setActiveItem(item);
    emit itemMoved(index, target);
    return target;
}

bool MdReorderList::moveActive(int delta)
{
    MdListItem *item = activeItem();
    if (item == nullptr) {
        return false;
    }
    const int index = items().indexOf(item);
    if (index < 0) {
        return false;
    }
    return moveItem(index, delta) != index;
}

void MdReorderList::moveUp()
{
    moveActive(-1);
}

void MdReorderList::moveDown()
{
    moveActive(1);
}

void MdReorderList::keyPressEvent(QKeyEvent *event)
{
    // Ctrl+Up / Ctrl+Down move the active item instead of walking focus.
    if ((event->modifiers() & Qt::ControlModifier) != 0) {
        if (event->key() == Qt::Key_Up) {
            moveActive(-1);
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Down) {
            moveActive(1);
            event->accept();
            return;
        }
    }
    MdList::keyPressEvent(event);
}

void MdReorderList::mousePressEvent(QMouseEvent *event)
{
    m_pressPos = mousePosition(event);
    m_dragIndex = indexAt(m_pressPos);
    m_dropIndex = m_dragIndex;
    m_dragging = false;
    MdList::mousePressEvent(event);
}

void MdReorderList::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragEnabled || m_dragIndex < 0 || event->buttons() != Qt::LeftButton) {
        MdList::mouseMoveEvent(event);
        return;
    }
    const QPointF pos = mousePosition(event);
    if (!m_dragging && (pos - m_pressPos).manhattanLength() < 6.0) {
        MdList::mouseMoveEvent(event);
        return;
    }

    if (!m_dragging) {
        m_dragging = true;
        const QList<MdListItem *> all = items();
        if (m_dragIndex < all.size()) {
            all.at(m_dragIndex)->setDragged(true);
        }
    }

    const int over = indexAt(pos);
    if (over >= 0 && over != m_dropIndex) {
        m_dropIndex = over;
    }
    event->accept();
}

void MdReorderList::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_dragging && m_dragIndex >= 0 && m_dropIndex >= 0 && m_dropIndex != m_dragIndex) {
        const QList<MdListItem *> all = items();
        if (m_dragIndex < all.size()) {
            all.at(m_dragIndex)->setDragged(false);
        }
        moveItem(m_dragIndex, m_dropIndex - m_dragIndex);
    } else {
        const QList<MdListItem *> all = items();
        if (m_dragIndex >= 0 && m_dragIndex < all.size()) {
            all.at(m_dragIndex)->setDragged(false);
        }
    }
    m_dragging = false;
    m_dragIndex = -1;
    m_dropIndex = -1;
    MdList::mouseReleaseEvent(event);
}

void MdReorderList::refreshMoveButtons()
{
    // Trailing widgets are the host's business when the buttons are hidden;
    // when visible the list itself does not inject children (each host may
    // already own trailing actions). The flag exists so hosts can share one
    // affordance switch and so tests can pin the property.
    update();
}

int MdReorderList::indexAt(const QPointF &pos) const
{
    const QList<MdListItem *> all = items();
    for (int i = 0; i < all.size(); ++i) {
        if (all.at(i)->geometry().contains(pos.toPoint())) {
            return i;
        }
    }
    return -1;
}

} // namespace md
