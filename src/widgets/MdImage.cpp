#include "MdImage.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QHideEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QShowEvent>
#include <QtGui/QWheelEvent>
#include <QtMath>

namespace md {

// ---------------------------------------------------------------------------
// MdImage
// ---------------------------------------------------------------------------

MdImage::MdImage(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdImage::MdImage(const QPixmap &pixmap, QWidget *parent)
    : QWidget(parent)
{
    init();
    setPixmap(pixmap);
}

MdImage::~MdImage() = default;

void MdImage::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("image"));
    setAttribute(Qt::WA_Hover, true);
    if (m_previewEnabled) {
        setCursor(Qt::PointingHandCursor);
    }
    MdStyleBase::connectThemeUpdate(this, &MdImage::onThemeChanged);
}

void MdImage::setPixmap(const QPixmap &pixmap)
{
    if (m_pixmap.cacheKey() == pixmap.cacheKey()) {
        return;
    }
    m_pixmap = pixmap;
    update();
    emit pixmapChanged(m_pixmap);
}

void MdImage::setSource(const QString &source)
{
    if (m_source == source) {
        return;
    }
    m_source = source;
    setAccessibleName(m_altText.isEmpty() ? m_source : m_altText);
    emit sourceChanged(m_source);
}

bool MdImage::load(const QString &path)
{
    QPixmap px(path);
    if (px.isNull()) {
        return false;
    }
    setSource(path);
    setPixmap(px);
    return true;
}

void MdImage::setFit(MdImageFit fit)
{
    if (fit >= MdImageFit::Count) {
        return;
    }
    if (m_fit == fit) {
        return;
    }
    m_fit = fit;
    update();
    emit fitChanged(m_fit);
}

void MdImage::setShape(ShapeCorner corner)
{
    if (corner >= ShapeCorner::Count) {
        return;
    }
    if (m_shape == corner) {
        return;
    }
    m_shape = corner;
    update();
    emit shapeChanged(m_shape);
}

void MdImage::setPreviewEnabled(bool enabled)
{
    if (m_previewEnabled == enabled) {
        return;
    }
    m_previewEnabled = enabled;
    setCursor(enabled ? Qt::PointingHandCursor : Qt::ArrowCursor);
    emit previewEnabledChanged(m_previewEnabled);
}

void MdImage::setAltText(const QString &text)
{
    if (m_altText == text) {
        return;
    }
    m_altText = text;
    setAccessibleName(m_altText.isEmpty() ? m_source : m_altText);
    emit altTextChanged(m_altText);
}

QColor MdImage::resolvedBackgroundColor() const
{
    // md.sys.color.surface-container-lowest
    return MdTheme::instance().color(ColorRole::SurfaceContainerLowest);
}

QColor MdImage::resolvedScrimColor() const
{
    // md.sys.color.scrim
    return MdTheme::instance().color(ColorRole::Scrim);
}

bool MdImage::isPreviewOpen() const
{
    return m_preview != nullptr && m_preview->isVisible();
}

QSize MdImage::sizeHint() const
{
    if (!m_pixmap.isNull()) {
        return m_pixmap.size() / m_pixmap.devicePixelRatioF();
    }
    return QSize(160, 120);
}

QSize MdImage::minimumSizeHint() const
{
    return QSize(48, 48);
}

void MdImage::openPreview()
{
    if (!m_previewEnabled) {
        return;
    }
    ensurePreview();
    m_preview->setImage(m_pixmap, m_altText.isEmpty() ? m_source : m_altText);
    m_preview->show();
    m_preview->raise();
    m_preview->activateWindow();
    emit previewOpened();
}

void MdImage::closePreview()
{
    if (!m_preview) {
        return;
    }
    m_preview->hide();
}

