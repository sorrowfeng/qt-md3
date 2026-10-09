#include "MdTopAppBar.h"

#include "styles/MdChildBox.h"
#include "styles/MdTopAppBarStyle.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdTopAppBar::MdTopAppBar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTopAppBar::MdTopAppBar(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_title(title)
{
    init();
}

MdTopAppBar::~MdTopAppBar() = default;

void MdTopAppBar::init()
{
    // The paint hub owns painting — registering the shared style as soon as
    // the first bar exists means a bar is never momentarily painted by the
    // platform style.
    MdTopAppBarStyle::shared();

    // Every interactive element of an app bar is a slot widget, so the bar
    // itself never takes focus; Compose's `isTraversalGroup = true` is about
    // traversal *order* among those children, which Qt gives us for free.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAttribute(Qt::WA_LayoutUsesWidgetRect, true);

    // The container colour is animated, not stepped: Compose wraps the target
    // in `animateColorAsState(..., DefaultEffects)`, which is why a pinned bar
    // fades to `surface-container` instead of snapping.
    m_scrollTimer = new QTimer(this);
    m_scrollTimer->setInterval(8);
    connect(m_scrollTimer, &QTimer::timeout, this, [this] {
        const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
        m_scrollElapsedMs += m_scrollTimer->interval();
        const qreal seconds = qreal(m_scrollElapsedMs) / 1000.0;
        const qreal progress = qBound(0.0, spring.valueAt(seconds), 1.0);
        m_scrollAmount = m_scrollFrom + (m_scrollTo - m_scrollFrom) * progress;

        if (seconds * 1000.0 >= spring.settlingDurationMs() || qFuzzyCompare(progress, 1.0)) {
            m_scrollTimer->stop();
            m_scrollAnimating = false;
            m_scrollAmount = m_scrollTo;
        }
        update();
    });

    MdStyleBase::connectThemeUpdate(this, &MdTopAppBar::onThemeChanged);
    syncHeightOffsetLimit();
    syncGeometry();
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

const MdAppBarTokens &MdTopAppBar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdAppBarTokens::resolve(m_variant, &m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdTopAppBar::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Arrangement
// ---------------------------------------------------------------------------

void MdTopAppBar::setVariant(MdAppBarVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    invalidateTokens();
    // A different variant is a different collapsible row: medium loses 48 px,
    // large loses 88, a flexible large loses 56 (or 88 with a subtitle).
    syncHeightOffsetLimit();
    syncGeometry();
    update();
    emit variantChanged(m_variant);
}

void MdTopAppBar::setAlignment(MdAppBarAlignment alignment)
{
    if (m_alignment == alignment) {
        return;
    }
    m_alignment = alignment;
    syncGeometry();
    update();
    emit alignmentChanged(m_alignment);
}

void MdTopAppBar::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    syncGeometry();
    update();
    emit titleChanged(m_title);
}

void MdTopAppBar::setSubtitle(const QString &subtitle)
{
    if (m_subtitle == subtitle) {
        return;
    }
    m_subtitle = subtitle;
    // Only the small and the two flexible variants grow for a subtitle, so the
    // collapsible row's height — and therefore the behaviour's limit — can
    // change here.
    syncHeightOffsetLimit();
    syncGeometry();
    update();
    emit subtitleChanged(m_subtitle);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void MdTopAppBar::setNavigationWidget(QWidget *widget)
{
    if (m_navigation == widget) {
        return;
    }
    if (m_navigation != nullptr) {
        m_navigation->hide();
        m_navigation->setParent(nullptr);
    }
    m_navigation = widget;
    if (m_navigation != nullptr) {
        m_navigation->setParent(this);
        m_navigation->show();
    }
    syncGeometry();
    update();
    emit navigationWidgetChanged();
}

void MdTopAppBar::setCenterWidget(QWidget *widget)
{
    if (m_center == widget) {
        return;
    }
    if (m_center != nullptr) {
        m_center->hide();
        m_center->setParent(nullptr);
    }
    m_center = widget;
    if (m_center != nullptr) {
        m_center->setParent(this);
        m_center->show();
    }
    syncGeometry();
    update();
    emit centerWidgetChanged();
}

void MdTopAppBar::addActionWidget(QWidget *widget)
{
    insertActionWidget(m_actions.size(), widget);
}

void MdTopAppBar::insertActionWidget(int index, QWidget *widget)
{
    if (widget == nullptr || m_actions.contains(widget)) {
        return;
    }
    widget->setParent(this);
    widget->show();
    m_actions.insert(qBound(0, index, m_actions.size()), widget);
    syncGeometry();
    update();
    emit actionWidgetsChanged();
}

void MdTopAppBar::removeActionWidget(QWidget *widget)
{
    if (widget == nullptr || !m_actions.removeOne(widget)) {
        return;
    }
    widget->hide();
    widget->setParent(nullptr);
    syncGeometry();
    update();
    emit actionWidgetsChanged();
}

void MdTopAppBar::clearActionWidgets()
{
    if (m_actions.isEmpty()) {
        return;
    }
    for (QWidget *widget : m_actions) {
        widget->hide();
        widget->setParent(nullptr);
    }
    m_actions.clear();
    syncGeometry();
    update();
    emit actionWidgetsChanged();
}

// ---------------------------------------------------------------------------
// Scrolling
// ---------------------------------------------------------------------------

void MdTopAppBar::setScrollBehavior(MdAppBarScrollBehavior *behavior)
{
    if (m_behavior == behavior) {
        return;
    }
    if (m_behavior != nullptr) {
        disconnect(m_behavior, nullptr, this, nullptr);
    }
    m_behavior = behavior;
    if (m_behavior != nullptr) {
        connect(m_behavior, &MdAppBarScrollBehavior::changed, this,
                &MdTopAppBar::onScrollChanged);
    }
    // A new behaviour has no idea how far this bar can collapse; the bar
    // always writes the limit, the same way `adjustHeightOffsetLimit` does.
    syncHeightOffsetLimit();
    onScrollChanged();
    emit scrollBehaviorChanged();
}

qreal MdTopAppBar::collapsedFraction() const
{
    return m_behavior != nullptr ? m_behavior->collapsedFraction() : 0.0;
}

qreal MdTopAppBar::overlappedFraction() const
{
    return m_behavior != nullptr ? m_behavior->overlappedFraction() : 0.0;
}

void MdTopAppBar::onScrollChanged()
{
    syncGeometry();
    retargetContainerColor();
    update();
}

void MdTopAppBar::retargetContainerColor()
{
    const qreal target = MdTopAppBarStyle::colorTransitionFraction(tokens(), collapsedFraction(),
                                                                   overlappedFraction());
    if (!m_scrollAnimating && qFuzzyCompare(m_scrollAmount, target)) {
        return;
    }
    if (qFuzzyCompare(m_scrollAmount, target)) {
        m_scrollTimer->stop();
        m_scrollAnimating = false;
        return;
    }
    m_scrollFrom = m_scrollAmount;
    m_scrollTo = target;
    m_scrollElapsedMs = 0;
    m_scrollAnimating = true;
    m_scrollTimer->start();
}

// ---------------------------------------------------------------------------
// Paint values
// ---------------------------------------------------------------------------

QColor MdTopAppBar::containerColor() const
{
    return MdTopAppBarStyle::containerColorFor(tokens(), m_scrollAmount);
}

qreal MdTopAppBar::leadingTitleAlpha() const
{
    return MdTopAppBarStyle::layoutFor(*this, tokens(), collapsedFraction()).leadingTitleAlpha;
}

qreal MdTopAppBar::titleAlpha() const
{
    return MdTopAppBarStyle::layoutFor(*this, tokens(), collapsedFraction()).titleAlpha;
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

qreal MdTopAppBar::expandedHeight() const
{
    return tokens().expandedHeight(!m_subtitle.isEmpty());
}

qreal MdTopAppBar::currentHeight() const
{
    return tokens().heightFor(collapsedFraction(), !m_subtitle.isEmpty());
}

QSize MdTopAppBar::sizeHint() const
{
    // The cross axis is the layout's business; the long axis is whatever the
    // slots need, which is what a bar dropped into a layout without a width
    // should get.
    qreal width = tokens().leadingSpace + tokens().edgeSpace;
    if (m_navigation != nullptr) {
        width += m_navigation->sizeHint().width();
    }
    width += 120.0; // a title's worth
    for (QWidget *widget : m_actions) {
        width += tokens().iconButtonSpace + widget->sizeHint().width();
    }
    width += tokens().trailingSpace + tokens().edgeSpace;
    return QSize(int(std::ceil(width)), int(std::ceil(expandedHeight())));
}

QSize MdTopAppBar::minimumSizeHint() const
{
    return QSize(0, int(std::ceil(currentHeight())));
}

void MdTopAppBar::syncHeightOffsetLimit()
{
    if (m_behavior == nullptr) {
        return;
    }
    const MdAppBarTokens &resolved = tokens();
    const qreal expanded = resolved.expandedHeight(!m_subtitle.isEmpty());
    // A single-row bar's whole height is collapsible; a two-row bar only ever
    // loses its text row — `adjustHeightOffsetLimit` runs on that row alone,
    // which is what keeps the icon row on screen.
    const qreal collapsible =
        resolved.isTwoRows() ? expanded - resolved.collapsedRowHeight : expanded;
    m_behavior->setHeightOffsetLimit(-qMax<qreal>(collapsible, 0.0));
}

void MdTopAppBar::syncGeometry()
{
    const qreal target = currentHeight();
    const int rounded = int(std::lround(target));
    if (height() != rounded || minimumHeight() != rounded || maximumHeight() != rounded) {
        // A fixed height rather than a size hint: the collapse shortens the
        // widget itself, which is what Compose's height offset does.
        setFixedHeight(rounded);
    }
    placeSlots();
}

void MdTopAppBar::placeSlots()
{
    const MdTopAppBarStyle::Layout layout =
        MdTopAppBarStyle::layoutFor(*this, tokens(), collapsedFraction());

    // Every slot is placed by its *container*: the layout hands back the box the
    // container belongs on, and `MdChildBox` backs the widget off by whatever
    // margin it reserved for its focus indicator. Placing the widget itself
    // would put a 40 px icon button's box 7.5 px inside the token position and
    // the nav 7.5 px in from the bar's 4 px edge.
    if (m_navigation != nullptr) {
        m_navigation->setGeometry(MdChildBox::measure(m_navigation).geometryOn(layout.navigation));
    }
    if (m_center != nullptr) {
        // The centre slot is a widget box, not a container box: the caller's
        // widget fills the gap.
        m_center->setGeometry(layout.center.toRect());
    }

    for (int i = 0; i < m_actions.size() && i < layout.actionBoxes.size(); ++i) {
        QWidget *widget = m_actions.at(i);
        widget->setGeometry(MdChildBox::measure(widget).geometryOn(layout.actionBoxes.at(i)));
    }
}

void MdTopAppBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeSlots();
}

void MdTopAppBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::LayoutDirectionChange:
        // A relayout, and nothing more: this port does not mirror the bar in an
        // RTL layout. Compose's `placeRelative` does, and the tokens are all
        // logical edges, but the layouts here place physical ones — so an RTL
        // app bar keeps its navigation on the left. Recorded in
        // docs/porting-todo.md as a family-level gap rather than quietly
        // assumed away.
        syncGeometry();
        update();
        break;
    case QEvent::EnabledChange:
        update();
        break;
    default:
        break;
    }
}

void MdTopAppBar::onThemeChanged()
{
    invalidateTokens();
    syncHeightOffsetLimit();
    syncGeometry();
    update();
}

} // namespace md
