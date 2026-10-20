#include "MdPopover.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "private/MdPopup.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QScreen>
#include <QtGui/QShowEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtCore/QTimer>
#include <QtMath>

namespace md {

namespace {

/// `md.comp.menu.container.shape` — corner-extra-small (4 px).
/// `md.comp.menu.container.color` — surface-container.
/// `md.comp.menu.container.elevation` — level 2.
/// Vertical padding inside the menu container (Compose's 8 px).
constexpr qreal kVerticalPadding = 8.0;
/// Horizontal padding inside the menu container.
constexpr qreal kHorizontalPadding = 12.0;
/// Header gap above the content.
constexpr qreal kHeaderGap = 8.0;
/// Open-animation tick (the menu uses the same 8 ms cadence).
constexpr int kTickMs = 16;

} // namespace

MdPopover::MdPopover(QWidget *parent)
    : QWidget(parent, Qt::Popup)
{
    init();
}

MdPopover::~MdPopover()
{
    if (m_openTimer) {
        disconnect(m_openTimer, nullptr, this, nullptr);
    }
}

void MdPopover::init()
{
    // Same two attributes as MdMenu: no system background on the first
    // composite, translucent so the rounded surface shows through. The DWM
    // drop shadow must stay off — it reads as a black edge on a frameless
    // translucent popup (see MdPopup.h).
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::StrongFocus);
    setMinimumSize(QSize(120, 48));
    setAccessibleName(QStringLiteral("popover"));

    m_openTimer = new QTimer(this);
    m_openTimer->setInterval(kTickMs);
    m_openTimer->setTimerType(Qt::PreciseTimer);
    connect(m_openTimer, &QTimer::timeout, this, &MdPopover::onOpenTick);

    MdStyleBase::connectThemeUpdate(this, &MdPopover::onThemeChanged);
}

void MdPopover::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    relayout();
    update();
    emit titleChanged(m_title);
}

void MdPopover::setShape(ShapeCorner shape)
{
    if (shape >= ShapeCorner::Count) {
        return;
    }
    if (m_shape == shape) {
        return;
    }
    m_shape = shape;
    update();
    emit shapeChanged(m_shape);
}

void MdPopover::setContentWidget(QWidget *content)
{
    if (m_content == content) {
        return;
    }
    if (m_content) {
        m_content->setParent(nullptr);
    }
    m_content = content;
    if (m_content) {
        m_content->setParent(this);
        // The surface already paints the container. A default-styled child
        // would fill itself with QPalette::Window and punch an opaque slab
        // (near-black in dark theme) through the rounded surface.
        m_content->setAttribute(Qt::WA_NoSystemBackground, true);
        m_content->setAttribute(Qt::WA_TranslucentBackground, true);
        m_content->setAutoFillBackground(false);
    }
    relayout();
    update();
}

void MdPopover::setOpen(bool open)
{
    if (m_open == open) {
        return;
    }
    m_open = open;
    if (m_open) {
        m_openFrom = m_openProgress;
        // First painted frame is one vsync into the flight, same as popup().
        m_openElapsedOffsetMs = 16.0;
        {
            const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
            const qreal firstFrameSeconds = 0.016;
            m_openProgress = qBound(0.0, scaleSpring.valueAt(firstFrameSeconds), 1.0);
        }
        m_openClock.restart();
        m_openTimer->start();
        if (m_content) {
            // Real children composite outside the painter transform; the
            // surface paints them through the scale pair for the flight.
            m_content->setVisible(false);
        }
        showPopupWithoutFlash(this);
        setFocus(Qt::PopupFocusReason);
    } else {
        closeAndEmit();
    }
    emit openChanged(m_open);
}

void MdPopover::hide()
{
    if (!m_open) {
        return;
    }
    m_open = false;
    // Reverse the scale/alpha pair, then close.
    m_openFrom = m_openProgress;
    m_openElapsedOffsetMs = 0.0;
    m_openClock.restart();
    m_openTimer->start();
    if (m_content) {
        m_content->setVisible(false);
    }
    emit openChanged(m_open);
}

