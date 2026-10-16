#include "MdStatistic.h"

#include "core/MdIcon.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <cmath>

namespace md {

namespace {

/// Gap between the title and the value row (`md.sys.spacing` 8).
constexpr qreal kTitleGap = 8.0;
/// Gap between the value row and the trend caption (`md.sys.spacing` 4).
constexpr qreal kTrendGap = 4.0;
/// Horizontal gap between prefix / value / suffix (`md.sys.spacing` 4).
constexpr qreal kInlineGap = 4.0;
/// Icon box edge for the trend glyph (`md.comp.icon.size` 24, optically 20).
constexpr qreal kTrendIcon = 20.0;

QString arrowName(MdStatisticTrend trend)
{
    switch (trend) {
    case MdStatisticTrend::Up:
        return QStringLiteral("trending_up");
    case MdStatisticTrend::Down:
        return QStringLiteral("trending_down");
    case MdStatisticTrend::None:
    case MdStatisticTrend::Count:
        break;
    }
    return QStringLiteral("trending_flat");
}

} // namespace

MdStatistic::MdStatistic(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdStatistic::~MdStatistic() = default;

void MdStatistic::init()
{
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setAccessibleName(QStringLiteral("statistic"));
    MdStyleBase::connectThemeUpdate(this, &MdStatistic::onThemeChanged);
}

void MdStatistic::setValue(qreal value)
{
    if (qFuzzyCompare(m_value, value)) {
        return;
    }
    m_value = value;
    updateGeometry();
    update();
    emit valueChanged(m_value);
}

void MdStatistic::setPrecision(int precision)
{
    const int clamped = qBound(0, precision, 12);
    if (m_precision == clamped) {
        return;
    }
    m_precision = clamped;
    updateGeometry();
    update();
    emit precisionChanged(m_precision);
}

void MdStatistic::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdStatistic::setPrefix(const QString &prefix)
{
    if (m_prefix == prefix) {
        return;
    }
    m_prefix = prefix;
    updateGeometry();
    update();
    emit prefixChanged(m_prefix);
}

void MdStatistic::setSuffix(const QString &suffix)
{
    if (m_suffix == suffix) {
        return;
    }
    m_suffix = suffix;
    updateGeometry();
    update();
    emit suffixChanged(m_suffix);
}

void MdStatistic::setValueStyle(TypeStyle style)
{
    if (style >= TypeStyle::Count) {
        return;
    }
    if (m_valueStyle == style) {
        return;
    }
    m_valueStyle = style;
    updateGeometry();
    update();
    emit valueStyleChanged(m_valueStyle);
}

void MdStatistic::setTrend(MdStatisticTrend trend)
{
    if (trend >= MdStatisticTrend::Count) {
        return;
    }
    if (m_trend == trend) {
        return;
    }
    m_trend = trend;
    updateGeometry();
    update();
    emit trendChanged(m_trend);
}

void MdStatistic::setTrendText(const QString &text)
{
    if (m_trendText == text) {
        return;
    }
    m_trendText = text;
    updateGeometry();
    update();
    emit trendTextChanged(m_trendText);
}

QColor MdStatistic::resolvedValueColor() const
{
    // md.sys.color.on-surface
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdStatistic::resolvedTitleColor() const
{
    // md.sys.color.on-surface-variant
    return MdTheme::instance().color(ColorRole::OnSurfaceVariant);
}

QColor MdStatistic::resolvedTrendColor() const
{
    switch (m_trend) {
    case MdStatisticTrend::Up:
        // md.sys.color.primary — the positive reading.
        return MdTheme::instance().color(ColorRole::Primary);
    case MdStatisticTrend::Down:
        // md.sys.color.error — the decline reading.
        return MdTheme::instance().color(ColorRole::Error);
    case MdStatisticTrend::None:
    case MdStatisticTrend::Count:
        break;
    }
    return resolvedTitleColor();
}

QString MdStatistic::formattedValue() const
{
    return QString::number(m_value, 'f', m_precision);
}

QSizeF MdStatistic::measure() const
{
    const QFont valueFont = MdTypeScale::font(m_valueStyle);
    const QFont titleFont = MdTypeScale::font(TypeStyle::LabelLarge);
    const QFont trendFont = MdTypeScale::font(TypeStyle::LabelMedium);

    const qreal valueLine = MdTypeScale::lineHeight(m_valueStyle);
    const QFontMetricsF valueFm(valueFont);
    const QFontMetricsF titleFm(titleFont);
    const QFontMetricsF trendFm(trendFont);

    qreal width = 0.0;
    qreal height = 0.0;

    if (!m_title.isEmpty()) {
        width = qMax(width, titleFm.horizontalAdvance(m_title));
        height += MdTypeScale::lineHeight(TypeStyle::LabelLarge) + kTitleGap;
    }

    qreal rowWidth = 0.0;
    if (!m_prefix.isEmpty()) {
        rowWidth += valueFm.horizontalAdvance(m_prefix) + kInlineGap;
    }
    rowWidth += valueFm.horizontalAdvance(formattedValue());
    if (!m_suffix.isEmpty()) {
        rowWidth += kInlineGap + valueFm.horizontalAdvance(m_suffix);
    }

    const bool hasTrend = m_trend != MdStatisticTrend::None || !m_trendText.isEmpty();
    if (hasTrend) {
        rowWidth += kInlineGap + kTrendIcon;
        if (!m_trendText.isEmpty()) {
            rowWidth += trendFm.horizontalAdvance(m_trendText);
        }
    }

    width = qMax(width, rowWidth);
    height += valueLine;

    if (hasTrend && !m_trendText.isEmpty()) {
        height += kTrendGap + MdTypeScale::lineHeight(TypeStyle::LabelMedium);
    }

    return QSizeF(width, height);
}

QSize MdStatistic::sizeHint() const
{
    const QSizeF s = measure();
    return QSize(qCeil(s.width()), qCeil(s.height()));
}

QSize MdStatistic::minimumSizeHint() const
{
    return sizeHint();
}

void MdStatistic::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF bounds = QRectF(rect());
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal originX = rtl ? bounds.right() : bounds.left();
    const int sign = rtl ? -1 : 1;

    qreal y = bounds.top();

    if (!m_title.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::LabelLarge));
        painter.setPen(resolvedTitleColor());
        const qreal h = MdTypeScale::lineHeight(TypeStyle::LabelLarge);
        const QRectF titleRect(bounds.left(), y, bounds.width(), h);
        const int align = (rtl ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter;
        painter.drawText(titleRect, align, m_title);
        y += h + kTitleGap;
    }

    const QFont valueFont = MdTypeScale::font(m_valueStyle);
    const QFontMetricsF valueFm(valueFont);
    const qreal valueLine = MdTypeScale::lineHeight(m_valueStyle);

    qreal x = originX;
    painter.setFont(valueFont);

    auto drawRun = [&](const QString &text, const QColor &color, qreal *cursor) {
        if (text.isEmpty()) {
            return;
        }
        painter.setPen(color);
        const qreal w = valueFm.horizontalAdvance(text);
        const QRectF r(sign > 0 ? *cursor : *cursor - w, y, w, valueLine);
        painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, text);
        *cursor += w * sign;
    };