void MdImage::ensurePreview()
{
    if (m_preview) {
        return;
    }
    // Top-level dialog-like surface: no parent so it can cover the screen,
    // flagged as a dialog so Esc / window close behave predictably.
    m_preview = new MdImagePreview(nullptr);
    m_preview->setAttribute(Qt::WA_DeleteOnClose, false);
    connect(m_preview, &MdImagePreview::closed,
            this, &MdImage::previewClosed);
    connect(m_preview, &QObject::destroyed,
            this, &MdImage::onPreviewDestroyed);
}

void MdImage::onPreviewDestroyed()
{
    m_preview = nullptr;
    emit previewClosed();
}

void MdImage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QRectF bounds = QRectF(rect());
    const QPainterPath clip = MdShape::roundedRect(bounds, m_shape);
    painter.fillPath(clip, resolvedBackgroundColor());

    if (m_pixmap.isNull()) {
        return;
    }

    painter.save();
    painter.setClipPath(clip);

    const qreal dpr = m_pixmap.devicePixelRatioF();
    const QSizeF natural(m_pixmap.width() / dpr, m_pixmap.height() / dpr);
    QRectF target = bounds;

    switch (m_fit) {
    case MdImageFit::Contain: {
        const qreal sx = bounds.width() / natural.width();
        const qreal sy = bounds.height() / natural.height();
        const qreal s = qMin(sx, sy);
        target = QRectF(bounds.center().x() - natural.width() * s * 0.5,
                        bounds.center().y() - natural.height() * s * 0.5,
                        natural.width() * s, natural.height() * s);
        break;
    }
    case MdImageFit::Cover:
    case MdImageFit::None:
    case MdImageFit::Count:
        break;
    }

    if (m_fit == MdImageFit::None) {
        painter.drawPixmap(QPointF(bounds.left(), bounds.top()), m_pixmap);
    } else {
        painter.drawPixmap(target, m_pixmap, QRectF(m_pixmap.rect()));
    }
    painter.restore();
}

void MdImage::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_previewEnabled && rect().contains(md::mousePosition(event).toPoint())) {
        openPreview();
        event->accept();
        return;
    }
    QWidget::mouseReleaseEvent(event);
}

void MdImage::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdImage::onThemeChanged()
{
    update();
    if (m_preview) {
        m_preview->update();
    }
}

// ---------------------------------------------------------------------------
// MdImagePreview
// ---------------------------------------------------------------------------

namespace {

/// Close button hit target (`md.comp.icon-button.standard.state-layer.size` 40).
constexpr qreal kCloseHit = 40.0;
/// Close glyph (`md.comp.icon.size` 24).
constexpr qreal kCloseIcon = 24.0;
/// Margin from the top-end corner (`md.sys.spacing` 16).
constexpr qreal kCloseMargin = 16.0;
/// Zoom wheel step (gesture, no published token).
constexpr qreal kZoomStep = 0.12;

} // namespace

MdImagePreview::MdImagePreview(QWidget *parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setWindowState(Qt::WindowFullScreen);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setFocusPolicy(Qt::StrongFocus);
    setAccessibleName(QStringLiteral("image preview"));
    setMouseTracking(true);
}

MdImagePreview::~MdImagePreview() = default;

void MdImagePreview::setImage(const QPixmap &pixmap, const QString &altText)
{
    m_pixmap = pixmap;
    m_altText = altText;
    m_zoom = 1.0;
    m_offset = QPointF();
    setAccessibleName(altText.isEmpty() ? QStringLiteral("image preview") : altText);
    update();
}

void MdImagePreview::zoomIn()
{
    m_zoom = qMin<qreal>(8.0, m_zoom + kZoomStep);
    update();
}

void MdImagePreview::zoomOut()
{
    m_zoom = qMax<qreal>(0.25, m_zoom - kZoomStep);
    update();
}

void MdImagePreview::resetZoom()
{
    m_zoom = 1.0;
    m_offset = QPointF();
    update();
}

