#include "MdMaterialShapes.h"

#include <QtGui/QPainterPath>

#include <algorithm>
#include <cmath>

namespace md {

namespace {

// The engine's epsilons and helpers — Utils.kt, verbatim semantics.
constexpr double kDistanceEpsilon = 1e-4;
constexpr double kPi = 3.14159265358979323846;
constexpr double kTwoPi = 2.0 * kPi;

double sq(double v) { return v * v; }

double dist(double x, double y) { return std::sqrt(sq(x) + sq(y)); }

// Point.rotate90(): (-y, x) — the CCW quarter turn the arc construction
// relies on.
QPointF rotate90(const QPointF &v) { return QPointF(-v.y(), v.x()); }

double dot(const QPointF &a, const QPointF &b) { return a.x() * b.x() + a.y() * b.y(); }

QPointF directionOf(const QPointF &v)
{
    const double d = dist(v.x(), v.y());
    Q_ASSERT(d > 0.0);
    return v / d;
}

QPointF radialToCartesian(double radius, double angleRadians)
{
    return QPointF(radius * std::cos(angleRadians), radius * std::sin(angleRadians));
}

// positiveModulo — always in [0, mod).
double positiveModulo(double num, double mod)
{
    return std::fmod(std::fmod(num, mod) + mod, mod);
}

double interpolate(double start, double stop, double fraction)
{
    return (1.0 - fraction) * start + fraction * stop;
}

// The affine matrix of MaterialShapes' Matrix.rotateZ(degrees): the standard
// xy rotation, (x, y) → (x·cosθ − y·sinθ, x·sinθ + y·cosθ). In Qt's
// row-vector QTransform mapping (x' = m11·x + m21·y) that is m11=cos,
// m21=−sin, m12=sin, m22=cos.
QTransform rotateZDegrees(double degrees)
{
    const double radians = degrees * kPi / 180.0;
    const double c = std::cos(radians);
    const double s = std::sin(radians);
    // Affine mapping: x' = c·x − s·y, y' = s·x + c·y.
    return QTransform(c, s, -s, c, 0.0, 0.0);
}

QTransform scaleMatrix(double sx, double sy)
{
    // Row-vector mapping: x' = x·sx, y' = y·sy.
    return QTransform(sx, 0.0, 0.0, sy, 0.0, 0.0);
}

// ---------------------------------------------------------------------------
// RoundedCorner — RoundedPolygon.kt's private helper, ported whole. It owns
// the two-step negotiation: first every corner states how much edge it wants
// (expectedRoundCut/expectedCut), then the sides hand back how much is
// actually available, and the corner scales radius/smoothing accordingly.
// ---------------------------------------------------------------------------
struct RoundedCorner
{
    QPointF p0;
    QPointF p1;
    QPointF p2;
    double cornerRadius = 0.0;
    double smoothing = 0.0;
    QPointF d1;
    QPointF d2;
    double cosAngle = 0.0;
    double sinAngle = 0.0;
    double expectedRoundCut = 0.0;
    QPointF center;

    RoundedCorner(const QPointF &prev, const QPointF &curr, const QPointF &next,
                  MdCornerRounding rounding)
        : p0(prev), p1(curr), p2(next),
          cornerRadius(rounding.isUnrounded() ? 0.0 : rounding.radius),
          smoothing(rounding.isUnrounded() ? 0.0 : rounding.smoothing)
    {
        const QPointF v01 = p0 - p1;
        const QPointF v21 = p2 - p1;
        const double d01 = dist(v01.x(), v01.y());
        const double d21 = dist(v21.x(), v21.y());
        if (d01 > 0.0 && d21 > 0.0) {
            d1 = v01 / d01;
            d2 = v21 / d21;
            cosAngle = dot(d1, d2);
            sinAngle = std::sqrt(std::max(0.0, 1.0 - sq(cosAngle)));
            // tan(A/2) = sinA / (1 + cosA) = radius / cut.
            expectedRoundCut = sinAngle > 1e-3 ? cornerRadius * (cosAngle + 1.0) / sinAngle : 0.0;
        }
    }

    // expectedCut: smoothing doubles the wanted cut at most.
    double expectedCut() const { return (1.0 + smoothing) * expectedRoundCut; }

    double actualSmoothing(double allowedCut) const
    {
        if (allowedCut > expectedCut()) {
            return smoothing;
        }
        if (allowedCut > expectedRoundCut) {
            const double span = expectedCut() - expectedRoundCut;
            return span > 0.0 ? smoothing * (allowedCut - expectedRoundCut) / span : 0.0;
        }
        return 0.0;
    }

    // The flanking cubic connecting the cut linear side to the arc.
    MdCubic flankingCurve(double actualRoundCut, double actualSmoothing, const QPointF &corner,
                          const QPointF &sideStart, const QPointF &circleSegmentIntersection,
                          const QPointF &otherCircleSegmentIntersection,
                          const QPointF &circleCenter, double actualR) const
    {
        const QPointF sideDirection = directionOf(sideStart - corner);
        const QPointF curveStart =
            corner + sideDirection * actualRoundCut * (1.0 + actualSmoothing);
        // Cut the arc proportional to 1 − smoothing: at 0 the full section,
        // at 1 nothing of it.
        const QPointF mid = (circleSegmentIntersection + otherCircleSegmentIntersection) / 2.0;
        const QPointF p = QPointF(interpolate(circleSegmentIntersection.x(), mid.x(),
                                              actualSmoothing),
                                  interpolate(circleSegmentIntersection.y(), mid.y(),
                                              actualSmoothing));
        const QPointF curveEnd =
            circleCenter + directionOf(p - circleCenter) * actualR;
        // The anchor on the arc side: tangent at curveEnd ∩ linear side.
        const QPointF circleTangent = rotate90(curveEnd - circleCenter);
        QPointF anchorEnd = circleSegmentIntersection;
        {
            // lineIntersection(sideStart, sideDirection, curveEnd, circleTangent)
            const QPointF rotatedD1 = rotate90(circleTangent);
            const double den = dot(sideDirection, rotatedD1);
            if (std::abs(den) >= kDistanceEpsilon) {
                const double num = dot(curveEnd - sideStart, rotatedD1);
                if (std::abs(den) >= kDistanceEpsilon * std::abs(num)) {
                    anchorEnd = sideStart + sideDirection * (num / den);
                }
            }
        }
        // 2/3 comes from design tools.
        const QPointF anchorStart = (curveStart + anchorEnd * 2.0) / 3.0;
        MdCubic cubic;
        cubic.p[0] = curveStart.x();
        cubic.p[1] = curveStart.y();
        cubic.p[2] = anchorStart.x();
        cubic.p[3] = anchorStart.y();
        cubic.p[4] = anchorEnd.x();
        cubic.p[5] = anchorEnd.y();
        cubic.p[6] = curveEnd.x();
        cubic.p[7] = curveEnd.y();
        return cubic;
    }

