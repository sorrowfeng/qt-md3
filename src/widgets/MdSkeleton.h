#ifndef MD_SKELETON_H
#define MD_SKELETON_H

// MdSkeleton — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Surface-container-highest 占位 + shimmer".
// MD3 has no skeleton family. The control is the standard loading placeholder
// every Material list / card surface eventually needs:
//
//   base        `md.sys.color.surface-container-highest`
//   shimmer     a travelling highlight band over the base, phase-driven
//               (MdSpring-shaped envelope; never QEasingCurve)
//   shape       rect / circle / text-line blocks, corners from the shape scale
//
// Nothing here invents a colour token. The base fill is the published
// `surface-container-highest` role; the shimmer highlight is that same role
// lifted toward `surface-container-high` (the next step on the surface
// ladder), so the motion stays inside the tonal family the surface tokens
// already define.
//
// A skeleton is *not interactive*: no hover, no press, no focus. Like
// MdBadge and MdProgressIndicator the contract is "nothing happens".
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

/// The placeholder silhouettes the control can draw.
enum class MdSkeletonShape {
    /// A filled rounded rectangle (media / card block).
    Rect,
    /// A circle (avatar / icon slot).
    Circle,
    /// A stack of text-line bars of decreasing width, like a paragraph.
    TextLine,
    Count,
};

class QT_MD3_EXPORT MdSkeleton : public QWidget
{
    Q_OBJECT

    /// Travelling shimmer highlight. Off paints the flat base only.
    Q_PROPERTY(bool shimmering READ isShimmering WRITE setShimmering NOTIFY shimmeringChanged)
    /// Placeholder silhouette.
    Q_PROPERTY(md::MdSkeletonShape shape READ shape WRITE setShape NOTIFY shapeChanged)
    /// Bar count for `TextLine` (ignored by Rect / Circle). Minimum 1.
    Q_PROPERTY(int lines READ lines WRITE setLines NOTIFY linesChanged)

public:
    explicit MdSkeleton(QWidget *parent = nullptr);
    explicit MdSkeleton(MdSkeletonShape shape, QWidget *parent = nullptr);
    ~MdSkeleton() override;

    bool isShimmering() const { return m_shimmering; }
    void setShimmering(bool shimmering);

    MdSkeletonShape shape() const { return m_shape; }
    void setShape(MdSkeletonShape shape);

    int lines() const { return m_lines; }
    void setLines(int lines);

    /// Resolved base fill: surface-container-highest.
    QColor resolvedBaseColor() const;
    /// Resolved shimmer highlight: surface-container-high (the next surface
    /// ladder step), so the sweep stays inside the published surface family.
    QColor resolvedShimmerColor() const;

    /// True while the shimmer repaint cycle is running.
    bool isAnimating() const;

    /// Milliseconds since construction — the clock the shimmer phase runs on.
    qint64 animationElapsedMs() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void shimmeringChanged(bool shimmering);
    void shapeChanged(md::MdSkeletonShape shape);
    void linesChanged(int lines);

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void updateAnimationState();
    /// Current shimmer phase in [0, 1). Phase-driven, not an easing curve.
    qreal shimmerPhase() const;

    bool m_shimmering = true;
    MdSkeletonShape m_shape = MdSkeletonShape::TextLine;
    int m_lines = 3;

    QElapsedTimer m_clock;
    QTimer *m_timer = nullptr;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdSkeletonShape)

#endif // MD_SKELETON_H
