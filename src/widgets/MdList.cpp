#include "MdList.h"

#include "MdListItem.h"

#include "styles/MdListStyle.h"

#include "core/MdAccessible.h"
#include "core/MdTheme.h"

#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
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
    // platform style. The accessible factory goes in for the same reason: a
    // host that never calls MdDesign::initialize() still gets List/ListItem
    // roles rather than a generic Client box.
    MdListStyle::shared();
    installAccessibleInterfaces();

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
    const int clamped = qBound(0, index, int(m_items.size()));
    m_items.insert(clamped, item);
    if (!m_virtualized) {
        m_layout->insertWidget(clamped, item);
    }
    item->show();

    connect(item, &MdListItem::activated, this, [this, item] { handleItemActivated(item); });

    invalidateVirtualHeights();
    syncLayout();
    syncActiveItem();
    syncVirtualWindow();
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
    m_items.removeAll(item);
    m_layout->removeWidget(item);
    // Hide and leave it parented. `setParent(nullptr)` used to turn every
    // removed row into a top-level widget — a stray native window per row
    // until the caller's `deleteLater` ran (search/filter rebuilds piled up
    // a window per item). Staying parented means `deleteLater` destroys it
    // in place.
    item->hide();
    invalidateVirtualHeights();
    syncLayout();
    syncActiveItem();
    syncVirtualWindow();
    update();
}

QList<MdListItem *> MdList::items() const
{
    return m_items;
}