    QList<MdCubic> getCubics(double allowedCut0, double allowedCut1)
    {
        const double allowedCut = std::min(allowedCut0, allowedCut1);
        if (expectedRoundCut < kDistanceEpsilon || allowedCut < kDistanceEpsilon
            || cornerRadius < kDistanceEpsilon) {
            center = p1;
            return { MdCubic::empty(p1.x(), p1.y()) };
        }
        const double actualRoundCut = std::min(allowedCut, expectedRoundCut);
        const double actualSmoothing0 = actualSmoothing(allowedCut0);
        const double actualSmoothing1 = actualSmoothing(allowedCut1);
        const double actualR = cornerRadius * actualRoundCut / expectedRoundCut;
        const double centerDistance = std::sqrt(sq(actualR) + sq(actualRoundCut));
        center = p1 + directionOf((d1 + d2) / 2.0) * centerDistance;
        const QPointF circleIntersection0 = p1 + d1 * actualRoundCut;
        const QPointF circleIntersection2 = p1 + d2 * actualRoundCut;
        MdCubic flanking0 = flankingCurve(actualRoundCut, actualSmoothing0, p1, p0,
                                          circleIntersection0, circleIntersection2, center,
                                          actualR);
        // The far flanking curve is built towards p2 and reversed so the
        // corner always reads p0-side → arc → p2-side.
        MdCubic flanking2 = flankingCurve(actualRoundCut, actualSmoothing1, p1, p2,
                                          circleIntersection2, circleIntersection0, center,
                                          actualR)
                                .reversed();
        MdCubic arc = MdCubic::circularArc(center.x(), center.y(), flanking0.anchor1X(),
                                           flanking0.anchor1Y(), flanking2.anchor0X(),
                                           flanking2.anchor0Y());
        return { flanking0, arc, flanking2 };
    }
};

// calculateCenter — the average of all vertices (RoundedPolygon.kt).
QPointF averageCenter(const QVector<double> &vertices)
{
    double sumX = 0.0;
    double sumY = 0.0;
    const int n = vertices.size() / 2;
    for (int i = 0; i < n; ++i) {
        sumX += vertices[i * 2];
        sumY += vertices[i * 2 + 1];
    }
    return n > 0 ? QPointF(sumX / n, sumY / n) : QPointF();
}

} // namespace

// ---------------------------------------------------------------------------
// MdCubic
// ---------------------------------------------------------------------------

QPointF MdCubic::pointOnCurve(double t) const
{
    const double u = 1.0 - t;
    return QPointF(anchor0X() * (u * u * u) + control0X() * (3.0 * t * u * u)
                       + control1X() * (3.0 * t * t * u) + anchor1X() * (t * t * t),
                   anchor0Y() * (u * u * u) + control0Y() * (3.0 * t * u * u)
                       + control1Y() * (3.0 * t * t * u) + anchor1Y() * (t * t * t));
}

void MdCubic::split(double t, MdCubic &first, MdCubic &second) const
{
    const double u = 1.0 - t;
    const QPointF point = pointOnCurve(t);
    first = MdCubic { { anchor0X(), anchor0Y(),
                        anchor0X() * u + control0X() * t, anchor0Y() * u + control0Y() * t,
                        anchor0X() * (u * u) + control0X() * (2.0 * u * t)
                            + control1X() * (t * t),
                        anchor0Y() * (u * u) + control0Y() * (2.0 * u * t)
                            + control1Y() * (t * t),
                        point.x(), point.y() } };
    second = MdCubic { { point.x(), point.y(),
                         control0X() * (u * u) + control1X() * (2.0 * u * t)
                             + anchor1X() * (t * t),
                         control0Y() * (u * u) + control1Y() * (2.0 * u * t)
                             + anchor1Y() * (t * t),
                         control1X() * u + anchor1X() * t, control1Y() * u + anchor1Y() * t,
                         anchor1X(), anchor1Y() } };
}

MdCubic MdCubic::reversed() const
{
    return MdCubic { { anchor1X(), anchor1Y(), control1X(), control1Y(), control0X(),
                       control0Y(), anchor0X(), anchor0Y() } };
}

bool MdCubic::isZeroLength() const
{
    return std::abs(anchor0X() - anchor1X()) < kDistanceEpsilon
        && std::abs(anchor0Y() - anchor1Y()) < kDistanceEpsilon;
}

void MdCubic::calculateBounds(double bounds[4]) const
{
    if (isZeroLength()) {
        bounds[0] = bounds[2] = anchor0X();
        bounds[1] = bounds[3] = anchor0Y();
        return;
    }
    double minX = std::min(anchor0X(), anchor1X());
    double minY = std::min(anchor0Y(), anchor1Y());
    double maxX = std::max(anchor0X(), anchor1X());
    double maxY = std::max(anchor0Y(), anchor1Y());

    // Derivative roots per axis — the quadratic formula on the derivative
    // Bézier (Cubic.calculateBounds, exact branch).
    const double xa = -anchor0X() + 3.0 * control0X() - 3.0 * control1X() + anchor1X();
    const double xb = 2.0 * anchor0X() - 4.0 * control0X() + 2.0 * control1X();
    const double xc = -anchor0X() + control0X();
    if (std::abs(xa) < kDistanceEpsilon) {
        if (xb != 0.0) {
            const double t = 2.0 * xc / (-2.0 * xb);
            if (t >= 0.0 && t <= 1.0) {
                minX = std::min(minX, pointOnCurve(t).x());
                maxX = std::max(maxX, pointOnCurve(t).x());
            }
        }
    } else {
        const double xs = sq(xb) - 4.0 * xa * xc;
        if (xs >= 0.0) {
            for (const double t : { (-xb + std::sqrt(xs)) / (2.0 * xa),
                                    (-xb - std::sqrt(xs)) / (2.0 * xa) }) {
                if (t >= 0.0 && t <= 1.0) {
                    minX = std::min(minX, pointOnCurve(t).x());
                    maxX = std::max(maxX, pointOnCurve(t).x());
                }
            }
        }
    }
    const double ya = -anchor0Y() + 3.0 * control0Y() - 3.0 * control1Y() + anchor1Y();
    const double yb = 2.0 * anchor0Y() - 4.0 * control0Y() + 2.0 * control1Y();
    const double yc = -anchor0Y() + control0Y();
    if (std::abs(ya) < kDistanceEpsilon) {
        if (yb != 0.0) {
            const double t = 2.0 * yc / (-2.0 * yb);
            if (t >= 0.0 && t <= 1.0) {
                minY = std::min(minY, pointOnCurve(t).y());
                maxY = std::max(maxY, pointOnCurve(t).y());
            }
        }
    } else {
        const double ys = sq(yb) - 4.0 * ya * yc;
        if (ys >= 0.0) {
            for (const double t : { (-yb + std::sqrt(ys)) / (2.0 * ya),
                                    (-yb - std::sqrt(ys)) / (2.0 * ya) }) {
                if (t >= 0.0 && t <= 1.0) {
                    minY = std::min(minY, pointOnCurve(t).y());
                    maxY = std::max(maxY, pointOnCurve(t).y());
                }
            }
        }
    }
    bounds[0] = minX;
    bounds[1] = minY;
    bounds[2] = maxX;
    bounds[3] = maxY;
}

MdCubic MdCubic::straightLine(double x0, double y0, double x1, double y1)
{
    return MdCubic { { x0, y0, interpolate(x0, x1, 1.0 / 3.0), interpolate(y0, y1, 1.0 / 3.0),
                       interpolate(x0, x1, 2.0 / 3.0), interpolate(y0, y1, 2.0 / 3.0), x1,
                       y1 } };
}

MdCubic MdCubic::circularArc(double centerX, double centerY, double x0, double y0, double x1,
                             double y1)
{
    const QPointF center(centerX, centerY);
    const QPointF p0(x0, y0);
    const QPointF p1(x1, y1);
    const QPointF p0d = directionOf(p0 - center);
    const QPointF p1d = directionOf(p1 - center);
    const QPointF rotatedP0 = rotate90(p0d);
    const QPointF rotatedP1 = rotate90(p1d);
    // Which of the two possible arcs is the short one — the reference
    // computes `clockwise` as rotate90(p0d)·(p1−center) ≥ 0 and flips k's
    // sign for the other direction.
    const bool clockwise = dot(rotatedP0, p1 - center) >= 0.0;
    const double cosa = dot(p0d, p1d);
    if (cosa > 0.999) {
        return straightLine(x0, y0, x1, y1);
    }
    const double radius = dist(x0 - centerX, y0 - centerY);
    double k = radius * 4.0 / 3.0 * (std::sqrt(2.0 * (1.0 - cosa)) - std::sqrt(1.0 - cosa * cosa))
        / (1.0 - cosa);
    k *= clockwise ? 1.0 : -1.0;
    return MdCubic { { x0, y0, x0 + rotatedP0.x() * k, y0 + rotatedP0.y() * k,
                       x1 - rotatedP1.x() * k, y1 - rotatedP1.y() * k, x1, y1 } };
}

MdCubic MdCubic::empty(double x, double y)
{
    return MdCubic { { x, y, x, y, x, y, x, y } };
}

// ---------------------------------------------------------------------------
// MdRoundedPolygon
// ---------------------------------------------------------------------------

MdRoundedPolygon::MdRoundedPolygon(const QVector<double> &vertices, MdCornerRounding rounding,
                                   const QVector<MdCornerRounding> &perVertexRounding,
                                   const QPointF &center)
{
    Q_ASSERT(vertices.size() >= 6);
    Q_ASSERT(vertices.size() % 2 == 0);
    Q_ASSERT(perVertexRounding.isEmpty() || perVertexRounding.size() * 2 == vertices.size());

    const int n = vertices.size() / 2;
    QVector<RoundedCorner> roundedCorners;
    roundedCorners.reserve(n);
    for (int i = 0; i < n; ++i) {
        const MdCornerRounding vtxRounding =
            perVertexRounding.isEmpty() ? rounding : perVertexRounding.at(i);
        const int prevIndex = ((i + n - 1) % n) * 2;
        const int nextIndex = ((i + 1) % n) * 2;
        roundedCorners.append(RoundedCorner(
            QPointF(vertices[prevIndex], vertices[prevIndex + 1]),
            QPointF(vertices[i * 2], vertices[i * 2 + 1]),
            QPointF(vertices[nextIndex], vertices[nextIndex + 1]), vtxRounding));
    }

    // Per side: is there room for both neighbouring corners' cuts? Rounding
    // is served first; smoothing gets whatever is left.
    QVector<QPair<double, double>> cutAdjusts(n);
    for (int ix = 0; ix < n; ++ix) {
        const double expectedRoundCut = roundedCorners[ix].expectedRoundCut
            + roundedCorners[(ix + 1) % n].expectedRoundCut;
        const double expectedCut =
            roundedCorners[ix].expectedCut() + roundedCorners[(ix + 1) % n].expectedCut();
        const QPointF vtx(vertices[ix * 2], vertices[ix * 2 + 1]);
        const QPointF nextVtx(vertices[((ix + 1) % n) * 2], vertices[((ix + 1) % n) * 2 + 1]);
        const double sideSize = dist(vtx.x() - nextVtx.x(), vtx.y() - nextVtx.y());
        if (expectedRoundCut > sideSize) {
            cutAdjusts[ix] = { sideSize / expectedRoundCut, 0.0 };
        } else if (expectedCut > sideSize) {
            const double span = expectedCut - expectedRoundCut;
            cutAdjusts[ix] = { 1.0, span > 0.0 ? (sideSize - expectedRoundCut) / span : 0.0 };
        } else {
            cutAdjusts[ix] = { 1.0, 1.0 };
        }
    }

    QList<QList<MdCubic>> corners;
    corners.reserve(n);
    for (int i = 0; i < n; ++i) {
        double allowedCuts[2];
        for (int delta = 0; delta <= 1; ++delta) {
            const auto adjust = cutAdjusts[(i + n - 1 + delta) % n];
            const RoundedCorner &corner = roundedCorners[i];
            allowedCuts[delta] = corner.expectedRoundCut * adjust.first
                + (corner.expectedCut() - corner.expectedRoundCut) * adjust.second;
        }
        corners.append(roundedCorners[i].getCubics(allowedCuts[0], allowedCuts[1]));
    }
    // The reference only rotates the outline onto the first corner's arc
    // midpoint when feature 0 is a *rounded* corner (three cubics).
    const bool firstCornerRounded = corners.at(0).size() == 3;

    // The feature list: every corner followed by the straight edge to the
    // next corner.
    QList<MdCubic> assembled;
    for (int i = 0; i < n; ++i) {
        for (const MdCubic &c : corners.at(i)) {
            assembled.append(c);
        }
        const QList<MdCubic> &nextCorner = corners.at((i + 1) % n);
        const QList<MdCubic> &currentCorner = corners.at(i);
        if (!currentCorner.isEmpty() && !nextCorner.isEmpty()) {
            const QPointF from(currentCorner.last().anchor1X(), currentCorner.last().anchor1Y());
            const QPointF to(nextCorner.first().anchor0X(), nextCorner.first().anchor0Y());
            assembled.append(MdCubic::straightLine(from.x(), from.y(), to.x(), to.y()));
        }
    }

    // The outline must start at the middle of corner 0's arc (the reference
    // splits features[0]'s centre cubic at 0.5 and rotates the list so the
    // path starts there).
    QList<MdCubic> outline;
    if (firstCornerRounded && assembled.size() >= 3) {
        MdCubic first;
        MdCubic second;
        assembled.at(1).split(0.5, first, second);
        // [arc end, flanking2, edges and later corners …, flanking0, arc start].
        outline.append(second);
        for (int i = 2; i < assembled.size(); ++i) {
            outline.append(assembled.at(i));
        }
        outline.append(assembled.at(0));
        outline.append(first);
    } else {
        outline = assembled;
    }

    // Drop zero-length curves, keeping anchors continuous (the reference's
    // last-cubic fixup), and hold the last cubic back so the closing cubic
    // can end exactly on the first anchor.
    m_cubics.clear();
    bool haveFirst = false;
    QPointF firstAnchor;
    bool haveLast = false;
    MdCubic lastStorage;
    for (const MdCubic &cubic : outline) {
        if (!cubic.isZeroLength()) {
            if (haveLast) {
                m_cubics.append(lastStorage);
            }
            lastStorage = cubic;
            haveLast = true;
            if (!haveFirst) {
                haveFirst = true;
                firstAnchor = QPointF(cubic.anchor0X(), cubic.anchor0Y());
            }
        } else if (haveLast) {
            // Enough discontinuity accumulates that the last cubic must
            // adopt the latest anchor point.
            lastStorage.p[6] = cubic.anchor1X();
            lastStorage.p[7] = cubic.anchor1Y();
        }
    }
    if (haveLast && haveFirst) {
        // The closing cubic takes the last cubic's start/controls and ends
        // on the first cubic's anchor.
        MdCubic closing;
        closing.p[0] = lastStorage.anchor0X();
        closing.p[1] = lastStorage.anchor0Y();
        closing.p[2] = lastStorage.control0X();
        closing.p[3] = lastStorage.control0Y();
        closing.p[4] = lastStorage.control1X();
        closing.p[5] = lastStorage.control1Y();
        closing.p[6] = firstAnchor.x();
        closing.p[7] = firstAnchor.y();
        m_cubics.append(closing);
    } else {
        // Empty / zero-sized polygon.
        m_cubics.append(MdCubic::empty(center.x(), center.y()));
    }

    m_center =
        std::isnan(center.x()) || std::isnan(center.y())
            ? averageCenter(vertices)
            : center;
}

MdRoundedPolygon MdRoundedPolygon::regular(int numVertices, double radius, const QPointF &center,
                                           MdCornerRounding rounding)
{
    Q_ASSERT(numVertices >= 3);
    QVector<double> vertices(numVertices * 2);
    for (int i = 0; i < numVertices; ++i) {
        const QPointF vertex =
            radialToCartesian(radius, kPi / numVertices * 2.0 * i) + center;
        vertices[i * 2] = vertex.x();
        vertices[i * 2 + 1] = vertex.y();
    }
    return MdRoundedPolygon(vertices, rounding, {}, center);
}

MdRoundedPolygon MdRoundedPolygon::rectangle(double width, double height,
                                             MdCornerRounding rounding,
                                             const QVector<MdCornerRounding> &perVertexRounding,
                                             const QPointF &center)
{
    const double left = center.x() - width / 2.0;
    const double top = center.y() - height / 2.0;
    const double right = center.x() + width / 2.0;
    const double bottom = center.y() + height / 2.0;
    return MdRoundedPolygon({ right, bottom, left, bottom, left, top, right, top }, rounding,
                            perVertexRounding, center);
}

MdRoundedPolygon MdRoundedPolygon::circle(int numVertices, double radius, const QPointF &center)
{
    Q_ASSERT(numVertices >= 3);
    // Half of the angle between two adjacent vertices; the polygon radius is
    // inflated so the rounded shape reaches the requested circle radius.
    const double theta = kPi / numVertices;
    const double polygonRadius = radius / std::cos(theta);
    return regular(numVertices, polygonRadius, center, MdCornerRounding { radius, 0.0 });
}

MdRoundedPolygon MdRoundedPolygon::star(int numVerticesPerRadius, double radius,
                                        double innerRadius, MdCornerRounding rounding,
                                        MdCornerRounding innerRounding, const QPointF &center)
{
    Q_ASSERT(radius > 0.0 && innerRadius > 0.0 && innerRadius < radius);
    QVector<double> vertices(numVerticesPerRadius * 4);
    QVector<MdCornerRounding> roundings(numVerticesPerRadius * 2);
    const bool perVertex = !innerRounding.isUnrounded();
    for (int i = 0; i < numVerticesPerRadius; ++i) {
        const QPointF outer = radialToCartesian(radius, kPi / numVerticesPerRadius * 2.0 * i);
        const QPointF inner = radialToCartesian(innerRadius,
                                                kPi / numVerticesPerRadius * (2.0 * i + 1));
        vertices[i * 4] = outer.x() + center.x();
        vertices[i * 4 + 1] = outer.y() + center.y();
        vertices[i * 4 + 2] = inner.x() + center.x();
        vertices[i * 4 + 3] = inner.y() + center.y();
        roundings[i * 2] = rounding;
        roundings[i * 2 + 1] = perVertex ? innerRounding : rounding;
    }
    return MdRoundedPolygon(vertices, rounding, perVertex ? roundings : QVector<MdCornerRounding>(),
                            center);
}

MdRoundedPolygon MdRoundedPolygon::custom(const QVector<Vertex> &inputPoints, int reps,
                                          const QPointF &center, bool mirroring)
{
    Q_ASSERT(reps >= 1);
    // doRepeat, verbatim semantics: mirroring walks every other section
    // backwards (dropping the duplicated first point), non-mirroring
    // rotates the section around the center.
    QVector<QPointF> points;
    QVector<MdCornerRounding> roundings;
    if (mirroring) {
        const int actualReps = reps * 2;
        const double sectionAngle = 360.0 / actualReps;
        QVector<double> angles;
        QVector<double> distances;
        for (const Vertex &v : inputPoints) {
            const QPointF rel = v.point - center;
            angles.append(std::atan2(rel.y(), rel.x()) * 180.0 / kPi);
            distances.append(dist(rel.x(), rel.y()));
        }
        for (int section = 0; section < actualReps; ++section) {
            for (int index = 0; index < inputPoints.size(); ++index) {
                const int i =
                    (section % 2 == 0) ? index : inputPoints.size() - 1 - index;
                if (i > 0 || section % 2 == 0) {
                    const double angleDegrees =
                        sectionAngle * section
                        + ((section % 2 == 0) ? angles[i]
                                              : sectionAngle - angles[i] + 2.0 * angles[0]);
                    const double a = angleDegrees * kPi / 180.0;
                    const QPointF finalPoint =
                        QPointF(std::cos(a), std::sin(a)) * distances[i] + center;
                    points.append(finalPoint);
                    roundings.append(inputPoints[i].rounding);
                }
            }
        }
    } else {
        const int np = inputPoints.size();
        for (int it = 0; it < np * reps; ++it) {
            const Vertex &v = inputPoints[it % np];
            const double angle = (it / np) * 360.0 / reps;
            const QPointF rel = v.point - center;
            const double a = angle * kPi / 180.0;
            const QPointF rotated(rel.x() * std::cos(a) - rel.y() * std::sin(a),
                                  rel.x() * std::sin(a) + rel.y() * std::cos(a));
            points.append(rotated + center);
            roundings.append(v.rounding);
        }
    }

    QVector<double> vertices(points.size() * 2);
    for (int i = 0; i < points.size(); ++i) {
        vertices[i * 2] = points[i].x();
        vertices[i * 2 + 1] = points[i].y();
    }
    return MdRoundedPolygon(vertices, MdCornerRounding::unrounded(), roundings, center);
}

MdRoundedPolygon MdRoundedPolygon::normalized() const
{
    // The reference normalizes against the control-point hull, not the true
    // curve bounds — kept for shape-for-shape fidelity.
    double bounds[4];
    calculateApproximateBounds(bounds);
    const double width = bounds[2] - bounds[0];
    const double height = bounds[3] - bounds[1];
    const double side = std::max(width, height);
    const double offsetX = (side - width) / 2.0 - bounds[0];
    const double offsetY = (side - height) / 2.0 - bounds[1];
    // (x + offset) / side, as one affine map.
    QTransform transform = QTransform::fromScale(1.0 / side, 1.0 / side);
    transform = QTransform::fromTranslate(offsetX, offsetY) * transform;
    return transformed(transform);
}

MdRoundedPolygon MdRoundedPolygon::transformed(const QTransform &transform) const
{
    MdRoundedPolygon result = *this;
    result.m_center = transform.map(m_center);
    for (MdCubic &cubic : result.m_cubics) {
        for (int i = 0; i < 4; ++i) {
            const QPointF mapped = transform.map(QPointF(cubic.p[i * 2], cubic.p[i * 2 + 1]));
            cubic.p[i * 2] = mapped.x();
            cubic.p[i * 2 + 1] = mapped.y();
        }
    }
    return result;
}

void MdRoundedPolygon::calculateBounds(double bounds[4]) const
{
    double minX = qInf();
    double minY = qInf();
    double maxX = -qInf();
    double maxY = -qInf();
    double curveBounds[4];
    for (const MdCubic &cubic : m_cubics) {
        cubic.calculateBounds(curveBounds);
        minX = std::min(minX, curveBounds[0]);
        minY = std::min(minY, curveBounds[1]);
        maxX = std::max(maxX, curveBounds[2]);
        maxY = std::max(maxY, curveBounds[3]);
    }
    bounds[0] = minX;
    bounds[1] = minY;
    bounds[2] = maxX;
    bounds[3] = maxY;
}

void MdRoundedPolygon::calculateApproximateBounds(double bounds[4]) const
{
    // The control-point hull: every anchor and control participates.
    double minX = qInf();
    double minY = qInf();
    double maxX = -qInf();
    double maxY = -qInf();
    for (const MdCubic &cubic : m_cubics) {
        for (int i = 0; i < 4; ++i) {
            minX = std::min(minX, cubic.p[i * 2]);
            maxX = std::max(maxX, cubic.p[i * 2]);
            minY = std::min(minY, cubic.p[i * 2 + 1]);
            maxY = std::max(maxY, cubic.p[i * 2 + 1]);
        }
    }
    bounds[0] = minX;
    bounds[1] = minY;
    bounds[2] = maxX;
    bounds[3] = maxY;
}

void MdRoundedPolygon::calculateMaxBounds(double bounds[4]) const
{    double maxDistSquared = 0.0;
    for (const MdCubic &cubic : m_cubics) {
        const double anchorDistance = sq(cubic.anchor0X() - m_center.x())
            + sq(cubic.anchor0Y() - m_center.y());
        const QPointF middle = cubic.pointOnCurve(0.5);
        const double middleDistance =
            sq(middle.x() - m_center.x()) + sq(middle.y() - m_center.y());
        maxDistSquared = std::max(maxDistSquared, std::max(anchorDistance, middleDistance));
    }
    const double distance = std::sqrt(maxDistSquared);
    bounds[0] = m_center.x() - distance;
    bounds[1] = m_center.y() - distance;
    bounds[2] = m_center.x() + distance;
    bounds[3] = m_center.y() + distance;
}

// ---------------------------------------------------------------------------
// MdMaterialShapes — MaterialShapes.kt entries, each normalized.
// ---------------------------------------------------------------------------

namespace MdMaterialShapes {

MdRoundedPolygon circle()
{
    static const MdRoundedPolygon shape = MdRoundedPolygon::circle(10).normalized();
    return shape;
}

MdRoundedPolygon oval()
{
    // circle → scale(1, 0.64) → rotate −45°, in that order.
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::circle(10).transformed(scaleMatrix(1.0, 0.64))
            .transformed(rotateZDegrees(-45.0))
            .normalized();
    return shape;
}

MdRoundedPolygon pill()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::custom(
            { { QPointF(0.961, 0.039), MdCornerRounding { 0.426, 0.0 } },
              { QPointF(1.001, 0.428), MdCornerRounding::unrounded() },
              { QPointF(1.000, 0.609), MdCornerRounding { 1.000, 0.0 } } },
            2, QPointF(0.5, 0.5), true)
            .normalized();
    return shape;
}

MdRoundedPolygon pentagon()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::custom(
            { { QPointF(0.500, -0.009), MdCornerRounding { 0.172, 0.0 } },
              { QPointF(1.030, 0.365), MdCornerRounding { 0.164, 0.0 } },
              { QPointF(0.828, 0.970), MdCornerRounding { 0.169, 0.0 } } },
            1, QPointF(0.5, 0.5), true)
            .normalized();
    return shape;
}

