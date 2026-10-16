#include "MdMenuBar.h"

#include "MdMenu.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// Horizontal padding inside each item's hit box (label-large tracking).
constexpr int kItemPad = 12;
/// Default bar height: label-large line height + 12 px vertical padding.
constexpr int kDefaultPad = 12;
/// Minimum hit-target width for a short title.
constexpr int kMinItemWidth = 48;

} // namespace

MdMenuBar::MdMenuBar(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdMenuBar::~MdMenuBar() = default;

void MdMenuBar::init()
{
    setFocusPolicy(Qt::WheelFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMouseTracking(true);
    setAccessibleName(QStringLiteral("menu bar"));
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    MdStyleBase::connectThemeUpdate(this, &MdMenuBar::onThemeChanged);
}

void MdMenuBar::setItems(const QStringList &titles)
{
    if (m_titles == titles) {
        return;
    }
    const int oldCount = m_titles.size();
    m_titles = titles;
    m_menus.resize(m_titles.size());
    m_enabled.resize(m_titles.size());
    for (int i = oldCount; i < m_titles.size(); ++i) {
        m_menus[i] = nullptr;
        m_enabled[i] = true;
    }
    if (m_currentIndex >= m_titles.size()) {
        m_currentIndex = m_titles.isEmpty() ? -1 : m_titles.size() - 1;
    }
    if (m_openIndex >= m_titles.size()) {
        m_openIndex = -1;
    }
    updateGeometry();
    update();
    emit itemsChanged(m_titles);
}

int MdMenuBar::addItem(const QString &title)
{
    insertItem(m_titles.size(), title);
    return m_titles.size() - 1;
}

void MdMenuBar::insertItem(int index, const QString &title)
{
    const int clamped = qBound(0, index, m_titles.size());
    m_titles.insert(clamped, title);
    m_menus.insert(clamped, nullptr);
    m_enabled.insert(clamped, true);
    if (m_currentIndex >= clamped && m_currentIndex >= 0) {
        // Keep the highlighted title stable.
    }
    updateGeometry();
    update();
    emit itemsChanged(m_titles);
}

void MdMenuBar::removeItem(int index)
{
    if (index < 0 || index >= m_titles.size()) {
        return;
    }
    if (m_openIndex == index) {
        closeMenu();
    }
    if (m_menus[index] && m_menus[index]->parent() == this) {
        m_menus[index]->setParent(nullptr);
    }
    m_titles.removeAt(index);
    m_menus.removeAt(index);
    m_enabled.removeAt(index);
    if (m_currentIndex == index) {
        m_currentIndex = -1;
    } else if (m_currentIndex > index) {
        --m_currentIndex;
    }
    if (m_openIndex == index) {
        m_openIndex = -1;
    } else if (m_openIndex > index) {
        --m_openIndex;
    }
    updateGeometry();
    update();
    emit itemsChanged(m_titles);
}

void MdMenuBar::clearItems()
{
    if (m_openIndex >= 0) {
        closeMenu();
    }
    m_titles.clear();
    m_menus.clear();
    m_enabled.clear();
    m_currentIndex = -1;
    m_openIndex = -1;
    updateGeometry();
    update();
    emit itemsChanged(m_titles);
}

QString MdMenuBar::itemText(int index) const
{
    if (index < 0 || index >= m_titles.size()) {
        return QString();
    }
    return m_titles.at(index);
}

void MdMenuBar::setItemText(int index, const QString &title)
{
    if (index < 0 || index >= m_titles.size() || m_titles.at(index) == title) {
        return;
    }
    m_titles[index] = title;
    updateGeometry();
    update();
    emit itemsChanged(m_titles);
}

bool MdMenuBar::isItemEnabled(int index) const
{
    if (index < 0 || index >= m_enabled.size()) {
        return false;
    }
    return m_enabled.at(index);
}

void MdMenuBar::setItemEnabled(int index, bool enabled)
{
    if (index < 0 || index >= m_enabled.size() || m_enabled.at(index) == enabled) {
        return;
    }
    m_enabled[index] = enabled;
    update();
}

void MdMenuBar::setMenu(int index, MdMenu *menu)
{
    if (index < 0 || index >= m_menus.size()) {
        return;
    }
    if (m_menus.at(index) == menu) {
        return;
    }
    m_menus[index] = menu;
    if (menu && menu->parent() == nullptr) {
        menu->setParent(this);
    }
    if (menu) {
        connect(menu, &MdMenu::closed, this, &MdMenuBar::onMenuClosed, Qt::UniqueConnection);
    }
}

MdMenu *MdMenuBar::menu(int index) const
{
    if (index < 0 || index >= m_menus.size()) {
        return nullptr;
    }
    return m_menus.at(index);
}

void MdMenuBar::setCurrentIndex(int index)
{
    const int clamped = m_titles.isEmpty() ? -1 : qBound(-1, index, m_titles.size() - 1);
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdMenuBar::setBarHeight(int height)
{
    const int clamped = height < 0 ? 0 : height;
    if (m_barHeight == clamped) {
        return;
    }
    m_barHeight = clamped;
    updateGeometry();
    update();
    emit barHeightChanged(m_barHeight);
}

void MdMenuBar::setBackgroundColor(const QColor &color)
{
    if (m_backgroundOverride == color) {
        return;
    }
    m_backgroundOverride = color;
    update();
    emit backgroundColorChanged(m_backgroundOverride);
}

QColor MdMenuBar::resolvedBackgroundColor() const
{
    return m_backgroundOverride.isValid() ? m_backgroundOverride
                                          : MdTheme::instance().color(ColorRole::Surface);
}

QColor MdMenuBar::resolvedContentColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QRect MdMenuBar::itemRect(int index) const
{
    if (index < 0 || index >= m_titles.size()) {
        return QRect();
    }
    const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFontMetrics metrics(font);
    int x = 0;
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    // Measure all items first so RTL can mirror the stack.
    QList<int> widths;
    for (const QString &title : m_titles) {
        widths.append(qMax(kMinItemWidth, metrics.horizontalAdvance(title) + kItemPad * 2));
    }
    if (rtl) {
        for (int i = m_titles.size() - 1; i > index; --i) {
            x += widths.at(i);
        }
    } else {
        for (int i = 0; i < index; ++i) {
            x += widths.at(i);
        }
    }
    return QRect(x, 0, widths.at(index), height());
}

QSize MdMenuBar::sizeHint() const
{
    const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFontMetrics metrics(font);
    int w = 0;
    for (const QString &title : m_titles) {
        w += qMax(kMinItemWidth, metrics.horizontalAdvance(title) + kItemPad * 2);
    }
    const int h = m_barHeight > 0
                      ? m_barHeight
                      : qRound(MdTypeScale::lineHeight(TypeStyle::LabelLarge)) + kDefaultPad * 2;
    return QSize(qMax(w, 64), h);
}

QSize MdMenuBar::minimumSizeHint() const
{
    return QSize(64, sizeHint().height());
}

void MdMenuBar::openItem(int index)
{
    if (index < 0 || index >= m_titles.size() || !m_enabled.value(index, true)) {
        return;
    }
    setCurrentIndex(index);
    MdMenu *popup = m_menus.value(index, nullptr);
    if (popup == nullptr) {
        emit itemTriggered(index, m_titles.at(index));
        return;
    }
    if (m_openIndex == index) {
        closeMenu();
        return;
    }
    if (m_openIndex >= 0) {
        closeMenu();
    }
    setOpenIndex(index);
    const QRect local = itemRect(index);
    const QRect anchor(mapToGlobal(local.topLeft()), local.size());
    popup->popup(anchor);
    emit menuOpened(index, popup);
}

void MdMenuBar::closeMenu()
{
    if (m_openIndex < 0) {
        return;
    }
    const int index = m_openIndex;
    setOpenIndex(-1);
    if (MdMenu *popup = m_menus.value(index, nullptr)) {
        popup->hide();
    }
    emit menuClosed(index);
}

void MdMenuBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), resolvedBackgroundColor());
    for (int i = 0; i < m_titles.size(); ++i) {
        paintItem(&painter, i);
    }
}

