#include "MdNavigationBar.h"

#include "styles/MdChildBox.h"
#include "styles/MdNavigationBarStyle.h"
#include "widgets/MdNavigationBarItem.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdNavigationBar::MdNavigationBar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNavigationBar::~MdNavigationBar() = default;

void MdNavigationBar::init()
{
    MdNavigationBarStyle::shared();

    // The destinations are the interactive parts; the row itself is not a Tab
    // stop, exactly as a button group's frame is not.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    MdStyleBase::connectThemeUpdate(this, &MdNavigationBar::onThemeChanged);
    placeChildren();
}

const MdNavigationBarTokens &MdNavigationBar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdNavigationBarTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdNavigationBar::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

void MdNavigationBar::applyToItem(MdNavigationBarItem *item)
{
    item->setVariant(m_variant);
    item->setIconPosition(m_itemLayout);
    // The family's rows are pushed, not left to be re-resolved: an item that
    // resolved its own table would keep the old numbers after `setVariant`,
    // and the bar's own per-instance overrides would never reach it at all.
    item->setVariantTokens(tokens().forVariant(m_variant));

    connect(item, &MdNavigationBarItem::selectedChanged, this, [this, item](bool selected) {
        if (m_updatingSelection) {
            return;
        }
        if (selected) {
            setCurrentIndex(indexOf(item));
        } else if (itemAt(m_currentIndex) == item) {
            // The selected destination was deselected by hand; a navigation bar
            // has no "nothing selected" state to fall back to, but the model
            // must not lie about it either.
            setCurrentIndex(-1);
        }
    });
}

void MdNavigationBar::applyToAllItems()
{
    for (MdNavigationBarItem *item : m_items) {
        applyToItem(item);
    }
}

void MdNavigationBar::addItem(MdNavigationBarItem *item)
{
    insertItem(int(m_items.size()), item);
}

void MdNavigationBar::insertItem(int index, MdNavigationBarItem *item)
{
    if (item == nullptr || m_items.contains(item)) {
        return;
    }
    item->setParent(this);
    item->show();
    const int at = qBound(0, index, int(m_items.size()));
    m_items.insert(at, item);
    applyToItem(item);

    if (m_currentIndex < 0) {
        // A navigation bar shows one destination as selected; the first item to
        // arrive is the one, rather than leaving the component in a state it
        // cannot be published in.
        setCurrentIndex(at);
    } else if (at <= m_currentIndex) {
        m_currentIndex += 1;
    }

    placeChildren();
    update();
    emit itemsChanged();
}

void MdNavigationBar::removeItem(MdNavigationBarItem *item)
{
    const int at = indexOf(item);
    if (at < 0) {
        return;
    }
    m_items.removeAt(at);
    item->disconnect(this);
    item->hide();
    item->setParent(nullptr);

    if (m_currentIndex == at) {
        setCurrentIndex(m_items.isEmpty() ? -1 : qMin(at, int(m_items.size()) - 1));
    } else if (at < m_currentIndex) {
        m_currentIndex -= 1;
    }

    placeChildren();
    update();
    emit itemsChanged();
}

void MdNavigationBar::clearItems()
{
    if (m_items.isEmpty()) {
        return;
    }
    for (MdNavigationBarItem *item : m_items) {
        item->disconnect(this);
        item->hide();
        item->setParent(nullptr);
    }
    m_items.clear();
    m_currentIndex = -1;
    placeChildren();
    update();
    emit itemsChanged();
}

MdNavigationBarItem *MdNavigationBar::itemAt(int index) const
{
    if (index < 0 || index >= int(m_items.size())) {
        return nullptr;
    }
    return m_items.at(index);
}

int MdNavigationBar::indexOf(const MdNavigationBarItem *item) const
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i) == item) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void MdNavigationBar::setCurrentIndex(int index)
{
    const int clamped = (index < 0 || index >= int(m_items.size())) ? -1 : index;
    m_currentIndex = clamped;

    // Mirror onto the items with the report-back suppressed: each
    // `setSelected` emits `selectedChanged`, and the handler above would
    // otherwise re-enter this function once per item.
    m_updatingSelection = true;
    for (int i = 0; i < m_items.size(); ++i) {
        m_items.at(i)->setSelected(i == clamped);
    }
    m_updatingSelection = false;

    emit currentIndexChanged(m_currentIndex);
}

