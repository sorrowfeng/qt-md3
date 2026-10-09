#include "MdTabs.h"

#include "styles/MdTabStyle.h"
#include "styles/MdTabsStyle.h"
#include "widgets/MdTab.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QEvent>
#include <QtGui/QKeyEvent>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdTabs::MdTabs(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTabs::~MdTabs() = default;

void MdTabs::init()
{
    MdTabsStyle::shared();
    MdTabStyle::shared();

    // The row itself is not a Tab stop; its tabs are.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_indicatorTimer = new QTimer(this);
    m_indicatorTimer->setInterval(8);
    m_indicatorTimer->setTimerType(Qt::PreciseTimer);
    connect(m_indicatorTimer, &QTimer::timeout, this, &MdTabs::onIndicatorTick);

    MdStyleBase::connectThemeUpdate(this, &MdTabs::onThemeChanged);
}

const MdTabsTokens &MdTabs::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdTabsTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTabs::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------

void MdTabs::applyToTab(MdTab *tab)
{
    tab->setVariantTokens(tokens().forVariant(m_variant));

    connect(tab, &MdTab::selectedChanged, this, [this, tab](bool selected) {
        if (m_updatingSelection) {
            return;
        }
        if (selected) {
            setCurrentIndex(indexOf(tab));
        } else if (tabAt(m_currentIndex) == tab) {
            // A tabs row has no "nothing selected" state to publish; the
            // model must not lie about it either.
            setCurrentIndex(-1);
        }
    });
}

void MdTabs::applyToAllTabs()
{
    for (MdTab *tab : m_tabs) {
        applyToTab(tab);
    }
}

void MdTabs::addTab(MdTab *tab)
{
    insertTab(int(m_tabs.size()), tab);
}

void MdTabs::insertTab(int index, MdTab *tab)
{
    if (tab == nullptr || m_tabs.contains(tab)) {
        return;
    }
    tab->setParent(this);
    tab->show();
    const int at = qBound(0, index, int(m_tabs.size()));
    m_tabs.insert(at, tab);
    applyToTab(tab);

    if (m_currentIndex < 0) {
        // A tabs row shows one page as selected; the first tab to arrive is
        // the one.
        setCurrentIndex(at);
    } else if (at <= m_currentIndex) {
        m_currentIndex += 1;
    }

    placeChildren();
    updateIndicatorTargets(true, false);
    update();
    emit tabsChanged();
}

void MdTabs::removeTab(MdTab *tab)
{
    const int at = indexOf(tab);
    if (at < 0) {
        return;
    }
    m_tabs.removeAt(at);
    tab->disconnect(this);
    tab->hide();
    tab->setParent(nullptr);

    if (m_currentIndex == at) {
        setCurrentIndex(m_tabs.isEmpty() ? -1 : qMin(at, int(m_tabs.size()) - 1));
    } else if (at < m_currentIndex) {
        m_currentIndex -= 1;
    }

    placeChildren();
    updateIndicatorTargets(true, false);
    update();
    emit tabsChanged();
}

void MdTabs::clearTabs()
{
    if (m_tabs.isEmpty()) {
        return;
    }
    for (MdTab *tab : m_tabs) {
        tab->disconnect(this);
        tab->hide();
        tab->setParent(nullptr);
    }
    m_tabs.clear();
    m_currentIndex = -1;
    placeChildren();
    updateIndicatorTargets(true, false);
    update();
    emit tabsChanged();
}

MdTab *MdTabs::tabAt(int index) const
{
    if (index < 0 || index >= int(m_tabs.size())) {
        return nullptr;
    }
    return m_tabs.at(index);
}

