#include "MdNav.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// Navigation-drawer item metrics (Compose's hard-coded 16 / 24 / 12 / 24).
constexpr qreal kStartPad = 16.0;
constexpr qreal kIconSize = 24.0;
constexpr qreal kIconLabelGap = 12.0;
constexpr qreal kEndPad = 24.0;
constexpr qreal kRowHeight = 56.0;
constexpr qreal kBadgePad = 8.0;

} // namespace

// --- MdNavItem --------------------------------------------------------------

MdNavItem::MdNavItem(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNavItem::MdNavItem(const QString &text, const QString &iconName, QWidget *parent)
    : QWidget(parent)
    , m_text(text)
    , m_iconName(iconName)
{
    init();
}

MdNavItem::~MdNavItem() = default;

void MdNavItem::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("nav item") : m_text);
}

void MdNavItem::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    setAccessibleName(m_text.isEmpty() ? QStringLiteral("nav item") : m_text);
    updateGeometry();
    update();
    emit textChanged(m_text);
}

void MdNavItem::setIconName(const QString &name)
{
    if (m_iconName == name) {
        return;
    }
    m_iconName = name;
    update();
    emit iconNameChanged(m_iconName);
}

void MdNavItem::setBadgeText(const QString &text)
{
    if (m_badgeText == text) {
        return;
    }
    m_badgeText = text;
    update();
    emit badgeTextChanged(m_badgeText);
}

void MdNavItem::setSelected(bool selected)
{
    if (m_selected == selected) {
        return;
    }
    m_selected = selected;
    update();
    emit selectedChanged(m_selected);
}

void MdNavItem::setItemEnabled(bool enabled)
{
    if (m_itemEnabled == enabled) {
        return;
    }
    m_itemEnabled = enabled;
    setEnabled(enabled);
    update();
    emit itemEnabledChanged(m_itemEnabled);
}

QColor MdNavItem::resolvedContentColor() const
{
    return m_selected ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
                      : MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdNavItem::resolvedSelectedBackground() const
{
    return MdTheme::instance().color(ColorRole::SecondaryContainer);
}

QSize MdNavItem::sizeHint() const
{
    const QFont labelFont = MdTypeScale::font(TypeStyle::LabelLarge);
    const qreal labelWidth = m_text.isEmpty() ? 0.0 : QFontMetrics(labelFont).horizontalAdvance(m_text);
    return QSize(int(kStartPad + kIconSize + kIconLabelGap + labelWidth + kEndPad),
                 int(kRowHeight));
}

QSize MdNavItem::minimumSizeHint() const
{
    return QSize(int(kStartPad + kIconSize + kEndPad), int(kRowHeight));
}

void MdNavItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds(0, 0, width(), height());
    const QPainterPath pill = MdShape::roundedRect(bounds, ShapeCorner::Full);
    if (m_selected) {
        painter.fillPath(pill, resolvedSelectedBackground());
    } else if (m_hovered || m_pressed) {
        QColor layer = MdStateLayer::overlay(MdTheme::instance().color(ColorRole::OnSurface),
                                             m_pressed ? StateLayerKind::Pressed
                                                       : StateLayerKind::Hover);
        painter.fillPath(pill, layer);
    }

    const bool rtl = layoutDirection() == Qt::RightToLeft;
    qreal x = rtl ? width() - kStartPad - kIconSize : kStartPad;
    const QRectF iconBox(x, (height() - kIconSize) * 0.5, kIconSize, kIconSize);
    if (!m_iconName.isEmpty()) {
        MdIcon::paint(&painter, iconBox, m_iconName, resolvedContentColor());
    }

    qreal labelX = rtl ? kEndPad : kStartPad + kIconSize + kIconLabelGap;
    qreal labelW = width() - labelX - kEndPad;
    if (!m_badgeText.isEmpty()) {
        const qreal badgeW =
            QFontMetrics(MdTypeScale::font(TypeStyle::LabelMedium)).horizontalAdvance(m_badgeText)
            + kBadgePad * 2;
        const QRectF badgeBox(rtl ? kEndPad : width() - kEndPad - badgeW, (height() - 24) * 0.5,
                              badgeW, 24);
        const QPainterPath badgePath = MdShape::roundedRect(badgeBox, ShapeCorner::Full);
        painter.fillPath(badgePath, MdTheme::instance().color(ColorRole::Error));
        painter.setFont(MdTypeScale::font(TypeStyle::LabelMedium));
        painter.setPen(MdTheme::instance().color(ColorRole::OnError));
        painter.drawText(badgeBox, Qt::AlignCenter | Qt::TextSingleLine, m_badgeText);
        labelW -= badgeW + kBadgePad;
    }

    if (!m_text.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::LabelLarge));
        painter.setPen(resolvedContentColor());
        painter.drawText(QRectF(labelX, 0, labelW, height()),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_text);
    }
}

void MdNavItem::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (!m_itemEnabled) {
        return;
    }
    m_pressed = true;
    update();
}

void MdNavItem::mouseReleaseEvent(QMouseEvent *event)
{
    const bool wasPressed = m_pressed;
    m_pressed = false;
    update();
    if (wasPressed && m_itemEnabled && QRectF(rect()).contains(md::mousePosition(event))) {
        emit clicked();
    }
    QWidget::mouseReleaseEvent(event);
}

