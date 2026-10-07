#include "MdRipple.h"

#include "MdMotion.h"
#include "MdStateLayer.h"

#include <QtCore/QTimer>
#include <QtCore/QtMath>
#include <QtGui/QPainter>
#include <QtGui/QRadialGradient>

namespace md {

namespace {

/// Repaint cadence while a ripple is alive. Material-web animates at the
/// display refresh rate; 60 Hz is the floor every Qt platform can honour.
constexpr int kTickMs = 16;

} // namespace

// ---------------------------------------------------------------------------
// Geometry and timing
// ---------------------------------------------------------------------------

MdRipple::Geometry MdRipple::geometryFor(const QSizeF &bounds)
{
    Geometry geometry;
    // Both extents must be positive. Clamping a negative extent to zero would
    // hide a caller bug and produce a geometry derived from a box that does not
    // exist; a zero extent means there is no area for the circle to cover.
    const qreal width = bounds.width();
    const qreal height = bounds.height();
    if (width <= 0.0 || height <= 0.0) {
        return geometry;
    }
    const qreal maxDimension = qMax(width, height);
    // determineRippleSize():
    //   softEdgeSize = max(0.35 * maxDim, 75)
    //   initialSize  = floor(maxDim * 0.2)
    //   maxRadius    = hypot(w, h) + 10
    //   rippleScale  = (maxRadius + softEdgeSize) / initialSize
    //
    // The published rippleScale is a *diameter* multiplier, so the radius the
    // circle reaches is half of the scaled bounding box.
    geometry.softEdgeSize =
        qMax(SoftEdgeContainerRatio * maxDimension, SoftEdgeMinimumSize);
    geometry.initialSize = std::floor(maxDimension * InitialOriginScale);
    if (geometry.initialSize < 1.0) {
        geometry.initialSize = 1.0;
    }
    const qreal maxRadius = std::hypot(width, height) + Padding;
    geometry.finalRadius = (maxRadius + geometry.softEdgeSize) / 2.0;

    // _ripple.scss paints `radial-gradient(closest-side, color
    // max(calc(100% - 70px), 65%), transparent 100%)` inside the unscaled
    // bounding box, whose closest side is initialSize / 2. The gradient scales
    // with the box, so the same normalised fraction applies at every size.
    const qreal softEdgeFraction = (geometry.initialSize / 2.0) > 0.0
                                       ? 70.0 / (geometry.initialSize / 2.0)
                                       : 1.0;
    geometry.softEdgeStart = qMax(1.0 - softEdgeFraction, 0.65);
    return geometry;
}

qreal MdRipple::progressAt(int elapsedMs)
{
    if (elapsedMs <= 0) {
        return 0.0;
    }
    if (elapsedMs >= PressGrowMs) {
        return 1.0;
    }
    return MdMotion::easedValue(GrowEasing, qreal(elapsedMs) / qreal(PressGrowMs));
}

QPointF MdRipple::centerAt(const MdRipple::Geometry &geometry,
                           const QPointF &pressPosition,
                           const QSizeF &bounds,
                           qreal progress)
{
    if (!geometry.isValid()) {
        return QPointF(bounds.width() / 2.0, bounds.height() / 2.0);
    }
    // getTranslationCoordinates(): the circle ends centred in the component.
    const QPointF start = pressPosition;
    const QPointF end(bounds.width() / 2.0, bounds.height() / 2.0);
    return QPointF(start.x() + (end.x() - start.x()) * progress,
                   start.y() + (end.y() - start.y()) * progress);
}

qreal MdRipple::radiusAt(const MdRipple::Geometry &geometry, qreal progress)
{
    const qreal startRadius = geometry.initialSize / 2.0;
    return startRadius + (geometry.finalRadius - startRadius) * progress;
}

MdRippleFrame MdRipple::frame(const MdRipple::Geometry &geometry,
                              const QPointF &pressPosition,
                              const QSizeF &bounds,
                              int elapsedMs,
                              int releasedAtMs)
{
    MdRippleFrame frame;
    if (!geometry.isValid() || elapsedMs < 0) {
        return frame;
    }

    const qreal growth = progressAt(elapsedMs);
    frame.center = centerAt(geometry, pressPosition, bounds, growth);
    frame.radius = radiusAt(geometry, growth);
    frame.softEdgeStart = geometry.softEdgeStart;
    frame.opacity = MdStateLayer::opacity(OverlayKind);
    frame.valid = true;

    if (releasedAtMs >= 0) {
        // The release is deferred until the ripple has been held for at least
        // MINIMUM_PRESS_MS so a quick tap still shows a full circle.
        const int fadeStart = qMax(releasedAtMs, MinimumPressMs);
        if (elapsedMs >= fadeStart) {
            const qreal fade =
                qMin<qreal>(qreal(elapsedMs - fadeStart) / qreal(FadeOutMs), 1.0);
            frame.opacity *= (1.0 - fade);
            if (fade >= 1.0) {
                frame.valid = false;
            }
        }
    }
    return frame;
}

int MdRipple::lifetimeMs(int releasedAtMs)
{
    if (releasedAtMs < 0) {
        // Still held: nothing to schedule, the press owns the lifetime.
        return -1;
    }
    return qMax(releasedAtMs, MinimumPressMs) + FadeOutMs;
}

void MdRipple::paint(QPainter *painter,
                     const MdRippleFrame &frame,
                     const QPainterPath &clipPath,
                     const QColor &contentColor,
                     qreal opacityScale)
{
    if (painter == nullptr || !frame.valid || frame.radius <= 0.0) {
        return;
    }
    const qreal alpha = qBound(0.0, frame.opacity * opacityScale, 1.0);
    if (alpha <= 0.0) {
        return;
    }

    QColor inner = contentColor;
    inner.setAlphaF(alpha);
    QColor outer = contentColor;
    outer.setAlphaF(0.0);

    QRadialGradient gradient(frame.center, frame.radius);
    gradient.setColorAt(0.0, inner);
    gradient.setColorAt(qBound(0.0, frame.softEdgeStart, 1.0), inner);
    gradient.setColorAt(1.0, outer);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    if (!clipPath.isEmpty()) {
        // The ripple is clipped to the component's current shape, so it
        // follows a corner radius that is being morphed.
        painter->setClipPath(clipPath);
    }
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawEllipse(frame.center, frame.radius, frame.radius);
    painter->restore();
}

// ---------------------------------------------------------------------------
// MdRippleController
// ---------------------------------------------------------------------------

MdRippleController::MdRippleController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(kTickMs);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MdRippleController::onTick);
}