void MdMenuBar::paintItem(QPainter *painter, int index) const
{
    const QRect r = itemRect(index);
    if (r.isEmpty()) {
        return;
    }
    const QColor content = resolvedContentColor();
    const bool enabled = m_enabled.value(index, true);
    const bool hovered = index == m_hoverIndex && enabled;
    const bool open = index == m_openIndex;
    const bool focused = index == m_currentIndex && m_focusIsKeyboard && enabled;

    // State layer: open takes the focus opacity (0.12), hover the hover one.
    if (enabled) {
        QColor layer;
        if (open) {
            layer = MdStateLayer::overlay(content, StateLayerKind::Focus);
        } else if (hovered) {
            layer = MdStateLayer::overlay(content, StateLayerKind::Hover);
        }
        if (layer.alpha() > 0) {
            painter->fillRect(r, layer);
        }
    }

    // Keyboard focus ring on the active item (`:focus-visible` only).
    if (focused) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPen pen(MdTheme::instance().color(ColorRole::Primary), 2.0);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(QRectF(r).adjusted(2.5, 2.5, -2.5, -2.5), 4.0, 4.0);
        painter->restore();
    }

    painter->save();
    painter->setFont(MdTypeScale::font(TypeStyle::LabelLarge));
    QColor text = content;
    if (!enabled) {
        text.setAlphaF(text.alphaF() * 0.38);
    }
    painter->setPen(text);
    painter->drawText(r, Qt::AlignCenter | Qt::TextSingleLine, m_titles.at(index));
    painter->restore();
}

