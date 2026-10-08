#include "MdProgressIndicatorStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdProgressIndicator.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

#include <cmath> // std::round / floor / fmod — MinGW 8.1 needs the explicit include

namespace md {

namespace {

// The indeterminate easings, keyed exactly as the CSS keyframes name them.
// material-web transplanted these numbers from MDC
// (mdc-linear-progress/_linear-progress.scss line refs live in the SCSS).
//
// QEasingCurve has no public per-t evaluation, and the keyframes need one —
// so this is a plain cubic-bezier solver: find the parameter whose x(u) == t
// (x is monotone in 0..1 for every timing function published here), then
// read y(u). 32 bisection steps put the error far below a pixel.
struct CubicBezier
{
    qreal x1, y1, x2, y2;

    qreal bx(qreal u) const
    {
        const qreal v = 1.0 - u;
        return 3.0 * v * v * u * x1 + 3.0 * v * u * u * x2 + u * u * u;
    }
    qreal by(qreal u) const
    {
        const qreal v = 1.0 - u;
        return 3.0 * v * v * u * y1 + 3.0 * v * u * u * y2 + u * u * u;
    }
    qreal value(qreal t) const
    {
        const qreal x = qBound(0.0, t, 1.0);
        qreal lo = 0.0;
        qreal hi = 1.0;
        for (int i = 0; i < 32; ++i) {
            const qreal mid = (lo + hi) / 2.0;
            if (bx(mid) < x) {
                lo = mid;
            } else {
                hi = mid;
            }
        }
        return by((lo + hi) / 2.0);
    }
};

struct LinearEasings
{
    // primary-indeterminate-translate
    CubicBezier primaryTranslateMid = {0.5, 0.0, 0.701732, 0.495819};
    CubicBezier primaryTranslateEnd = {0.302435, 0.381352, 0.55, 0.956352};
    // primary-indeterminate-scale
    CubicBezier primaryScaleMid = {0.334731, 0.12482, 0.785844, 1.0};
    CubicBezier primaryScaleEnd = {0.06, 0.11, 0.6, 1.0};
    // secondary-indeterminate-translate
    CubicBezier secondaryTranslate1 = {0.15, 0.0, 0.515058, 0.409685};
    CubicBezier secondaryTranslate2 = {0.31033, 0.284058, 0.8, 0.733712};
    CubicBezier secondaryTranslate3 = {0.4, 0.627035, 0.6, 0.902026};
    // secondary-indeterminate-scale
    CubicBezier secondaryScale1 = {0.205028, 0.057051, 0.57661, 0.453971};
    CubicBezier secondaryScale2 = {0.152313, 0.196432, 0.648374, 1.00432};
    CubicBezier secondaryScale3 = {0.257759, -0.003163, 0.211762, 1.38179};
    // The circular per-segment easing (expand-arc, rotate-arc and the
    // circular four-color all ride this one).
    CubicBezier circular = {0.4, 0.0, 0.2, 1.0};
};

const LinearEasings &easings()
{
    static const LinearEasings instance;
    return instance;
}

/// Piecewise keyframe evaluation: `marks` are strictly increasing positions
/// in 0..1 with `values` the same length; `easingFor(i)` returns the timing
/// function of the segment *starting* at mark i (the CSS rule: the timing
/// function specified at a keyframe applies to the segment that follows it).
/// The first segment rides `firstSegment` (the element's own
/// animation-timing-function; linear everywhere in this family).
template <typename EasingFor>
qreal keyframed(const QList<qreal> &marks, const QList<qreal> &values,
                const EasingFor &easingFor, qreal progress)
{
    static const CubicBezier kLinear{0.0, 0.0, 1.0, 1.0};
    const qreal p = qBound(0.0, progress, 1.0);
    for (int i = 0; i < marks.size() - 1; ++i) {
        if (p <= marks.at(i + 1) || i == marks.size() - 2) {
            const qreal span = marks.at(i + 1) - marks.at(i);
            const qreal t = span > 0.0 ? (p - marks.at(i)) / span : 0.0;
            const qreal eased = easingFor(i).value(qBound(0.0, t, 1.0));
            return values.at(i) + (values.at(i + 1) - values.at(i)) * eased;
        }
    }
    return values.last();
}

const QList<qreal> &fourColorMarks()
{
    // The four-color keyframe marks, identical for both shapes
    // (0 / 15 / 25 / 40 / 50 / 65 / 75 / 90 / 100 %).
    static const QList<qreal> marks = {0.0,  0.15, 0.25, 0.40, 0.50,
                                       0.65, 0.75, 0.90, 1.0};
    return marks;
}

/// The mark colours as role indices: one, one, two, two, three, three, four,
/// four, one — identical marks for both shapes.
int fourColorRoleIndexAtMark(int mark)
{
    static const QList<int> indices = {0, 0, 1, 1, 2, 2, 3, 3, 0};
    return indices.at(mark);
}

QColor fourColorRoleAt(const MdProgressIndicatorTokens &tokens, int index)
{
    switch (index) {
    case 0: return MdTheme::instance().color(tokens.fourColorOne);
    case 1: return MdTheme::instance().color(tokens.fourColorTwo);
    case 2: return MdTheme::instance().color(tokens.fourColorThree);
    default: return MdTheme::instance().color(tokens.fourColorFour);
    }
}

/// CSS interpolates the animated *colour* between keyframes, not an index —
/// so the four-color cycle lerps the two roles in RGB over each segment.
QColor lerpColour(const QColor &from, const QColor &to, qreal t)
{
    const qreal clamped = qBound(0.0, t, 1.0);
    return QColor(int(std::round(from.red() + (to.red() - from.red()) * clamped)),
                  int(std::round(from.green() + (to.green() - from.green()) * clamped)),
                  int(std::round(from.blue() + (to.blue() - from.blue()) * clamped)),
                  int(std::round(from.alpha() + (to.alpha() - from.alpha()) * clamped)));
}

/// A round-ended horizontal bar: the corner-full active indicator.
void fillBar(QPainter &painter, const QRectF &rect, const QColor &colour, qreal height)
{
    if (!rect.isValid() || rect.width() <= 0.0 || !colour.isValid()) {
        return;
    }
    painter.setPen(Qt::NoPen);
    painter.setBrush(colour);
    painter.drawPath(MdShape::roundedRect(rect, ShapeCorner::Full));
    Q_UNUSED(height);
}

} // namespace

MdProgressIndicatorStyle::MdProgressIndicatorStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdProgressIndicatorStyle *MdProgressIndicatorStyle::shared()
{
    static QMutex mutex;
    static MdProgressIndicatorStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdProgressIndicatorStyle;
        installPaintFilter<MdProgressIndicator>(instance);
    }
    return instance;
}