int MdTabs::indexOf(const MdTab *tab) const
{
    for (int i = 0; i < m_tabs.size(); ++i) {
        if (m_tabs.at(i) == tab) {
            return i;
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

void MdTabs::setCurrentIndex(int index)
{
    const int clamped = (index < 0 || index >= int(m_tabs.size())) ? -1 : index;
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;

    // Mirror onto the tabs with the report-back suppressed.
    m_updatingSelection = true;
    for (int i = 0; i < m_tabs.size(); ++i) {
        m_tabs.at(i)->setSelected(i == clamped);
    }
    m_updatingSelection = false;

    if (clamped >= 0) {
        updateIndicatorTargets(false, false);
        if (m_layout == MdTabsLayout::Scrollable) {
            restartScrollAnimation();
        }
    }
    update();
    emit currentChanged(m_currentIndex);
}

// ---------------------------------------------------------------------------
// Variant and layout
// ---------------------------------------------------------------------------

void MdTabs::setVariant(MdTabsVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    applyToAllTabs();
    updateGeometry();
    placeChildren();
    updateIndicatorTargets(true, false);
    update();
    emit variantChanged(m_variant);
}

void MdTabs::setLayout(MdTabsLayout layout)
{
    if (m_layout == layout) {
        return;
    }
    m_layout = layout;
    if (layout == MdTabsLayout::Fixed) {
        m_scrollOffset = 0.0;
    }
    updateGeometry();
    placeChildren();
    updateIndicatorTargets(true, false);
    update();
    emit layoutChanged(m_layout);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdTabs::sizeHint() const
{
    const MdTabsVariantTokens &t = tokens().forVariant(m_variant);

    qreal width = 0.0;
    if (m_layout == MdTabsLayout::Scrollable) {
        // The content's own width: edge padding plus every tab at its
        // content-derived width — the same arithmetic `layoutFor` runs.
        qreal x = t.scrollableEdgePadding;
        for (MdTab *tab : m_tabs) {
            x += qMax<qreal>(t.scrollableMinTabWidth, qreal(tab->sizeHint().width()));
        }
        width = x + t.scrollableEdgePadding;
    } else {
        // A fixed row divides whatever width it is given; the hint is the
        // content's minimum so nothing clips.
        qreal minimum = 0.0;
        for (MdTab *tab : m_tabs) {
            minimum += tab->minimumSizeHint().width();
        }
        width = minimum;
    }

    qreal rowHeight = 0.0;
    for (MdTab *tab : m_tabs) {
        rowHeight = qMax(rowHeight, qreal(tab->sizeHint().height()));
    }
    if (m_tabs.isEmpty()) {
        rowHeight = t.containerHeight;
    }

    return QSize(int(std::ceil(width)), int(std::ceil(rowHeight)));
}

QSize MdTabs::minimumSizeHint() const
{
    return QSize(0, int(std::ceil(tokens().forVariant(m_variant).containerHeight)));
}

void MdTabs::placeChildren()
{
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*this, tokens());
    for (int i = 0; i < m_tabs.size() && i < layout.tabRects.size(); ++i) {
        const QRectF &rect = layout.tabRects.at(i);
        // The tab fills its slot: its content centres inside its own rect,
        // which is what Compose's `TabBaselineLayout` does in the equal share.
        m_tabs.at(i)->setGeometry(int(std::round(rect.x() - m_scrollOffset)), 0,
                                  int(std::round(rect.width())),
                                  int(std::round(layout.rowHeight)));
    }
}

// ---------------------------------------------------------------------------
// The indicator's spring
// ---------------------------------------------------------------------------

void MdTabs::updateIndicatorTargets(bool initial, bool markRun)
{
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*this, tokens());
    const MdTabsVariantTokens &t = tokens().forVariant(m_variant);
    const int index = m_currentIndex;
    if (index < 0 || index >= layout.tabRects.size()) {
        m_indicatorTo = 0.0;
        m_widthTo = 0.0;
        return;
    }

    const MdTab *tab = tabAt(index);
    const QRectF target = MdTabsStyle::indicatorRectFor(
        layout, index, t, tab ? tab->indicatorContentWidth(layout.tabRects.at(index).width()) : 0.0);

    if (initial || !m_indicatorHasRun) {
        m_indicatorFrom = target.x();
        m_indicatorTo = target.x();
        m_widthFrom = target.width();
        m_widthTo = target.width();
        m_indicatorProgress = 1.0;
        // Only the row's own resize marks the placement as made: an insertion
        // at the constructor's default geometry is not a layout, and the
        // row's first real layout must still land without a spring.
        if (markRun) {
            m_indicatorHasRun = true;
        }
        return;
    }

    m_indicatorFrom = indicatorOffset();
    m_indicatorTo = target.x();
    m_widthFrom = indicatorWidth();
    m_widthTo = target.width();
    m_indicatorClock.restart();
    m_indicatorTimer->start();
}

void MdTabs::restartIndicatorAnimation(bool initial)
{
    updateIndicatorTargets(initial, false);
}

void MdTabs::onIndicatorTick()
{
    // Compose animates the indicator's offset and width on the *default
    // spatial* spring (its `TabIndicatorScope.tabIndicatorOffset` comment:
    // `MotionSchemeKeyTokens.DefaultSpatial`). The two values share one
    // clock, which is what the two `Animatable`s started together amount to.
    const MdSpring spring = MdMotion::spring(MotionSpring::SpatialDefault);
    const qreal seconds = qreal(m_indicatorClock.elapsed()) / 1000.0;
    const qreal travelled = spring.valueAt(seconds);
    m_indicatorProgress = travelled;

    // The scrollable row's auto-centre scroll rides the same tick on the same
    // spring — Compose's `ScrollableTabData` animates the scroll offset on
    // the spatial scheme too.
    const qreal scrollTravelled = spring.valueAt(seconds);
    m_scrollOffset = m_scrollFrom + (m_scrollTo - m_scrollFrom) * scrollTravelled;

    bool done = qFuzzyCompare(m_indicatorProgress + 1.0, 2.0)
                || qreal(m_indicatorClock.elapsed()) >= spring.settlingDurationMs();
    const bool scrollDone = qFuzzyCompare(m_scrollOffset + 1.0, m_scrollTo + 1.0)
                            || qreal(m_scrollClock.elapsed()) >= spring.settlingDurationMs();

    if (done && scrollDone) {
        m_indicatorProgress = 1.0;
        m_scrollOffset = m_scrollTo;
        m_indicatorTimer->stop();
    }
    placeChildren();
    update();
}

void MdTabs::restartScrollAnimation()
{
    if (m_layout != MdTabsLayout::Scrollable || m_currentIndex < 0) {
        return;
    }
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*this, tokens());
    if (m_currentIndex >= layout.tabRects.size()) {
        return;
    }
    const QRectF rect = layout.tabRects.at(m_currentIndex);

    // Scroll the tab towards the centre of the viewport, clamped — Compose's
    // `TabPosition.calculateTabOffset`, "trying to place it in the center of
    // the screen or as close to the center as possible".
    const qreal viewport = qreal(width());
    const qreal target = qBound<qreal>(0.0, rect.center().x() - viewport / 2.0,
                                       qMax<qreal>(0.0, layout.contentWidth - viewport));
    if (qFuzzyCompare(target + 1.0, m_scrollTo + 1.0)) {
        return;
    }
    m_scrollFrom = m_scrollOffset;
    m_scrollTo = target;
    m_scrollClock.restart();
    m_indicatorTimer->start();
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void MdTabs::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
    // The equal share moves with the width; the indicator follows without a
    // spring from nowhere on the first lay-out.
    updateIndicatorTargets(!m_indicatorHasRun, true);
    update();
}

void MdTabs::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    MdTabsStyle::paintTabs(painter, *this, tokens());
}

