#ifndef MD_MATERIAL_SHAPES_H
#define MD_MATERIAL_SHAPES_H

// MdMaterialShapes — the geometry engine behind the M3 Expressive shape
// morphing (the loading indicator family, and any later consumer of the
// Expressive shape language).
//
// Provenance, ported field by field from androidx (Apache-2.0):
//
//   * graphics-shapes Cubic / RoundedPolygon — the polygon-with-corner-
//     rounding to cubic Bézier construction, including the two-step cut
//     negotiation between adjacent corners, the smoothing flanking curves,
//     the first-corner split (the outline starts at the middle of corner 0's
//     arc) and the zero-length cubic dropping. `normalized()` and the two
//     bounds calculations (axis-aligned and max-rotation square) are the
//     same algorithms.
//   * compose.material3 MaterialShapes — the shape factories. Only the
//     shapes the loading indicator sequence needs are ported (circle, oval,
//     pill, pentagon, sunny, cookie4, cookie9, soft-burst); the generic
//     polygon/star/custom machinery is exposed so more of the catalogue can
//     be added without touching the engine.
//
// One recorded divergence (docs/porting-todo.md): `MdMorph` does **not**
// port graphics-shapes' MeasuredPolygon/featureMapper machinery. That
// algorithm matches individual curve features (corners vs. edges) between
// two shapes before interpolating them; it is roughly 1500 more lines for a
// benefit that is invisible on this family. Every shape in the loading
// indicator's sequence is star-convex around its center, so MdMorph samples
// both outlines radially at a fixed angular resolution and interpolates the
// matching points. At 240 samples the polyline deviation from the true cubic
// outline is far below a pixel at the family's 38 px active-indicator size.

#include "QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QPointF>
#include <QtCore/QVector>
#include <QtGui/QTransform>

class QPainterPath;

namespace md {

/// One cubic Bézier segment — the anchor/control layout mirrors
/// graphics-shapes' Cubic (8 coordinates, anchors at 0/6).
struct QT_MD3_EXPORT MdCubic
{
    double p[8];

    double anchor0X() const { return p[0]; }
    double anchor0Y() const { return p[1]; }
    double control0X() const { return p[2]; }
    double control0Y() const { return p[3]; }
    double control1X() const { return p[4]; }
    double control1Y() const { return p[5]; }
    double anchor1X() const { return p[6]; }
    double anchor1Y() const { return p[7]; }

    /// The point at parameter t (0 = anchor0, 1 = anchor1).
    QPointF pointOnCurve(double t) const;
    /// Split at t into the two halves (De Casteljau).
    void split(double t, MdCubic &first, MdCubic &second) const;
    /// Anchor/control order reversed.
    MdCubic reversed() const;
    /// Both anchors coincide (within the engine's distance epsilon).
    bool isZeroLength() const;
    /// True bounds of the curve (derivative roots), left/top/right/bottom.
    void calculateBounds(double bounds[4]) const;

    /// A straight segment between two anchors (controls at thirds).
    static MdCubic straightLine(double x0, double y0, double x1, double y1);
    /// The cubic approximating the shorter arc from (x0,y0) to (x1,y1)
    /// around (centerX, centerY) — both anchors equidistant from the center.
    static MdCubic circularArc(double centerX, double centerY, double x0, double y0,
                               double x1, double y1);
    /// A degenerate cubic located at one point.
    static MdCubic empty(double x, double y);
};

/// Corner rounding parameters — graphics-shapes' CornerRounding. A radius of
/// 0 (or `Unrounded`) keeps the corner sharp; `smoothing` (0..1) replaces the
/// pure circular arc with a three-curve smoothing construction.
struct QT_MD3_EXPORT MdCornerRounding
{
    double radius = 0.0;
    double smoothing = 0.0;

    constexpr bool isUnrounded() const { return radius <= 0.0; }

    static constexpr MdCornerRounding unrounded() { return {}; }
};

/// A polygon whose corners may be rounded — the port of graphics-shapes'
/// RoundedPolygon. Constructed from vertices plus per-vertex rounding; the
/// resulting outline is the cubic list. The engine's distance epsilon and
/// the cut-negotiation between neighbouring corners are the reference's.
class QT_MD3_EXPORT MdRoundedPolygon
{
public:
    /// One vertex with its own rounding, for the custom factory.
    struct Vertex
    {
        QPointF point;
        MdCornerRounding rounding;
    };

    /// Vertices are xy pairs in outline order; the center is estimated by
    /// averaging the anchors when not supplied (the reference contract).
    MdRoundedPolygon(const QVector<double> &vertices, MdCornerRounding rounding = {},
                     const QVector<MdCornerRounding> &perVertexRounding = {},
                     const QPointF &center = QPointF(qQNaN(), qQNaN()));