void MdMenuBar::mousePressEvent(QMouseEvent *event)
{
    const int index = indexAt(event->pos());
    if (index < 0) {
        QWidget::mousePressEvent(event);
        return;
    }
    setFocus(Qt::MouseFocusReason);
    openItem(index);
    event->accept();
}

void MdMenuBar::mouseMoveEvent(QMouseEvent *event)
{
    setHoverIndex(indexAt(QPoint(qRound(md::mousePosition(event).x()),
                                 qRound(md::mousePosition(event).y()))));
    QWidget::mouseMoveEvent(event);
}

void MdMenuBar::leaveEvent(QEvent *event)
{
    setHoverIndex(-1);
    QWidget::leaveEvent(event);
}

void MdMenuBar::keyPressEvent(QKeyEvent *event)
{
    const int count = m_titles.size();
    if (count == 0) {
        QWidget::keyPressEvent(event);
        return;
    }
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_Right: {
        const bool next = (event->key() == Qt::Key_Right) != rtl;
        int index = m_currentIndex;
        if (index < 0) {
            index = next ? 0 : count - 1;
        } else {
            index = next ? (index + 1) % count : (index - 1 + count) % count;
        }
        setCurrentIndex(index);
        event->accept();
        return;
    }
    case Qt::Key_Down:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        if (m_currentIndex >= 0) {
            openItem(m_currentIndex);
            event->accept();
            return;
        }
        break;
    case Qt::Key_Escape:
        if (m_openIndex >= 0) {
            closeMenu();
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdMenuBar::focusInEvent(QFocusEvent *event)
{
    switch (event->reason()) {
    case Qt::MouseFocusReason:
    case Qt::PopupFocusReason:
    case Qt::ActiveWindowFocusReason:
        m_focusIsKeyboard = false;
        break;
    default:
        m_focusIsKeyboard = true;
        break;
    }
    if (m_currentIndex < 0 && !m_titles.isEmpty()) {
        setCurrentIndex(0);
    }
    update();
    QWidget::focusInEvent(event);
}

void MdMenuBar::focusOutEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = false;
    update();
    QWidget::focusOutEvent(event);
}

void MdMenuBar::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
    QWidget::changeEvent(event);
}

void MdMenuBar::onThemeChanged()
{
    update();
}

void MdMenuBar::onMenuClosed()
{
    if (m_openIndex >= 0) {
        const int index = m_openIndex;
        setOpenIndex(-1);
        emit menuClosed(index);
    }
}

void MdMenuBar::setHoverIndex(int index)
{
    if (m_hoverIndex == index) {
        return;
    }
    m_hoverIndex = index;
    update();
}

void MdMenuBar::setOpenIndex(int index)
{
    if (m_openIndex == index) {
        return;
    }
    m_openIndex = index;
    update();
    emit openIndexChanged(m_openIndex);
}

int MdMenuBar::indexAt(const QPoint &pos) const
{
    for (int i = 0; i < m_titles.size(); ++i) {
        if (itemRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

} // namespace md
