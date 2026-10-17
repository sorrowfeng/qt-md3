#include "MdBreadcrumb.h"

#include "core/MdIcon.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtMath>

namespace md {

namespace {

constexpr qreal kIconSize = 16.0;
constexpr qreal kGap = 4.0;
constexpr qreal kSeparatorGap = 4.0;

} // namespace

MdBreadcrumb::MdBreadcrumb(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdBreadcrumb::~MdBreadcrumb() = default;

void MdBreadcrumb::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(QStringLiteral("breadcrumb"));
    MdStyleBase::connectThemeUpdate(this, &MdBreadcrumb::onThemeChanged);
}

void MdBreadcrumb::setItems(const QStringList &items)
{
    if (m_items == items) {
        return;
    }
    m_items = items;
    if (m_currentIndex >= int(m_items.size())) {
        m_currentIndex = int(m_items.size()) - 1;
        emit currentIndexChanged(m_currentIndex);
    }
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdBreadcrumb::addItem(const QString &label)
{
    m_items.append(label);
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdBreadcrumb::clearItems()
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

void MdBreadcrumb::setCurrentIndex(int index)
{
    const int clamped = index < -1 ? -1 : index;
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdBreadcrumb::setSeparatorIconName(const QString &name)
{
    if (m_separatorIcon == name) {
        return;
    }
    m_separatorIcon = name;
    updateGeometry();
    update();
    emit separatorIconNameChanged(m_separatorIcon);
}

QColor MdBreadcrumb::resolvedItemColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdBreadcrumb::resolvedCurrentColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdBreadcrumb::resolvedSeparatorColor() const
{
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

MdBreadcrumb::LayoutRow MdBreadcrumb::measure(int width) const
{
    Q_UNUSED(width);
    LayoutRow row;
    const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFontMetrics fm(font);
    const bool rtl = layoutDirection() == Qt::RightToLeft;

    qreal x = 0.0;
    for (int i = 0; i < m_items.size(); ++i) {
        const QString &label = m_items.at(i);
        const qreal w = fm.horizontalAdvance(label);
        const qreal h = fm.height();
        row.crumbs.append(QRectF(x, 0, w, h));
        x += w;
        if (i + 1 < m_items.size()) {
            x += kSeparatorGap;
            row.separators.append(QRectF(x, (h - kIconSize) * 0.5, kIconSize, kIconSize));
            x += kIconSize + kSeparatorGap;
        }
    }
    row.height = fm.height();

    if (rtl) {
        const qreal total = x;
        for (QRectF &r : row.crumbs) {
            r.moveLeft(total - r.right());
        }
        for (QRectF &r : row.separators) {
            r.moveLeft(total - r.right());
        }
    }
    Q_UNUSED(kGap);
    return row;
}

int MdBreadcrumb::crumbAt(const QPointF &pos) const
{
    const LayoutRow row = measure(width());
    for (int i = 0; i < row.crumbs.size(); ++i) {
        if (row.crumbs.at(i).adjusted(-2, -2, 2, 2).contains(pos)) {
            return i;
        }
    }
    return -1;
}

QSize MdBreadcrumb::sizeHint() const
{
    const LayoutRow row = measure(QWIDGETSIZE_MAX);
    return QSize(int(row.crumbs.isEmpty() ? 64 : row.crumbs.last().right()) + 8,
                 int(row.height) + 8);
}

QSize MdBreadcrumb::minimumSizeHint() const
{
    return QSize(64, 24);
}

int MdBreadcrumb::heightForWidth(int width) const
{
    Q_UNUSED(width);
    return sizeHint().height();
}

void MdBreadcrumb::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const LayoutRow row = measure(width());
    const qreal yOffset = (height() - row.height) * 0.5;

    QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
    painter.setFont(font);

    for (int i = 0; i < row.crumbs.size(); ++i) {
        const QRectF box = row.crumbs.at(i).translated(0, yOffset);
        const bool current = i == m_currentIndex
                             || (m_currentIndex < 0 && i + 1 == row.crumbs.size());
        painter.setPen(current ? resolvedCurrentColor() : resolvedItemColor());
        painter.drawText(box, Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine,
                         m_items.at(i));
    }

    const QString separatorName = layoutDirection() == Qt::RightToLeft && m_separatorIcon == QLatin1String("chevron_right")
                                      ? QStringLiteral("chevron_left")
                                      : m_separatorIcon;
    for (const QRectF &sep : row.separators) {
        MdIcon::paint(&painter, sep.translated(0, yOffset), separatorName,
                      resolvedSeparatorColor());
    }
}

void MdBreadcrumb::mousePressEvent(QMouseEvent *event)
{
    const int index = crumbAt(md::mousePosition(event));
    if (index >= 0 && index < m_items.size()) {
        setCurrentIndex(index);
        emit crumbClicked(index);
    }
    QWidget::mousePressEvent(event);
}

void MdBreadcrumb::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
    QWidget::changeEvent(event);
}

void MdBreadcrumb::onThemeChanged()
{
    update();
}

} // namespace md
