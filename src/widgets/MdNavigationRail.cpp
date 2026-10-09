#include "MdNavigationRail.h"

#include "styles/MdChildBox.h"
#include "styles/MdNavigationRailStyle.h"
#include "widgets/MdNavigationBarItem.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdNavigationRail::MdNavigationRail(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNavigationRail::~MdNavigationRail() = default;

void MdNavigationRail::init()
{
    MdNavigationRailStyle::shared();

    // The destinations are the interactive parts; the column itself is not a
    // Tab stop, exactly as the bar's row is not.
    setFocusPolicy(Qt::NoFocus);
    // The width is the rail's own decision (a token when collapsed, its
    // content between bounds when expanded); the height belongs to the parent.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);

    m_currentWidth = tokens().forVariant(m_variant).containerWidth;

    m_widthTimer = new QTimer(this);
    m_widthTimer->setInterval(8);
    m_widthTimer->setTimerType(Qt::PreciseTimer);
    connect(m_widthTimer, &QTimer::timeout, this, &MdNavigationRail::onWidthTick);

    MdStyleBase::connectThemeUpdate(this, &MdNavigationRail::onThemeChanged);
    placeChildren();
}

const MdNavigationRailTokens &MdNavigationRail::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdNavigationRailTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdNavigationRail::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Header
// ---------------------------------------------------------------------------

