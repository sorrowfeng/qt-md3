#include "MdFocusRing.h"

#include "MdMotion.h"

#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPainterPath>

#include <cmath>

namespace md {

namespace {

constexpr int kTickMs = 16;

/// Grow a rect by `amount` on every side. Positive keeps the CSS `outline`
/// spirit (stroke centre line `width / 2` beyond the ring's own rect) and
/// negative gives the `border` spirit for the inward variant.
QRectF inflate(const QRectF &rect, qreal amount)
{
    return QRectF(rect.left() - amount, rect.top() - amount,
                  rect.width() + 2.0 * amount, rect.height() + 2.0 * amount);
}

} // namespace

int MdFocusRing::growMs(const MdFocusRingSpec &spec)
{
    // animation-duration: calc(duration * 0.25)
    return qMax(1, int(std::lround(spec.durationMs * 0.25)));
}

int MdFocusRing::settleMs(const MdFocusRingSpec &spec)
{
    // animation-duration: calc(duration * 0.75)
    return qMax(1, spec.durationMs - growMs(spec));
}

int MdFocusRing::totalMs(const MdFocusRingSpec &spec)
{
    return growMs(spec) + settleMs(spec);
}

qreal MdFocusRing::widthAt(const MdFocusRingSpec &spec, int elapsedMs)
{
    if (elapsedMs < 0) {
        // Settled.
        return spec.width;
    }
    const int grow = growMs(spec);
    if (elapsedMs < grow) {
        // @keyframes outward-grow { from { outline-width: 0 } to { ... } }
        return spec.activeWidth * MdMotion::easedValue(spec.easing, qreal(elapsedMs) / qreal(grow));
    }
    const int settle = settleMs(spec);
    if (elapsedMs >= grow + settle) {
        return spec.width;
    }
    // @keyframes outward-shrink { from { outline-width: active-width } }
    // The `to` value is the element's own `outline-width`, i.e. `width`.
    const qreal t = qreal(elapsedMs - grow) / qreal(settle);
    return spec.activeWidth
           + (spec.width - spec.activeWidth) * MdMotion::easedValue(spec.easing, t);
}

QRectF MdFocusRing::ringRect(const QRectF &componentBounds, const MdFocusRingSpec &spec)
{
    // Both variants are "gap, then half a stroke, then the stroke reaches the
    // component edge". Outward grows, inward shrinks; the gap is the variant's
    // own offset token. Writing it once keeps the two branches from drifting.
    const qreal centreLine = spec.offset() + spec.width / 2.0;
    return inflate(componentBounds, spec.inward ? -centreLine : centreLine);
}

QList<qreal> MdFocusRing::ringRadii(const QList<qreal> &componentRadii,
                                    const MdFocusRingSpec &spec)
{
    QList<qreal> radii;
    if (componentRadii.isEmpty()) {
        return radii;
    }
    const qreal centreLine = spec.offset() + spec.width / 2.0;
    const qreal delta = spec.inward ? -centreLine : centreLine;
    radii.reserve(componentRadii.size());
    for (qreal radius : componentRadii) {
        const qreal resolved = radius < 0.0 ? 0.0 : radius;
        radii.append(qMax<qreal>(resolved + delta, 0.0));
    }
    return radii;
}

void MdFocusRing::paint(QPainter *painter,
                        const QRectF &componentBounds,
                        const QList<qreal> &componentRadii,
                        const QColor &color,
                        const MdFocusRingSpec &spec,
                        int elapsedMs)
{
    if (painter == nullptr || !color.isValid()) {
        return;
    }

    QList<qreal> radii = componentRadii;
    if (radii.isEmpty()) {
        // No explicit radii: fall back to the `shape` token, resolved against
        // the component's size so `corner-full` becomes a pill.
        radii = MdShape::resolvedRadii(spec.shape, componentBounds.size());
    }

    const qreal width = widthAt(spec, elapsedMs);
    if (width <= 0.0) {
        return;
    }

    const QRectF rect = ringRect(componentBounds, spec);
    const QList<qreal> strokeRadii = ringRadii(radii, spec);

    QPen pen(color);
    pen.setWidthF(width);
    pen.setJoinStyle(Qt::RoundJoin);
    pen.setCapStyle(Qt::RoundCap);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(MdShape::roundedRect(rect, strokeRadii));
    painter->restore();
}

// ---------------------------------------------------------------------------
// MdFocusRingController
// ---------------------------------------------------------------------------

MdFocusRingController::MdFocusRingController(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
{
    m_timer->setInterval(kTickMs);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MdFocusRingController::onTick);
}

MdFocusRingController::~MdFocusRingController() = default;

void MdFocusRingController::setSpec(const MdFocusRingSpec &spec)
{
    m_spec = spec;
    emit repaintRequested();
}

qreal MdFocusRingController::currentWidth() const
{
    if (!m_animating) {
        return m_spec.width;
    }
    return MdFocusRing::widthAt(m_spec, m_elapsedMs);
}

void MdFocusRingController::start()
{
    m_elapsedMs = 0;
    m_animating = true;
    m_timer->start();
    emit repaintRequested();
}

void MdFocusRingController::stop()
{
    if (!m_animating) {
        return;
    }
    m_timer->stop();
    m_animating = false;
    m_elapsedMs = 0;
    emit repaintRequested();
}

void MdFocusRingController::onTick()
{
    m_elapsedMs += m_timer->interval();
    if (m_elapsedMs >= MdFocusRing::totalMs(m_spec)) {
        m_timer->stop();
        m_animating = false;
        m_elapsedMs = 0;
    }
    emit repaintRequested();
}

} // namespace md
