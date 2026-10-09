#include "MdBottomAppBar.h"

#include "styles/MdBottomAppBarStyle.h"
#include "styles/MdChildBox.h"

#include "core/MdTheme.h"

#include <cmath>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QSizePolicy>

namespace md {

MdBottomAppBar::MdBottomAppBar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdBottomAppBar::~MdBottomAppBar() = default;

void MdBottomAppBar::init()
{
    MdBottomAppBarStyle::shared();

    // The row's children are the interactive parts (icon buttons, a docked
    // FAB), so the bar itself never takes focus.
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    MdStyleBase::connectThemeUpdate(this, &MdBottomAppBar::onThemeChanged);
    setFixedHeight(int(std::lround(tokens().containerHeight)));
}

const MdBottomAppBarTokens &MdBottomAppBar::tokens() const
{
    if (m_tokensDirty) {
        m_tokens = MdBottomAppBarTokens::resolve(&m_componentTokens);
        m_tokensDirty = false;
    }
    return m_tokens;
}

void MdBottomAppBar::invalidateTokens()
{
    m_tokensDirty = true;
}

void MdBottomAppBar::setArrangement(Arrangement arrangement)
{
    if (m_arrangement == arrangement) {
        return;
    }
    m_arrangement = arrangement;
    placeChildren();
    update();
    emit arrangementChanged(m_arrangement);
}

// ---------------------------------------------------------------------------
// Children
// ---------------------------------------------------------------------------

void MdBottomAppBar::addWidget(QWidget *widget)
{
    insertWidget(m_widgets.size(), widget);
}

void MdBottomAppBar::insertWidget(int index, QWidget *widget)
{
    if (widget == nullptr || widget == m_fab || m_widgets.contains(widget)) {
        return;
    }
    widget->setParent(this);
    widget->show();
    m_widgets.insert(qBound(0, index, m_widgets.size()), widget);
    placeChildren();
    update();
    emit widgetsChanged();
}

void MdBottomAppBar::removeWidget(QWidget *widget)
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

void MdBottomAppBar::clearWidgets()
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

void MdBottomAppBar::setFloatingActionButton(QWidget *widget)
{
    if (m_fab == widget) {
        return;
    }
    if (m_fab != nullptr) {
        m_fab->hide();
        m_fab->setParent(nullptr);
    }
    m_fab = widget;
    if (m_fab != nullptr) {
        m_fab->setParent(this);
        m_fab->show();
    }
    placeChildren();
    update();
    emit floatingActionButtonChanged();
}

// ---------------------------------------------------------------------------
// Geometry
// ---------------------------------------------------------------------------

QSize MdBottomAppBar::sizeHint() const
{
    qreal width = tokens().contentLeadingSpace + tokens().contentTrailingSpace;
    for (QWidget *widget : m_widgets) {
        width += widget->sizeHint().width();
    }
    if (m_fab != nullptr) {
        width += tokens().fabLeadingSpace + m_fab->sizeHint().width();
    }
    return QSize(int(std::ceil(width)), int(std::ceil(tokens().containerHeight)));
}

QSize MdBottomAppBar::minimumSizeHint() const
{
    return QSize(0, int(std::ceil(tokens().containerHeight)));
}

void MdBottomAppBar::placeChildren()
{
    const MdBottomAppBarStyle::Layout layout = MdBottomAppBarStyle::layoutFor(*this, tokens());

    // The layout did the arithmetic and handed back the *container* box each
    // child belongs on; putting a widget onto its own container's box is the
    // one thing `MdChildBox` exists for. The widget's position is necessarily
    // integral while the offset is not, so a container can land half a pixel
    // off — that is the floor, not a shortcut.
    for (int i = 0; i < m_widgets.size() && i < layout.childBoxes.size(); ++i) {
        QWidget *widget = m_widgets.at(i);
        widget->setGeometry(MdChildBox::measure(widget).geometryOn(layout.childBoxes.at(i)));
    }

    if (m_fab != nullptr && !layout.fabBox.isEmpty()) {
        // A docked FAB is a slot, so it is measured the same way — which is
        // exactly the case an earlier revision got wrong by placing the widget
        // on the box and parking the painted disc 7.5 px inside the token
        // position. The gallery page's pixel audit caught it.
        m_fab->setGeometry(MdChildBox::measure(m_fab).geometryOn(layout.fabBox));
    }
}

void MdBottomAppBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeChildren();
}

void MdBottomAppBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    switch (event->type()) {
    case QEvent::LayoutDirectionChange:
        // A relayout, and nothing more: the row is not mirrored in RTL. See the
        // matching note in MdTopAppBar and the family-level entry in
        // docs/porting-todo.md.
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

void MdBottomAppBar::onThemeChanged()
{
    invalidateTokens();
    setFixedHeight(int(std::lround(tokens().containerHeight)));
    placeChildren();
    update();
}

} // namespace md