void MdNavigationRail::setHeader(QWidget *header)
{
    if (m_header == header) {
        return;
    }
    if (m_header != nullptr) {
        m_header->hide();
        m_header->setParent(nullptr);
    }
    m_header = header;
    if (m_header != nullptr) {
        m_header->setParent(this);
        m_header->show();
    }
    placeChildren();
    updateGeometry();
    update();
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

void MdNavigationRail::applyToItem(MdNavigationBarItem *item)
{
    // The family's item rows are pushed, not re-resolved — the same rule as
    // the bar's. The shared item switches arrangement with the state: the
    // collapsed rail shows bare pills, the expanded one pills with labels.
    const bool itemsExpanded = m_expanded && tokens().forVariant(m_variant).supportsExpanded();
    item->setVariantTokens(tokens().forVariant(m_variant).item);
    item->setIconPosition(itemsExpanded ? MdNavigationItemIconPosition::Start
                                        : MdNavigationItemIconPosition::Top);
    item->setAlwaysShowLabel(itemsExpanded);

    connect(item, &MdNavigationBarItem::selectedChanged, this, [this, item](bool selected) {
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

void MdNavigationRail::applyToAllItems()
{
    for (MdNavigationBarItem *item : m_items) {
        applyToItem(item);
    }
}

void MdNavigationRail::addItem(MdNavigationBarItem *item)
{
    insertItem(int(m_items.size()), item);
}

void MdNavigationRail::insertItem(int index, MdNavigationBarItem *item)
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

    if (m_expanded) {
        restartWidthAnimation();
    }
    placeChildren();
    update();
    emit itemsChanged();
}

void MdNavigationRail::removeItem(MdNavigationBarItem *item)
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

    if (m_expanded) {
        restartWidthAnimation();
    }
    placeChildren();
    update();
    emit itemsChanged();
}

void MdNavigationRail::clearItems()
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

MdNavigationBarItem *MdNavigationRail::itemAt(int index) const
{
    if (index < 0 || index >= int(m_items.size())) {
        return nullptr;
    }
    return m_items.at(index);
}

int MdNavigationRail::indexOf(const MdNavigationBarItem *item) const
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

void MdNavigationRail::setCurrentIndex(int index)
{
    const int clamped = (index < 0 || index >= int(m_items.size())) ? -1 : index;
    m_currentIndex = clamped;

    // Mirror onto the items with the report-back suppressed — the bar's
    // re-entrancy guard, for the same reason.
    m_updatingSelection = true;
    for (int i = 0; i < m_items.size(); ++i) {
        m_items.at(i)->setSelected(i == clamped);
    }
    m_updatingSelection = false;

    emit currentIndexChanged(m_currentIndex);
}

// ---------------------------------------------------------------------------
// Variant and state
// ---------------------------------------------------------------------------

void MdNavigationRail::setVariant(MdNavigationRailVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    applyToAllItems();
    // A family that cannot expand collapses; the new family's collapsed width
    // becomes the answer either way.
    if (!tokens().forVariant(m_variant).supportsExpanded()) {
        m_expanded = false;
        emit expandedChanged(false);
    }
    restartWidthAnimation();
    updateGeometry();
    placeChildren();
    update();
    emit variantChanged(m_variant);
}

void MdNavigationRail::setArrangement(MdNavigationRailArrangement arrangement)
{
    if (m_arrangement == arrangement) {
        return;
    }
    m_arrangement = arrangement;
    placeChildren();
    update();
    emit arrangementChanged(m_arrangement);
}

void MdNavigationRail::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    // The baseline family publishes no expanded rows; the state is refused
    // rather than approximated.
    if (expanded && !tokens().forVariant(m_variant).supportsExpanded()) {
        return;
    }
    m_expanded = expanded;
    applyToAllItems();
    restartWidthAnimation();
    updateGeometry();
    placeChildren();
    update();
    emit expandedChanged(m_expanded);
}

void MdNavigationRail::setModal(bool modal)
{
    if (m_modal == modal) {
        return;
    }
    m_modal = modal;
    // The modal rail arrives over a scrim on the fast spring.
    if (m_widthTimer->isActive()) {
        m_widthClock.restart();
    }
    update();
    emit modalChanged(m_modal);
}

// ---------------------------------------------------------------------------
// The width spring
// ---------------------------------------------------------------------------

qreal MdNavigationRail::desiredWidth() const
{
    const MdNavigationRailVariantTokens &v = tokens().forVariant(m_variant);
    if (v.supportsExpanded() && m_expanded) {
        return MdNavigationRailStyle::expandedWidthFor(*this, tokens());
    }
    return v.containerWidth;
}

void MdNavigationRail::restartWidthAnimation()
{
    const qreal target = desiredWidth();
    if (qFuzzyCompare(m_currentWidth + 1.0, target + 1.0)) {
        m_currentWidth = target;
        return;
    }
    m_widthFrom = m_currentWidth;
    m_widthTo = target;
    m_widthClock.restart();
    m_widthTimer->start();
}

void MdNavigationRail::onWidthTick()
{
    // Compose animates the wide rail's width with `DefaultSpatial` — a shape
    // change — and the modal rail with `FastSpatial`.
    const MdSpring spring = MdMotion::spring(m_modal ? MotionSpring::SpatialFast
                                                     : MotionSpring::SpatialDefault);
    const qreal seconds = qreal(m_widthClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_currentWidth = m_widthFrom + (m_widthTo - m_widthFrom) * travelled;

    if (qFuzzyCompare(m_currentWidth + 1.0, m_widthTo + 1.0)
        || qreal(m_widthClock.elapsed()) >= spring.settlingDurationMs()) {
        m_currentWidth = m_widthTo;
        m_widthTimer->stop();
    }

    // Apply the frame. `resize` re-enters through `resizeEvent`, which is the
    // relayout; skip the double pass when the width already landed.
    if (width() != int(std::lround(m_currentWidth))) {
        resize(int(std::lround(m_currentWidth)), height());
    } else {
        placeChildren();
    }
    update();
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdNavigationRail::sizeHint() const
{
    return QSize(int(std::ceil(m_currentWidth)), 0);
}

QSize MdNavigationRail::minimumSizeHint() const
{
    return QSize(int(std::ceil(tokens().forVariant(m_variant).containerWidth)), 0);
}

void MdNavigationRail::placeChildren()
{
    const MdNavigationRailStyle::Layout layout = MdNavigationRailStyle::layoutFor(
        *this, tokens(), m_expanded, m_modal);

    if (m_header != nullptr && !layout.headerBox.isEmpty()) {
        // The header spans the width and keeps its own height: a FAB centres
        // itself in what it is given.
        m_header->setGeometry(MdChildBox::measure(m_header).resizedGeometryOn(
            QRectF(layout.headerBox.left(), layout.headerBox.top(),
                   layout.headerBox.width(), m_header->sizeHint().height())));
    }

    for (int i = 0; i < m_items.size() && i < layout.itemBoxes.size(); ++i) {
        MdNavigationBarItem *item = m_items.at(i);
        item->setGeometry(MdChildBox::measure(item).resizedGeometryOn(layout.itemBoxes.at(i)));
    }
}

void MdNavigationRail::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdNavigationRail::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::LayoutDirectionChange:
        // A relayout and nothing more: the column is not mirrored in RTL, the
        // same pinned library-wide gap as the bar's.
        placeChildren();
        update();
        break;
    default:
        break;
    }
}

void MdNavigationRail::onThemeChanged()
{
    invalidateTokens();
    applyToAllItems();
    restartWidthAnimation();
    placeChildren();
    update();
}

} // namespace md