bool MdProgressIndicatorStyle::isInstalled()
{
    return hasPaintFilter(&MdProgressIndicator::staticMetaObject);
}

QEasingCurve MdProgressIndicatorStyle::determinateEasing(ProgressIndicatorShape shape)
{
    // Linear: cubic-bezier(0.4, 0, 0.6, 1) — the MDC determinate transition
    // the material-web SCSS cites. Circular: cubic-bezier(0, 0, 0.2, 1) — the
    // stroke-dashoffset transition in the circular SCSS.
    QEasingCurve curve(QEasingCurve::BezierSpline);
    if (shape == ProgressIndicatorShape::Linear) {
        curve.addCubicBezierSegment(QPointF(0.4, 0.0), QPointF(0.6, 1.0), QPointF(1.0, 1.0));
    } else {
        curve.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.2, 1.0), QPointF(1.0, 1.0));
    }
    return curve;
}

MdProgressIndicatorStyle::LinearIndeterminateFrame
MdProgressIndicatorStyle::linearIndeterminateFrame(qreal cycleProgress)
{
    const LinearEasings &e = easings();

    // primary-indeterminate-translate: 0 % → 0, 20 % → 0, 59.15 % →
    // 83.6714 %, 100 % → 200.611 % (percent of the bar's own width, which is
    // the widget width).
    const QList<qreal> primaryTranslateMarks = {0.0, 0.20, 0.5915, 1.0};
    const QList<qreal> primaryTranslateValues = {0.0, 0.0, 0.836714, 2.00611};
    // primary-indeterminate-scale: 0 % → 0.08, 36.65 % → 0.08,
    // 69.15 % → 0.661479, 100 % → 0.08.
    const QList<qreal> primaryScaleMarks = {0.0, 0.3665, 0.6915, 1.0};
    const QList<qreal> primaryScaleValues = {0.08, 0.08, 0.661479, 0.08};
    // secondary-indeterminate-translate: 0 % → 0, 25 % → 37.6519 %,
    // 48.35 % → 84.3862 %, 100 % → 160.278 %.
    const QList<qreal> secondaryTranslateMarks = {0.0, 0.25, 0.4835, 1.0};
    const QList<qreal> secondaryTranslateValues = {0.0, 0.376519, 0.843862, 1.60278};
    // secondary-indeterminate-scale: 0 % → 0.08, 19.15 % → 0.457104,
    // 44.15 % → 0.72796, 100 % → 0.08.
    const QList<qreal> secondaryScaleMarks = {0.0, 0.1915, 0.4415, 1.0};
    const QList<qreal> secondaryScaleValues = {0.08, 0.457104, 0.72796, 0.08};

    LinearIndeterminateFrame frame;
    frame.primaryTranslate = keyframed(
        primaryTranslateMarks, primaryTranslateValues,
        [&e](int i) -> const CubicBezier & {
            // The first segment is linear (the element's own timing); the
            // timing functions below are specified at 20 % and 59.15 %.
            if (i == 1) {
                return e.primaryTranslateMid;
            }
            if (i == 2) {
                return e.primaryTranslateEnd;
            }
            static const CubicBezier kLinear{0.0, 0.0, 1.0, 1.0};
            return kLinear;
        },
        cycleProgress);
    frame.primaryScale = keyframed(
        primaryScaleMarks, primaryScaleValues,
        [&e](int i) -> const CubicBezier & {
            if (i == 1) {
                return e.primaryScaleMid;
            }
            if (i == 2) {
                return e.primaryScaleEnd;
            }
            static const CubicBezier kLinear{0.0, 0.0, 1.0, 1.0};
            return kLinear;
        },
        cycleProgress);
    frame.secondaryTranslate = keyframed(
        secondaryTranslateMarks, secondaryTranslateValues,
        [&e](int i) -> const CubicBezier & {
            if (i == 0) {
                return e.secondaryTranslate1;
            }
            if (i == 1) {
                return e.secondaryTranslate2;
            }
            return e.secondaryTranslate3;
        },
        cycleProgress);
    frame.secondaryScale = keyframed(
        secondaryScaleMarks, secondaryScaleValues,
        [&e](int i) -> const CubicBezier & {
            if (i == 0) {
                return e.secondaryScale1;
            }
            if (i == 1) {
                return e.secondaryScale2;
            }
            return e.secondaryScale3;
        },
        cycleProgress);
    return frame;
}

