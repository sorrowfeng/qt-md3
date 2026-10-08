#ifndef MD_PROGRESS_INDICATOR_STYLE_H
#define MD_PROGRESS_INDICATOR_STYLE_H

#include "core/MdProgressIndicatorTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

class QPainter;

namespace md {

class MdProgressIndicator;

/// Pattern A style for `MdProgressIndicator`: painting, geometry and the
/// indeterminate animation math for the merged `md.comp.progress-indicator.*`
/// family, registered in the paint hub so the first construction installs it
/// application-wide.
///
/// The animation numbers are material-web's, itself transplanted from MDC:
/// the linear two-bar 2 s keyframes and the circular three-composed-rotations
/// spinner. The keyframes are exposed as *pure functions of elapsed time* so
/// the test suite pins them field by field without waiting on an event loop.
class QT_MD3_EXPORT MdProgressIndicatorStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdProgressIndicatorStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdProgressIndicatorStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- published timing (material-web internal SCSS, MDC heritage) --------
    /// Linear determinate transition: 250 ms cubic-bezier(0.4, 0, 0.6, 1).
    static constexpr int kLinearDeterminateDurationMs = 250;
    /// Circular determinate transition: 500 ms cubic-bezier(0, 0, 0.2, 1).
    static constexpr int kCircularDeterminateDurationMs = 500;
    /// Linear indeterminate cycle: 2 s, linear-timed keyframes.
    static constexpr int kLinearIndeterminateDurationMs = 2000;
    /// Linear four-color cycle: twice the indeterminate cycle.
    static constexpr int kLinearFourColorDurationMs = 4000;
    /// Circular arc duration (expand + the per-segment group rotate):
    /// ARCTIME = 1333 ms; the whole cycle is 4× that.
    static constexpr int kCircularArcDurationMs = 1333;
    static constexpr int kCircularCycleDurationMs = 4 * kCircularArcDurationMs;
    /// Circular four-color runs over the whole cycle.
    static constexpr int kCircularFourColorDurationMs = kCircularCycleDurationMs;
    /// The linear spin wrapped around everything: ARCTIME * 360 / 306 ms.
    static constexpr double kCircularLinearRotateDurationMs =
        kCircularArcDurationMs * 360.0 / 306.0;

    /// The determinate transition easing for one shape, as a QEasingCurve.
    static QEasingCurve determinateEasing(ProgressIndicatorShape shape);

    // --- pure animation math ------------------------------------------------
    /// One frame of the linear indeterminate animation. `cycleProgress` is
    /// the position in the 2 s cycle, 0..1. Translates are fractions of the
    /// widget width, scales are fractions of the bar width — both in the
    /// CSS keyframe's own coordinate convention (bar origin at the left
    /// edge), which the painter applies verbatim.
    struct LinearIndeterminateFrame
    {
        qreal primaryTranslate = 0.0;
        qreal primaryScale = 0.0;
        qreal secondaryTranslate = 0.0;
        qreal secondaryScale = 0.0;
    };
    static LinearIndeterminateFrame linearIndeterminateFrame(qreal cycleProgress);

    /// One frame of the circular indeterminate animation, in degrees:
    /// `globalRotation` (the linear spin), `groupRotation` (the stepped
    /// 4-cycle rotate) and the two half-ring rotations (the expand arc plus
    /// each half's base offset). The painter composes them exactly the way
    /// the CSS border-trick construction does — see paintCircular().
    struct CircularIndeterminateFrame
    {
        qreal globalRotation = 0.0;
        qreal groupRotation = 0.0;
        qreal leftRotation = 0.0;
        qreal rightRotation = 0.0;
    };
    static CircularIndeterminateFrame circularIndeterminateFrame(qint64 elapsedMs);

    /// The four-color cycle at `cycleProgress` (0..1 over the shape's
    /// four-color duration). Linear timing for the linear shape; the
    /// circular's segments ride the indeterminate easing, matching each
    /// keyframe rule's animation-timing-function.
    static QColor fourColorAt(const MdProgressIndicatorTokens &tokens,
                              ProgressIndicatorShape shape, qreal cycleProgress);

    // --- layout ---------------------------------------------------------------
    struct Layout
    {
        // Linear (widget coordinates; the painter scales bars horizontally).
        QRectF track;
        QRectF activeIndicator;
        /// Invalid when the stop indicator is hidden (indeterminate,
        /// complete, or the gap+dot would not fit).
        QRectF stopIndicator;
        /// The buffer-dot region: from the (scaled) track's end to the
        /// widget edge, when the buffer is visible at all.
        QRectF bufferDots;
        /// The scale the inactive track shows: the buffer fraction when a
        /// buffer is set, 1 otherwise (the CSS `scaleX` contract).
        qreal trackScale = 1.0;
        // Circular.
        QRectF container;
        /// The stroke centred on this radius.
        qreal radius = 0.0;
        QSizeF preferredSize;
    };
    static Layout layoutFor(const MdProgressIndicator &indicator,
                            const MdProgressIndicatorTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite.
    static void paintLinear(QPainter &painter, const MdProgressIndicator &indicator,
                            const MdProgressIndicatorTokens &tokens, const Layout &layout,
                            qint64 elapsedMs);
    static void paintCircular(QPainter &painter, const MdProgressIndicator &indicator,
                              const MdProgressIndicatorTokens &tokens, const Layout &layout,
                              qint64 elapsedMs);
};

} // namespace md

#endif // MD_PROGRESS_INDICATOR_STYLE_H
