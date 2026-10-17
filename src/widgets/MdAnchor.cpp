#include "MdAnchor.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QScrollBar>

namespace md {

namespace {

constexpr qreal kRowHeight = 56.0;
constexpr qreal kRowPad = 16.0;

int scrollOffsetOf(QAbstractScrollArea *area)
{
    return area && area->verticalScrollBar() ? area->verticalScrollBar()->value() : 0;
}

} // namespace

// --- MdAffix ----------------------------------------------------------------

MdAffix::MdAffix(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdAffix::~MdAffix()
{
    // Cut wires before members die (the MdInputNumber pattern).
    disconnect(this, nullptr, nullptr, nullptr);
    unhookScrollArea();
}

void MdAffix::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("affix"));
    MdStyleBase::connectThemeUpdate(this, &MdAffix::onThemeChanged);
}

void MdAffix::setScrollArea(QAbstractScrollArea *area)
{
    if (m_scrollArea == area) {
        return;
    }
    unhookScrollArea();
    m_scrollArea = area;
    hookScrollArea();
    refreshPinned();
    emit scrollAreaChanged(m_scrollArea);
}

void MdAffix::setOffset(int offset)
{
    const int clamped = qMax(0, offset);
    if (m_offset == clamped) {
        return;
    }
    m_offset = clamped;
    refreshPinned();
    emit offsetChanged(m_offset);
}

void MdAffix::setContentWidget(QWidget *content)
{
    if (m_content == content) {
        return;
    }
    if (m_content && m_content->parent() == this) {
        m_content->setParent(nullptr);
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        m_content->setGeometry(rect());
    }
    emit contentWidgetChanged(m_content);
}

QColor MdAffix::resolvedBackgroundColor() const
{
    return m_pinned ? MdTheme::instance().color(ColorRole::SurfaceContainer)
                    : QColor(Qt::transparent);
}

QSize MdAffix::sizeHint() const
{
    return QSize(240, int(kRowHeight));
}

QSize MdAffix::minimumSizeHint() const
{
    return QSize(96, 32);
}

void MdAffix::hookScrollArea()
{
    if (!m_scrollArea) {
        return;
    }
    if (m_scrollArea->verticalScrollBar()) {
        connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &MdAffix::onScrollChanged);
    }
}

void MdAffix::unhookScrollArea()
{
    if (!m_scrollArea) {
        return;
    }
    if (m_scrollArea->verticalScrollBar()) {
        disconnect(m_scrollArea->verticalScrollBar(), nullptr, this, nullptr);
    }
}

void MdAffix::refreshPinned()
{
    const bool pinned = scrollOffsetOf(m_scrollArea) > m_offset;
    if (m_pinned == pinned) {
        return;
    }
    m_pinned = pinned;
    update();
    emit pinnedChanged(m_pinned);
}

void MdAffix::onScrollChanged()
{
    refreshPinned();
}

void MdAffix::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (!m_pinned) {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QPainterPath path =
        MdShape::roundedRect(QRectF(0, 0, width(), height()), ShapeCorner::None);
    painter.fillPath(path, resolvedBackgroundColor());
}

void MdAffix::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_content) {
        m_content->setGeometry(rect());
    }
}

void MdAffix::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
    QWidget::changeEvent(event);
}

void MdAffix::onThemeChanged()
{
    update();
}

// --- MdAnchor ---------------------------------------------------------------

MdAnchor::MdAnchor(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdAnchor::~MdAnchor()
{
    disconnect(this, nullptr, nullptr, nullptr);
    unhookScrollArea();
}

void MdAnchor::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("anchor"));
    MdStyleBase::connectThemeUpdate(this, &MdAnchor::onThemeChanged);
}

