#include "MdFloatingToolbar.h"

#include "styles/MdChildBox.h"
#include "styles/MdFloatingToolbarStyle.h"

#include "core/MdMotion.h"
#include "core/MdTheme.h"

#include <cmath>
#include <QtCore/QTimer>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdFloatingToolbar::MdFloatingToolbar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdFloatingToolbar::~MdFloatingToolbar() = default;

void MdFloatingToolbar::init()
{
    // The paint hub owns painting — registering the shared style as soon as the
    // first toolbar exists means it is never momentarily painted by the
    // platform style.
    MdFloatingToolbarStyle::shared();

    // Every interactive element is a slot widget, so the toolbar itself never
    // takes focus.
    setFocusPolicy(Qt::NoFocus);
    // Both extents are the toolbar's own: a floating toolbar hugs its slots.
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // The expand / collapse transition is Compose's
    // `FloatingToolbarDefaults.animationSpec()`, which is the same effects
    // spring the app bars animate their container colour with.
    m_expandTimer = new QTimer(this);
    m_expandTimer->setInterval(8);
    connect(m_expandTimer, &QTimer::timeout, this, [this] {
        const MdSpring spring = MdMotion::spring(MotionSpring::EffectsDefault);
        m_expandElapsedMs += m_expandTimer->interval();
        const qreal seconds = qreal(m_expandElapsedMs) / 1000.0;
        const qreal eased = qBound(0.0, spring.valueAt(seconds), 1.0);
        m_progress = m_expandFrom + (m_expandTo - m_expandFrom) * eased;

        if (seconds * 1000.0 >= spring.settlingDurationMs() || qFuzzyCompare(eased, 1.0)) {
            m_expandTimer->stop();
            m_progress = m_expandTo;
        }
        placeChildren();
        update();
        emit expansionProgressChanged(m_progress);
    });

    MdStyleBase::connectThemeUpdate(this, &MdFloatingToolbar::onThemeChanged);
    repolish();
}

const MdFloatingToolbarTokens &MdFloatingToolbar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdFloatingToolbarTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdFloatingToolbar::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Shape
// ---------------------------------------------------------------------------

void MdFloatingToolbar::setOrientation(MdToolbarOrientation orientation)
{
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    repolish();
    emit orientationChanged(m_orientation);
}

void MdFloatingToolbar::setColorScheme(MdToolbarColorScheme scheme)
{
    if (m_colorScheme == scheme) {
        return;
    }
    m_colorScheme = scheme;
    // Only the colours change, but they are read from the resolved set, so the
    // cache goes and a repaint is all that is needed.
    invalidateTokens();
    update();
    emit colorSchemeChanged(m_colorScheme);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

bool MdFloatingToolbar::adopt(QWidget **slot, QWidget *widget)
{
    if (*slot == widget) {
        return false;
    }
    if (*slot != nullptr) {
        (*slot)->hide();
        (*slot)->setParent(nullptr);
    }
    *slot = widget;
    if (widget != nullptr) {
        widget->setParent(this);
        widget->show();
    }
    return true;
}

void MdFloatingToolbar::setLeadingWidget(QWidget *widget)
{
    if (!adopt(&m_leading, widget)) {
        return;
    }
    repolish();
    emit widgetsChanged();
}

void MdFloatingToolbar::setTrailingWidget(QWidget *widget)
{
    if (!adopt(&m_trailing, widget)) {
        return;
    }
    repolish();
    emit widgetsChanged();
}

void MdFloatingToolbar::setFab(QWidget *widget)
{
    if (!adopt(&m_fab, widget)) {
        return;
    }
    // Docking or removing an action button changes the widget's own bounds —
    // the strip it reserves — so this is a full repolish, not a repaint.
    repolish();
    emit widgetsChanged();
}

void MdFloatingToolbar::setFabPosition(MdToolbarFabPosition position)
{
    if (m_fabPosition == position) {
        return;
    }
    m_fabPosition = position;
    placeChildren();
    update();
    emit fabPositionChanged(m_fabPosition);
}

void MdFloatingToolbar::addContentWidget(QWidget *widget)
{
    insertContentWidget(m_content.size(), widget);
}

void MdFloatingToolbar::insertContentWidget(int index, QWidget *widget)
{
    if (widget == nullptr || m_content.contains(widget)) {
        return;
    }
    widget->setParent(this);
    widget->show();
    m_content.insert(qBound(0, index, m_content.size()), widget);
    repolish();
    emit widgetsChanged();
}

void MdFloatingToolbar::removeContentWidget(QWidget *widget)
{
    if (widget == nullptr || !m_content.removeOne(widget)) {
        return;
    }
    widget->hide();
    widget->setParent(nullptr);
    repolish();
    emit widgetsChanged();
}

void MdFloatingToolbar::clearContentWidgets()
{
    if (m_content.isEmpty()) {
        return;
    }
    for (QWidget *widget : m_content) {
        widget->hide();
        widget->setParent(nullptr);
    }
    m_content.clear();
    repolish();
    emit widgetsChanged();
}

// ---------------------------------------------------------------------------
// Expansion
// ---------------------------------------------------------------------------

qreal MdFloatingToolbar::fabSize() const
{
    return tokens().fab.sizeFor(m_progress);
}

void MdFloatingToolbar::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;

    m_expandFrom = m_progress;
    m_expandTo = expanded ? 1.0 : 0.0;
    m_expandElapsedMs = 0;
    if (!m_expandTimer->isActive()) {
        m_expandTimer->start();
    }

    emit expandedChanged(m_expanded);
}