MdRippleController::~MdRippleController() = default;

void MdRippleController::setBounds(const QSizeF &bounds)
{
    if (bounds == m_bounds) {
        return;
    }
    m_bounds = bounds;
    refreshGeometry();
    if (m_active) {
        emit repaintRequested();
    }
}

void MdRippleController::setContentColor(const QColor &contentColor)
{
    if (m_contentColor == contentColor) {
        return;
    }
    m_contentColor = contentColor;
    if (m_active) {
        emit repaintRequested();
    }
}

void MdRippleController::setClipPath(const QPainterPath &clipPath)
{
    // Deliberately silent: widgets call this right before painting, and the
    // repaint they are already inside is the one that needs it.
    m_clipPath = clipPath;
}

void MdRippleController::refreshGeometry()
{
    m_geometry = MdRipple::geometryFor(m_bounds);
}

void MdRippleController::press(const QPointF &position)
{
    refreshGeometry();
    if (!m_geometry.isValid()) {
        return;
    }
    m_pressPosition = position;
    m_elapsedMs = 0;
    m_releasedAtMs = -1;
    m_active = true;
    m_timer->start();
    emit repaintRequested();
}

void MdRippleController::pressCentered()
{
    press(QPointF(m_bounds.width() / 2.0, m_bounds.height() / 2.0));
}

void MdRippleController::release()
{
    if (!m_active || m_releasedAtMs >= 0) {
        return;
    }
    m_releasedAtMs = m_elapsedMs;
    // No further growth is needed if the ripple was already fully grown; the
    // fade still has to be driven, so the timer keeps running.
    if (!m_timer->isActive()) {
        m_timer->start();
    }
    emit repaintRequested();
}

void MdRippleController::cancel()
{
    if (!m_active) {
        return;
    }
    m_timer->stop();
    m_active = false;
    m_releasedAtMs = -1;
    m_elapsedMs = 0;
    emit repaintRequested();
}

MdRippleFrame MdRippleController::currentFrame() const
{
    // An idle controller has nothing to paint. `frame()` alone would happily
    // describe a fully-visible ripple at t = 0, so a caller that paints on
    // `frame.valid` rather than on `isActive()` would otherwise get a phantom
    // blob in the top-left corner before the first press.
    if (!m_active) {
        return MdRippleFrame{};
    }
    return MdRipple::frame(m_geometry, m_pressPosition, m_bounds, m_elapsedMs,
                           m_releasedAtMs);
}

void MdRippleController::onTick()
{
    if (!m_active) {
        m_timer->stop();
        return;
    }
    m_elapsedMs += m_timer->interval();

    if (m_releasedAtMs >= 0 && m_elapsedMs >= MdRipple::lifetimeMs(m_releasedAtMs)) {
        m_timer->stop();
        m_active = false;
        m_releasedAtMs = -1;
        m_elapsedMs = 0;
        emit repaintRequested();
        return;
    }
    // A held ripple stops growing after PressGrowMs but must keep repainting
    // only if it is still fading; once fully grown and still held, the frame
    // is static.
    if (m_releasedAtMs < 0 && m_elapsedMs >= MdRipple::PressGrowMs) {
        m_elapsedMs = MdRipple::PressGrowMs;
        m_timer->stop();
        emit repaintRequested();
        return;
    }
    emit repaintRequested();
}

} // namespace md