MdRoundedPolygon sunny()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::star(8, 1.0, 0.8, MdCornerRounding { 0.15, 0.0 }).normalized();
    return shape;
}

MdRoundedPolygon cookie4Sided()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::custom(
            { { QPointF(1.237, 1.236), MdCornerRounding { 0.258, 0.0 } },
              { QPointF(0.500, 0.918), MdCornerRounding { 0.233, 0.0 } } },
            4, QPointF(0.5, 0.5), false)
            .normalized();
    return shape;
}

MdRoundedPolygon cookie9Sided()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::star(9, 1.0, 0.8, MdCornerRounding { 0.5, 0.0 })
            .transformed(rotateZDegrees(-90.0))
            .normalized();
    return shape;
}

MdRoundedPolygon softBurst()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::custom(
            { { QPointF(0.193, 0.277), MdCornerRounding { 0.053, 0.0 } },
              { QPointF(0.176, 0.055), MdCornerRounding { 0.053, 0.0 } } },
            10, QPointF(0.5, 0.5), false)
            .normalized();
    return shape;
}

// --- the rest of the 35-entry catalogue (MaterialShapes.kt, verbatim) -------

namespace {

// MaterialShapes.kt's shared CornerRounding constants.
MdCornerRounding round15() { return MdCornerRounding { 0.15, 0.0 }; }
MdCornerRounding round20() { return MdCornerRounding { 0.2, 0.0 }; }
MdCornerRounding round30() { return MdCornerRounding { 0.3, 0.0 }; }
MdCornerRounding round50() { return MdCornerRounding { 0.5, 0.0 }; }
MdCornerRounding round100() { return MdCornerRounding { 1.0, 0.0 }; }

// A regular numVertices-gon with per-vertex rounding — RoundedPolygon(
// numVertices=…, perVertexRounding=…), which the public factories do not
// expose directly.
MdRoundedPolygon regularRounded(int numVertices,
                                const QVector<MdCornerRounding> &perVertex,
                                double radius = 1.0,
                                const QPointF &center = QPointF(0.0, 0.0))
{
    Q_ASSERT(numVertices >= 3);
    Q_ASSERT(perVertex.size() == numVertices);
    QVector<double> vertices(numVertices * 2);
    for (int i = 0; i < numVertices; ++i) {
        const QPointF vertex =
            radialToCartesian(radius, kPi / numVertices * 2.0 * i) + center;
        vertices[i * 2] = vertex.x();
        vertices[i * 2 + 1] = vertex.y();
    }
    return MdRoundedPolygon(vertices, MdCornerRounding::unrounded(), perVertex, center);
}

MdRoundedPolygon custom0(const QVector<MdRoundedPolygon::Vertex> &points, int reps,
                         bool mirroring = false)
{
    return MdRoundedPolygon::custom(points, reps, QPointF(0.5, 0.5), mirroring);
}

MdRoundedPolygon::Vertex pt(double x, double y, double radius = 0.0, double smoothing = 0.0)
{
    return { QPointF(x, y), radius > 0.0 ? MdCornerRounding { radius, smoothing }
                                         : MdCornerRounding::unrounded() };
}

} // namespace

