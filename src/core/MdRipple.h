#ifndef MD_RIPPLE_H
#define MD_RIPPLE_H

// MdRipple — the press ripple.
//
// A ripple is a circle that expands radially from the point that was pressed
// and is clipped to the component's *current* corner radii, so it follows a
// shape that is morphing under it. It is a different mechanism from the state
// layer and must not be merged with it:
//
//   state layer — static tint for as long as hover / focus / press holds
//   ripple      — transient circle that expands on press and then fades
//
// Every constant below is transcribed from material-components/material-web:
//
//   ripple/internal/ripple.ts   PRESS_GROW_MS, MINIMUM_PRESS_MS,
//                              INITIAL_ORIGIN_SCALE, PADDING,
//                              SOFT_EDGE_MINIMUM_SIZE,
//                              SOFT_EDGE_CONTAINER_RATIO,
//                              determineRippleSize(),
//                              getTranslationCoordinates()
//   ripple/internal/_ripple.scss  the 375 ms linear fade-out and the
//                              radial-gradient soft edge
//   tokens/_md-comp-ripple.scss   ripple's colours and opacities, which are
//                              md.sys.color.on-surface at the md.sys.state
//                              hover / pressed opacities
//   internal/motion/animation.ts  EASING.STANDARD = cubic-bezier(.2, 0, 0, 1)
//
// Nothing here is guessed, and nothing is taken from Ant Design — Ant Design
// has a wave effect with entirely different geometry and timing.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtCore/QObject>
#include <QtCore/QPointF>
#include <QtCore/QSizeF>
#include <QtGui/QColor>
#include <QtGui/QPainterPath>

class QPainter;
class QTimer;

namespace md {

/// A paint-ready snapshot of one ripple at one instant.
struct QT_MD3_EXPORT MdRippleFrame
{
    /// False when there is nothing to draw (no press, or the fade finished).
    bool valid = false;
    QPointF center;
    qreal radius = 0.0;
    /// Normalised (0..1 of `radius`) position at which the soft edge begins.
    qreal softEdgeStart = 0.65;
    /// Final alpha multiplier, folding in pressed-state opacity and the fade.
    qreal opacity = 0.0;
};

/// Ripple geometry, timing and painting. A value type: it owns no state, so it
/// is trivially testable without a widget or an event loop.
class QT_MD3_EXPORT MdRipple
{
public:
    // --- published constants ---------------------------------------------
    /// Press growth duration.
    static constexpr int PressGrowMs = 450;
    /// A press is held at full size for at least this long, even if the
    /// pointer is released sooner, so a tap still reads as a ripple.
    static constexpr int MinimumPressMs = 225;
    /// Fade-out after release (`opacity 375ms linear` in _ripple.scss).
    static constexpr int FadeOutMs = 375;
    /// The circle starts at this fraction of the component's longest side.
    static constexpr qreal InitialOriginScale = 0.2;
    /// Overhang added to the corner-to-corner hypotenuse.
    static constexpr qreal Padding = 10.0;
    static constexpr qreal SoftEdgeMinimumSize = 75.0;
    static constexpr qreal SoftEdgeContainerRatio = 0.35;

    /// EASING.STANDARD.
    static constexpr MotionEasing GrowEasing = MotionEasing::Standard;
    /// Ripple paints with md.sys.color.on-surface at the pressed opacity.
    static constexpr StateLayerKind OverlayKind = StateLayerKind::Pressed;

    /// Per-bounds geometry: material-web's determineRippleSize().
    struct Geometry
    {
        /// Size of the unscaled circle (its bounding box) at press time.
        qreal initialSize = 0.0;
        /// Radius the circle reaches at the end of the growth.
        qreal finalRadius = 0.0;
        /// Soft-edge band width in unscaled pixels.
        qreal softEdgeSize = 0.0;
        /// Normalised radius at which the soft edge starts.
        qreal softEdgeStart = 0.65;

