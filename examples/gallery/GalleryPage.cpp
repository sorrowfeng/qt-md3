#include "GalleryPage.h"

#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetricsF>
#include <QtGui/QTextOption>
#include <QtGui/QPainterPath>

#include <cmath>

namespace gallery {

namespace {

/// Content gutter inside a page.
constexpr qreal kGutter = 32.0;
/// Space between a section heading and the block under it.
constexpr qreal kHeadingGap = 12.0;

} // namespace

// ---------------------------------------------------------------------------
// GalleryContext
// ---------------------------------------------------------------------------

GalleryContext::GalleryContext(QPainter *painter, qreal width, qreal top)
    : m_painter(painter)
    , m_width(width)
    , m_y(top)
{
}

qreal GalleryContext::textHeight(const QFont &font, const QString &text, qreal width) const
{
    const QFontMetricsF metrics(font);
    const QRectF bounds =
        metrics.boundingRect(QRectF(0, 0, width, 1e6),
                             Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    return bounds.height();
}

void GalleryContext::section(const QString &text)
{
    const QFont font = GalleryPage::fontFor(md::TypeStyle::TitleMedium);
    const qreal height = textHeight(font, text, m_width);
    if (m_painter != nullptr) {
        m_painter->setFont(font);
        m_painter->setPen(GalleryPage::role(md::ColorRole::OnSurface));
        m_painter->drawText(QRectF(0, m_y, m_width, height),
                            Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    }
    m_y += height + kHeadingGap;
}

void GalleryContext::paragraph(const QString &text)
{
    const QFont font = GalleryPage::fontFor(md::TypeStyle::BodyMedium);
    const qreal height = textHeight(font, text, m_width);
    if (m_painter != nullptr) {
        m_painter->setFont(font);
        m_painter->setPen(GalleryPage::role(md::ColorRole::OnSurfaceVariant));
        m_painter->drawText(QRectF(0, m_y, m_width, height),
                            Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    }
    m_y += height + 8.0;
}

void GalleryContext::detail(const QString &text)
{
    const QFont font = GalleryPage::fontFor(md::TypeStyle::LabelSmall);
    const QFontMetricsF metrics(font);
    const qreal height = metrics.height();
    if (m_painter != nullptr) {
        m_painter->setFont(font);
        m_painter->setPen(GalleryPage::role(md::ColorRole::OnSurfaceVariant));
        m_painter->drawText(QPointF(0.0, m_y + metrics.ascent()), text);
    }
    m_y += height + 2.0;
}

void GalleryContext::space(qreal amount)
{
    m_y += amount;
}

QRectF GalleryContext::band(qreal height)
{
    const QRectF rect(0.0, m_y, m_width, height);
    m_y += height;
    return rect;
}

void GalleryContext::chip(const QString &text, const QColor &background, const QColor &foreground)
{
    const QFont font = GalleryPage::fontFor(md::TypeStyle::LabelMedium);
    const QFontMetricsF metrics(font);
    const qreal height = metrics.height() + 8.0;
    const qreal width = metrics.horizontalAdvance(text) + 20.0;
    const QRectF rect(0.0, m_y, width, height);
    if (m_painter != nullptr) {
        m_painter->save();
        m_painter->setRenderHint(QPainter::Antialiasing, true);
        GalleryPage::fillRounded(*m_painter, rect, background, md::ShapeCorner::Small);
        m_painter->setFont(font);
        m_painter->setPen(foreground);
        m_painter->drawText(rect, Qt::AlignCenter, text);
        m_painter->restore();
    }
    m_y += height + 6.0;
}

// ---------------------------------------------------------------------------
// GalleryPage
// ---------------------------------------------------------------------------

GalleryPage::GalleryPage(QWidget *parent)
    : QWidget(parent)
{
    // React to the theme the sanctioned way: subscribe, do not scan.
    md::MdStyleBase::connectThemeUpdate(this, &GalleryPage::update);
    connect(&md::MdTheme::instance(), &md::MdTheme::themeModeChanged, this, [this] {
        updateGeometry();
        update();
    });
}

int GalleryPage::heightForWidth(int width) const
{
    return int(std::ceil(measure(qreal(width))));
}

QSize GalleryPage::sizeHint() const
{
    const qreal width = qMax<qreal>(qreal(this->width()) - 2.0 * kGutter, 320.0);
    return QSize(int(width + 2.0 * kGutter), int(std::ceil(measure(width))));
}

QSize GalleryPage::minimumSizeHint() const
{
    return QSize(320, 200);
}

qreal GalleryPage::measure(qreal width) const
{
    GalleryContext context(nullptr, qMax<qreal>(width, 100.0), kGutter);
    // build() is const-correct in spirit; the context owns the cursor.
    const_cast<GalleryPage *>(this)->build(context);
    return context.y() + kGutter;
}

void GalleryPage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    // Page background.
    painter.fillRect(rect(), role(md::ColorRole::Surface));

    const QRectF content(kGutter, kGutter, qMax<qreal>(width() - 2.0 * kGutter, 100.0),
                         qMax<qreal>(height() - 2.0 * kGutter, 0.0));
    m_contentRect = content;

    painter.save();
    painter.setClipRect(content);
    painter.translate(content.topLeft());

    // Page heading.
    const QFont titleFont = fontFor(md::TypeStyle::HeadlineMedium);
    const QFontMetricsF titleMetrics(titleFont);
    painter.setFont(titleFont);
    painter.setPen(role(md::ColorRole::OnSurface));
    painter.drawText(QPointF(0.0, titleMetrics.ascent()), title());

    qreal y = titleMetrics.height() + 4.0;
    const QString sub = subtitle();
    if (!sub.isEmpty()) {
        const QFont subFont = fontFor(md::TypeStyle::BodyLarge);
        const QFontMetricsF subMetrics(subFont);
        painter.setFont(subFont);
        painter.setPen(role(md::ColorRole::OnSurfaceVariant));
        painter.drawText(QPointF(0.0, y + subMetrics.ascent()), sub);
        y += subMetrics.height() + 12.0;
    }

    GalleryContext context(&painter, content.width(), y + 8.0);
    build(context);
    painter.restore();
}

void GalleryPage::beginOverlay(QPainter &painter) const
{
    painter.save();
    painter.setClipRect(m_contentRect);
    painter.translate(m_contentRect.topLeft());
}

QFont GalleryPage::fontFor(md::TypeStyle style, md::TypeEmphasis emphasis)
{
    return md::MdTypeScale::font(style, emphasis, md::MdTheme::instance().scriptCategory());
}

QColor GalleryPage::role(md::ColorRole colorRole)
{
    return md::MdTheme::instance().color(colorRole);
}

void GalleryPage::fillRounded(QPainter &painter, const QRectF &rect, const QColor &color,
                              md::ShapeCorner corner)
{
    const QList<qreal> radii = md::MdShape::resolvedRadii(corner, rect.size());
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPath(md::MdShape::roundedRect(rect, radii));
}

void GalleryPage::paintCard(QPainter &painter, const QRectF &rect)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    // MD3 expresses containment with a tonal surface, not a shadow.
    fillRounded(painter, rect, role(md::ColorRole::SurfaceContainer), md::ShapeCorner::Medium);

    const QList<qreal> radii = md::MdShape::resolvedRadii(md::ShapeCorner::Medium, rect.size());
    QPen pen(role(md::ColorRole::OutlineVariant));
    pen.setWidthF(1.0);
    pen.setCosmetic(true);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(md::MdShape::roundedRect(rect.adjusted(0.5, 0.5, -0.5, -0.5), radii));
    painter.restore();
}

void GalleryPage::paintElevated(QPainter &painter, const QRectF &rect, md::ElevationLevel level)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    fillRounded(painter, rect, md::MdElevation::surfaceColor(level, md::MdTheme::instance().scheme()),
                md::ShapeCorner::Medium);
    painter.restore();
}

} // namespace gallery
