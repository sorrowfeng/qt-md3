#include "MdTimeline.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtMath>

namespace md {

namespace {

/// Gutter width reserved for the dot + connector (`md.sys.spacing` 24).
constexpr qreal kGutter = 24.0;
/// Horizontal inset of the text column (`md.sys.spacing` 16).
constexpr qreal kTextInset = 16.0;
/// Vertical padding of a row (`md.comp.list.list-item.*` 12).
constexpr qreal kRowPadY = 12.0;
/// Gap between headline and supporting (`md.sys.spacing` 4).
constexpr qreal kTextGap = 4.0;
/// Dot diameter (the progress-track active-indicator reading, 8).
constexpr qreal kDot = 8.0;
/// Connector thickness (`md.comp.divider.thickness` 1).
constexpr qreal kLine = 1.0;
/// Outer ring on an Active node (focus-indicator stroke 3, optically 2).
constexpr qreal kActiveRing = 2.0;

} // namespace

MdTimeline::MdTimeline(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTimeline::~MdTimeline() = default;

void MdTimeline::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("timeline"));
    MdStyleBase::connectThemeUpdate(this, &MdTimeline::onThemeChanged);
}

void MdTimeline::setItems(const QList<MdTimelineItem> &items)
{
    if (m_items == items) {
        return;
    }
    m_items = items;
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdTimeline::addItem(const MdTimelineItem &item)
{
    m_items.append(item);
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdTimeline::clearItems()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    updateGeometry();
    update();
    emit itemsChanged(m_items);
}

void MdTimeline::setConnectorVisible(bool visible)
{
    if (m_connectorVisible == visible) {
        return;
    }
    m_connectorVisible = visible;
    update();
    emit connectorVisibleChanged(m_connectorVisible);
}

void MdTimeline::setTimestampsVisible(bool visible)
{
    if (m_timestampsVisible == visible) {
        return;
    }
    m_timestampsVisible = visible;
    updateGeometry();
    update();
    emit timestampsVisibleChanged(m_timestampsVisible);
}

QColor MdTimeline::resolvedTitleColor() const
{
    // md.sys.color.on-surface
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdTimeline::resolvedSupportingColor() const
{
    // md.sys.color.on-surface-variant
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdTimeline::resolvedConnectorColor() const
{
    // md.sys.color.outline-variant
    return MdTheme::instance().color(ColorRole::OutlineVariant);
}

QColor MdTimeline::resolvedDotColor(MdTimelineStatus status) const
{
    switch (status) {
    case MdTimelineStatus::Active:
    case MdTimelineStatus::Complete:
        // md.sys.color.primary
        return MdTheme::instance().color(ColorRole::Primary);
    case MdTimelineStatus::Error:
        // md.sys.color.error
        return MdTheme::instance().color(ColorRole::Error);
    case MdTimelineStatus::Default:
    case MdTimelineStatus::Count:
        break;
    }
    return resolvedConnectorColor();
}

qreal MdTimeline::itemHeight(int index, qreal textWidth) const
{
    const MdTimelineItem &item = m_items.at(index);
    const QFont titleFont = MdTypeScale::font(TypeStyle::TitleMedium);
    const QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const qreal titleLine = MdTypeScale::lineHeight(TypeStyle::TitleMedium);
    const qreal bodyLine = MdTypeScale::lineHeight(TypeStyle::BodyMedium);
    const QFontMetricsF titleFm(titleFont);
    const QFontMetricsF bodyFm(bodyFont);

    qreal textH = titleLine;
    if (!item.supporting.isEmpty()) {
        textH += kTextGap
                + qMax(bodyLine,
                       qreal(bodyFm.boundingRect(QRectF(0, 0, textWidth, 10000).toRect(),
                                                 Qt::TextWordWrap, item.supporting)
                                 .height()));
    }
    Q_UNUSED(titleFm);
    return textH + kRowPadY * 2.0;
}

QSizeF MdTimeline::measure(int width) const
{
    if (m_items.isEmpty()) {
        return QSizeF(width, 0.0);
    }
    const qreal textW = qMax<qreal>(1.0, width - kGutter - kTextInset - 8.0);
    qreal h = 0.0;
    for (int i = 0; i < m_items.size(); ++i) {
        h += itemHeight(i, textW);
    }
    return QSizeF(width, h);
}

QSize MdTimeline::sizeHint() const
{
    const QSizeF s = measure(320);
    return QSize(qCeil(s.width()), qCeil(s.height()));
}

QSize MdTimeline::minimumSizeHint() const
{
    return QSize(160, 0);
}

int MdTimeline::heightForWidth(int width) const
{
    return qCeil(measure(width).height());
}

void MdTimeline::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF bounds = QRectF(rect());
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal textW = qMax<qreal>(1.0, bounds.width() - kGutter - kTextInset - 8.0);
    const QFont titleFont = MdTypeScale::font(TypeStyle::TitleMedium);
    const QFont bodyFont = MdTypeScale::font(TypeStyle::BodyMedium);
    const QFont labelFont = MdTypeScale::font(TypeStyle::LabelMedium);
    const QColor titleColor = resolvedTitleColor();
    const QColor supportingColor = resolvedSupportingColor();
    const QColor connectorColor = resolvedConnectorColor();

    // Gutter centre: 16 px inset + half the 8 px dot.
    const qreal dotCx = rtl ? bounds.right() - kGutter * 0.5 : bounds.left() + kGutter * 0.5;

    qreal y = bounds.top();
    for (int i = 0; i < m_items.size(); ++i) {
        const MdTimelineItem &item = m_items.at(i);
        const qreal h = itemHeight(i, textW);
        const qreal cy = y + kRowPadY + MdTypeScale::lineHeight(TypeStyle::TitleMedium) * 0.5;

        // Connector above (and below) the dot, in the gutter.
        if (m_connectorVisible) {
            painter.setPen(Qt::NoPen);
            if (i > 0) {
                const QColor up = resolvedDotColor(m_items.at(i - 1).status) == connectorColor
                        ? connectorColor
                        : resolvedDotColor(m_items.at(i - 1).status);
                painter.setBrush(i > 0 && m_items.at(i - 1).status == MdTimelineStatus::Error
                                         ? resolvedDotColor(MdTimelineStatus::Error)
                                         : up);
                // The segment leading into this node takes the previous
                // node's colour once that node is done/active/error.
                painter.drawRect(QRectF(dotCx - kLine * 0.5, y - 8.0, kLine, cy - y + 8.0));
            }
            if (i + 1 < m_items.size()) {
                painter.setBrush(connectorColor);
                painter.drawRect(QRectF(dotCx - kLine * 0.5, cy, kLine, h - (cy - y) + 8.0));
            }
        }

        // Dot.
        const QColor dotColor = resolvedDotColor(item.status);
        painter.setPen(Qt::NoPen);
        painter.setBrush(dotColor);
        if (item.status == MdTimelineStatus::Active) {
            // Outer ring around a solid core (the progress active indicator).
            painter.drawEllipse(QPointF(dotCx, cy), kDot * 0.5 + kActiveRing, kDot * 0.5 + kActiveRing);
            painter.setBrush(MdTheme::instance().color(ColorRole::Surface));
            painter.drawEllipse(QPointF(dotCx, cy), kDot * 0.5, kDot * 0.5);
            painter.setBrush(dotColor);
            painter.drawEllipse(QPointF(dotCx, cy), kDot * 0.25, kDot * 0.25);
        } else if (item.status == MdTimelineStatus::Default) {
            painter.drawEllipse(QPointF(dotCx, cy), kDot * 0.5, kDot * 0.5);
        } else {
            painter.drawEllipse(QPointF(dotCx, cy), kDot * 0.5, kDot * 0.5);
        }

        // Text column.
        const qreal textLeft = rtl ? bounds.left() + 8.0
                                   : bounds.left() + kGutter + kTextInset;
        QRectF textRect(textLeft, y + kRowPadY, textW,
                        h - kRowPadY * 2.0);

        painter.setFont(titleFont);
        painter.setPen(titleColor);
        const qreal titleLine = MdTypeScale::lineHeight(TypeStyle::TitleMedium);
        painter.drawText(QRectF(textRect.left(), textRect.top(), textRect.width(), titleLine),
                         Qt::AlignLeft | Qt::AlignVCenter, item.title);

        if (!item.supporting.isEmpty()) {
            painter.setFont(bodyFont);
            painter.setPen(supportingColor);
            painter.drawText(QRectF(textRect.left(), textRect.top() + titleLine + kTextGap,
                                    textRect.width(),
                                    textRect.height() - titleLine - kTextGap),
                             Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, item.supporting);
        }

        if (m_timestampsVisible && !item.timestamp.isEmpty()) {
            painter.setFont(labelFont);
            painter.setPen(supportingColor);
            const QFontMetricsF fm(labelFont);
            const qreal tw = fm.horizontalAdvance(item.timestamp);
            const QRectF stampRect = rtl
                    ? QRectF(textLeft + textRect.width() - tw, y + kRowPadY, tw, titleLine)
                    : QRectF(bounds.right() - 8.0 - tw, y + kRowPadY, tw, titleLine);
            painter.drawText(stampRect, Qt::AlignLeft | Qt::AlignVCenter, item.timestamp);
        }

        y += h;
    }
}

void MdTimeline::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
}

void MdTimeline::onThemeChanged()
{
    update();
}

} // namespace md