MdRoundedPolygon square()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::rectangle(1.0, 1.0, round30()).normalized();
    return shape;
}

MdRoundedPolygon slanted()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.926, 0.970, 0.189, 0.811), pt(-0.021, 0.967, 0.187, 0.057) }, 2)
            .normalized();
    return shape;
}

MdRoundedPolygon arch()
{
    static const MdRoundedPolygon shape =
        regularRounded(4, { round100(), round100(), round20(), round20() })
            .transformed(rotateZDegrees(-135.0))
            .normalized();
    return shape;
}

MdRoundedPolygon fan()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(1.004, 1.000, 0.148, 0.417), pt(0.000, 1.000, 0.151),
                  pt(0.000, -0.003, 0.148), pt(0.978, 0.020, 0.803) },
                1)
            .normalized();
    return shape;
}

MdRoundedPolygon arrow()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.892, 0.313), pt(-0.216, 1.050, 0.207),
                  pt(0.499, -0.160, 0.215, 1.000), pt(1.225, 1.060, 0.211) },
                1)
            .normalized();
    return shape;
}

MdRoundedPolygon semiCircle()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::rectangle(1.6, 1.0, MdCornerRounding::unrounded(),
                                    { round20(), round20(), round100(), round100() })
            .normalized();
    return shape;
}

