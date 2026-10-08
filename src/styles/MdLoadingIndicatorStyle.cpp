#include "MdLoadingIndicatorStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdLoadingIndicator.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <cmath> // std::exp / fmod / sqrt — MinGW 8.1 needs the explicit include

namespace md {

namespace {

// The default shape sequences (normalized unit-square polygons).
QList<MdRoundedPolygon> determinatePolygons()
{
    // LoadingIndicatorDefaults.DeterminateIndicatorPolygons: the rotated
    // circle first (the rotation makes the morph into the soft burst smooth,
    // which carries the matching rotation), then SoftBurst.
    static const QList<MdRoundedPolygon> polygons = {
        MdMaterialShapes::circleRotatedForDeterminate(),
        MdMaterialShapes::softBurst(),
    };
    return polygons;
}

QList<MdRoundedPolygon> indeterminatePolygons()
{
    static const QList<MdRoundedPolygon> polygons = MdMaterialShapes::indeterminatePolygons();
    return polygons;
}

// processPath: scale the unit-space morph path into the square, then align
// its bounds centre with the square's centre.
QPainterPath scaledInto(const QPainterPath &unitPath, const QRectF &square, qreal scaleFactor)
{
    const qreal side = qMin(square.width(), square.height());
    QPainterPath path = QTransform::fromScale(side * scaleFactor, side * scaleFactor)
                            .map(unitPath);
    const QRectF bounds = path.boundingRect();
    path.translate(square.center().x() - bounds.center().x(),
                   square.center().y() - bounds.center().y());
    return path;
}

QColor resolvedColor(ColorRole role)
{
    return MdTheme::instance().color(role);
}

} // namespace

MdLoadingIndicatorStyle::MdLoadingIndicatorStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdLoadingIndicatorStyle *MdLoadingIndicatorStyle::shared()
{
    static QMutex mutex;
    static MdLoadingIndicatorStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdLoadingIndicatorStyle;
        installPaintFilter<MdLoadingIndicator>(instance);
    }
    return instance;
}

bool MdLoadingIndicatorStyle::isInstalled()
{
    return hasPaintFilter(&MdLoadingIndicator::staticMetaObject);
}

qreal MdLoadingIndicatorStyle::springValue(qreal elapsedSeconds)
{
    // Closed form of the underdamped spring the reference drives with
    // spring(dampingRatio = 0.6, stiffness = 200): start 0, target 1, zero
    // initial velocity.
    //
    //   x(t) = 1 − e^(−ζω₀t) · [cos(ω_d t) + (ζω₀/ω_d) · sin(ω_d t)]
    //
    // with ω₀ = √stiffness and ω_d = ω₀·√(1−ζ²). The reference freezes the
    // value once it enters the 0.1 visibility threshold with a settling
    // velocity; at this damping the residual before the 650 ms snap is a
    // fraction of a percent — treated as full convergence (recorded in the
    // style header).
    const qreal omega0 = std::sqrt(kSpringStiffness);
    const qreal omegaD = omega0 * std::sqrt(1.0 - kSpringDampingRatio * kSpringDampingRatio);
    const qreal t = std::max(0.0, elapsedSeconds);
    const qreal decay = std::exp(-kSpringDampingRatio * omega0 * t);
    const qreal value = 1.0
        - decay * (std::cos(omegaD * t)
                   + (kSpringDampingRatio * omega0 / omegaD) * std::sin(omegaD * t));
    return value;
}

MdLoadingIndicatorStyle::IndeterminateFrame
MdLoadingIndicatorStyle::indeterminateFrame(qint64 elapsedMs, int morphCount)
{
    Q_ASSERT(morphCount > 0);
    IndeterminateFrame frame;

    // The morph grid: one slot per kMorphIntervalMs. The spring converges
    // inside the slot, so the grid index is the completed-morph count.
    const qint64 slot = elapsedMs < 0 ? 0 : elapsedMs / kMorphIntervalMs;
    frame.morphIndex = int(slot % morphCount);
    const qreal inSlotSeconds = qreal(elapsedMs < 0 ? 0 : elapsedMs % kMorphIntervalMs) / 1000.0;
    // Unclamped on purpose: the spring overshoots past 1 (up to ≈1.08 with
    // the published damping) and the morph extrapolates linearly — that is
    // the shape bounce the reference shows.
    frame.morphProgress = springValue(inSlotSeconds);

    // Rotation: the in-morph quarter turn, the stepped quarter-turn target
    // (incremented per completed morph, wrapping at the full turn) and the
    // linear global spin, all clockwise.
    const qreal stepped = std::fmod(double(slot + 1) * kQuarterRotationDeg, kFullRotationDeg);
    const qreal global =
        std::fmod(qreal(elapsedMs < 0 ? 0 : elapsedMs % kGlobalRotationDurationMs)
                          / kGlobalRotationDurationMs * kFullRotationDeg,
                  kFullRotationDeg);
    frame.rotationDeg = frame.morphProgress * kQuarterRotationDeg + stepped + global;
    return frame;
}

MdLoadingIndicatorStyle::DeterminateFrame
MdLoadingIndicatorStyle::determinateFrame(qreal progress, int morphCount)
{
    Q_ASSERT(morphCount > 0);
    const qreal clamped = qBound(0.0, progress, 1.0);
    DeterminateFrame frame;

    // Adjust the active morph index according to the progress.
    frame.morphIndex = int(qreal(morphCount) * clamped);
    if (frame.morphIndex > morphCount - 1) {
        frame.morphIndex = morphCount - 1;
    }
    // The progress value for the active morph — at full progress the last
    // morph reads exactly 1 instead of 0.
    if (clamped == 1.0 && frame.morphIndex == morphCount - 1) {
        frame.adjustedProgress = 1.0;
    } else {
        frame.adjustedProgress = std::fmod(clamped * morphCount, 1.0);
    }
    // Rotate counter-clockwise.
    frame.rotationDeg = -clamped * kDeterminateRotationDeg;
    return frame;
}

