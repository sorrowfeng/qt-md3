#include "MdSkeleton.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QLinearGradient>

#include <cmath>

namespace md {

namespace {

/// Default text-line height (label-large line box).
constexpr qreal kLineHeight = 16.0;
/// Gap between text-line bars.
constexpr qreal kLineGap = 8.0;
/// Corner for the Rect silhouette. Shape-scale ExtraSmall (4 px), the same
/// step MdCard's compact blocks use — never a hard-coded radius at paint time.
constexpr ShapeCorner kRectCorner = ShapeCorner::ExtraSmall;

/// One shimmer cycle, in ms. Derived from `md.sys.motion.duration-extra-long4`
/// (700 ms) × 2 so a full left-to-right sweep reads at the long4 tempo
/// without looking hurried. See docs/porting-todo.md (Stage 2, MdSkeleton).
constexpr int kShimmerCycleMs = 1400;

/// Shimmer band half-width as a fraction of the widget diagonal.
constexpr qreal kShimmerHalfWidth = 0.22;

} // namespace

MdSkeleton::MdSkeleton(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSkeleton::MdSkeleton(MdSkeletonShape shape, QWidget *parent)
    : QWidget(parent)
    , m_shape(shape)
{
    init();
}

MdSkeleton::~MdSkeleton() = default;

void MdSkeleton::init()
{
    // Display slot only: no focus, no state layer, no ripple.
    setFocusPolicy(Qt::NoFocus);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAccessibleName(QStringLiteral("skeleton"));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_clock.start();
    m_timer = new QTimer(this);
    m_timer->setTimerType(Qt::PreciseTimer);
    connect(m_timer, &QTimer::timeout, this, &MdSkeleton::onAnimationFrame);

    MdStyleBase::connectThemeUpdate(this, &MdSkeleton::onThemeChanged);
    updateAnimationState();
}

void MdSkeleton::setShimmering(bool shimmering)
{
    if (m_shimmering == shimmering) {
        return;
    }
    m_shimmering = shimmering;
    updateAnimationState();
    update();
    emit shimmeringChanged(m_shimmering);
}

void MdSkeleton::setShape(MdSkeletonShape shape)
{
    if (shape >= MdSkeletonShape::Count) {
        return;
    }
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    updateGeometry();
    update();
    emit shapeChanged(m_shape);
}

void MdSkeleton::setLines(int lines)
{
    const int clamped = qMax(1, lines);
    if (m_lines == clamped) {
        return;
    }
    m_lines = clamped;
    updateGeometry();
    update();
    emit linesChanged(m_lines);
}

QColor MdSkeleton::resolvedBaseColor() const
{
    // md.sys.color.surface-container-highest — the appendix D anchor.
    return MdTheme::instance().color(ColorRole::SurfaceContainerHighest);
}

QColor MdSkeleton::resolvedShimmerColor() const
{
    // The next step down the published surface ladder (surface-container-high),
    // so the highlight is a real tonal neighbour rather than a white overlay.
    return MdTheme::instance().color(ColorRole::SurfaceContainerHigh);
}

bool MdSkeleton::isAnimating() const
{
    return m_timer != nullptr && m_timer->isActive();
}

qint64 MdSkeleton::animationElapsedMs() const
{
    return m_clock.elapsed();
}

QSize MdSkeleton::sizeHint() const
{
    switch (m_shape) {
    case MdSkeletonShape::Circle:
        return QSize(48, 48);
    case MdSkeletonShape::Rect:
        return QSize(160, 96);
    case MdSkeletonShape::TextLine:
    case MdSkeletonShape::Count:
        break;
    }
    const qreal height = m_lines * kLineHeight + (m_lines - 1) * kLineGap;
    return QSize(200, qMax(kLineHeight, height));
}

QSize MdSkeleton::minimumSizeHint() const
{
    return QSize(24, 24);
}

void MdSkeleton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    if (box.width() <= 1.0 || box.height() <= 1.0) {
        return;
    }

    const QColor base = resolvedBaseColor();

    QVector<QPainterPath> blocks;
    switch (m_shape) {
    case MdSkeletonShape::Circle: {
        QPainterPath circle;
        const qreal side = qMin(box.width(), box.height());
        const QRectF disc(box.center().x() - side / 2.0, box.center().y() - side / 2.0,
                          side, side);
        circle.addEllipse(disc);
        blocks.append(circle);
        break;
    }
    case MdSkeletonShape::Rect: {
        const QList<qreal> radii =
            MdShape::resolvedRadii(kRectCorner, box.size());
        blocks.append(MdShape::roundedRect(box, radii));
        break;
    }
    case MdSkeletonShape::TextLine:
    case MdSkeletonShape::Count: {
        // Paragraph silhouette: full-width first bar, then shorter ones.
        // Width fractions are a presentation choice (not a published token);
        // the *height* and *gap* are the type-scale line box and the 8 px
        // intra-stack gap used across List / Dialog content.
        static const qreal kWidthFractions[] = {1.0, 0.92, 0.68};
        const qreal totalHeight =
            m_lines * kLineHeight + (m_lines - 1) * kLineGap;
        qreal y = box.top() + qMax(0.0, (box.height() - totalHeight) / 2.0);
        for (int i = 0; i < m_lines; ++i) {
            const qreal fraction =
                kWidthFractions[i % int(sizeof(kWidthFractions) / sizeof(kWidthFractions[0]))];
            const qreal w = box.width() * fraction;
            const QRectF lineRect(box.left(), y, w, kLineHeight);
            const QList<qreal> radii =
                MdShape::resolvedRadii(kRectCorner, lineRect.size());
            blocks.append(MdShape::roundedRect(lineRect, radii));
            y += kLineHeight + kLineGap;
        }
        break;
    }
    }

    for (const QPainterPath &block : blocks) {
        painter.fillPath(block, base);
    }

    if (!m_shimmering || !isVisible()) {
        return;
    }

    // Phase-driven travelling band. The phase advances linearly on the
    // animation clock (the same rule as the progress-indicator wave phase);
    // the band *profile* is a raised cosine, never a QEasingCurve.
    const qreal phase = shimmerPhase();
    const qreal travel = box.width() + box.height();
    const qreal bandCentre = -kShimmerHalfWidth * travel
                             + phase * travel * (1.0 + 2.0 * kShimmerHalfWidth);
    const qreal half = kShimmerHalfWidth * travel;

    const QColor highlight = resolvedShimmerColor();
    for (const QPainterPath &block : blocks) {
        const QRectF bounds = block.boundingRect();
        QLinearGradient gradient(bounds.topLeft(), bounds.bottomRight());
        // Sample the raised-cosine band along the gradient axis (the diagonal
        // of the block, matching the gradient direction above).
        const int samples = 12;
        for (int i = 0; i <= samples; ++i) {
            const qreal t = qreal(i) / qreal(samples);
            // Position along the travel axis in the block's diagonal space.
            const qreal axis = (bounds.width() + bounds.height()) * t;
            const qreal distance = std::fabs(axis - bandCentre);
            qreal intensity = 0.0;
            if (distance < half) {
                // cos-shaped falloff, 1 at the centre of the band.
                intensity = 0.5 * (1.0 + std::cos(distance / half * 3.14159265358979323846));
            }
            const QColor mixed = QColor::fromRgbF(
                base.redF() + (highlight.redF() - base.redF()) * intensity,
                base.greenF() + (highlight.greenF() - base.greenF()) * intensity,
                base.blueF() + (highlight.blueF() - base.blueF()) * intensity);
            gradient.setColorAt(t, mixed);
        }
        painter.fillPath(block, gradient);
    }
}

void MdSkeleton::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    updateAnimationState();
}

void MdSkeleton::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    updateAnimationState();
}

void MdSkeleton::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange) {
        update();
    }
}

void MdSkeleton::onThemeChanged()
{
    update();
}

void MdSkeleton::onAnimationFrame()
{
    update();
}

void MdSkeleton::updateAnimationState()
{
    const bool run = m_shimmering && isVisible();
    if (m_timer == nullptr) {
        return;
    }
    if (run && !m_timer->isActive()) {
        // ~60 fps. The phase is clock-derived, so a dropped frame only skips
        // a repaint — it does not desynchronise the sweep.
        m_timer->start(16);
    } else if (!run && m_timer->isActive()) {
        m_timer->stop();
    }
}

qreal MdSkeleton::shimmerPhase() const
{
    const qint64 elapsed = m_clock.elapsed();
    if (kShimmerCycleMs <= 0) {
        return 0.0;
    }
    // Linear phase wrap. No easing curve is consulted anywhere on this path.
    return std::fmod(qreal(elapsed) / qreal(kShimmerCycleMs), 1.0);
}

} // namespace md