MdRoundedPolygon triangle()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::regular(3, 1.0, QPointF(0.0, 0.0), round20())
            .transformed(rotateZDegrees(-90.0))
            .normalized();
    return shape;
}

MdRoundedPolygon diamond()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 1.096, 0.151, 0.524), pt(0.040, 0.500, 0.159) }, 2)
            .normalized();
    return shape;
}

MdRoundedPolygon clamShell()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.171, 0.841, 0.159), pt(-0.020, 0.500, 0.140),
                  pt(0.170, 0.159, 0.159) },
                2)
            .normalized();
    return shape;
}

MdRoundedPolygon gem()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.499, 1.023, 0.241, 0.778), pt(-0.005, 0.792, 0.208),
                  pt(0.073, 0.258, 0.228), pt(0.433, 0.000, 0.491) },
                1, true)
            .normalized();
    return shape;
}

MdRoundedPolygon verySunny()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 1.080, 0.085), pt(0.358, 0.843, 0.085) }, 8).normalized();
    return shape;
}

MdRoundedPolygon cookie6Sided()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.723, 0.884, 0.394), pt(0.500, 1.099, 0.398) }, 6).normalized();
    return shape;
}

MdRoundedPolygon cookie7Sided()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::star(7, 1.0, 0.75, round50())
            .transformed(rotateZDegrees(-90.0))
            .normalized();
    return shape;
}