qreal MdLoadingIndicatorStyle::shapeScaleFactor(const QList<MdRoundedPolygon> &polygons,
                                                const MdLoadingIndicatorTokens &tokens)
{
    // calculateScaleFactor: the shape may rotate, so the axis-aligned bounds
    // are compared against the max-rotation square bounds, per shape, and
    // the smallest factor wins. Pill-like shapes throw the axis comparison
    // off, hence max(scaleX, scaleY) per shape.
    qreal factor = 1.0;
    double bounds[4];
    double maxBounds[4];
    for (const MdRoundedPolygon &polygon : polygons) {
        // The reference uses the approximate (control-point) bounds here.
        polygon.calculateApproximateBounds(bounds);
        polygon.calculateMaxBounds(maxBounds);
        const double scaleX = (bounds[2] - bounds[0]) / (maxBounds[2] - maxBounds[0]);
        const double scaleY = (bounds[3] - bounds[1]) / (maxBounds[3] - maxBounds[1]);
        factor = qreal(std::min(double(factor), std::max(scaleX, scaleY)));
    }
    // ActiveIndicatorScale: the active indicator's share of the container.
    const qreal containerMin = qMin(tokens.containerWidth, tokens.containerHeight);
    const qreal activeIndicatorScale =
        containerMin > 0.0 ? tokens.activeIndicatorSize / containerMin : 1.0;
    return factor * activeIndicatorScale;
}

QPainterPath MdLoadingIndicatorStyle::indicatorPath(const MdRoundedPolygon &start,
                                                    const MdRoundedPolygon &end, qreal progress,
                                                    const QRectF &square, qreal scaleFactor)
{
    const MdMorph morph(start, end);
    return scaledInto(morph.pathAt(progress), square, scaleFactor);
}

void MdLoadingIndicatorStyle::paint(QPainter &painter, const MdLoadingIndicator &indicator,
                                    const MdLoadingIndicatorTokens &tokens, qint64 elapsedMs)
{
    const QRectF square = QRectF(indicator.rect());
    if (square.isEmpty()) {
        return;
    }

    const bool contained = indicator.variant() == LoadingIndicatorVariant::Contained;
    const QColor indicatorColor = resolvedColor(contained ? tokens.containedActiveIndicatorColor
                                                          : tokens.activeIndicatorColor);
    if (!indicatorColor.isValid()) {
        return;
    }

    painter.save();

    // The container: the corner-full square is a circle; only the contained
    // variant paints it (the plain variant's container colour row is the
    // deprecated one and is never read).
    if (contained) {
        const QColor containerColor = resolvedColor(tokens.containedContainerColor);
        if (containerColor.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(containerColor);
            painter.drawPath(MdShape::roundedRect(square, tokens.containerShape));
        }
        painter.setClipPath(MdShape::roundedRect(square, tokens.containerShape));
    }

    // The indicator lives on a square canvas centred in the container.
    const qreal side = qMin(square.width(), square.height());
    const QRectF indicatorSquare(square.center().x() - side / 2.0,
                                 square.center().y() - side / 2.0, side, side);

    const QList<MdRoundedPolygon> polygons = indicator.isIndeterminate()
        ? indeterminatePolygons()
        : determinatePolygons();
    // Indeterminate: circular sequence (last → first morph included);
    // determinate: open sequence (progress walks start → end).
    const int morphCount = indicator.isIndeterminate() ? polygons.size() : polygons.size() - 1;
    const qreal scaleFactor = shapeScaleFactor(polygons, tokens);
    if (morphCount <= 0 || scaleFactor <= 0.0) {
        painter.restore();
        return;
    }

    QPainterPath path;
    qreal rotationDeg = 0.0;
    if (indicator.isIndeterminate()) {
        const IndeterminateFrame frame =
            indeterminateFrame(elapsedMs, morphCount);
        const int start = frame.morphIndex;
        const int end = (frame.morphIndex + 1) % polygons.size();
        path = indicatorPath(polygons.at(start), polygons.at(end), frame.morphProgress,
                             indicatorSquare, scaleFactor);
        rotationDeg = frame.rotationDeg;
    } else {
        const DeterminateFrame frame = determinateFrame(indicator.progress(), morphCount);
        const int start = frame.morphIndex;
        const int end = qMin(start + 1, polygons.size() - 1);
        path = indicatorPath(polygons.at(start), polygons.at(end), frame.adjustedProgress,
                             indicatorSquare, scaleFactor);
        rotationDeg = frame.rotationDeg;
    }

    // The rotation pivots on the canvas centre (Compose's draw rotate).
    painter.translate(indicatorSquare.center());
    painter.rotate(rotationDeg);
    painter.translate(-indicatorSquare.center());

    painter.setPen(Qt::NoPen);
    painter.setBrush(indicatorColor);
    painter.drawPath(path);

    painter.restore();
}

void MdLoadingIndicatorStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *indicator = qobject_cast<MdLoadingIndicator *>(widget);
    if (!indicator) {
        return;
    }
    paint(*painter, *indicator, indicator->tokens(), indicator->animationElapsedMs());
}

} // namespace md
