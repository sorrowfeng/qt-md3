#include "MdPopover.h"

#include "core/MdMotion.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
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
        m_openClock.restart();
        m_openTimer->start();
        show();
        raise();
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
    m_openClock.restart();
    m_openTimer->start();
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
    m_openProgress = 0.0;
    m_openClock.restart();
    m_openTimer->start();
    show();
    raise();
    setFocus(Qt::PopupFocusReason);
    emit openChanged(m_open);
}

void MdPopover::closeAndEmit()
{
    m_open = false;
    m_openFrom = m_openProgress;
    m_openClock.restart();
    m_openTimer->start();
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
    const qreal seconds = qreal(m_openClock.elapsed()) / 1000.0;
    const qreal target = m_open ? 1.0 : 0.0;
    const qreal scaleTravel = scaleSpring.valueAt(seconds);
    const qreal alphaTravel = alphaSpring.valueAt(seconds);
    const qreal scaleProgress = m_openFrom + (target - m_openFrom) * scaleTravel;
    const qreal alphaProgress = m_openFrom + (target - m_openFrom) * alphaTravel;
    const bool settled = seconds * 1000.0 >= qMax(scaleSpring.settlingDurationMs(),
                                                  alphaSpring.settlingDurationMs());

    m_openProgress = qBound(0.0, qMin(scaleProgress, alphaProgress), 1.0);
    update();
    if (settled) {
        m_openTimer->stop();
        m_openProgress = target;
        if (!m_open) {
            hide();
        }
    }
}

void MdPopover::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = QRectF(rect());
    const QList<qreal> radii = MdShape::resolvedRadii(m_shape, bounds.size());
    const QPainterPath path = MdShape::roundedRect(bounds, radii);

    painter.fillPath(path, resolvedSurfaceColor());

    if (!m_title.isEmpty()) {
        painter.setPen(resolvedTitleColor());
        painter.setFont(MdTypeScale::font(TypeStyle::TitleMedium));
        const qreal h = MdTypeScale::lineHeight(TypeStyle::TitleMedium);
        painter.drawText(QRectF(kHorizontalPadding, kVerticalPadding,
                                bounds.width() - kHorizontalPadding * 2.0, h),
                         Qt::AlignLeft | Qt::AlignVCenter, m_title);
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
