// Gallery pages, part 2: surface (shape + elevation) and motion.

#include "GalleryPages.h"

#include "core/MdMotion.h"

#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontMetricsF>
#include <QtGui/QMouseEvent>

#include <cmath>

namespace gallery {

namespace {

constexpr qreal kMorphTrackHeight = 40.0;

} // namespace

// ---------------------------------------------------------------------------
// SurfacePage
// ---------------------------------------------------------------------------

SurfacePage::SurfacePage(QWidget *parent)
    : GalleryPage(parent)
{
    setCursor(Qt::ArrowCursor);
}

QString SurfacePage::title() const
{
    return QStringLiteral("Shape & elevation");
}

QString SurfacePage::subtitle() const
{
    return QStringLiteral("The corner scale, shape morphing, and containment expressed as tone.");
}

void SurfacePage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    m_sliderTracks.clear();

    context.section(QStringLiteral("Corner scale"));
    context.paragraph(QStringLiteral(
        "Every corner in MD3 is a token. Nothing in the library hard-codes a radius, which is "
        "what makes shape morphing possible: a component animates between two radius sets "
        "instead of snapping. The four M3 Expressive additions — large-increased, "
        "extra-large-increased and extra-extra-large — are included."));

    for (int i = 0; i < int(md::ShapeCorner::Count); ++i) {
        const auto corner = md::ShapeCorner(i);
        const QRectF row = context.band(44.0);
        if (painter == nullptr) {
            continue;
        }
        const qreal tokenRadius = md::MdShape::radius(corner);
        const QString tokenText =
            tokenRadius < 0.0 ? QStringLiteral("corner-full")
                              : QStringLiteral("%1 dp").arg(tokenRadius, 0, 'f', 0);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        // Draw the shape at a fixed size so 0dp and full are comparable.
        const QRectF sample(row.left() + 300.0, row.top() + 6.0, 72.0, 32.0);
        fillRounded(*painter, sample, role(md::ColorRole::PrimaryContainer), corner);
        painter->restore();

        painter->save();
        painter->setFont(fontFor(md::TypeStyle::LabelLarge));
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(row.left(), row.top(), 200.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, md::shapeCornerName(corner));
        painter->setFont(fontFor(md::TypeStyle::LabelSmall));
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(row.left() + 200.0, row.top(), 96.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, tokenText);
        painter->restore();
    }
    context.space(16.0);

    context.section(QStringLiteral("Shape morph"));
    context.paragraph(QStringLiteral(
        "Shape morphing interpolates the path, not a CSS transition on border-radius, so a "
        "shape can pass through intermediate forms — square to pill, circle to squircle — "
        "while keeping a single crisp outline. Drag the track below."));

    const QRectF morphBand = context.band(120.0);
    if (painter != nullptr) {
        const qreal morphWidth = qMin<qreal>(morphBand.width(), 320.0);
        const QRectF left(morphBand.left() + 10.0, morphBand.top() + 30.0, morphWidth, 60.0);
        const QRectF right(morphBand.left() + morphWidth + 60.0, morphBand.top() + 30.0,
                           morphWidth, 60.0);

        const QRectF target(left.left(), left.top(), right.right() - left.left(), 60.0);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QList<qreal> from = md::MdShape::resolvedRadii(md::ShapeCorner::ExtraSmall, target.size());
        const QList<qreal> to = md::MdShape::resolvedRadii(md::ShapeCorner::Full, target.size());
        const QList<qreal> current = md::MdShape::lerpRadii(from, to, m_morph);
        painter->setPen(Qt::NoPen);
        painter->setBrush(role(md::ColorRole::TertiaryContainer));
        painter->drawPath(md::MdShape::roundedRect(target, current));
        painter->restore();
    }

    const QRectF trackBand = context.band(kMorphTrackHeight);
    m_sliderTracks.append(trackBand.adjusted(0.0, 14.0, 0.0, -14.0));
    if (painter != nullptr) {
        const QRectF track = m_sliderTracks.last();
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(Qt::NoPen);
        painter->setBrush(role(md::ColorRole::SurfaceContainerHighest));
        painter->drawPath(md::MdShape::roundedRect(
            track, md::MdShape::resolvedRadii(md::ShapeCorner::Full, track.size())));
        const QRectF filled(track.left(), track.top(),
                            qMax<qreal>(track.width() * m_morph, track.height()), track.height());
        painter->setBrush(role(md::ColorRole::Primary));
        painter->drawPath(md::MdShape::roundedRect(
            filled, md::MdShape::resolvedRadii(md::ShapeCorner::Full, filled.size())));
        const QPointF knob(filled.right(), filled.center().y());
        painter->setBrush(role(md::ColorRole::Primary));
        painter->drawEllipse(knob, 11.0, 11.0);
        painter->restore();
    }
    context.space(20.0);

    context.section(QStringLiteral("Elevation"));
    context.paragraph(QStringLiteral(
        "MD3 expresses containment with tonal surface, not with shadow. Each elevation level "
        "maps to a surface-container role whose tone steps up in light mode and down in dark "
        "mode, so a raised element reads as raised in both. Shadows exist in the API but are "
        "opt-in per component spec, because most MD3 surfaces must not have one."));

