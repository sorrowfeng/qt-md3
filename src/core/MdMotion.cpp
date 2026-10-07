#include "MdMotion.h"

#include "MdTokens.h"

#include <QtCore/QPointF>
#include <algorithm>
#include <cmath>

namespace md {

namespace {

/// Solves the CSS cubic-bezier timing function: find s such that
/// bezierX(s) == t, then return bezierY(s).
qreal cubicBezier(qreal x1, qreal y1, qreal x2, qreal y2, qreal t)
{
    if (t <= 0.0) {
        return 0.0;
    }
    if (t >= 1.0) {
        return 1.0;
    }

    const auto bezier = [](qreal a, qreal b, qreal s) {
        // P0 = 0, P3 = 1.
        const qreal u = 1.0 - s;
        return 3.0 * u * u * s * a + 3.0 * u * s * s * b + s * s * s;
    };

    // Newton-Raphson with a bisection fallback, the standard approach.
    qreal s = t;
    for (int i = 0; i < 8; ++i) {
        const qreal x = bezier(x1, x2, s) - t;
        if (std::fabs(x) < 1e-6) {
            break;
        }
        const qreal u = 1.0 - s;
        const qreal dx = 3.0 * u * u * x1 + 6.0 * u * s * (x2 - x1) + 3.0 * s * s * (1.0 - x2);
        if (std::fabs(dx) < 1e-9) {
            break;
        }
        s -= x / dx;
        s = std::clamp(s, 0.0, 1.0);
    }
    return bezier(y1, y2, s);
}

} // namespace

// ---------------------------------------------------------------------------
// MdSpring
// ---------------------------------------------------------------------------

MdSpring::MdSpring(qreal stiffness, qreal dampingRatio)
    : m_stiffness(stiffness)
    , m_dampingRatio(dampingRatio)
{
}

MdSpring MdSpring::fromToken(MotionSpring spring)
{
    qreal stiffness = 380.0;
    qreal dampingRatio = 0.8;
    MdSystemTokens::springParameters(spring, &stiffness, &dampingRatio);
    return MdSpring(stiffness, dampingRatio);
}

qreal MdSpring::valueAt(qreal seconds) const
{
    if (seconds <= 0.0 || !isValid()) {
        return 0.0;
    }

    const qreal omega = std::sqrt(m_stiffness);
    const qreal zeta = m_dampingRatio;

    if (std::fabs(zeta - 1.0) < 1e-6) {
        // Critically damped.
        return 1.0 - std::exp(-omega * seconds) * (1.0 + omega * seconds);
    }

    if (zeta < 1.0) {
        // Underdamped; may overshoot.
        const qreal omegaD = omega * std::sqrt(1.0 - zeta * zeta);
        const qreal decay = std::exp(-zeta * omega * seconds);
        return 1.0
               - decay
                     * (std::cos(omegaD * seconds)
                        + (zeta * omega / omegaD) * std::sin(omegaD * seconds));
    }

    // Overdamped.
    const qreal root = omega * std::sqrt(zeta * zeta - 1.0);
    const qreal r1 = -zeta * omega + root;
    const qreal r2 = -zeta * omega - root;
    const qreal a = r2 / (r1 - r2);
    const qreal b = -a;
    return 1.0 + a * std::exp(r1 * seconds) + b * std::exp(r2 * seconds);
}

qreal MdSpring::settlingDurationMs(qreal tolerance) const
{
    if (!isValid()) {
        return 0.0;
    }
    // Springs here settle well inside 5 s; 1 ms resolution is plenty.
    constexpr int kMaxMs = 5000;
    int lastUnsettled = 0;
    for (int ms = 1; ms <= kMaxMs; ++ms) {
        const qreal value = valueAt(ms / 1000.0);
        if (std::fabs(1.0 - value) > tolerance) {
            lastUnsettled = ms;
        }
    }
    return qreal(lastUnsettled + 1);
}

// ---------------------------------------------------------------------------
// MdMotion
// ---------------------------------------------------------------------------

int MdMotion::durationMs(MotionDuration duration)
{
    return MdSystemTokens::durationMs(duration);
}

QList<qreal> MdMotion::easingControlPoints(MotionEasing easing)
{
    return MdSystemTokens::easingBezier(easing);
}

QEasingCurve MdMotion::easing(MotionEasing easing)
{
    const QList<qreal> points = easingControlPoints(easing);
    QEasingCurve curve(QEasingCurve::BezierSpline);
    // A cubic-bezier has two control points and an implicit endpoint at (1, 1).
    curve.addCubicBezierSegment(QPointF(points.at(0), points.at(1)),
                                QPointF(points.at(2), points.at(3)),
                                QPointF(1.0, 1.0));
    return curve;
}

MdSpring MdMotion::spring(MotionSpring spring)
{
    return MdSpring::fromToken(spring);
}

qreal MdMotion::easedValue(MotionEasing easing, qreal t)
{
    const QList<qreal> points = easingControlPoints(easing);
    return cubicBezier(points.at(0), points.at(1), points.at(2), points.at(3), t);
}

} // namespace md
