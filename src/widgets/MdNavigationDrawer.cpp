#include "MdNavigationDrawer.h"

#include "styles/MdNavigationDrawerStyle.h"
#include "widgets/MdNavigationDrawerItem.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdNavigationDrawer::MdNavigationDrawer(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNavigationDrawer::~MdNavigationDrawer() = default;

void MdNavigationDrawer::init()
{
    MdNavigationDrawerStyle::shared();

    // The destinations are the interactive parts; the sheet itself is not a
    // Tab stop.
    setFocusPolicy(Qt::NoFocus);
    // The width is the container's own token; the height belongs to the
    // parent (`container-height: 100%`).
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    MdStyleBase::connectThemeUpdate(this, &MdNavigationDrawer::onThemeChanged);
    placeChildren();
}

const MdNavigationDrawerTokens &MdNavigationDrawer::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdNavigationDrawerTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdNavigationDrawer::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

void MdNavigationDrawer::applyToItem(MdNavigationDrawerItem *item)
{
    // The family's item rows are pushed, not re-resolved — the bar's rule.
    // The item rows are the same for both container groups (one file).
    item->setItemTokens(tokens().forVariant(m_variant).item);

    connect(item, &MdNavigationDrawerItem::selectedChanged, this,
            [this, item](bool selected) {
                if (m_updatingSelection) {
                    return;
                }
                if (selected) {
                    setCurrentIndex(indexOf(item));
                } else if (itemAt(m_currentIndex) == item) {
                    setCurrentIndex(-1);
                }
            });
}

void MdNavigationDrawer::applyToAllItems()
{
    for (MdNavigationDrawerItem *item : m_items) {
        applyToItem(item);
    }
}

void MdNavigationDrawer::addItem(MdNavigationDrawerItem *item)
{
    insertItem(int(m_items.size()), item);
}

void MdNavigationDrawer::insertItem(int index, MdNavigationDrawerItem *item)
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
        setCurrentIndex(at);
    } else if (at <= m_currentIndex) {
        m_currentIndex += 1;
    }

    placeChildren();
    update();
    emit itemsChanged();
}

void MdNavigationDrawer::removeItem(MdNavigationDrawerItem *item)
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

void MdNavigationDrawer::clearItems()
{
    if (m_items.isEmpty()) {
        return;
    }
    for (MdNavigationDrawerItem *item : m_items) {
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

MdNavigationDrawerItem *MdNavigationDrawer::itemAt(int index) const
{
    if (index < 0 || index >= int(m_items.size())) {
        return nullptr;
    }
    return m_items.at(index);
}

int MdNavigationDrawer::indexOf(const MdNavigationDrawerItem *item) const
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

void MdNavigationDrawer::setCurrentIndex(int index)
{
    const int clamped = (index < 0 || index >= int(m_items.size())) ? -1 : index;
    m_currentIndex = clamped;

    // Mirror onto the items with the report-back suppressed — the bar's
    // re-entrancy guard.
    m_updatingSelection = true;
    for (int i = 0; i < m_items.size(); ++i) {
        m_items.at(i)->setSelected(i == clamped);
    }
    m_updatingSelection = false;

    emit currentIndexChanged(m_currentIndex);
}

// ---------------------------------------------------------------------------
// Variant and chrome
// ---------------------------------------------------------------------------

void MdNavigationDrawer::setVariant(MdNavigationDrawerVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    applyToAllItems();
    update();
    emit variantChanged(m_variant);
}

void MdNavigationDrawer::setHeadline(const QString &headline)
{
    if (m_headline == headline) {
        return;
    }
    m_headline = headline;
    updateGeometry();
    placeChildren();
    update();
    emit headlineChanged(m_headline);
}

void MdNavigationDrawer::setDividerVisible(bool visible)
{
    if (m_dividerVisible == visible) {
        return;
    }
    m_dividerVisible = visible;
    placeChildren();
    update();
    emit dividerVisibleChanged(m_dividerVisible);
}

ColorRole MdNavigationDrawer::scrimColor() const
{
    return tokens().forVariant(m_variant).scrimColor;
}

qreal MdNavigationDrawer::scrimOpacity() const
{
    return tokens().forVariant(m_variant).scrimOpacity;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdNavigationDrawer::sizeHint() const
{
    // The width is the container's token; the height is the content's — the
    // parent decides the real one (`container-height: 100%`).
    const MdNavigationDrawerStyle::Layout layout =
        MdNavigationDrawerStyle::layoutFor(*this, tokens());
    qreal height = 0.0;
    for (const QRectF &box : layout.itemBoxes) {
        height = qMax(height, box.bottom() + 12.0);
    }
    return QSize(int(std::ceil(tokens().forVariant(m_variant).containerWidth)),
                 int(std::ceil(height)));
}

QSize MdNavigationDrawer::minimumSizeHint() const
{
    return sizeHint();
}

void MdNavigationDrawer::placeChildren()
{
    const MdNavigationDrawerStyle::Layout layout =
        MdNavigationDrawerStyle::layoutFor(*this, tokens());

    for (int i = 0; i < m_items.size() && i < layout.itemBoxes.size(); ++i) {
        MdNavigationDrawerItem *item = m_items.at(i);
        // Direct geometry: the pill is the item, its width is the slot's, and
        // there is nothing to centre — the centring helpers would shrink the
        // row back to its content width.
        const QRectF slot = layout.itemBoxes.at(i);
        item->setGeometry(int(std::lround(slot.left())), int(std::lround(slot.top())),
                          int(std::lround(slot.width())), int(std::lround(slot.height())));
    }
}

void MdNavigationDrawer::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdNavigationDrawer::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::LayoutDirectionChange:
        // The sheet is not mirrored as a whole (the library-wide pinned gap);
        // the items' content rows and the end-pair radii re-read the
        // direction themselves.
        placeChildren();
        update();
        break;
    default:
        break;
    }
}

void MdNavigationDrawer::onThemeChanged()
{
    invalidateTokens();
    applyToAllItems();
    placeChildren();
    update();
}

} // namespace md
