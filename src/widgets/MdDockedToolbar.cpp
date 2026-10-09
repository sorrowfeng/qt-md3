#include "MdDockedToolbar.h"

#include "styles/MdChildBox.h"
#include "styles/MdDockedToolbarStyle.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdDockedToolbar::MdDockedToolbar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdDockedToolbar::~MdDockedToolbar() = default;

void MdDockedToolbar::init()
{
    MdDockedToolbarStyle::shared();

    // The row's children are the interactive parts (icon buttons, buttons), so
    // the bar itself never takes focus.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    MdStyleBase::connectThemeUpdate(this, &MdDockedToolbar::onThemeChanged);
    syncGeometry();
}

const MdDockedToolbarTokens &MdDockedToolbar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdDockedToolbarTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdDockedToolbar::invalidateTokens()
{
    m_tokensDirty = true;
}

// ---------------------------------------------------------------------------
// Children
// ---------------------------------------------------------------------------

void MdDockedToolbar::addWidget(QWidget *widget)
{
    insertWidget(m_widgets.size(), widget);
}

void MdDockedToolbar::insertWidget(int index, QWidget *widget)
{
    if (widget == nullptr || m_widgets.contains(widget)) {
        return;
    }
    widget->setParent(this);
    widget->show();
    m_widgets.insert(qBound(0, index, m_widgets.size()), widget);
    placeChildren();
    update();
    emit widgetsChanged();
}

void MdDockedToolbar::removeWidget(QWidget *widget)
{
    if (widget == nullptr || !m_widgets.removeOne(widget)) {
        return;
    }
    widget->hide();
    widget->setParent(nullptr);
    placeChildren();
    update();
    emit widgetsChanged();
}

void MdDockedToolbar::clearWidgets()
{
    if (m_widgets.isEmpty()) {
        return;
    }
    for (QWidget *widget : m_widgets) {
        widget->hide();
        widget->setParent(nullptr);
    }
    m_widgets.clear();
    placeChildren();
    update();
    emit widgetsChanged();
}

// ---------------------------------------------------------------------------
// Collapsing
// ---------------------------------------------------------------------------

qreal MdDockedToolbar::expandedHeight() const
{
    return tokens().containerHeight;
}

qreal MdDockedToolbar::heightOffsetLimit() const
{
    // Compose: `heightOffsetLimit = -placeable.height`, i.e. the bar can go
    // away entirely. That is the docked toolbar's whole point — a single-row
    // bar has no row to keep.
    return -expandedHeight();
}

qreal MdDockedToolbar::collapsedFraction() const
{
    const qreal limit = heightOffsetLimit();
    if (limit == 0.0) {
        return 0.0;
    }
    return m_heightOffset / limit;
}

qreal MdDockedToolbar::currentHeight() const
{
    return qMax<qreal>(0.0, expandedHeight() + m_heightOffset);
}

QRect MdDockedToolbar::expandedRect() const
{
    return QRect(0, 0, width(), int(std::lround(expandedHeight())));
}

void MdDockedToolbar::setHeightOffset(qreal offset)
{
    const qreal clamped = qBound(heightOffsetLimit(), offset, 0.0);
    if (qFuzzyCompare(clamped, m_heightOffset)) {
        return;
    }
    m_heightOffset = clamped;
    syncGeometry();
    emit heightOffsetChanged(m_heightOffset);
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdDockedToolbar::sizeHint() const
{
    const MdDockedToolbarTokens &t = tokens();

    // Measured as *containers*, because that is what the arrangement advances
    // by; a 40 px icon button is a 55 px widget and a hint built from
    // `sizeHint()` would over-report the row by 15 px per child.
    qreal width = t.containerLeadingSpace + t.containerTrailingSpace;
    int counted = 0;
    for (QWidget *widget : m_widgets) {
        const QSizeF size = MdChildBox::measure(widget).containerSize();
        width += size.width();
        ++counted;
    }
    if (counted > 1) {
        width += t.containerMaxSpacing * qreal(counted - 1);
    }
    return QSize(int(std::ceil(width)), int(std::ceil(t.containerHeight)));
}

QSize MdDockedToolbar::minimumSizeHint() const
{
    return QSize(0, int(std::ceil(tokens().containerHeight)));
}

void MdDockedToolbar::syncGeometry()
{
    setFixedHeight(int(std::lround(currentHeight())));
    placeChildren();
    update();
}

void MdDockedToolbar::placeChildren()
{
    const MdDockedToolbarStyle::Layout layout = MdDockedToolbarStyle::layoutFor(*this, tokens());

    // The layout did the arithmetic and handed back the *container* box each
    // child belongs on; putting a widget onto its own container's box is the
    // one thing `MdChildBox` exists for. The boxes are computed against the
    // expanded geometry, so the content does not reflow while the bar
    // collapses — the widget's own shorter rect clips it instead, exactly as
    // Compose lays a `Surface` out shorter than the `Row` inside it.
    for (int i = 0; i < m_widgets.size() && i < layout.childBoxes.size(); ++i) {
        QWidget *widget = m_widgets.at(i);
        widget->setGeometry(MdChildBox::measure(widget).geometryOn(layout.childBoxes.at(i)));
    }
}

void MdDockedToolbar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdDockedToolbar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::LayoutDirectionChange:
        // A relayout, and nothing more: the row is not mirrored in RTL. See the
        // matching note in MdTopAppBar / MdBottomAppBar and the family-level
        // entry in docs/porting-todo.md.
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

void MdDockedToolbar::onThemeChanged()
{
    invalidateTokens();
    syncGeometry();
}

} // namespace md