    for (int i = 0; i < int(md::ElevationLevel::Count); ++i) {
        const auto level = md::ElevationLevel(i);
        const QRectF row = context.band(72.0);
        if (painter == nullptr) {
            continue;
        }
        const QRectF card(row.left(), row.top() + 6.0, row.width() - 4.0, 60.0);
        painter->save();
        paintElevated(*painter, card, level);
        painter->restore();

        painter->save();
        painter->setFont(fontFor(md::TypeStyle::TitleSmall));
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(card.adjusted(20.0, 0.0, -20.0, 0.0), Qt::AlignLeft | Qt::AlignVCenter,
                          md::elevationLevelName(level));
        painter->setFont(fontFor(md::TypeStyle::LabelMedium));
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(card.adjusted(20.0, 0.0, -20.0, 0.0), Qt::AlignRight | Qt::AlignVCenter,
                          QStringLiteral("%1 dp · %2")
                              .arg(md::MdElevation::shadowDp(level), 0, 'f', 0)
                              .arg(md::colorRoleName(md::MdElevation::surfaceRole(level))));
        painter->restore();
    }
}

void SurfacePage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    for (const QRectF &track : m_sliderTracks) {
        if (track.adjusted(-10.0, -14.0, 10.0, 14.0).contains(event->pos())) {
            m_dragging = true;
            mouseMoveEvent(event);
            return;
        }
    }
}

void SurfacePage::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragging || m_sliderTracks.isEmpty()) {
        return;
    }
    const QRectF track = m_sliderTracks.first();
    if (track.width() <= 0.0) {
        return;
    }
    m_morph = qBound(0.0, (event->pos().x() - track.left()) / track.width(), 1.0);
    update();
}

void SurfacePage::paintEvent(QPaintEvent *event)
{
    if (m_dragging && !(QGuiApplication::mouseButtons() & Qt::LeftButton)) {
        m_dragging = false;
    }
    GalleryPage::paintEvent(event);
}

// ---------------------------------------------------------------------------
// MotionPage
// ---------------------------------------------------------------------------

MotionPage::MotionPage(QWidget *parent)
    : GalleryPage(parent)
{
    m_clock.start();
    auto *timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, [this] {
        if (isVisible()) {
            update();
        }
    });
    timer->start();
}

QString MotionPage::title() const
{
    return QStringLiteral("Motion");
}

QString MotionPage::subtitle() const
{
    return QStringLiteral("Duration tokens, easing curves, and the six M3 Expressive springs.");
}

void MotionPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();

    context.section(QStringLiteral("Two motion systems"));
    context.paragraph(QStringLiteral(
        "MD3 has two ways to move something and they coexist. Token motion is a fixed "
        "duration plus a cubic-bezier easing curve — predictable, and what most transitions "
        "use. M3 Expressive spring motion is a damped harmonic oscillator: no duration, just "
        "stiffness and damping, which is what makes a control feel physical. The springs "
        "cannot be expressed as an easing curve and vice versa, so qt-md3 models both."));
    context.space(10.0);

    // --- easing curves ----------------------------------------------------
    context.section(QStringLiteral("Easing"));
    const int easingCount = int(md::MotionEasing::Count);
    const qreal plotSize = 88.0;
    const int columns = qMax(1, int(context.width() / (plotSize + 24.0)));

    for (int row = 0; row * columns < easingCount; ++row) {
        const QRectF rowRect = context.band(plotSize + 44.0);
        if (painter == nullptr) {
            continue;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        for (int column = 0; column < columns; ++column) {
            const int index = row * columns + column;
            if (index >= easingCount) {
                break;
            }
            const auto easing = md::MotionEasing(index);
            const QRectF box(rowRect.left() + column * (plotSize + 24.0), rowRect.top(),
                             plotSize, plotSize);

            painter->setPen(Qt::NoPen);
            painter->setBrush(role(md::ColorRole::SurfaceContainer));
            painter->drawPath(md::MdShape::roundedRect(
                box, md::MdShape::resolvedRadii(md::ShapeCorner::Small, box.size())));
            // Baseline.
            QPen axis(role(md::ColorRole::OutlineVariant));
            axis.setCosmetic(true);
            painter->setPen(axis);
            painter->drawLine(QPointF(box.left() + 8.0, box.bottom() - 8.0),
                              QPointF(box.right() - 8.0, box.bottom() - 8.0));

            // The curve itself: t horizontally, eased value vertically.
            QPainterPath path;
            const int steps = 48;
            for (int step = 0; step <= steps; ++step) {
                const qreal t = qreal(step) / qreal(steps);
                const qreal value = md::MdMotion::easedValue(easing, t);
                const QPointF point(box.left() + 8.0 + t * (plotSize - 16.0),
                                    box.bottom() - 8.0 - value * (plotSize - 16.0));
                if (step == 0) {
                    path.moveTo(point);
                } else {
                    path.lineTo(point);
                }
            }
            QPen curve(role(md::ColorRole::Primary));
            curve.setWidthF(2.0);
            curve.setCosmetic(true);
            painter->setPen(curve);
            painter->setBrush(Qt::NoBrush);
            painter->drawPath(path);

            painter->setFont(fontFor(md::TypeStyle::LabelSmall));
            painter->setPen(role(md::ColorRole::OnSurface));
            painter->drawText(QRectF(box.left(), box.bottom() + 2.0, plotSize, 16.0),
                              Qt::AlignLeft | Qt::AlignTop,
                              md::easingName(easing).remove(QStringLiteral("easing-")));
            const QList<qreal> points = md::MdMotion::easingControlPoints(easing);
            painter->setPen(role(md::ColorRole::OnSurfaceVariant));
            painter->drawText(
                QRectF(box.left(), box.bottom() + 18.0, plotSize, 16.0),
                Qt::AlignLeft | Qt::AlignTop,
                QStringLiteral("(%1, %2, %3, %4)")
                    .arg(points.value(0), 0, 'f', 2)
                    .arg(points.value(1), 0, 'f', 2)
                    .arg(points.value(2), 0, 'f', 2)
                    .arg(points.value(3), 0, 'f', 2));
        }
        painter->restore();
    }
    context.space(16.0);

    // --- durations --------------------------------------------------------
    context.section(QStringLiteral("Durations"));
    context.paragraph(QStringLiteral(
        "Sixteen tokens, from short1 at 50 ms to extra-long4 at 1000 ms. Bars are scaled "
        "against the longest token."));
    for (int i = 0; i < int(md::MotionDuration::Count); ++i) {
        const auto duration = md::MotionDuration(i);
        const int ms = md::MdMotion::durationMs(duration);
        const QRectF row = context.band(22.0);
        if (painter == nullptr) {
            continue;
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setFont(fontFor(md::TypeStyle::LabelSmall));
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(row.left(), row.top(), 150.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, md::durationName(duration));
        const qreal barLeft = row.left() + 160.0;
        const qreal barWidth = qMax<qreal>(row.width() - 240.0, 40.0);
        const QRectF bar(barLeft, row.top() + 5.0, barWidth * (ms / 1000.0), row.height() - 10.0);
        painter->setPen(Qt::NoPen);
        painter->setBrush(role(md::ColorRole::Primary));
        painter->drawPath(md::MdShape::roundedRect(
            bar, md::MdShape::resolvedRadii(md::ShapeCorner::Full, bar.size())));
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(barLeft + barWidth + 8.0, row.top(), 70.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("%1 ms").arg(ms));
        painter->restore();
    }
    context.space(16.0);

    // --- springs ----------------------------------------------------------
    context.section(QStringLiteral("Springs"));
    context.paragraph(QStringLiteral(
        "The six Expressive slots: spatial and effects, each at fast, default and slow. "
        "Spatial springs are underdamped and visibly overshoot; effects springs are "
        "critically damped because a colour or opacity that bounces looks like a bug. The "
        "markers animate from the published stiffness and damping ratio."));

    const qreal elapsed = qreal(m_clock.elapsed()) / 1000.0;
    const int springCount = int(md::MotionSpring::Count);
    for (int i = 0; i < springCount; ++i) {
        const auto token = md::MotionSpring(i);
        const md::MdSpring spring = md::MdMotion::spring(token);
        const QRectF row = context.band(56.0);
        if (painter == nullptr || !spring.isValid()) {
            continue;
        }

        const qreal settle = qMax<qreal>(spring.settlingDurationMs() / 1000.0, 0.4);
        const qreal cycle = settle + 0.6;
        const qreal phase = std::fmod(elapsed + i * 0.12, cycle);
        const qreal value = spring.valueAt(phase);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        const QRectF track(row.left() + 220.0, row.top() + 14.0, qMax<qreal>(row.width() - 340.0, 80.0),
                           28.0);
        painter->setPen(Qt::NoPen);
        painter->setBrush(role(md::ColorRole::SurfaceContainerHighest));
        painter->drawPath(md::MdShape::roundedRect(
            track, md::MdShape::resolvedRadii(md::ShapeCorner::Full, track.size())));

        // Travel 0..1 across the track; springs may overshoot past the end.
        const QPointF knob(track.left() + track.height() / 2.0
                               + value * (track.width() - track.height()),
                           track.center().y());
        painter->setBrush(role(md::ColorRole::Primary));
        painter->drawEllipse(knob, track.height() / 2.0 - 4.0, track.height() / 2.0 - 4.0);

        painter->setFont(fontFor(md::TypeStyle::LabelMedium));
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(row.left(), row.top(), 210.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter, md::springName(token));
        painter->setFont(fontFor(md::TypeStyle::LabelSmall));
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(track.right() + 8.0, row.top(), 110.0, row.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          QStringLiteral("k%1 ζ%2")
                              .arg(spring.stiffness(), 0, 'f', 0)
                              .arg(spring.dampingRatio(), 0, 'f', 2));
        painter->restore();
    }
}

void MotionPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
}

} // namespace gallery