void MdFloatingToolbar::setExpansionProgress(qreal progress)
{
    const qreal clamped = qBound<qreal>(0.0, progress, 1.0);
    m_expandTimer->stop();
    if (qFuzzyCompare(clamped, m_progress)) {
        return;
    }
    m_progress = clamped;
    // The pill lengths with the progress and the action button is resized by
    // it, so this is a placement pass and not just a repaint.
    placeChildren();
    update();
    emit expansionProgressChanged(m_progress);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdFloatingToolbar::sizeHint() const
{
    const MdFloatingToolbarTokens &t = tokens();
    const bool horizontal = m_orientation != MdToolbarOrientation::Vertical;

    // Measured as *containers*, exactly as the layout advances by them — a
    // 40 px icon button is a 55 px widget and a hint built from `sizeHint()`
    // would over-report the pill by 15 px per child.
    const auto extents = [&](const QList<QWidget *> &children) {
        qreal total = 0.0;
        for (QWidget *child : children) {
            const QSizeF size = MdChildBox::measure(child).containerSize();
            total += horizontal ? size.width() : size.height();
        }
        return total;
    };

    qreal main = t.containerLeadingSpace + t.containerTrailingSpace;
    // Measured at **full** expansion whatever the current progress. Compose
    // sizes the outer Layout from `maxIntrinsicWidth`, so the toolbar's own
    // bounds do not move while the pill inside them shortens; only the pill and
    // the visible slots change.
    //
    // The published `container.between-space` separates every neighbouring pair
    // of slots, so the run carries one gap fewer than it has items; the leading
    // and trailing slots count even while the toolbar is collapsed, because the
    // bounds do not shrink with the slot set either.
    int slotCount = 0;
    if (m_leading != nullptr) {
        main += extents({m_leading});
        ++slotCount;
    }
    if (m_trailing != nullptr) {
        main += extents({m_trailing});
        ++slotCount;
    }
    main += extents(m_content);
    slotCount += int(m_content.size());
    main += t.containerBetweenSpace * qMax(0, slotCount - 1);

    qreal cross = t.containerCrossExtent(m_orientation);
    if (m_fab != nullptr) {
        // Compose reserves the strip at the expanded size and lifts the cross
        // extent to the collapsed one (`defaultMinSize(minHeight =
        // FabSizeRange.endInclusive)`).
        main += t.fab.betweenSpace + t.fab.expandedSize;
        cross = qMax(cross, t.fab.collapsedSize);
    }

    return horizontal ? QSize(int(std::ceil(main)), int(std::ceil(cross)))
                      : QSize(int(std::ceil(cross)), int(std::ceil(main)));
}

QSize MdFloatingToolbar::minimumSizeHint() const
{
    // The cross extent is a token and never shrinks; the main axis is the
    // pill's content and may be zero.
    const qreal cross = tokens().containerCrossExtent(m_orientation);
    const bool horizontal = m_orientation != MdToolbarOrientation::Vertical;
    return horizontal ? QSize(0, int(std::ceil(cross))) : QSize(int(std::ceil(cross)), 0);
}

void MdFloatingToolbar::repolish()
{
    const QSize hint = sizeHint();
    if (size() != hint) {
        resize(hint);
    }
    placeChildren();
    updateGeometry();
    update();
}

void MdFloatingToolbar::placeChildren()
{
    const MdFloatingToolbarStyle::Layout layout =
        MdFloatingToolbarStyle::layoutFor(*this, tokens());

    // The layout handed back the *container* box each child belongs on, at
    // full expansion, so nothing reflows while the pill shortens — the pill's
    // own rectangle clips instead, exactly as Compose lets the shrinking
    // `Surface` clip the Row inside it.
    //
    // Qt clips a child to its *widget*, not to the pill drawn inside it, so a
    // child that has left the pill would otherwise float over the page behind
    // it. Compose's `graphicsLayer { clip = true; shape = shape }` is emulated
    // by discretising: a child whose container is no longer inside the pill is
    // hidden outright rather than partially clipped. Recorded in
    // docs/porting-todo.md.
    const QRectF pill = layout.container;
    const auto place = [&pill](QWidget *child, const QRectF &box) {
        const bool inside = box.isValid() && !pill.isEmpty()
                            && box.left() >= pill.left() - 0.5
                            && box.right() <= pill.right() + 0.5
                            && box.top() >= pill.top() - 0.5
                            && box.bottom() <= pill.bottom() + 0.5;
        if (child == nullptr || !inside) {
            if (child != nullptr) {
                child->hide();
            }
            return;
        }
        child->show();
        child->setGeometry(MdChildBox::measure(child).geometryOn(box));
    };

    place(m_leading, layout.leadingChildBoxes.value(0));
    for (int i = 0; i < m_content.size() && i < layout.contentChildBoxes.size(); ++i) {
        place(m_content.at(i), layout.contentChildBoxes.at(i));
    }
    place(m_trailing, layout.trailingChildBoxes.value(0));

    // The action button is the one child whose size is not its own: the
    // toolbar asks it to take the FAB token's 56 or 80 px, so the widget is
    // resized onto the box rather than moved onto it.
    if (m_fab != nullptr) {
        if (layout.fabBox.isValid()) {
            m_fab->show();
            m_fab->setGeometry(MdChildBox::measure(m_fab).resizedGeometryOn(layout.fabBox));
        } else {
            m_fab->hide();
        }
    }
}

void MdFloatingToolbar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdFloatingToolbar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::LayoutDirectionChange:
        // A relayout, and nothing more: the toolbar is not mirrored in RTL.
        // See the matching note in the other bars and the library-level entry
        // in docs/porting-todo.md.
        placeChildren();
        update();
        break;
    case QEvent::EnabledChange:
        update();
        break;
    default:
        break;
    }
}

void MdFloatingToolbar::onThemeChanged()
{
    invalidateTokens();
    repolish();
}

} // namespace md
