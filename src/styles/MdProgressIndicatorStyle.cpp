#include "MdProgressIndicatorStyle.h"

#include "core/MdMaterialShapes.h"
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

// --- Expressive wave ------------------------------------------------------
// Source: androidx compose/material3 `WavyProgressIndicator.kt` +
// `internal/LinearWavyProgressModifiers.kt` (`updateFullPaths` /
// `updateDrawPaths`). Every constant below is quoted from those files.

QPainterPath MdProgressIndicatorStyle::linearWavePath(qreal width, qreal height, qreal wavelength,
                                                      qreal strokeWidth)
{
    QPainterPath path;
    path.moveTo(0.0, 0.0);
    if (wavelength <= 0.0 || width <= 0.0) {
        path.lineTo(width, 0.0);
        path.translate(0.0, height / 2.0);
        return path;
    }
    // Compose updateFullPaths, amplitude > 0 branch:
    //   halfWavelengthPx = wavelength / 2
    //   anchorX starts at halfWavelengthPx, controlX at half/2
    //   controlY = height - stroke.width  (peaks at half that)
    //   the path is plotted 2 extra wavelengths long so a waveOffset shift
    //   always has path to draw, then translated to the vertical centre.
    const qreal halfWavelength = wavelength / 2.0;
    qreal anchorX = halfWavelength;
    qreal controlX = halfWavelength / 2.0;
    qreal controlY = height - strokeWidth;
    const qreal widthWithExtraPhase = width + wavelength * 2.0;
    while (anchorX <= widthWithExtraPhase) {
        path.quadTo(controlX, controlY, anchorX, 0.0);
        anchorX += halfWavelength;
        controlX += halfWavelength;
        controlY = -controlY;
    }
    if (path.currentPosition().x() < width) {
        path.lineTo(width, 0.0);
    }
    path.translate(0.0, height / 2.0);
    return path;
}

QPainterPath MdProgressIndicatorStyle::linearWaveSegment(qreal width, qreal height, qreal strokeWidth,
                                                         qreal startFraction, qreal endFraction,
                                                         qreal amplitude, qreal waveOffset,
                                                         qreal wavelength)
{
    QPainterPath segment;
    if (endFraction <= startFraction || width <= 0.0) {
        return segment;
    }
    const qreal halfHeight = height / 2.0;
    if (wavelength <= 0.0 || amplitude == 0.0) {
        // Flat baseline — Compose's amplitude == 0 path is a plain lineTo.
        const qreal y = halfHeight;
        segment.moveTo(startFraction * width, y);
        segment.lineTo(endFraction * width, y);
        return segment;
    }
    const qreal halfWavelength = wavelength / 2.0;
    // Compose updateFullPaths: controlY = height - stroke.width at each
    // half-wavelength midpoint, alternating sign. Anchors sit on y=0 every
    // half-wavelength starting at 0. Equal-spaced anchors make the quad's x
    // linear in t, so y is exactly 2(1-t)t * controlY * sign.
    const qreal controlY = height - strokeWidth;
    const qreal waveShift = waveOffset * wavelength;
    // The drawn window is [startFraction, endFraction] of the widget; the
    // path is evaluated at x + waveShift and then shifted back (Compose's
    // getSegment + translate(-waveShift) contract).
    const qreal x0 = startFraction * width;
    const qreal x1 = endFraction * width;
    const int samples = qMax(16, int(std::ceil((x1 - x0) / 2.0)) + 1);
    bool started = false;
    for (int i = 0; i <= samples; ++i) {
        const qreal drawnX = x0 + (x1 - x0) * (qreal(i) / qreal(samples));
        const qreal pathX = drawnX + waveShift;
        // Which half-wavelength quad, and where inside it.
        const int n = int(std::floor(pathX / halfWavelength));
        const qreal t = (pathX - qreal(n) * halfWavelength) / halfWavelength;
        const qreal sign = (n % 2 == 0) ? 1.0 : -1.0;
        const qreal yPeak = 2.0 * (1.0 - t) * t * controlY * sign;
        // Compose's updateFullPaths translates the chain onto the centreline
        // (y = height/2) before the segment is cut; updateDrawPaths then
        // scales y by the amplitude and translates by (1 - amplitude) *
        // halfHeight. Algebraically that is yPeak * amplitude + halfHeight —
        // amplitude 0 flattens onto the centreline, amplitude 1 keeps the
        // full published peak.
        const qreal drawnY = yPeak * amplitude + halfHeight;
        if (!started) {
            segment.moveTo(drawnX, drawnY);
            started = true;
        } else {
            segment.lineTo(drawnX, drawnY);
        }
    }
    return segment;
}