void MdTabs::keyPressEvent(QKeyEvent *event)
{
    // The row is a selectable group: Left/Right move the selection, Home/End
    // jump to the ends. A convention, not a published row — see the header.
    const int count = int(m_tabs.size());
    if (count > 0) {
        int next = m_currentIndex;
        switch (event->key()) {
        case Qt::Key_Left:
            next = m_currentIndex <= 0 ? count - 1 : m_currentIndex - 1;
            break;
        case Qt::Key_Right:
            next = m_currentIndex < 0 || m_currentIndex >= count - 1 ? 0 : m_currentIndex + 1;
            break;
        case Qt::Key_Home:
            next = 0;
            break;
        case Qt::Key_End:
            next = count - 1;
            break;
        default:
            break;
        }
        if (next != m_currentIndex && next >= 0 && next < count) {
            setCurrentIndex(next);
            if (MdTab *tab = tabAt(next)) {
                tab->setFocus(Qt::TabFocusReason);
            }
            event->accept();
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdTabs::wheelEvent(QWheelEvent *event)
{
    if (m_layout != MdTabsLayout::Scrollable) {
        QWidget::wheelEvent(event);
        return;
    }
    // Wheel scrolling lands instantly; only a selection change animates.
    const MdTabsStyle::Layout layout = MdTabsStyle::layoutFor(*this, tokens());
    const qreal delta = qreal(event->angleDelta().y()) / 120.0 * 48.0;
    m_scrollOffset = qBound<qreal>(0.0, m_scrollOffset - delta,
                                   qMax<qreal>(0.0, layout.contentWidth - qreal(width())));
    m_scrollTo = m_scrollOffset;
    placeChildren();
    update();
    event->accept();
}

void MdTabs::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::EnabledChange:
        update();
        break;
    case QEvent::FontChange:
        updateGeometry();
        placeChildren();
        update();
        break;
    case QEvent::LayoutDirectionChange:
        // A relayout and nothing more: the row is not mirrored in RTL. See
        // the family-level note in docs/porting-todo.md.
        placeChildren();
        update();
        break;
    default:
        break;
    }
}

void MdTabs::onThemeChanged()
{
    invalidateTokens();
    applyToAllTabs();
    placeChildren();
    update();
}

} // namespace md