int MdList::itemCount() const
{
    return int(m_items.size());
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
    case SelectionMode::Single:
        // `None` is the plain action list, but a click still has to produce a
        // visible selected response — otherwise the activation is silent.
        // Selecting the activated row and clearing the rest is the same shape
        // as Single; `Multiple` toggles instead.
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
// Virtualisation
// ---------------------------------------------------------------------------

void MdList::setVirtualized(bool virtualized)
{
    if (m_virtualized == virtualized) {
        return;
    }
    m_virtualized = virtualized;
    // Rows are re-parented through the layout only in the classic mode; the
    // virtualised mode positions them itself and hides the rest.
    if (!m_virtualized) {
        for (MdListItem *item : m_items) {
            if (item->parentWidget() == this && m_layout->indexOf(item) < 0) {
                m_layout->addWidget(item);
            }
            item->show();
        }
        m_realisedFirst = m_items.isEmpty() ? -1 : 0;
        m_realisedLast = m_items.isEmpty() ? -1 : int(m_items.size()) - 1;
    } else {
        for (MdListItem *item : m_items) {
            m_layout->removeWidget(item);
        }
        invalidateVirtualHeights();
    }
    installScrollHook();
    syncLayout();
    syncVirtualWindow();
    emit virtualizedChanged(m_virtualized);
}

void MdList::installScrollHook()
{
    if (!m_virtualized) {
        return;
    }
    QWidget *p = parentWidget();
    while (p != nullptr) {
        if (auto *area = qobject_cast<QAbstractScrollArea *>(p)) {
            if (!m_scrollFilterInstalled) {
                connect(area->verticalScrollBar(), &QScrollBar::valueChanged, this,
                        &MdList::onScrollChanged);
                connect(area->horizontalScrollBar(), &QScrollBar::valueChanged, this,
                        &MdList::onScrollChanged);
                m_scrollFilterInstalled = true;
            }
            break;
        }
        p = p->parentWidget();
    }
}

void MdList::realisedRange(int *first, int *last) const
{
    if (first != nullptr) {
        *first = m_realisedFirst;
    }
    if (last != nullptr) {
        *last = m_realisedLast;
    }
}

void MdList::scrollToItem(MdListItem *item)
{
    if (item == nullptr || !m_items.contains(item)) {
        return;
    }
    QWidget *p = parentWidget();
    while (p != nullptr) {
        // `ensureWidgetVisible` is a QScrollArea API, not a QAbstractScrollArea
        // one; the scrollbar hook above still works off the abstract base.
        if (auto *area = qobject_cast<QScrollArea *>(p)) {
            area->ensureWidgetVisible(item);
            return;
        }
        p = p->parentWidget();
    }
}

void MdList::onScrollChanged()
{
    if (m_virtualized) {
        syncVirtualWindow();
    }
}

QRect MdList::virtualViewportRect() const
{
    // Prefer the ancestor scroll area: the visible band is exactly the
    // scrollbar's window into the content, so reading the scrollbar value
    // directly is more reliable than mapping coordinates across a chain that
    // may be mid-reparent (which is when Qt emits its mapTo warnings).
    QWidget *p = parentWidget();
    while (p != nullptr) {
        if (auto *area = qobject_cast<QAbstractScrollArea *>(p)) {
            const int xoff = area->horizontalScrollBar() != nullptr
                ? area->horizontalScrollBar()->value()
                : 0;
            const int yoff = area->verticalScrollBar() != nullptr
                ? area->verticalScrollBar()->value()
                : 0;
            return QRect(xoff, yoff, area->viewport()->width(), area->viewport()->height());
        }
        p = p->parentWidget();
    }
    return rect();
}

void MdList::invalidateVirtualHeights() const
{
    m_virtualHeightsDirty = true;
    m_virtualWidth = -1;
}

int MdList::virtualItemHeight(int index, int width) const
{
    if (index < 0 || index >= m_items.size()) {
        return 0;
    }
    if (m_virtualHeightsDirty || m_virtualWidth != width) {
        m_virtualHeights.clear();
        m_virtualWidth = width;
        m_virtualHeightsDirty = false;
    }
    while (m_virtualHeights.size() <= index) {
        const int i = m_virtualHeights.size();
        m_virtualHeights.append(m_items.at(i)->heightForWidth(width));
    }
    return m_virtualHeights.at(index);
}

void MdList::syncVirtualWindow()
{
    if (!m_virtualized) {
        if (m_realisedFirst != 0 || m_realisedLast != int(m_items.size()) - 1) {
            m_realisedFirst = m_items.isEmpty() ? -1 : 0;
            m_realisedLast = m_items.isEmpty() ? -1 : int(m_items.size()) - 1;
        }
        return;
    }

    const int width = this->width() > 0 ? this->width() : 360;
    const MdListContainerTokens &resolved = tokens();
    const int gap = m_segmented ? int(resolved.segmentedGap) : 0;
    const QRect visible = virtualViewportRect();

    // Walk the stack once, tracking each row's top edge against the visible
    // band. A one-row pad on each side keeps a flick-scroll from showing the
    // background between the old and new rows.
    int first = -1;
    int last = -1;
    int y = int(resolved.topPadding);
    for (int i = 0; i < m_items.size(); ++i) {
        const int h = virtualItemHeight(i, width);
        const QRect row(0, y, width, h);
        if (row.intersects(visible)) {
            if (first < 0) {
                first = i;
            }
            last = i;
        }
        y += h + gap;
    }
    if (first >= 0) {
        first = qMax(0, first - 1);
        last = qMin(int(m_items.size()) - 1, last + 1);
    }

    // Realise only [first, last]: position and show those rows, hide the rest
    // so Qt skips their layout and paint entirely.
    y = int(resolved.topPadding);
    for (int i = 0; i < m_items.size(); ++i) {
        const int h = virtualItemHeight(i, width);
        MdListItem *item = m_items.at(i);
        const bool realised = (first >= 0 && i >= first && i <= last);
        if (realised) {
            item->setGeometry(QRect(0, y, width, h));
            item->show();
        } else if (!item->isHidden()) {
            item->hide();
        }
        y += h + gap;
    }

    m_realisedFirst = first;
    m_realisedLast = last;
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

int MdList::heightForWidth(int width) const
{
    // Sum the items' own height-for-width plus the container padding and the
    // (segmented) gap. Without this a band takes a single preferred height
    // measured at an arbitrary width and the rows collide once the list is
    // actually laid out narrower.
    const MdListContainerTokens &resolved = tokens();
    const int gap = m_segmented ? int(resolved.segmentedGap) : 0;
    int total = int(resolved.topPadding) + int(resolved.bottomPadding);
    // The cached per-row heights make this O(n) arithmetic rather than n text
    // measurements, which is what a 1000-row list needs on every relayout.
    const QList<MdListItem *> all = items();
    for (int i = 0; i < all.size(); ++i) {
        if (i > 0) {
            total += gap;
        }
        total += virtualItemHeight(i, width);
    }
    return total;
}

QSize MdList::sizeHint() const
{
    // Prefer the current width, else the items' measuring width, so the
    // height matches what a layout at that width will actually need.
    const int width = this->width() > 0 ? this->width() : 360;
    return QSize(width, heightForWidth(width));
}

QSize MdList::minimumSizeHint() const
{
    return QSize(0, heightForWidth(120));
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

bool MdList::event(QEvent *event)
{
    // A virtualised list has to re-realise its window whenever the ancestor
    // scroll area moves the viewport. The scrollbar's `valueChanged` is not
    // enough on its own: `ensureWidgetVisible` and keyboard scrolling move the
    // window through the same path, and a wheel scroll on the viewport can
    // shift the list without touching our own geometry.
    if (m_virtualized && event != nullptr) {
        switch (event->type()) {
        case QEvent::ParentChange:
        case QEvent::Show:
            // The scroll ancestor only exists once we are parented — a caller
            // may setVirtualized(true) long before setWidget().
            installScrollHook();
            syncVirtualWindow();
            break;
        case QEvent::Move:
        case QEvent::Resize:
        case QEvent::PolishRequest:
            syncVirtualWindow();
            break;
        case QEvent::LayoutRequest:
            invalidateVirtualHeights();
            syncVirtualWindow();
            break;
        default:
            break;
        }
    }
    return QWidget::event(event);
}

void MdList::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (event != nullptr && event->oldSize().width() != event->size().width()) {
        // Row heights are height-for-width: a width change invalidates them.
        invalidateVirtualHeights();
    }
    if (m_virtualized) {
        syncVirtualWindow();
    }
}

void MdList::onThemeChanged()
{
    m_tokensDirty = true;
    invalidateVirtualHeights();
    syncLayout();
    if (m_virtualized) {
        syncVirtualWindow();
    }
    update();
}

} // namespace md