void MdAnchor::setItems(const QList<MdAnchorItem> &items)
{
    m_items = items;
    if (m_currentIndex >= int(m_items.size())) {
        m_currentIndex = int(m_items.size()) - 1;
        emit currentIndexChanged(m_currentIndex);
    }
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdAnchor::addItem(const QString &text, int contentY)
{
    m_items.append(MdAnchorItem(text, contentY));
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdAnchor::clearItems()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    if (m_currentIndex != -1) {
        m_currentIndex = -1;
        emit currentIndexChanged(m_currentIndex);
    }
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdAnchor::setCurrentIndex(int index)
{
    const int clamped = index < -1 ? -1 : index;
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdAnchor::setScrollArea(QAbstractScrollArea *area)
{
    if (m_scrollArea == area) {
        return;
    }
    unhookScrollArea();
    m_scrollArea = area;
    hookScrollArea();
    syncToScroll();
    emit scrollAreaChanged(m_scrollArea);
}

MdAnchorItem MdAnchor::itemAt(int index) const
{
    if (index < 0 || index >= int(m_items.size())) {
        return MdAnchorItem();
    }
    return m_items.at(index);
}

qreal MdAnchor::rowHeight() const
{
    return kRowHeight;
}

int MdAnchor::itemAtPosition(const QPointF &pos) const
{
    const qreal rh = rowHeight();
    if (rh <= 0) {
        return -1;
    }
    const int row = int(pos.y() / rh);
    if (row < 0 || row >= int(m_items.size())) {
        return -1;
    }
    return row;
}

QColor MdAnchor::resolvedItemColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdAnchor::resolvedSelectedBackground() const
{
    return MdTheme::instance().color(ColorRole::SecondaryContainer);
}

QSize MdAnchor::sizeHint() const
{
    int w = 96;
    const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFontMetrics fm(font);
    for (const MdAnchorItem &item : m_items) {
        w = qMax(w, int(fm.horizontalAdvance(item.text) + kRowPad * 2 + 24));
    }
    return QSize(w, int(rowHeight() * m_items.size()));
}

QSize MdAnchor::minimumSizeHint() const
{
    return QSize(64, int(rowHeight()));
}

void MdAnchor::hookScrollArea()
{
    if (!m_scrollArea) {
        return;
    }
    if (m_scrollArea->verticalScrollBar()) {
        connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
                &MdAnchor::onScrollChanged);
    }
}

void MdAnchor::unhookScrollArea()
{
    if (!m_scrollArea) {
        return;
    }
    if (m_scrollArea->verticalScrollBar()) {
        disconnect(m_scrollArea->verticalScrollBar(), nullptr, this, nullptr);
    }
}

void MdAnchor::syncToScroll()
{
    const int y = scrollOffsetOf(m_scrollArea);
    int best = -1;
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).contentY <= y + 8) {
            best = i;
        }
    }
    setCurrentIndex(best);
}

void MdAnchor::onScrollChanged()
{
    syncToScroll();
}

void MdAnchor::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal rh = rowHeight();
    for (int i = 0; i < m_items.size(); ++i) {
        const QRectF row(0, i * rh, width(), rh);
        if (i == m_currentIndex) {
            const QPainterPath path = MdShape::roundedRect(row, ShapeCorner::Full);
            painter.fillPath(path, resolvedSelectedBackground());
        }
        painter.setFont(MdTypeScale::font(TypeStyle::LabelLarge));
        painter.setPen(i == m_currentIndex
                           ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
                           : resolvedItemColor());
        painter.drawText(row.adjusted(kRowPad, 0, -kRowPad, 0),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                         m_items.at(i).text);
    }
}

void MdAnchor::mousePressEvent(QMouseEvent *event)
{
    const int index = itemAtPosition(md::mousePosition(event));
    if (index >= 0) {
        setCurrentIndex(index);
        emit itemClicked(index);
        if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
            m_scrollArea->verticalScrollBar()->setValue(m_items.at(index).contentY);
        }
    }
    QWidget::mousePressEvent(event);
}

void MdAnchor::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdAnchor::onThemeChanged()
{
    update();
}

} // namespace md