MdRoundedPolygon cookie12Sided()
{
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::star(12, 1.0, 0.8, round50())
            .transformed(rotateZDegrees(-90.0))
            .normalized();
    return shape;
}

MdRoundedPolygon ghostish()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.0, 1.000), pt(1.0, 0.0, 1.000), pt(1.0, 1.140, 0.254, 0.106),
                  pt(0.575, 0.906, 0.253) },
                1, true)
            .normalized();
    return shape;
}

MdRoundedPolygon clover4Leaf()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.074), pt(0.725, -0.099, 0.476) }, 4, true).normalized();
    return shape;
}

MdRoundedPolygon clover8Leaf()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.036), pt(0.758, -0.101, 0.209) }, 8).normalized();
    return shape;
}

MdRoundedPolygon burst()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, -0.006, 0.006), pt(0.592, 0.158, 0.006) }, 12).normalized();
    return shape;
}

MdRoundedPolygon boom()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.457, 0.296, 0.007), pt(0.500, -0.051, 0.007) }, 15).normalized();
    return shape;
}

MdRoundedPolygon softBoom()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.733, 0.454), pt(0.839, 0.437, 0.532),
                  pt(0.949, 0.449, 0.439, 1.000), pt(0.998, 0.478, 0.174) },
                16, true)
            .normalized();
    return shape;
}