qreal MdProgressIndicatorStyle::wavePhase(qint64 elapsedMs, qreal wavelength, qreal waveSpeed)
{
    // Compose BaseLinearWavyProgressNode.updateOffsetAnimation:
    //   durationMillis = (wavelength / waveSpeed) * 1000
    //   the offset animates 0 → 1 over that duration, then restarts.
    // Default waveSpeed == wavelength → one wavelength per second.
    if (wavelength <= 0.0 || waveSpeed <= 0.0) {
        return 0.0;
    }
    const qreal durationMs = (wavelength / waveSpeed) * 1000.0;
    if (durationMs <= 0.0) {
        return 0.0;
    }
    return std::fmod(qreal(elapsedMs) / durationMs, 1.0);
}

qreal MdProgressIndicatorStyle::indicatorAmplitude(qreal progress, bool indeterminate)
{
    // WavyProgressIndicatorDefaults.indicatorAmplitude:
    //   "Sets the amplitude to the max on 10%, and back to zero on 95%."
    if (indeterminate) {
        return 1.0;
    }
    return (progress <= 0.1 || progress >= 0.95) ? 0.0 : 1.0;
}

MdProgressIndicatorStyle::Layout MdProgressIndicatorStyle::layoutFor(
    const MdProgressIndicator &indicator, const MdProgressIndicatorTokens &tokens)
{
    Layout layout;
    const QRectF widgetRect(indicator.rect());

    if (indicator.shape() == ProgressIndicatorShape::Linear) {
        // --- md.comp.progress-indicator.linear.* ----------------------------
        // The with-wave height is the container when the Expressive wave is
        // on (`linear.with-wave-height` 10 px vs `linear.height` 4 px).
        const qreal height = indicator.isWave()
            ? tokens.linearWithWaveHeight
            : qMax<qreal>(tokens.linearHeight,
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
    // The with-wave size is the container when the wave is on.
    const qreal preferredSide =
        indicator.isWave() ? tokens.circularWithWaveSize : tokens.circularSize;
    layout.preferredSize = QSizeF(preferredSide, preferredSide);
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

    if (indicator.isWave()) {
        // Expressive wavy track (Compose LinearWavyProgressIndicator).
        // Wavelength: 40 px determinate / 20 px indeterminate (the export's
        // `linear.active-wave-wavelength` / `linear.indeterminate-wave-
        // wavelength`). waveSpeed defaults to the wavelength — one
        // wavelength of travel per second.
        const qreal wavelength = indicator.isIndeterminate()
            ? tokens.linearIndeterminateWaveWavelength
            : tokens.linearWaveWavelength;
        const qreal amplitude = indicator.waveAmplitude(); // 0..1 morphed
        const qreal phase =
            wavePhase(elapsedMs, wavelength, wavelength);

        painter.setPen(QPen(activeColour, barThickness, Qt::SolidLine, Qt::RoundCap,
                            Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);

        if (indicator.isIndeterminate()) {
            // The two indeterminate bars, each drawn as a wave segment.
            const LinearIndeterminateFrame frame =
                linearIndeterminateFrame(qreal(elapsedMs % kLinearIndeterminateDurationMs)
                                         / qreal(kLinearIndeterminateDurationMs));
            const QPainterPath secondary = linearWaveSegment(
                rect.width(), height, barThickness, frame.secondaryTranslate,
                frame.secondaryTranslate + frame.secondaryScale, amplitude, phase, wavelength);
            const QPainterPath primary = linearWaveSegment(
                rect.width(), height, barThickness, frame.primaryTranslate,
                frame.primaryTranslate + frame.primaryScale, amplitude, phase, wavelength);
            if (!secondary.isEmpty()) {
                painter.drawPath(secondary);
            }
            if (!primary.isEmpty()) {
                painter.drawPath(primary);
            }
        } else {
            const QPainterPath bar = linearWaveSegment(
                rect.width(), height, barThickness, 0.0, indicator.displayFraction(), amplitude,
                phase, wavelength);
            if (!bar.isEmpty()) {
                painter.drawPath(bar);
            }
        }

        // The stop indicator trails the wave the same way it trails the flat
        // bar; the track was already painted above.
        if (layout.stopIndicator.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme.color(tokens.stopIndicatorColor));
            painter.drawPath(
                MdShape::roundedRect(layout.stopIndicator, tokens.stopIndicatorShape));
        }
        painter.restore();
        return;
    }

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

    if (indicator.isWave()) {
        // Expressive wavy ring — Compose CircularWavyProgressIndicator +
        // internal/CircularWavyProgressModifiers `CircularShapes`. The wave
        // is a RoundedPolygon.star morphed from a circle: vertices =
        // max(5, round(2πr / wavelength)), innerRadius 0.75, rounding
        // {radius 0.35, smoothing 0.4}, innerRounding {radius 0.5} (quoted
        // from CircularShapes.update). The published
        // `circular.wave-amplitude` 1.6 px / `circular.wave-wavelength` 15 px
        // / `circular.with-wave-size` 48 px are the export rows; the star
        // geometry is Compose's, and the amplitude 0..1 morphs circle→star.
        const qreal wavelength = tokens.circularWaveWavelength;
        const qreal amplitude = indicator.waveAmplitude();
        const qreal side = container.width();
        const qreal strokeHalf = activeThickness / 2.0;
        const qreal r = side / 2.0 - strokeHalf;
        const int numVertices = qMax(5, int(std::lround(2.0 * M_PI * r / wavelength)));

        MdRoundedPolygon trackPoly = MdRoundedPolygon::circle(numVertices).normalized();
        MdRoundedPolygon starPoly =
            MdRoundedPolygon::star(numVertices, 1.0, 0.75, MdCornerRounding{0.35, 0.4},
                                   MdCornerRounding{0.5, 0.0})
                .normalized();
        MdMorph morph(trackPoly, starPoly);
        QPainterPath wavePath = morph.pathAt(amplitude);

        // Scale the unit-square path out to the stroke-centre diameter and
        // centre it in the container (Compose processPath).
        const qreal diameter = r * 2.0;
        QTransform scale;
        scale.translate(container.center().x() - diameter / 2.0,
                        container.center().y() - diameter / 2.0);
        scale.scale(diameter, diameter);
        wavePath = scale.map(wavePath);

        QPen pen(activeColour);
        pen.setWidthF(activeThickness);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);

        // Wave travel: Compose rotates the morph path by the wave phase
        // (the circular counterpart of the linear waveShift).
        const qreal phase =
            wavePhase(elapsedMs, wavelength, wavelength);
        painter.translate(container.center());
        painter.rotate(phase * 360.0);
        painter.translate(-container.center());

        if (indicator.isIndeterminate()) {
            painter.drawPath(wavePath);
        } else {
            // Determinate: stroke the arc from 12 o'clock, sweep =
            // fraction × 360°. Qt's dash pattern is in pen-width multiples;
            // the visible run is `fraction` of the path perimeter.
            if (indicator.displayFraction() > 0.0) {
                const qreal perimeter = wavePath.length();
                if (perimeter > 0.0 && activeThickness > 0.0) {
                    QVector<qreal> scaled;
                    scaled << (perimeter * indicator.displayFraction()) / activeThickness
                           << perimeter / activeThickness;
                    pen.setDashPattern(scaled);
                    pen.setDashOffset(0.0);
                    painter.setPen(pen);
                }
                painter.drawPath(wavePath);
            }
        }
        painter.restore();
        return;
    }

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