        bool isValid() const { return initialSize > 0.0; }
    };

    /// Geometry for a component of `bounds`.
    ///
    /// A non-positive width or height yields an invalid geometry. Both are
    /// required: a ripple is a circle covering the whole box, so a box with no
    /// area has nothing to cover, and a negative extent is a caller bug that
    /// must not be silently rounded up to zero.
    static Geometry geometryFor(const QSizeF &bounds);

    /// Eased growth progress for `elapsedMs` since the press began.
    static qreal progressAt(int elapsedMs);

    /// Centre of the circle at `progress` (0..1, already eased). It travels
    /// from the press position to the centre of `bounds`.
    static QPointF centerAt(const Geometry &geometry,
                            const QPointF &pressPosition,
                            const QSizeF &bounds,
                            qreal progress);

    /// Radius of the circle at `progress` (0..1, already eased).
    static qreal radiusAt(const Geometry &geometry, qreal progress);

    /// The frame to paint.
    /// `releasedAtMs` is the elapsed time at which the pointer was released,
    /// or a negative value while the ripple is still held.
    static MdRippleFrame frame(const Geometry &geometry,
                               const QPointF &pressPosition,
                               const QSizeF &bounds,
                               int elapsedMs,
                               int releasedAtMs = -1);

    /// Milliseconds until the ripple is finished and can be dropped.
    static int lifetimeMs(int releasedAtMs);

    /// Fill `painter` with the ripple, clipped to `clipPath` — pass the
    /// component's rounded-rect path built from its *current* corner radii.
    static void paint(QPainter *painter,
                      const MdRippleFrame &frame,
                      const QPainterPath &clipPath,
                      const QColor &contentColor,
                      qreal opacityScale = 1.0);
};

/// Drives a ripple for one widget: owns the timeline and emits
/// repaintRequested() while the ripple is alive. Widgets call press()/
/// release() from their mouse handlers and repaint on the signal.
class QT_MD3_EXPORT MdRippleController : public QObject
{
    Q_OBJECT

public:
    explicit MdRippleController(QObject *parent = nullptr);
    ~MdRippleController() override;

    bool isActive() const { return m_active; }

    /// Last known content extent. The controller recomputes the ripple
    /// geometry whenever this changes so a resize mid-press stays correct.
    QSizeF bounds() const { return m_bounds; }
    void setBounds(const QSizeF &bounds);

    /// Colour the ripple is painted with; normally md.sys.color.on-surface.
    QColor contentColor() const { return m_contentColor; }
    void setContentColor(const QColor &contentColor);

    /// Clip path, normally the component's rounded rect. Set it every paint so
    /// a shape morph is followed.
    QPainterPath clipPath() const { return m_clipPath; }
    void setClipPath(const QPainterPath &clipPath);

    const MdRipple::Geometry &geometry() const { return m_geometry; }

    /// Frame for the current instant.
    ///
    /// Returns an invalid frame while the controller is idle, so painting on
    /// `currentFrame().valid` is safe and equivalent to testing `isActive()`.
    MdRippleFrame currentFrame() const;

public slots:
    /// Begin a ripple at `position` (widget coordinates).
    void press(const QPointF &position);
    /// Keyboard activation: begin at the centre of the bounds.
    void pressCentered();
    /// Begin the fade-out.
    void release();
    /// Drop the ripple immediately.
    void cancel();

signals:
    /// Emitted whenever the visual state changed and a repaint is due.
    void repaintRequested();

private:
    void onTick();
    void refreshGeometry();

    QTimer *m_timer = nullptr;
    QSizeF m_bounds;
    QColor m_contentColor;
    QPainterPath m_clipPath;
    MdRipple::Geometry m_geometry;
    QPointF m_pressPosition;
    int m_elapsedMs = 0;
    int m_releasedAtMs = -1;
    bool m_active = false;
};

} // namespace md

#endif // MD_RIPPLE_H