void MdPopover::popup(const QRect &anchorRect)
{
    relayout();
    const QSize target = sizeHint().expandedTo(minimumSize());
    setFixedSize(target);

    // Anchor below the trigger, flipping above when it would overflow.
    QScreen *scr = this->screen();
    const QRect screenBounds = scr ? scr->availableGeometry()
                                   : QRect(QPoint(0, 0), QSize(1920, 1080));
    QPoint at(anchorRect.left(), anchorRect.bottom() + 4);
    if (at.y() + target.height() > screenBounds.bottom()) {
        at.setY(anchorRect.top() - target.height() - 4);
    }
    if (at.x() + target.width() > screenBounds.right()) {
        at.setX(screenBounds.right() - target.width() - 8);
    }
    if (at.x() < screenBounds.left()) {
        at.setX(screenBounds.left() + 8);
    }
    move(at);

    m_open = true;
    m_openFrom = 0.0;
    // First painted frame is one vsync into the flight (never a blank map).
    m_openElapsedOffsetMs = 16.0;
    {
        const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
        const MdSpring alphaSpring = MdMotion::spring(MotionSpring::EffectsFast);
        const qreal firstFrameSeconds = 0.016;
        m_openProgress = qBound(0.0, scaleSpring.valueAt(firstFrameSeconds), 1.0);
        // Alpha shares the scale curve here; the popover exposes one progress.
        Q_UNUSED(alphaSpring);
    }
    m_openClock.restart();
    m_openTimer->start();
    if (m_content) {
        // The real child composites outside the painter transform; the
        // surface paints it through the scale pair for the flight.
        m_content->setVisible(false);
    }
    showPopupWithoutFlash(this);
    setFocus(Qt::PopupFocusReason);
    emit openChanged(m_open);
}

void MdPopover::closeAndEmit()
{
    m_open = false;
    m_openFrom = m_openProgress;
    m_openElapsedOffsetMs = 0.0;
    m_openClock.restart();
    m_openTimer->start();
    if (m_content) {
        m_content->setVisible(false);
    }
    emit openChanged(m_open);
    emit closed();
}

