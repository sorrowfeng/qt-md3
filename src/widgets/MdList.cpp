#include "MdList.h"

#include "MdListItem.h"

#include "styles/MdListStyle.h"

#include "core/MdTheme.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QVBoxLayout>

namespace md {

MdList::MdList(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdList::~MdList() = default;

void MdList::init()
{
    // The paint hub owns painting — registering the shared style as soon as
    // the first list exists means a list is never momentarily painted by the
    // platform style.
    MdListStyle::shared();

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(0);

    setFocusPolicy(Qt::NoFocus); // the items take focus, not the container
    syncLayout();
    MdStyleBase::connectThemeUpdate(this, &MdList::onThemeChanged);
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

void MdList::addItem(MdListItem *item)
{
    insertItem(itemCount(), item);
}

void MdList::insertItem(int index, MdListItem *item)
{
    if (item == nullptr) {
        return;
    }
    item->setParent(this);
    const int clamped = qBound(0, index, m_layout->count());
    m_layout->insertWidget(clamped, item);
    item->show();

    connect(item, &MdListItem::activated, this, [this, item] { handleItemActivated(item); });

    syncLayout();
    syncActiveItem();
    update();
}

void MdList::removeItem(MdListItem *item)
{
    if (item == nullptr) {
        return;
    }
    if (m_activeItem == item) {
        m_activeItem = nullptr;
    }
    m_layout->removeWidget(item);
    item->setParent(nullptr);
    syncLayout();
    syncActiveItem();
    update();
}

QList<MdListItem *> MdList::items() const
{
    QList<MdListItem *> result;
    for (int i = 0; i < m_layout->count(); ++i) {
        if (auto *item = qobject_cast<MdListItem *>(m_layout->itemAt(i)->widget())) {
            result.append(item);
        }
    }
    return result;
}

int MdList::itemCount() const
{
    return int(items().size());
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void MdList::setSelectionMode(SelectionMode mode)
{
    if (m_selectionMode == mode) {
        return;
    }
    m_selectionMode = mode;
    if (mode == SelectionMode::None) {
        clearSelection();
    }
    emit selectionModeChanged(m_selectionMode);
}

QList<MdListItem *> MdList::selectedItems() const
{
    QList<MdListItem *> result;
    for (MdListItem *item : items()) {
        if (item->isSelected()) {
            result.append(item);
        }
    }
    return result;
}

void MdList::clearSelection()
{
    for (MdListItem *item : items()) {
        if (item->isSelected()) {
            item->setSelected(false);
            emit selectionChanged(item, false);
        }
    }
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

bool MdList::isActivatable(const MdListItem *item)
{
    // material-web's `isItemNotDisabled`, narrowed by the item's own
    // interactivity: a `type="text"` item is not a navigation target.
    return item != nullptr && item->isEnabled() && item->isInteractive();
}

void MdList::setWrapNavigation(bool wrap)
{
    if (m_wrapNavigation == wrap) {
        return;
    }
    m_wrapNavigation = wrap;
    emit wrapNavigationChanged(m_wrapNavigation);
}

MdListItem *MdList::nextItem(MdListItem *from, int step) const
{
    if (step == 0) {
        return from;
    }
    const QList<MdListItem *> all = items();
    if (all.isEmpty()) {
        return nullptr;
    }

    int index = from != nullptr ? int(all.indexOf(from)) : (step > 0 ? -1 : 0);
    if (index < 0 && from != nullptr) {
        return nullptr; // not one of ours
    }

    for (int i = 0; i < all.size(); ++i) {
        index += step > 0 ? 1 : -1;
        if (index < 0 || index >= all.size()) {
            if (!m_wrapNavigation) {
                return from;
            }
            index = index < 0 ? all.size() - 1 : 0;
        }
        if (isActivatable(all.at(index))) {
            return all.at(index);
        }
    }
    // No activatable item at all.
    return nullptr;
}

void MdList::setActiveItem(MdListItem *item)
{
    if (m_activeItem == item) {
        return;
    }
    // The roving tab stop: exactly one item carries Qt::TabFocus-style reach.
    m_activeItem = item;
    const QList<MdListItem *> all = items();
    for (MdListItem *candidate : all) {
        if (!isActivatable(candidate)) {
            candidate->setFocusPolicy(Qt::NoFocus);
            continue;
        }
        candidate->setFocusPolicy(candidate == m_activeItem ? Qt::StrongFocus : Qt::ClickFocus);
    }
}

bool MdList::activateItem(MdListItem *item)
{
    if (!isActivatable(item)) {
        return false;
    }

    switch (m_selectionMode) {
    case SelectionMode::None:
        break;
    case SelectionMode::Single:
        for (MdListItem *other : items()) {
            if (other != item && other->isSelected()) {
                other->setSelected(false);
                emit selectionChanged(other, false);
            }
        }
        if (!item->isSelected()) {
            item->setSelected(true);
            emit selectionChanged(item, true);
        }
        break;
    case SelectionMode::Multiple:
        item->setSelected(!item->isSelected());
        emit selectionChanged(item, item->isSelected());
        break;
    }

    setActiveItem(item);
    item->setFocus(Qt::OtherFocusReason);
    emit itemActivated(item);
    return true;
}

void MdList::handleItemActivated(MdListItem *item)
{
    if (m_selectionMode == SelectionMode::None) {
        setActiveItem(item);
        emit itemActivated(item);
        return;
    }
    activateItem(item);
}

void MdList::syncActiveItem()
{
    const QList<MdListItem *> all = items();
    if (m_activeItem != nullptr && all.contains(m_activeItem) && isActivatable(m_activeItem)) {
        return;
    }
    for (MdListItem *candidate : all) {
        if (isActivatable(candidate)) {
            setActiveItem(candidate);
            return;
        }
    }
    m_activeItem = nullptr;
}

// ---------------------------------------------------------------------------
// Layout / tokens
// ---------------------------------------------------------------------------

void MdList::setSegmented(bool segmented)
{
    if (m_segmented == segmented) {
        return;
    }
    m_segmented = segmented;
    syncLayout();
    update();
    emit segmentedChanged(m_segmented);
}

void MdList::syncLayout()
{
    const MdListContainerTokens &resolved = tokens();
    // `_list.scss` `padding: 8px 0`, and the segmented gap (0 otherwise —
    // the baseline list's items touch).
    m_layout->setContentsMargins(0, int(resolved.topPadding), 0, int(resolved.bottomPadding));
    m_layout->setSpacing(m_segmented ? int(resolved.segmentedGap) : 0);

    // The segmented positions: the shape rows need index/count per item.
    const QList<MdListItem *> all = items();
    for (int i = 0; i < all.size(); ++i) {
        all.at(i)->setSegmentedPosition(m_segmented ? i : -1, m_segmented ? int(all.size()) : 0);
    }
    updateGeometry();
}

const MdListContainerTokens &MdList::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdListContainerTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdList::keyPressEvent(QKeyEvent *event)
{
    // material-web's `ListController.handleKeydown`: arrows move the active
    // item, Home/End jump to the ends, and the inline keys behave like the
    // block ones for a vertical list.
    int step = 0;
    switch (event->key()) {
    case Qt::Key_Down:
    case Qt::Key_Right:
        step = 1;
        break;
    case Qt::Key_Up:
    case Qt::Key_Left:
        step = -1;
        break;
    case Qt::Key_Home:
    case Qt::Key_End: {
        if (itemCount() == 0) {
            QWidget::keyPressEvent(event);
            return;
        }
        MdListItem *target = nullptr;
        const QList<MdListItem *> all = items();
        if (event->key() == Qt::Key_Home) {
            for (MdListItem *candidate : all) {
                if (isActivatable(candidate)) {
                    target = candidate;
                    break;
                }
            }
        } else {
            for (int i = int(all.size()) - 1; i >= 0; --i) {
                if (isActivatable(all.at(i))) {
                    target = all.at(i);
                    break;
                }
            }
        }
        if (target != nullptr) {
            setActiveItem(target);
            target->setFocus(Qt::TabFocusReason);
            event->accept();
            return;
        }
        break;
    }
    default:
        break;
    }

    if (step != 0) {
        // Navigation starts from the focused item when there is one, else
        // from the tab stop.
        MdListItem *from = m_activeItem;
        for (MdListItem *candidate : items()) {
            if (candidate->hasFocus()) {
                from = candidate;
                break;
            }
        }
        MdListItem *target = nextItem(from, step);
        if (target != nullptr) {
            setActiveItem(target);
            target->setFocus(Qt::TabFocusReason);
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void MdList::onThemeChanged()
{
    m_tokensDirty = true;
    syncLayout();
    update();
}

} // namespace md