MdRoundedPolygon flower()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.370, 0.187), pt(0.416, 0.049, 0.381), pt(0.479, 0.001, 0.095) }, 8, true)
            .normalized();
    return shape;
}

MdRoundedPolygon puffy()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.053),
                  pt(0.545, -0.040, 0.405),
                  pt(0.670, -0.035, 0.426),
                  pt(0.717, 0.066, 0.574),
                  pt(0.722, 0.128),
                  pt(0.777, 0.002, 0.360),
                  pt(0.914, 0.149, 0.660),
                  pt(0.926, 0.289, 0.660),
                  pt(0.881, 0.346),
                  pt(0.940, 0.344, 0.126),
                  pt(1.003, 0.437, 0.255) },
                2, true)
            .transformed(scaleMatrix(1.0, 0.742))
            .normalized();
    return shape;
}

MdRoundedPolygon puffyDiamond()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.870, 0.130, 0.146), pt(0.818, 0.357), pt(1.000, 0.332, 0.853) }, 4, true)
            .normalized();
    return shape;
}

MdRoundedPolygon pixelCircle()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.000), pt(0.704, 0.000), pt(0.704, 0.065), pt(0.843, 0.065),
                  pt(0.843, 0.148), pt(0.926, 0.148), pt(0.926, 0.296), pt(1.000, 0.296) },
                2, true)
            .normalized();
    return shape;
}

MdRoundedPolygon pixelTriangle()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.110, 0.500), pt(0.113, 0.000), pt(0.287, 0.000), pt(0.287, 0.087),
                  pt(0.421, 0.087), pt(0.421, 0.170), pt(0.560, 0.170), pt(0.560, 0.265),
                  pt(0.674, 0.265), pt(0.675, 0.344), pt(0.789, 0.344), pt(0.789, 0.439),
                  pt(0.888, 0.439) },
                1, true)
            .normalized();
    return shape;
}

MdRoundedPolygon bun()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.796, 0.500), pt(0.853, 0.518, 1.000), pt(0.992, 0.631, 1.000),
                  pt(0.968, 1.000, 1.000) },
                2, true)
            .normalized();
    return shape;
}

MdRoundedPolygon heart()
{
    static const MdRoundedPolygon shape =
        custom0({ pt(0.500, 0.268, 0.016), pt(0.792, -0.066, 0.958), pt(1.064, 0.276, 1.000),
                  pt(0.501, 0.946, 0.129) },
                1, true)
            .normalized();
    return shape;
}

MdRoundedPolygon circleRotatedForDeterminate()
{
    // DeterminateIndicatorPolygons[0]: Circle transformed by rotateZ(360/20).
    static const MdRoundedPolygon shape =
        MdRoundedPolygon::circle(10).transformed(rotateZDegrees(360.0 / 20.0)).normalized();
    return shape;
}

QList<MdRoundedPolygon> indeterminatePolygons()
{
    static const QList<MdRoundedPolygon> polygons = {
        softBurst(), cookie9Sided(), pentagon(), pill(), sunny(), cookie4Sided(), oval(),
    };
    return polygons;
}


// --- the 35-entry catalogue, as an indexed list ----------------------------

int decorativeShapeCount()
{
    return 35;
}

QString decorativeShapeName(int index)
{
    static const char *kNames[] = {
        "circle",       "square",       "slanted",      "arch",
        "fan",          "arrow",        "semi-circle",  "oval",
        "pill",         "triangle",     "diamond",      "clam-shell",
        "pentagon",     "gem",          "sunny",        "very-sunny",
        "cookie-4",     "cookie-6",     "cookie-7",     "cookie-9",
        "cookie-12",    "ghostish",     "clover-4",     "clover-8",
        "burst",        "soft-burst",   "boom",         "soft-boom",
        "flower",       "puffy",        "puffy-diamond","pixel-circle",
        "pixel-triangle","bun",         "heart",
    };
    static_assert(sizeof(kNames) / sizeof(kNames[0]) == 35, "catalogue size");
    if (index < 0 || index >= 35) {
        return QString();
    }
    return QString::fromUtf8(kNames[index]);
}

