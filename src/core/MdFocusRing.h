#ifndef MD_FOCUS_RING_H
#define MD_FOCUS_RING_H

// MdFocusRing — the M3 focus indicator.
//
// The M3 focus indicator is a *ring outside the component*, separated from it
// by a small gap. Material-web implements it as the `outward` variant:
//
//   inset: -2px            -> the ring's own box is 2px outside the component
//   outline: 3px           -> the ring is drawn 3px further out from that box
//   border-radius: r + 2px -> radii follow the component's, offset by the gap
//   color: md.sys.color.secondary
//
// so the visible annulus spans 2px..5px outside the component edge. That is
// the "3dp outline + 2dp gap" the M3 documentation describes.
//
// On focus the ring *grows* past its resting width and settles back, which is
// what makes the indicator catch the eye:
//
//   t = 0            .. 150 ms  0 -> 8px      (emphasized easing)
//   t = 150 ms       .. 600 ms  8px -> 3px    (emphasized easing)
//
// The `inward` variant draws the ring inside the component instead; it exists
// for components that sit flush against another surface.
//
// Constants are transcribed from material-components/material-web:
//   tokens/_md-comp-focus-ring.scss   active-width, color, duration,
//                                     inward-offset, outward-offset, shape,
//                                     width
//   focus/internal/_focus-ring.scss   the outward-grow / outward-shrink
//                                     keyframes and the 25% / 75% split
//
// Like MdRipple this is a value type with no widget dependency.

#include "MdShape.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QRectF>
#include <QtGui/QColor>

class QPainter;
class QTimer;

namespace md {

/// The published `md.comp.focus-ring.*` tokens, plus the two switches that
/// select the inward variant. Every field has a token behind it; nothing here
/// is a private invention.
struct QT_MD3_EXPORT MdFocusRingSpec
{
    /// `width` — resting outline width.
    qreal width = 3.0;
    /// `active-width` — the width the ring grows to on focus.
    qreal activeWidth = 8.0;
    /// `outward-offset` — the gap, for the outward variant.
    qreal outwardOffset = 2.0;
    /// `inward-offset` — the gap, for the inward variant.
    qreal inwardOffset = 0.0;
    /// `duration` — ms.sys.motion.duration-long4.
    int durationMs = 600;
    /// `easing-emphasized`.
    MotionEasing easing = MotionEasing::Emphasized;
    /// `color` — md.sys.color.secondary. Left invalid to mean "ask the theme".
    QColor color;
    /// Draw inside the component instead of outside.
    bool inward = false;
    /// `shape` — md.sys.shape.corner.full by default. When the component has
    /// explicit per-corner radii they win; this is only the fallback.
    ShapeCorner shape = ShapeCorner::Full;

    /// The gap actually in force for the selected variant.
    qreal offset() const { return inward ? inwardOffset : outwardOffset; }
};

class QT_MD3_EXPORT MdFocusRing
{
public:
    /// Growth phase length: a quarter of the token duration.
    static int growMs(const MdFocusRingSpec &spec = MdFocusRingSpec());
    /// Settle phase length: the remaining three quarters.
    static int settleMs(const MdFocusRingSpec &spec = MdFocusRingSpec());
    /// Full animation length, i.e. `duration`.
    static int totalMs(const MdFocusRingSpec &spec = MdFocusRingSpec());

    /// Outline width `elapsedMs` after focus was gained. Returns 0 before the
    /// ring becomes visible, the resting `width` once settled.
    static qreal widthAt(const MdFocusRingSpec &spec, int elapsedMs);

    /// The rect the ring is stroked around, for the chosen variant.
    /// This is the centre line of the stroke, not its outer edge.
    static QRectF ringRect(const QRectF &componentBounds,
                           const MdFocusRingSpec &spec = MdFocusRingSpec());

    /// Per-corner radii of the stroke centre line. Pass the component's
    /// current radii so the ring follows a shape morph.
    static QList<qreal> ringRadii(const QList<qreal> &componentRadii,
                                  const MdFocusRingSpec &spec = MdFocusRingSpec());

    /// Paint the ring. `componentRadii` empty means "use spec.shape".
    /// Pass `elapsedMs < 0` to draw the settled state with no animation.
    static void paint(QPainter *painter,
                      const QRectF &componentBounds,
                      const QList<qreal> &componentRadii,
                      const QColor &color,
                      const MdFocusRingSpec &spec = MdFocusRingSpec(),
                      int elapsedMs = -1);
};

/// Drives the focus-ring animation for one widget.
class QT_MD3_EXPORT MdFocusRingController : public QObject
{
    Q_OBJECT

public:
    explicit MdFocusRingController(QObject *parent = nullptr);
    ~MdFocusRingController() override;

    bool isAnimating() const { return m_animating; }
    int elapsedMs() const { return m_elapsedMs; }

    MdFocusRingSpec spec() const { return m_spec; }
    void setSpec(const MdFocusRingSpec &spec);

    /// Current stroke width; the settled `width` when idle.
    qreal currentWidth() const;

public slots:
    /// Focus gained: replay the grow/settle animation from t = 0.
    void start();
    /// Focus lost: stop and report that a repaint is due.
    void stop();

signals:
    void repaintRequested();

private:
    void onTick();

    QTimer *m_timer = nullptr;
    MdFocusRingSpec m_spec;
    int m_elapsedMs = 0;
    bool m_animating = false;
};

} // namespace md

#endif // MD_FOCUS_RING_H