void MdNavItem::enterEvent(MdEnterEvent *event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void MdNavItem::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false;
    m_pressed = false;
    update();
}

void MdNavItem::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter) {
        if (m_itemEnabled) {
            emit clicked();
        }
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdNavItem::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

// --- MdNav ------------------------------------------------------------------

MdNav::MdNav(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdNav::~MdNav() = default;

void MdNav::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("nav"));
    MdStyleBase::connectThemeUpdate(this, &MdNav::onThemeChanged);
}

void MdNav::addItem(MdNavItem *item)
{
    if (!item) {
        return;
    }
    item->setParent(this);
    m_items.append(item);
    connect(item, &MdNavItem::clicked, this, &MdNav::onItemClick);
    if (m_currentIndex < 0) {
        m_currentIndex = 0;
        syncSelection();
        emit currentIndexChanged(m_currentIndex);
    }
    relayout();
}

void MdNav::insertItem(int index, MdNavItem *item)
{
    if (!item) {
        return;
    }
    if (index < 0 || index > int(m_items.size())) {
        addItem(item);
        return;
    }
    item->setParent(this);
    m_items.insert(index, item);
    connect(item, &MdNavItem::clicked, this, &MdNav::onItemClick);
    if (m_currentIndex >= index) {
        ++m_currentIndex;
    }
    syncSelection();
    relayout();
}

void MdNav::removeItem(MdNavItem *item)
{
    const int index = m_items.indexOf(item);
    if (index < 0) {
        return;
    }
    m_items.removeAt(index);
    disconnect(item, nullptr, this, nullptr);
    if (item->parent() == this) {
        item->setParent(nullptr);
    }
    if (m_currentIndex == index) {
        m_currentIndex = qMin(index, int(m_items.size()) - 1);
        syncSelection();
        emit currentIndexChanged(m_currentIndex);
    } else if (m_currentIndex > index) {
        --m_currentIndex;
    }
    relayout();
}

void MdNav::clearItems()
{
    const QList<MdNavItem *> items = m_items;
    m_items.clear();
    m_currentIndex = -1;
    for (MdNavItem *item : items) {
        disconnect(item, nullptr, this, nullptr);
        if (item->parent() == this) {
            item->setParent(nullptr);
        }
    }
    emit currentIndexChanged(m_currentIndex);
}

MdNavItem *MdNav::itemAt(int index) const
{
    if (index < 0 || index >= int(m_items.size())) {
        return nullptr;
    }
    return m_items.at(index);
}

int MdNav::indexOf(const MdNavItem *item) const
{
    return m_items.indexOf(const_cast<MdNavItem *>(item));
}

void MdNav::setCurrentIndex(int index)
{
    const int clamped = index < -1 ? -1 : index;
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    syncSelection();
    emit currentIndexChanged(m_currentIndex);
}

void MdNav::setOrientation(MdNavOrientation orientation)
{
    if (orientation >= MdNavOrientation::Count || m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    relayout();
    updateGeometry();
    emit orientationChanged(m_orientation);
}

void MdNav::setItemLabelsVisible(bool visible)
{
    if (m_labelsVisible == visible) {
        return;
    }
    m_labelsVisible = visible;
    relayout();
    update();
    emit itemLabelsVisibleChanged(m_labelsVisible);
}

QSize MdNav::sizeHint() const
{
    if (m_items.isEmpty()) {
        return QSize(80, 56);
    }
    if (m_orientation == MdNavOrientation::Horizontal) {
        int w = 0;
        for (MdNavItem *item : m_items) {
            w += item->sizeHint().width();
        }
        return QSize(w, int(kRowHeight));
    }
    int w = 0;
    for (MdNavItem *item : m_items) {
        w = qMax(w, item->sizeHint().width());
    }
    return QSize(w, int(kRowHeight) * int(m_items.size()));
}

QSize MdNav::minimumSizeHint() const
{
    return QSize(56, int(kRowHeight));
}

void MdNav::relayout()
{
    const bool horizontal = m_orientation == MdNavOrientation::Horizontal;
    const int n = int(m_items.size());
    if (n == 0) {
        return;
    }
    for (int i = 0; i < n; ++i) {
        MdNavItem *item = m_items.at(i);
        if (horizontal) {
            const qreal cellW = width() / qreal(n);
            item->setGeometry(QRectF(i * cellW, 0, cellW, height()).toRect());
        } else {
            const qreal cellH = height() / qreal(n);
            item->setGeometry(QRectF(0, i * cellH, width(), cellH).toRect());
        }
        item->setVisible(true);
    }
}

void MdNav::syncSelection()
{
    for (int i = 0; i < m_items.size(); ++i) {
        m_items.at(i)->setSelected(i == m_currentIndex);
    }
}

void MdNav::onItemClick()
{
    auto *item = qobject_cast<MdNavItem *>(sender());
    const int index = indexOf(item);
    if (index < 0) {
        return;
    }
    setCurrentIndex(index);
    emit itemClicked(index);
}

void MdNav::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdNav::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        relayout();
    }
    QWidget::changeEvent(event);
}

void MdNav::onThemeChanged()
{
    update();
}

} // namespace md