MdProgressIndicatorStyle::CircularIndeterminateFrame
MdProgressIndicatorStyle::circularIndeterminateFrame(qint64 elapsedMs)
{
    const LinearEasings &e = easings();

    CircularIndeterminateFrame frame;

    // 3. linear-rotate: one full turn per ARCTIME*360/306 ms, linear.
    const qreal linearDuration = kCircularLinearRotateDurationMs;
    frame.globalRotation = 360.0 * (double(elapsedMs % qint64(linearDuration))
                                    / linearDuration);

    // 2. rotate-arc: eight eased +135° steps over the 4×ARCTIME cycle
    // (1080° total ≡ 3 full turns of the 3/4 arc).
    const qreal segment = qreal(kCircularArcDurationMs) / 2.0; // 666.5 ms
    const qint64 segmentIndex = qint64(std::floor(double(elapsedMs) / segment)) % 8;
    const qreal segmentProgress = (std::fmod(double(elapsedMs), segment)) / segment;
    frame.groupRotation = (qreal(segmentIndex) + e.circular.value(segmentProgress)) * 135.0;

    // 1. expand-arc: 265° → 130° → 265° over ARCTIME, the easing applied per
    // half; the right half rides the same animation delayed by half of it
    // (animation-delay: -0.5 * ARCTIME).
    const auto expandAt = [e](qint64 ms) {
        const qreal p = qreal(ms % kCircularArcDurationMs) / qreal(kCircularArcDurationMs);
        if (p < 0.5) {
            return 265.0 + (130.0 - 265.0) * e.circular.value(p / 0.5);
        }
        return 130.0 + (265.0 - 130.0) * e.circular.value((p - 0.5) / 0.5);
    };
    // The base rotations the two border rings start at (.left .circle 135°,
    // .right .circle 100°).
    frame.leftRotation = expandAt(elapsedMs) + 135.0;
    frame.rightRotation = expandAt(elapsedMs + kCircularArcDurationMs / 2) + 100.0;

    return frame;
}