    drawRun(m_prefix, resolvedValueColor(), &x);
    x += kInlineGap * sign;
    drawRun(formattedValue(), resolvedValueColor(), &x);
    if (!m_suffix.isEmpty()) {
        x += kInlineGap * sign;
        drawRun(m_suffix, resolvedTitleColor(), &x);
    }

    const bool hasTrend = m_trend != MdStatisticTrend::None || !m_trendText.isEmpty();
    if (hasTrend) {
        x += kInlineGap * sign;
        const QRectF iconRect(sign > 0 ? x : x - kTrendIcon,
                              y + (valueLine - kTrendIcon) / 2.0,
                              kTrendIcon, kTrendIcon);
        MdIcon::paint(&painter, iconRect, arrowName(m_trend), resolvedTrendColor());
        x += kTrendIcon * sign;
        if (!m_trendText.isEmpty()) {
            painter.setFont(MdTypeScale::font(TypeStyle::LabelMedium));
            painter.setPen(resolvedTrendColor());
            const QFontMetricsF trendFm(painter.font());
            const qreal w = trendFm.horizontalAdvance(m_trendText);
            const qreal h = MdTypeScale::lineHeight(TypeStyle::LabelMedium);
            const QRectF r(sign > 0 ? x : x - w, y + (valueLine - h) / 2.0, w, h);
            painter.drawText(r, Qt::AlignLeft | Qt::AlignVCenter, m_trendText);
        }
    }
}

void MdStatistic::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        update();
    }
}

void MdStatistic::onThemeChanged()
{
    update();
}

} // namespace md