// ---------------------------------------------------------------------------
// Variant and arrangement
// ---------------------------------------------------------------------------

void MdNavigationBar::setVariant(MdNavigationBarVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    applyToAllItems();
    updateGeometry();
    placeChildren();
    update();
    emit variantChanged(m_variant);
}

void MdNavigationBar::setArrangement(MdNavigationBarArrangement arrangement)
{
    if (m_arrangement == arrangement) {
        return;
    }
    m_arrangement = arrangement;
    updateGeometry();
    placeChildren();
    update();
    emit arrangementChanged(m_arrangement);
}

void MdNavigationBar::setItemLayout(MdNavigationItemIconPosition layout)
{
    if (m_itemLayout == layout) {
        return;
    }
    m_itemLayout = layout;
    applyToAllItems();
    updateGeometry();
    placeChildren();
    update();
    emit itemLayoutChanged(m_itemLayout);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdNavigationBar::sizeHint() const
{
    const MdNavigationBarVariantTokens &t = tokens().forVariant(m_variant);

    // Measured as *containers*: an item reserves the focus ring inside itself
    // and is its own container, so this is its `sizeHint()` either way — but
    // going through `MdChildBox` is what keeps the rule in one place.
    qreal band = 0.0;
    for (MdNavigationBarItem *item : m_items) {
        band += MdChildBox::measure(item).containerSize().width();
    }
    if (m_items.size() > 1) {
        band += tokens().itemBetweenSpace * qreal(m_items.size() - 1);
    }

    // A centred run occupies a fraction of the bar, so the bar has to be wider
    // than the run for the run to be centred in it.
    qreal width = band;
    if (m_arrangement == MdNavigationBarArrangement::Centered && !m_items.isEmpty()) {
        const qreal sideFraction =
            qMax<qreal>(0.0, (70.0 - 10.0 * qreal(m_items.size())) / 200.0);
        const qreal share = 1.0 - 2.0 * sideFraction;
        if (share > 0.0) {
            width = band / share;
        }
    }

    return QSize(int(std::ceil(width)), int(std::ceil(t.containerHeight)));
}

QSize MdNavigationBar::minimumSizeHint() const
{
    return QSize(0, int(std::ceil(tokens().forVariant(m_variant).containerHeight)));
}

void MdNavigationBar::placeChildren()
{
    const MdNavigationBarStyle::Layout layout = MdNavigationBarStyle::layoutFor(*this, tokens());
    for (int i = 0; i < m_items.size() && i < layout.itemBoxes.size(); ++i) {
        MdNavigationBarItem *item = m_items.at(i);
        // Centring, not top-left alignment. The item is its own container, so
        // the usual button rule (`geometryOn`, hold the widget at its hint and
        // let the margin overhang) would park a 64 x 52 item in the top-left
        // corner of its 84 x 80 slot. The item is instead the one component
        // that *fills* the box it is given — its content centres inside, which
        // is what Compose's `Box(weight(1f), contentAlignment = Center)` does —
        // so it is held at the box's size and the centring is the item's own
        // `boxes()` arithmetic. See the note on `resizedGeometryOn`.
        item->setGeometry(MdChildBox::measure(item).resizedGeometryOn(layout.itemBoxes.at(i)));
    }
}

void MdNavigationBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdNavigationBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::FontChange:
        // The labels are measured with the item's own type scale, so a font
        // change moves the items' natural widths.
        updateGeometry();
        placeChildren();
        update();
        break;
    case QEvent::LayoutDirectionChange:
        // A relayout and nothing more: the row is not mirrored in RTL. See the
        // family-level note in docs/porting-todo.md.
        placeChildren();
        update();
        break;
    default:
        break;
    }
}

void MdNavigationBar::onThemeChanged()
{
    invalidateTokens();
    applyToAllItems();
    placeChildren();
    update();
}

} // namespace md