MdRoundedPolygon decorativeShape(int index)
{
    switch (index) {
    case 0: return circle();
    case 1: return square();
    case 2: return slanted();
    case 3: return arch();
    case 4: return fan();
    case 5: return arrow();
    case 6: return semiCircle();
    case 7: return oval();
    case 8: return pill();
    case 9: return triangle();
    case 10: return diamond();
    case 11: return clamShell();
    case 12: return pentagon();
    case 13: return gem();
    case 14: return sunny();
    case 15: return verySunny();
    case 16: return cookie4Sided();
    case 17: return cookie6Sided();
    case 18: return cookie7Sided();
    case 19: return cookie9Sided();
    case 20: return cookie12Sided();
    case 21: return ghostish();
    case 22: return clover4Leaf();
    case 23: return clover8Leaf();
    case 24: return burst();
    case 25: return softBurst();
    case 26: return boom();
    case 27: return softBoom();
    case 28: return flower();
    case 29: return puffy();
    case 30: return puffyDiamond();
    case 31: return pixelCircle();
    case 32: return pixelTriangle();
    case 33: return bun();
    case 34: return heart();
    default: return circle();
    }
}

QList<MdRoundedPolygon> decorativeShapes()
{
    QList<MdRoundedPolygon> shapes;
    shapes.reserve(decorativeShapeCount());
    for (int i = 0; i < decorativeShapeCount(); ++i) {
        shapes.append(decorativeShape(i));
    }
    return shapes;
}

QPainterPath toPath(const MdRoundedPolygon &polygon)
{
    QPainterPath path;
    const QList<MdCubic> &cubics = polygon.cubics();
    if (cubics.isEmpty()) {
        return path;
    }
    path.moveTo(cubics.first().anchor0X(), cubics.first().anchor0Y());
    for (const MdCubic &cubic : cubics) {
        path.cubicTo(cubic.control0X(), cubic.control0Y(),
                     cubic.control1X(), cubic.control1Y(),
                     cubic.anchor1X(), cubic.anchor1Y());
    }
    path.closeSubpath();
    return path;
}
} // namespace MdMaterialShapes

// ---------------------------------------------------------------------------
// MdMorph — radial sampling (see the header comment for the divergence note).
// ---------------------------------------------------------------------------

MdMorph::MdMorph(const MdRoundedPolygon &start, const MdRoundedPolygon &end, int angularSamples)
    : m_sampleCount(angularSamples)
{
    Q_ASSERT(angularSamples >= 8);

    const auto measure = [](const MdRoundedPolygon &polygon, QVector<double> &angles,
                            QVector<double> &radii) {
        // Flatten the cubics to a dense boundary polyline.
        QVector<QPointF> boundary;
        const int segmentsPerCubic = 16;
        for (const MdCubic &cubic : polygon.cubics()) {
            for (int i = 0; i < segmentsPerCubic; ++i) {
                boundary.append(cubic.pointOnCurve(double(i) / segmentsPerCubic));
            }
        }
        const QPointF c = polygon.center();
        // Winding: the outline's signed angle turn decides the direction;
        // reverse so angles ascend.
        double turn = 0.0;
        double prevAngle = std::atan2(boundary.first().y() - c.y(), boundary.first().x() - c.x());
        for (int i = 1; i < boundary.size(); ++i) {
            const double angle =
                std::atan2(boundary[i].y() - c.y(), boundary[i].x() - c.x());
            double delta = angle - prevAngle;
            delta = positiveModulo(delta + kPi, kTwoPi) - kPi;
            turn += delta;
            prevAngle = angle;
        }
        if (turn < 0.0) {
            std::reverse(boundary.begin(), boundary.end());
        }
        // Unwrap the angles around the first sample so the table ascends
        // across exactly one turn (the closing sample pins the span to 2π).
        const double baseAngle =
            std::atan2(boundary.first().y() - c.y(), boundary.first().x() - c.x());
        angles.reserve(boundary.size() + 1);
        radii.reserve(boundary.size() + 1);
        double previous = baseAngle;
        for (const QPointF &point : boundary) {
            const double raw = std::atan2(point.y() - c.y(), point.x() - c.x());
            double unwrapped = raw;
            while (unwrapped < previous - kPi) {
                unwrapped += kTwoPi;
            }
            while (unwrapped > previous + kPi) {
                unwrapped -= kTwoPi;
            }
            previous = unwrapped;
            angles.append(unwrapped);
            radii.append(dist(point.x() - c.x(), point.y() - c.y()));
        }
        // Virtual closing sample: the outline returns to its first point,
        // one full turn later. Both tables then span exactly 2π, so a query
        // angle resolves to the same *absolute* direction on both shapes.
        angles.append(angles.first() + kTwoPi);
        radii.append(radii.first());
    };

    measure(start, m_startAngles, m_startRadii);
    measure(end, m_endAngles, m_endRadii);
    m_startCenter = start.center();
    m_endCenter = end.center();
}

QPointF MdMorph::pointAtAngle(const QVector<double> &angles, const QVector<double> &radii,
                              double theta) const
{
    Q_ASSERT(angles.size() == radii.size() && angles.size() >= 2);
    const int n = angles.size();
    const double span = angles.last() - angles.first();
    const double query =
        span > 0.0 ? angles.first() + positiveModulo(theta - angles.first(), span) : theta;
    // Binary search for the bracketing sample.
    int lo = 0;
    int hi = n - 1;
    while (hi - lo > 1) {
        const int mid = (lo + hi) / 2;
        if (angles[mid] <= query) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    const double a0 = angles[lo];
    const double a1 = angles[hi];
    const double fraction = a1 > a0 ? qBound(0.0, (query - a0) / (a1 - a0), 1.0) : 0.0;
    const double radius = interpolate(radii[lo], radii[hi], fraction);
    // The returned point sits on the *query* direction — with the tables
    // spanning exactly 2π this is the same absolute direction on both sides.
    return QPointF(radius * std::cos(query), radius * std::sin(query));
}

QVector<QPointF> MdMorph::samplesAt(double progress) const
{
    QVector<QPointF> samples(m_sampleCount);
    for (int k = 0; k < m_sampleCount; ++k) {
        const double theta = kTwoPi * double(k) / m_sampleCount;
        const QPointF a = m_startCenter + pointAtAngle(m_startAngles, m_startRadii, theta);
        const QPointF b = m_endCenter + pointAtAngle(m_endAngles, m_endRadii, theta);
        samples[k] = QPointF(interpolate(a.x(), b.x(), progress),
                             interpolate(a.y(), b.y(), progress));
    }
    return samples;
}

QPainterPath MdMorph::pathAt(double progress) const
{
    QPainterPath path;
    const QVector<QPointF> samples = samplesAt(progress);
    path.moveTo(samples.first());
    for (int i = 1; i < samples.size(); ++i) {
        path.lineTo(samples[i]);
    }
    path.closeSubpath();
    return path;
}

} // namespace md