QColor MdProgressIndicatorStyle::fourColorAt(const MdProgressIndicatorTokens &tokens,
                                             ProgressIndicatorShape shape,
                                             qreal cycleProgress)
{
    const QList<qreal> &marks = fourColorMarks();
    const bool eased = shape == ProgressIndicatorShape::Circular;
    const qreal p = qBound(0.0, cycleProgress, 1.0);
    for (int i = 0; i < marks.size() - 1; ++i) {
        if (p <= marks.at(i + 1) || i == marks.size() - 2) {
            const qreal span = marks.at(i + 1) - marks.at(i);
            const qreal t = span > 0.0 ? (p - marks.at(i)) / span : 0.0;
            const qreal easedT =
                eased ? easings().circular.value(qBound(0.0, t, 1.0)) : qBound(0.0, t, 1.0);
            return lerpColour(
                fourColorRoleAt(tokens, fourColorRoleIndexAtMark(i)),
                fourColorRoleAt(tokens, fourColorRoleIndexAtMark(i + 1)), easedT);
        }
    }
    return fourColorRoleAt(tokens, fourColorRoleIndexAtMark(0));
}

MdProgressIndicatorStyle::Layout MdProgressIndicatorStyle::layoutFor(
    const MdProgressIndicator &indicator, const MdProgressIndicatorTokens &tokens)
{
    Layout layout;
    const QRectF widgetRect(indicator.rect());

    if (indicator.shape() == ProgressIndicatorShape::Linear) {
        // --- md.comp.progress-indicator.linear.* ----------------------------
        const qreal height = qMax<qreal>(tokens.linearHeight,
                                         qMax(tokens.linearActiveIndicatorThickness,
                                              tokens.linearTrackThickness));
        layout.track = widgetRect;
        layout.activeIndicator = widgetRect;
        layout.preferredSize = QSizeF(80.0, height); // min-width: 80px [material-web css]

        const qreal fraction = indicator.displayFraction();
        const bool complete = fraction >= 1.0;
        const qreal activeEnd = fraction * widgetRect.width();
        const qreal stopLeft = activeEnd + tokens.linearTrackActiveIndicatorSpace;
        if (!indicator.isIndeterminate() && !complete
            && stopLeft + tokens.linearStopIndicatorSize <= widgetRect.width()) {
            layout.stopIndicator = QRectF(stopLeft, (height - tokens.linearStopIndicatorSize) / 2.0,
                                          tokens.linearStopIndicatorSize,
                                          tokens.linearStopIndicatorSize);
        }

        // The buffer contract: the inactive track scales to the buffer
        // fraction when one is set (the CSS dotSize/scaleX rule), and the
        // animated dots are visible wherever the scaled track and the bars
        // do not cover them.
        const bool hasBuffer = indicator.buffer() > 0.0 && indicator.buffer() < indicator.max();
        layout.trackScale = hasBuffer ? qBound(0.0, indicator.buffer() / indicator.max(), 1.0)
                                      : 1.0;
        const qreal trackEnd = layout.trackScale * widgetRect.width();
        layout.bufferDots = QRectF(trackEnd, 0.0, widgetRect.width() - trackEnd, height);
        return layout;
    }

    // --- md.comp.progress-indicator.circular.* ------------------------------
    const qreal side = qMin<qreal>(widgetRect.width(), widgetRect.height());
    const qreal originX = widgetRect.left() + (widgetRect.width() - side) / 2.0;
    const qreal originY = widgetRect.top() + (widgetRect.height() - side) / 2.0;
    layout.container = QRectF(originX, originY, side, side);
    // The stroke centres on this radius so the drawn ring spans exactly
    // `size` — Compose's geometry (material-web's percentage-stroke CSS
    // resolves differently and is recorded as the divergent legacy detail).
    layout.radius = (side - tokens.circularActiveIndicatorThickness) / 2.0;
    layout.preferredSize = QSizeF(tokens.circularSize, tokens.circularSize);
    return layout;
}

