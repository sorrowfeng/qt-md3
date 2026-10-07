#ifndef MD_MOTION_H
#define MD_MOTION_H

// Motion: the md.sys.motion easing and duration tokens, and the M3 Expressive
// spring physics that runs alongside them.
//
// Durations and cubic-bezier easings are transcribed from material-web
// tokens/versions/v0_192/_md-sys-motion.scss. The six spring slots come from
// Compose Material3's ExpressiveMotionTokens.kt — material-web has no
// Expressive support, so Compose is the authority there.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QList>
#include <QtCore/QtGlobal>

namespace md {

/// A damped harmonic oscillator with unit mass, as used by Compose's
/// `Spring.Stiffness*` / damping-ratio pair.
///
/// `valueAt` returns a normalised 0..1 displacement released from rest, which
/// is what a shape/position animation needs.
class QT_MD3_EXPORT MdSpring
{
public:
    MdSpring() = default;
    MdSpring(qreal stiffness, qreal dampingRatio);
    static MdSpring fromToken(MotionSpring spring);

    qreal stiffness() const { return m_stiffness; }
    qreal dampingRatio() const { return m_dampingRatio; }

    /// Normalised progress in [0, 1] at `seconds` after release. May slightly
    /// overshoot 1.0 for underdamped springs (damping ratio < 1).
    qreal valueAt(qreal seconds) const;

    /// Time until the spring stays within `tolerance` of its resting value.
    qreal settlingDurationMs(qreal tolerance = 0.001) const;

    bool isValid() const { return m_stiffness > 0.0 && m_dampingRatio > 0.0; }

private:
    qreal m_stiffness = 380.0;
    qreal m_dampingRatio = 0.8;
};

class QT_MD3_EXPORT MdMotion
{
public:
    static int durationMs(MotionDuration duration);
    static QEasingCurve easing(MotionEasing easing);
    static MdSpring spring(MotionSpring spring);

    /// Cubic-bezier control points {x1, y1, x2, y2} as published.
    static QList<qreal> easingControlPoints(MotionEasing easing);

    /// Eased progress for `t` in [0, 1], evaluated through the token's
    /// cubic-bezier curve. Matches CSS `cubic-bezier(x1, y1, x2, y2)`.
    static qreal easedValue(MotionEasing easing, qreal t);
};

} // namespace md

#endif // MD_MOTION_H
