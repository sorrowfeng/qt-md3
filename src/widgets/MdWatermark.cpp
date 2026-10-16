#include "MdWatermark.h"

#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetricsF>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtMath>

namespace md {

MdWatermark::MdWatermark(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdWatermark::~MdWatermark() = default;

void MdWatermark::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("watermark"));
    // Transparent for mouse so content underneath stays interactive.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    MdStyleBase::connectThemeUpdate(this, &MdWatermark::onThemeChanged);
}

void MdWatermark::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    update();
    emit textChanged(m_text);
}

void MdWatermark::setPixmap(const QPixmap &pixmap)
{
    if (m_pixmap.cacheKey() == pixmap.cacheKey()) {
        return;
    }
    m_pixmap = pixmap;
    update();
    emit pixmapChanged(m_pixmap);
}

void MdWatermark::setAngle(qreal angle)
{
    if (qFuzzyCompare(m_angle, angle)) {
        return;
    }
    m_angle = angle;
    update();
    emit angleChanged(m_angle);
}

void MdWatermark::setGap(qreal gap)
{
    const qreal clamped = qMax<qreal>(8.0, gap);
    if (qFuzzyCompare(m_gap, clamped)) {
        return;
    }
    m_gap = clamped;
    update();
    emit gapChanged(m_gap);
}

void MdWatermark::setFontSize(qreal size)
{
    const qreal clamped = qBound<qreal>(6.0, size, 256.0);
    if (qFuzzyCompare(m_fontSize, clamped)) {
        return;
    }
    m_fontSize = clamped;
    update();
    emit fontSizeChanged(m_fontSize);
}

void MdWatermark::setOpacity(qreal opacity)
{
    const qreal clamped = qBound<qreal>(0.0, opacity, 1.0);
    if (qFuzzyCompare(m_opacity, clamped)) {
        return;
    }
    m_opacity = clamped;
    update();
    emit opacityChanged(m_opacity);
}

QColor MdWatermark::resolvedStampColor() const
{
    // md.sys.color.on-surface — alpha applied at paint time from `opacity`.
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QSize MdWatermark::sizeHint() const
{
    return QSize(160, 120);
}

QSize MdWatermark::minimumSizeHint() const
{
    return QSize(40, 40);
}

void MdWatermark::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    if (m_text.isEmpty() && m_pixmap.isNull()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QColor stamp = resolvedStampColor();
    stamp.setAlphaF(qBound<qreal>(0.0, m_opacity, 1.0) * stamp.alphaF());

    if (!m_pixmap.isNull()) {
        const qreal dpr = m_pixmap.devicePixelRatioF();
        const QSizeF natural(m_pixmap.width() / dpr, m_pixmap.height() / dpr);
        const qreal stepX = qMax(natural.width() + m_gap, m_gap);
        const qreal stepY = qMax(natural.height() + m_gap, m_gap);
        painter.save();
        painter.translate(rect().center());
        painter.rotate(m_angle);
        // Walk a generous rotated bound so the corners stay covered.
        const qreal reach = qMax(width(), height()) * 1.5;
        painter.setOpacity(m_opacity);
        for (qreal y = -reach; y <= reach; y += stepY) {
            for (qreal x = -reach; x <= reach; x += stepX) {
                painter.drawPixmap(QPointF(x, y), m_pixmap);
            }
        }
        painter.restore();
        return;
    }

    QFont font = painter.font();
    font.setPixelSize(qRound(m_fontSize));
    painter.setFont(font);
    const QFontMetricsF fm(font);
    const qreal tw = fm.horizontalAdvance(m_text);
    const qreal th = fm.height();
    const qreal stepX = qMax(tw + m_gap, m_gap);
    const qreal stepY = qMax(th + m_gap, m_gap);

    painter.save();
    painter.translate(rect().center());
    painter.rotate(m_angle);
    painter.setPen(stamp);
    const qreal reach = qMax(width(), height()) * 1.5;
    for (qreal y = -reach; y <= reach; y += stepY) {
        for (qreal x = -reach; x <= reach; x += stepX) {
            painter.drawText(QRectF(x, y, tw + 8.0, th), Qt::AlignLeft | Qt::AlignVCenter,
                             m_text);
        }
    }
    painter.restore();
}

void MdWatermark::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdWatermark::onThemeChanged()
{
    update();
}

} // namespace md
