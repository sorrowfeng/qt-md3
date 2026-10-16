#ifndef MD_PROGRESS_INDICATOR_STYLE_H
#define MD_PROGRESS_INDICATOR_STYLE_H

#include "core/MdProgressIndicatorTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QEasingCurve>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>
#include <QtGui/QPainterPath>

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

    /// The published determinate transition easing for one shape
    /// (material-web CSS rows). The widget drives the value change with
    /// MdSpring, not this curve — it is kept so the token stays auditable.
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

    // --- Expressive wave (Compose WavyProgressIndicator.kt + the
    //     *WavyProgressModifiers internals) ------------------------------------
    //
    // The wave is a chain of quadratic Beziers alternating above/below the
    // centreline: anchors on the centreline every half-wavelength, control
    // points at ±(height − strokeWidth) so the plotted peak is half that —
    // `(withWaveHeight − thickness) / 2`, which is exactly the published
    // linear amplitude of 3 px (and the circular 1.6 px on its arc).
    // Travel is the Compose waveOffset contract: shift the drawn segment
    // along the full path by `waveOffset * wavelength` and compensate with a
    // translation, so the wave appears to scroll without the path moving.
    /// The full linear wave path in widget coordinates (centred on
    /// `height / 2`), built at maximum amplitude — the Compose
    /// `updateFullPaths` construction. `amplitude == 0` yields the flat
    /// baseline line.
    static QPainterPath linearWavePath(qreal width, qreal height, qreal wavelength,
                                       qreal strokeWidth);
    /// The drawn segment `[startFraction, endFraction]` of the wave chain,
    /// Y-scaled to `amplitude` (0..1) and shifted by `waveOffset` (0..1 of
    /// one wavelength) — Compose's `updateDrawPaths` transform. `strokeWidth`
    /// is the active-indicator thickness, which sets the Bezier control
    /// height exactly as Compose's `updateFullPaths` does.
    static QPainterPath linearWaveSegment(qreal width, qreal height, qreal strokeWidth,
                                          qreal startFraction, qreal endFraction, qreal amplitude,
                                          qreal waveOffset, qreal wavelength);
    /// Compose's `waveOffset` for a steady scroll: one full 0→1 cycle every
    /// `wavelength / waveSpeed` seconds (the default waveSpeed equals the
    /// wavelength, i.e. one wavelength per second).
    static qreal wavePhase(qint64 elapsedMs, qreal wavelength, qreal waveSpeed);
    /// Compose `WavyProgressIndicatorDefaults.indicatorAmplitude`: 0 at or
    /// below 10 % and at or above 95 % of progress, 1 in between. 1 while
    /// indeterminate.
    static qreal indicatorAmplitude(qreal progress, bool indeterminate);

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