void MdProgressIndicatorStyle::paintLinear(QPainter &painter,
                                           const MdProgressIndicator &indicator,
                                           const MdProgressIndicatorTokens &tokens,
                                           const Layout &layout, qint64 elapsedMs)
{
    const MdTheme &theme = MdTheme::instance();
    const QRectF rect = layout.track;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Animations need LTR; RTL is supported by mirroring the whole indicator
    // (the CSS :dir(rtl) scale(-1)).
    const bool rtl = indicator.layoutDirection() == Qt::RightToLeft;
    if (rtl) {
        painter.translate(rect.width(), 0.0);
        painter.scale(-1.0, 1.0);
    }
    // The indeterminate bars sweep past both ends; CSS relies on
    // overflow:hidden.
    painter.setClipRect(rect);

    const qreal height = rect.height();

    // 0. Buffer dots — behind everything, the track-coloured circles the CSS
    //    mask reveals outside the scaled track. Dot diameter = track height
    //    / 2, spacing period 2.5 diameters (the 5x2 mask viewBox), scrolled
    //    one two-dot period per 250 ms cycle.
    const bool dotsVisible = !indicator.isIndeterminate() && layout.bufferDots.width() > 0.0
                             && layout.bufferDots.width() < rect.width()
                             && indicator.value() < indicator.max();
    if (dotsVisible) {
        const qreal dotDiameter = height / 2.0;
        const qreal period = dotDiameter * 2.5;
        const qreal cycle = qreal(elapsedMs % kLinearDeterminateDurationMs)
                            / qreal(kLinearDeterminateDurationMs);
        const qreal scroll = (1.0 - cycle) * 2.0 * period; // 0%: +10px → 100%: 0
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme.color(tokens.trackColor));
        const qreal y = rect.top() + height / 2.0;
        for (qreal x = rect.left() - period + scroll;
             x < rect.right() + period; x += period) {
            if (x + dotDiameter < layout.bufferDots.left()) {
                continue; // covered by the scaled track anyway
            }
            painter.drawEllipse(QPointF(x, y), dotDiameter / 2.0, dotDiameter / 2.0);
        }
    }

    // 1. The track, scaled to the buffer fraction when one is set (scaleX
    //    around the left edge — the CSS .inactive-track contract).
    const QRectF trackRect(rect.left(), rect.top(), rect.width() * layout.trackScale, height);
    if (trackRect.width() > 0.0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme.color(tokens.trackColor));
        painter.drawPath(MdShape::roundedRect(trackRect, tokens.trackShape));
    }

    const QColor activeColour =
        indicator.isFourColor()
            ? fourColorAt(tokens, ProgressIndicatorShape::Linear,
                          qreal(elapsedMs % kLinearFourColorDurationMs)
                              / qreal(kLinearFourColorDurationMs))
            : theme.color(tokens.activeIndicatorColor);

    const qreal barThickness = tokens.linearActiveIndicatorThickness;
    const qreal barY = rect.top() + (height - barThickness) / 2.0;

    if (indicator.isIndeterminate()) {
        // 2. The two indeterminate bars, straight out of the MDC keyframes:
        //    each bar is the full widget wide, translated by its keyframe
        //    fraction and scaled around its left edge by its scale fraction.
        const LinearIndeterminateFrame frame =
            linearIndeterminateFrame(qreal(elapsedMs % kLinearIndeterminateDurationMs)
                                     / qreal(kLinearIndeterminateDurationMs));
        const QRectF primary(rect.left() + frame.primaryTranslate * rect.width(), barY,
                             rect.width() * frame.primaryScale, barThickness);
        const QRectF secondary(rect.left() + frame.secondaryTranslate * rect.width(), barY,
                               rect.width() * frame.secondaryScale, barThickness);
        fillBar(painter, secondary, activeColour, barThickness);
        fillBar(painter, primary, activeColour, barThickness);
    } else {
        // 2. The determinate active indicator: scaleX(displayFraction) around
        //    the left edge — the same transform the CSS applies.
        const QRectF bar(rect.left(), barY, rect.width() * indicator.displayFraction(),
                         barThickness);
        fillBar(painter, bar, activeColour, barThickness);
    }

    // 3. The stop indicator — the dot that trails the active indicator with
    //    the published gap; the linear shape only, never while indeterminate
    //    or complete (the layout already decided).
    if (layout.stopIndicator.isValid()) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(theme.color(tokens.stopIndicatorColor));
        painter.drawPath(MdShape::roundedRect(layout.stopIndicator, tokens.stopIndicatorShape));
    }

    painter.restore();
}

