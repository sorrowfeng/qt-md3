#ifndef MD_LOADING_INDICATOR_STYLE_H
#define MD_LOADING_INDICATOR_STYLE_H

#include "core/MdLoadingIndicatorTokens.h"
#include "core/MdMaterialShapes.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtGui/QPainterPath>

class QPainter;

namespace md {

class MdLoadingIndicator;

/// Pattern A style for `MdLoadingIndicator`: painting, geometry and the
/// Expressive morph/spin animation math for the
/// `md.comp.loading-indicator.*` family, registered in the paint hub so the
/// first construction installs it application-wide.
///
/// The export is token-only (material-web ships no web component), so the
/// behaviour port is Compose M3 Expressive's LoadingIndicator
/// (androidx-main). The animation numbers are Compose's:
///
///   * the morph advances on a 650 ms grid;
///   * each morph runs on a spring (dampingRatio 0.6, stiffness 200) from
///     0 to 1 — evaluated here as the closed-form underdamped solution, a
///     pure function of elapsed time (recorded divergence: the reference's
///     end-threshold freeze is treated as full convergence, a sub-pixel
///     difference at the family's size);
///   * the shape's own rotation steps a quarter turn per completed morph on
///     top of the 4666 ms linear global spin;
///   * the determinate mode rotates counter-clockwise by progress×180° and
///     walks the morph sequence by progress itself.
///
/// All frame math is exposed as *pure functions* so the test suite pins it
/// field by field without waiting on an event loop.
class QT_MD3_EXPORT MdLoadingIndicatorStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdLoadingIndicatorStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdLoadingIndicatorStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    // --- published timing (Compose LoadingIndicator.kt) ---------------------
    /// One morph per 650 ms grid slot; the spring finishes inside it.
    static constexpr int kMorphIntervalMs = 650;
    /// The linear global spin: one full turn every 4666 ms.
    static constexpr int kGlobalRotationDurationMs = 4666;
    static constexpr double kSpringDampingRatio = 0.6;
    static constexpr double kSpringStiffness = 200.0;
    /// The reference's visibility threshold (0.1) — see the divergence note.
    static constexpr double kSpringVisibilityThreshold = 0.1;
    static constexpr double kQuarterRotationDeg = 90.0;
    static constexpr double kFullRotationDeg = 360.0;
    /// Determinate mode sweeps a half turn across the full progress.
    static constexpr double kDeterminateRotationDeg = 180.0;

    // --- pure animation math ------------------------------------------------
    /// The closed-form underdamped spring (start 0 → target 1, zero initial
    /// velocity) for the published damping/stiffness, at `elapsedSeconds`
    /// since the morph began. Converges to 1 well inside the 650 ms slot.
    static qreal springValue(qreal elapsedSeconds);

    /// One frame of the indeterminate animation. `morphCount` is the number
    /// of morphs in the (circular) sequence — 7 for the default shapes.
    /// `rotationDeg` is the total draw rotation the painter applies.
    struct IndeterminateFrame
    {
        int morphIndex = 0;
        qreal morphProgress = 0.0;
        qreal rotationDeg = 0.0;
    };
    static IndeterminateFrame indeterminateFrame(qint64 elapsedMs, int morphCount);

    /// One frame of the determinate animation for `progress` (0..1).
    /// `adjustedProgress` is the in-morph value, `rotationDeg` the
    /// counter-clockwise sweep.
    struct DeterminateFrame
    {
        int morphIndex = 0;
        qreal adjustedProgress = 0.0;
        qreal rotationDeg = 0.0;
    };
    static DeterminateFrame determinateFrame(qreal progress, int morphCount);

    /// Compose's calculateScaleFactor × ActiveIndicatorScale: the largest
    /// scale at which every polygon fits its rotation-holding square inside
    /// the container, times activeIndicatorSize / min(container side).
    static qreal shapeScaleFactor(const QList<MdRoundedPolygon> &polygons,
                                  const MdLoadingIndicatorTokens &tokens);

    /// The morphed, scaled and centred outline for one frame, ready to fill
    /// inside `square` (the indicator's own square canvas).
    static QPainterPath indicatorPath(const MdRoundedPolygon &start, const MdRoundedPolygon &end,
                                      qreal progress, const QRectF &square, qreal scaleFactor);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite.
    static void paint(QPainter &painter, const MdLoadingIndicator &indicator,
                      const MdLoadingIndicatorTokens &tokens, qint64 elapsedMs);
};

} // namespace md

#endif // MD_LOADING_INDICATOR_STYLE_H