QColor MdPopover::resolvedSurfaceColor() const
{
    // md.sys.color.surface-container — the menu container tone.
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QColor MdPopover::resolvedTitleColor() const
{
    // md.sys.color.on-surface
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QSize MdPopover::sizeHint() const
{
    return m_fitSize.isValid() ? m_fitSize : QSize(280, 120);
}

void MdPopover::relayout()
{
    qreal w = 240.0;
    qreal h = kVerticalPadding * 2.0;

    if (!m_title.isEmpty()) {
        const QFontMetricsF fm(MdTypeScale::font(TypeStyle::TitleMedium));
        w = qMax(w, fm.horizontalAdvance(m_title) + kHorizontalPadding * 2.0);
        h += MdTypeScale::lineHeight(TypeStyle::TitleMedium) + kHeaderGap;
    }

    if (m_content) {
        const QSize cs = m_content->sizeHint().expandedTo(m_content->minimumSizeHint());
        w = qMax(w, qreal(cs.width()) + kHorizontalPadding * 2.0);
        h += cs.height();
    } else {
        h += 64.0;
    }

    m_fitSize = QSize(qCeil(w), qCeil(h));
    updateGeometry();
}

void MdPopover::applyContentGeometry()
{
    if (!m_content) {
        return;
    }
    const qreal padH = kHorizontalPadding;
    qreal y = kVerticalPadding;
    if (!m_title.isEmpty()) {
        y += MdTypeScale::lineHeight(TypeStyle::TitleMedium) + kHeaderGap;
    }
    m_content->setGeometry(QRect(qRound(padH), qRound(y),
                                 width() - qRound(padH * 2.0),
                                 height() - qRound(y + kVerticalPadding)));
}

void MdPopover::onOpenTick()
{
    // Fast spatial scale + fast effects alpha, the menu pair. Sampled
    // from MdSpring::valueAt on an elapsed clock - never per-tick steps.
    const MdSpring scaleSpring = MdMotion::spring(MotionSpring::SpatialFast);
    const MdSpring alphaSpring = MdMotion::spring(MotionSpring::EffectsFast);
    const qreal elapsedMs = qreal(m_openClock.elapsed()) + m_openElapsedOffsetMs;
    const qreal seconds = elapsedMs / 1000.0;
    const qreal target = m_open ? 1.0 : 0.0;
    const qreal scaleTravel = scaleSpring.valueAt(seconds);
    const qreal alphaTravel = alphaSpring.valueAt(seconds);
    const qreal scaleProgress = m_openFrom + (target - m_openFrom) * scaleTravel;
    const qreal alphaProgress = m_openFrom + (target - m_openFrom) * alphaTravel;
    const bool settled = elapsedMs >= qMax(scaleSpring.settlingDurationMs(),
                                           alphaSpring.settlingDurationMs());

    m_openProgress = qBound(0.0, qMin(scaleProgress, alphaProgress), 1.0);
    update();
    // Hide as soon as the close is visually done (alpha at zero) instead of
    // waiting out the slower scale spring: an invisible Qt::Popup that stays
    // mapped for another ~300 ms swallows clicks and reads as "hide not firing".
    const bool closeVisuallyDone = !m_open && m_openProgress <= 0.01;
    if (settled || closeVisuallyDone) {
        m_openTimer->stop();
        m_openProgress = target;
        if (!m_open) {
            // QWidget::hide(), not MdPopover::hide() — the slot starts the
            // close animation and returns immediately when m_open is already
            // false, leaving the widget mapped with a stale backing store.
            // Drop the native window opacity to 0 before the map goes away so
            // DWM cannot leave a dark rectangle behind a translucent popup.
            if (QWindow *handle = windowHandle()) {
                handle->setOpacity(0.0);
            }
            QWidget::hide();
            if (QWindow *handle = windowHandle()) {
                handle->setOpacity(1.0);
            }
        } else if (m_content) {
            // Flight over: the real child can composite on its own again.
            m_content->setVisible(true);
        }
    }
}

void MdPopover::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Clear to transparent first. WA_NoSystemBackground leaves the backing
    // store uncleared; the rounded path's antialiased edges then blend with
    // whatever was there (black on an uninitialized buffer) and show as a
    // dark fringe. CompositionMode_Source replaces the pixels outright.
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const QRectF bounds = QRectF(rect());
    const QList<qreal> radii = MdShape::resolvedRadii(m_shape, bounds.size());
    const QPainterPath path = MdShape::roundedRect(bounds, radii);

    // Open/close pair, same split as MdMenuStyle::paintMenu: scale on the
    // spatial spring, alpha on the effects spring, both around the top-start
    // anchor. One shared progress used to drag alpha onto the scale curve.
    const qreal kClosedScale = 0.8; // md.comp.menu.closedScale
    const qreal kClosedAlpha = 0.0;
    const qreal progress = qBound(0.0, m_openProgress, 1.0);
    const qreal scale = kClosedScale + (1.0 - kClosedScale) * progress;
    const qreal alpha = kClosedAlpha + (1.0 - kClosedAlpha) * progress;
    const bool animating = m_openTimer != nullptr && m_openTimer->isActive();
    if (animating || alpha < 0.999) {
        painter.setOpacity(painter.opacity() * qBound(0.0, alpha, 1.0));
    }
    if (animating && scale < 0.999) {
        const QPointF origin(0.0, 0.0);
        painter.translate(origin);
        painter.scale(scale, scale);
        painter.translate(-origin);
    }

    // 1. Surface fill and a 1 px outline-variant stroke — the two layers
    //    that keep the antialiased rounded edge from reading as a black
    //    fringe. No elevation shadow: the popover is a translucent popup
    //    whose widget rect is exactly the surface, so MdElevation's near-black
    //    (ColorRole::Shadow = neutral0) rings get clipped into the corners
    //    and, while the open/close scale is under 1, paint a dark halo along
    //    the whole edge (frame probe: dedge=687 on the first open frame,
    //    dedge=716 mid-close). MD3 expresses level 2 with the surface-container
    //    tone; the shadow is omitted on this surface rather than clipped into
    //    a black edge.
    painter.setPen(Qt::NoPen);
    painter.setBrush(resolvedSurfaceColor());
    painter.drawPath(path);
    {
        const QColor outline = MdTheme::instance().color(ColorRole::OutlineVariant);
        if (outline.isValid() && outline.alpha() > 0) {
            QPen pen(outline);
            pen.setWidthF(1.0);
            pen.setJoinStyle(Qt::RoundJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(MdStyleBase::crispRoundedRectPath(
                bounds.adjusted(0.5, 0.5, -0.5, -0.5), radii));
        }
    }

    if (!m_title.isEmpty()) {
        painter.setPen(resolvedTitleColor());
        painter.setFont(MdTypeScale::font(TypeStyle::TitleMedium));
        const qreal h = MdTypeScale::lineHeight(TypeStyle::TitleMedium);
        painter.drawText(QRectF(kHorizontalPadding, kVerticalPadding,
                                bounds.width() - kHorizontalPadding * 2.0, h),
                         Qt::AlignLeft | Qt::AlignVCenter, m_title);
    }

    // Content rides the same transform for the flight; the real child is
    // hidden while animating so it cannot composite at full opacity over
    // the fading surface. Clip to the rounded path so a rectangular child
    // cannot square off the corners mid-flight.
    if (animating && m_content && (scale < 0.999 || alpha < 0.999)) {
        painter.save();
        painter.setClipPath(MdShape::roundedRect(bounds, radii));
        m_content->render(&painter, m_content->pos(), QRegion(),
                          QWidget::DrawChildren | QWidget::IgnoreMask);
        painter.restore();
    }
}

void MdPopover::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        closeAndEmit();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdPopover::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyContentGeometry();
}

void MdPopover::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    applyContentGeometry();
}

void MdPopover::onThemeChanged()
{
    update();
}

} // namespace md