QRectF MdImagePreview::imageRect() const
{
    if (m_pixmap.isNull()) {
        return QRectF();
    }
    const qreal dpr = m_pixmap.devicePixelRatioF();
    const QSizeF natural(m_pixmap.width() / dpr, m_pixmap.height() / dpr);
    const QRectF avail = QRectF(rect()).adjusted(kCloseHit + kCloseMargin, kCloseMargin,
                                                 -(kCloseHit + kCloseMargin), -kCloseMargin);
    const qreal sx = avail.width() / natural.width();
    const qreal sy = avail.height() / natural.height();
    const qreal fit = qMin<qreal>(1.0, qMin(sx, sy));
    const qreal s = fit * m_zoom;
    return QRectF(avail.center().x() - natural.width() * s * 0.5 + m_offset.x(),
                  avail.center().y() - natural.height() * s * 0.5 + m_offset.y(),
                  natural.width() * s, natural.height() * s);
}

void MdImagePreview::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // md.sys.color.scrim over the surface behind the dialog.
    QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
    if (scrim.alpha() < 255) {
        // Scrim is published as an opaque role colour; the overlay reading
        // composites it at the dialog's translucent surface.
        scrim.setAlpha(230);
    }
    painter.fillRect(rect(), scrim);

    if (!m_pixmap.isNull()) {
        painter.drawPixmap(imageRect(), m_pixmap, QRectF(m_pixmap.rect()));
    }

    // Close IconButton, top-end: surface-container disc + on-surface glyph.
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const qreal cx = rtl ? kCloseMargin + kCloseHit * 0.5
                         : width() - kCloseMargin - kCloseHit * 0.5;
    const qreal cy = kCloseMargin + kCloseHit * 0.5;
    const QRectF closeRect(cx - kCloseHit * 0.5, cy - kCloseHit * 0.5, kCloseHit, kCloseHit);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(ColorRole::SurfaceContainerHigh));
    painter.drawEllipse(closeRect);
    const QRectF iconRect(cx - kCloseIcon * 0.5, cy - kCloseIcon * 0.5, kCloseIcon, kCloseIcon);
    MdIcon::paint(&painter, iconRect, QStringLiteral("close"),
                  MdTheme::instance().color(ColorRole::OnSurface));
}

void MdImagePreview::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    const QPoint delta = event->angleDelta();
#else
    const QPoint delta = event->angleDelta();
#endif
    if (delta.y() > 0) {
        zoomIn();
    } else if (delta.y() < 0) {
        zoomOut();
    }
    event->accept();
}

void MdImagePreview::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Escape:
    case Qt::Key_Back:
        hide();
        emit closed();
        event->accept();
        return;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        zoomIn();
        event->accept();
        return;
    case Qt::Key_Minus:
        zoomOut();
        event->accept();
        return;
    case Qt::Key_0:
        resetZoom();
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdImagePreview::mousePressEvent(QMouseEvent *event)
{
    const bool rtl = layoutDirection() == Qt::RightToLeft;
    const QPointF closeCentre(rtl ? kCloseMargin + kCloseHit * 0.5
                                  : width() - kCloseMargin - kCloseHit * 0.5,
                              kCloseMargin + kCloseHit * 0.5);
    const QPointF pos = md::mousePosition(event);
    if (QLineF(pos, closeCentre).length() <= kCloseHit * 0.5) {
        hide();
        emit closed();
        event->accept();
        return;
    }
    m_panning = true;
    m_panStart = pos;
    m_offsetStart = m_offset;
    event->accept();
}

void MdImagePreview::mouseMoveEvent(QMouseEvent *event)
{
    if (m_panning) {
        const QPointF pos = md::mousePosition(event);
        m_offset = m_offsetStart + (pos - m_panStart);
        update();
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void MdImagePreview::mouseReleaseEvent(QMouseEvent *event)
{
    m_panning = false;
    event->accept();
}

void MdImagePreview::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    setFocus(Qt::OtherFocusReason);
}

void MdImagePreview::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    emit closed();
}

} // namespace md