void MdProgressIndicatorStyle::paintCircular(QPainter &painter,
                                             const MdProgressIndicator &indicator,
                                             const MdProgressIndicatorTokens &tokens,
                                             const Layout &layout, qint64 elapsedMs)
{
    const MdTheme &theme = MdTheme::instance();
    const QRectF container = layout.container;

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal activeThickness = tokens.circularActiveIndicatorThickness;
    const QColor activeColour =
        indicator.isFourColor()
            ? fourColorAt(tokens, ProgressIndicatorShape::Circular,
                          qreal(elapsedMs % kCircularFourColorDurationMs)
                              / qreal(kCircularFourColorDurationMs))
            : theme.color(tokens.activeIndicatorColor);

    if (indicator.isIndeterminate()) {
        // The border-trick construction, reproduced composition for
        // composition: the ring rotates globally (linear spin) and in eased
        // 135° steps; inside, each half-plane shows its own ring rotated by
        // the expand arc plus that half's base offset.
        const CircularIndeterminateFrame frame = circularIndeterminateFrame(elapsedMs);
        const qreal side = container.width();
        const qreal radius =
            (side - activeThickness) / 2.0; // stroke centre radius for the half rings
        // The painter is translated to the container centre below, so the
        // ring rect is centred on the origin.
        const QRectF ringRect(-radius, -radius, radius * 2.0, radius * 2.0);

        painter.translate(container.center());
        painter.rotate(frame.globalRotation + frame.groupRotation);

        const struct
        {
            QRectF clip;
            qreal rotation;
        } halves[2] = {
            // .left {inset: 0 50% 0 0} and .right {inset: 0 0 0 50%} in the
            // rotating spinner frame.
            {QRectF(-side / 2.0, -side / 2.0, side / 2.0, side), frame.leftRotation},
            {QRectF(0.0, -side / 2.0, side / 2.0, side), frame.rightRotation},
        };

        QPen pen(activeColour);
        pen.setWidthF(activeThickness);
        pen.setCapStyle(Qt::RoundCap);

        for (const auto &half : halves) {
            painter.save();
            painter.setClipRect(half.clip);
            painter.rotate(half.rotation);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            // The ring's coloured 180°: CSS border-top + border-right cover
            // the clockwise range -45°..+135° from 12 o'clock. With
            // qt = 90 + css (qt angles run counterclockwise from 3 o'clock)
            // that is Qt 45°..225°, drawn clockwise: start 45°, span -180°.
            painter.drawArc(ringRect, int(45.0 * 16.0), int(-180.0 * 16.0));
            painter.restore();
        }
        painter.restore();
        return;
    }

    // --- determinate ---------------------------------------------------------
    const qreal trackRadius =
        (container.width() - tokens.circularTrackThickness) / 2.0;
    const QRectF trackRect(container.center().x() - trackRadius,
                           container.center().y() - trackRadius, trackRadius * 2.0,
                           trackRadius * 2.0);

    // The track — full circle. (material-web's .track stroke is transparent,
    // a legacy M2 detail; the merged export publishes track.color and this
    // port honours it — recorded as the deliberate divergence.)
    QPen trackPen(theme.color(tokens.trackColor));
    trackPen.setWidthF(tokens.circularTrackThickness);
    trackPen.setCapStyle(Qt::RoundCap);
    painter.setPen(trackPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawArc(trackRect, 0, 360 * 16);

    // The active indicator: from 12 o'clock clockwise, sweep = fraction ×
    // 360°, rounded caps (corner-full).
    QPen activePen(activeColour);
    activePen.setWidthF(activeThickness);
    activePen.setCapStyle(Qt::RoundCap);
    painter.setPen(activePen);
    const int start = 90 * 16; // Qt angles: 90° = 12 o'clock
    const int sweep = int(-indicator.displayFraction() * 360.0 * 16.0);
    if (indicator.displayFraction() > 0.0) {
        painter.drawArc(trackRect, start, sweep);
    }

    painter.restore();
}

void MdProgressIndicatorStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *indicator = qobject_cast<MdProgressIndicator *>(widget);
    if (painter == nullptr || indicator == nullptr) {
        return;
    }
    const MdProgressIndicatorTokens &tokens = indicator->tokens();
    const Layout layout = layoutFor(*indicator, tokens);
    const qint64 elapsedMs = indicator->animationElapsedMs();
    if (indicator->shape() == ProgressIndicatorShape::Linear) {
        paintLinear(*painter, *indicator, tokens, layout, elapsedMs);
    } else {
        paintCircular(*painter, *indicator, tokens, layout, elapsedMs);
    }
}

} // namespace md