    // --- factories (the Shapes.kt helpers) ---------------------------------
    /// A regular polygon on a circle of `radius` vertices.
    static MdRoundedPolygon regular(int numVertices, double radius = 1.0,
                                    const QPointF &center = QPointF(0.0, 0.0),
                                    MdCornerRounding rounding = {});
    /// A rectangle with optional corner rounding.
    static MdRoundedPolygon rectangle(double width = 2.0, double height = 2.0,
                                      MdCornerRounding rounding = {},
                                      const QVector<MdCornerRounding> &perVertexRounding = {},
                                      const QPointF &center = QPointF(0.0, 0.0));
    /// Shapes.kt circle(): a polygon whose rounding approximates the circle
    /// of `radius`; the polygon radius is inflated by 1/cos(π/n).
    static MdRoundedPolygon circle(int numVertices = 10, double radius = 1.0,
                                   const QPointF &center = QPointF(0.0, 0.0));
    /// Shapes.kt star(): every other vertex on the inner radius.
    static MdRoundedPolygon star(int numVerticesPerRadius, double radius = 1.0,
                                 double innerRadius = 0.5, MdCornerRounding rounding = {},
                                 MdCornerRounding innerRounding = {},
                                 const QPointF &center = QPointF(0.0, 0.0));
    /// MaterialShapes.kt customPolygon(): repeats the vertex list `reps`
    /// times around `center`, optionally mirroring every other repetition.
    static MdRoundedPolygon custom(const QVector<Vertex> &vertices, int reps,
                                   const QPointF &center = QPointF(0.5, 0.5),
                                   bool mirroring = false);

    // --- queries -----------------------------------------------------------
    const QList<MdCubic> &cubics() const { return m_cubics; }
    const QPointF &center() const { return m_center; }

    /// Move/resize so the outline fits the (0,0)→(1,1) square, centred when
    /// there is slack in one direction (RoundedPolygon.normalized(), which
    /// normalizes against the *approximate* control-point bounds — the
    /// reference contract, kept for shape-for-shape fidelity).
    MdRoundedPolygon normalized() const;
    /// An affine transform of this polygon (cubics and center).
    MdRoundedPolygon transformed(const QTransform &transform) const;

    /// The exact axis-aligned bounds (left/top/right/bottom).
    void calculateBounds(double bounds[4]) const;
    /// The axis-aligned bounds of all anchors and control points — the
    /// reference's `approximate` variant, the one `normalized()` and the
    /// scale-factor calculation actually use (control points may sit
    /// slightly outside the true curve).
    void calculateApproximateBounds(double bounds[4]) const;
    /// The square that holds the shape in *any* rotation — the largest
    /// anchor/midpoint distance from the center (RoundedPolygon's contract).
    void calculateMaxBounds(double bounds[4]) const;

private:
    QList<MdCubic> m_cubics;
    QPointF m_center;
};

/// The Material shape catalogue entries the loading indicator sequence needs
/// (MaterialShapes.kt, each returned normalized to the unit square).
namespace MdMaterialShapes {

QT_MD3_EXPORT MdRoundedPolygon circle();
QT_MD3_EXPORT MdRoundedPolygon oval();
QT_MD3_EXPORT MdRoundedPolygon pill();
QT_MD3_EXPORT MdRoundedPolygon pentagon();
QT_MD3_EXPORT MdRoundedPolygon sunny();
QT_MD3_EXPORT MdRoundedPolygon cookie4Sided();
QT_MD3_EXPORT MdRoundedPolygon cookie9Sided();
QT_MD3_EXPORT MdRoundedPolygon softBurst();

/// Compose's determinate sequence rotates the circle by 360/20 degrees so it
/// morphs into the soft burst (which carries the same rotation) smoothly.
QT_MD3_EXPORT MdRoundedPolygon circleRotatedForDeterminate();

/// The indeterminate morph sequence — SoftBurst, Cookie9, Pentagon, Pill,
/// Sunny, Cookie4, Oval (LoadingIndicatorDefaults.IndeterminateIndicatorPolygons).
QT_MD3_EXPORT QList<MdRoundedPolygon> indeterminatePolygons();

} // namespace MdMaterialShapes

/// The morph between two (already normalized) polygons — see the header
/// comment for why this is radial sampling rather than the reference's
/// feature-matched cubics.
class QT_MD3_EXPORT MdMorph
{
public:
    explicit MdMorph(const MdRoundedPolygon &start, const MdRoundedPolygon &end,
                     int angularSamples = 240);

    /// The interpolated outline at `progress` (0 = start, 1 = end; values
    /// outside extrapolate linearly — the spring's bounce uses that).
    QPainterPath pathAt(double progress) const;

    /// The sampled outline of one side, in sample order (exposed for tests).
    QVector<QPointF> samplesAt(double progress) const;

    int sampleCount() const { return m_sampleCount; }

private:
    QVector<QPointF> sampleOutline(const MdRoundedPolygon &polygon) const;
    QPointF pointAtAngle(const QVector<double> &angles, const QVector<double> &radii,
                         double theta) const;

    int m_sampleCount = 0;
    // Angular sample tables for each side: the unwrapped angle per sample
    // (ascending, spanning exactly one 2π turn — a virtual closing sample
    // pins the span) and the matching radius from the polygon center. Both
    // sides are queried in the same absolute angle space.
    QVector<double> m_startAngles;
    QVector<double> m_startRadii;
    QVector<double> m_endAngles;
    QVector<double> m_endRadii;
    QPointF m_startCenter;
    QPointF m_endCenter;
};

} // namespace md

#endif // MD_MATERIAL_SHAPES_H
