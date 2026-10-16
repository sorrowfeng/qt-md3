#include "MdDockWidget.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdIconButton.h"

#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// `md.comp.side-sheet` header height — 48 px on the default track.
constexpr int kHeaderHeight = 48;
/// Close button side (`md.comp.icon-button.xsmall.container` 32 + margin).
constexpr int kCloseSide = 32;
/// Divider thickness (`md.comp.divider.thickness` 1 px).
constexpr qreal kDivider = 1.0;

} // namespace

MdDockWidget::MdDockWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    init();
    setTitle(title);
}

MdDockWidget::~MdDockWidget()
{
    // Children are destroyed in ~QWidget, which runs *before* ~QObject
    // auto-disconnects. Cut the wires first (the MdInputNumber pattern).
    if (m_closeButton) {
        disconnect(m_closeButton, nullptr, this, nullptr);
    }
}

void MdDockWidget::init()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(QSize(120, kHeaderHeight));
    setAccessibleName(QStringLiteral("dock widget"));

    m_closeButton = new MdIconButton(QStringLiteral("close"), this);
    m_closeButton->setVariant(IconButtonVariant::Standard);
    m_closeButton->setButtonSize(ButtonSize::XSmall);
    m_closeButton->setFocusPolicy(Qt::NoFocus);
    m_closeButton->setAccessibleName(QStringLiteral("Close"));
    connect(m_closeButton, &MdIconButton::clicked, this, &MdDockWidget::onCloseClicked);

    MdStyleBase::connectThemeUpdate(this, &MdDockWidget::onThemeChanged);
}

int MdDockWidget::headerHeight()
{
    return kHeaderHeight;
}

void MdDockWidget::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    setAccessibleName(title.isEmpty() ? QStringLiteral("dock widget") : title);
    update();
    emit titleChanged(m_title);
}

void MdDockWidget::setArea(MdDockArea area)
{
    if (area >= MdDockArea::Count) {
        return;
    }
    if (m_area == area) {
        return;
    }
    m_area = area;
    update();
    emit areaChanged(m_area);
}

void MdDockWidget::setClosable(bool closable)
{
    if (m_closable == closable) {
        return;
    }
    m_closable = closable;
    if (m_closeButton) {
        m_closeButton->setVisible(closable);
    }
    update();
    emit closableChanged(m_closable);
}

void MdDockWidget::setOpen(bool open)
{
    if (m_open == open) {
        return;
    }
    m_open = open;
    update();
    emit openChanged(m_open);
}

void MdDockWidget::setContentWidget(QWidget *content)
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
    layoutChildren();
    update();
}

QColor MdDockWidget::resolvedSurfaceColor() const
{
    // md.sys.color.surface-container — the side-sheet container tone.
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QColor MdDockWidget::resolvedTitleColor() const
{
    // md.sys.color.on-surface
    return MdTheme::instance().color(ColorRole::OnSurface);
}

QColor MdDockWidget::resolvedDividerColor() const
{
    // md.sys.color.outline-variant — the side-sheet divider.
    return MdTheme::instance().color(ColorRole::OutlineVariant);
}

QSize MdDockWidget::sizeHint() const
{
    return QSize(240, 180);
}

QSize MdDockWidget::minimumSizeHint() const
{
    return QSize(120, kHeaderHeight + 40);
}

QRect MdDockWidget::headerRect() const
{
    return QRect(0, 0, width(), kHeaderHeight);
}

QRect MdDockWidget::contentRect() const
{
    return QRect(0, kHeaderHeight, width(), qMax(0, height() - kHeaderHeight));
}

void MdDockWidget::layoutChildren()
{
    if (m_closeButton) {
        const int x = width() - kCloseSide - 4;
        m_closeButton->setGeometry(QRect(x, (kHeaderHeight - kCloseSide) / 2,
                                         kCloseSide, kCloseSide));
    }
    if (m_content) {
        m_content->setGeometry(contentRect());
    }
}

void MdDockWidget::onCloseClicked()
{
    setOpen(false);
    emit closeRequested();
}

void MdDockWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF bounds = QRectF(rect());
    painter.fillRect(bounds, resolvedSurfaceColor());

    // Header label.
    if (!m_title.isEmpty()) {
        painter.setPen(resolvedTitleColor());
        painter.setFont(MdTypeScale::font(TypeStyle::TitleMedium));
        const qreal rightReserve = m_closable ? kCloseSide + 8.0 : 8.0;
        const QRectF labelRect(12.0, 0.0,
                               bounds.width() - rightReserve - 12.0, kHeaderHeight);
        painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    }

    // Divider on the bottom of the header.
    painter.fillRect(QRectF(0.0, kHeaderHeight - kDivider, bounds.width(), kDivider),
                     resolvedDividerColor());
}

void MdDockWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    const QPointF pos = mousePosition(event);
    if (headerRect().contains(QPoint(qRound(pos.x()), qRound(pos.y())))) {
        m_dragging = true;
        m_dragOrigin = event->globalPos();
    }
}

void MdDockWidget::mouseMoveEvent(QMouseEvent *event)
{
    QWidget::mouseMoveEvent(event);
    if (m_dragging) {
        // The manager listens for the drag; the widget itself stays put.
        const QPoint delta = event->globalPos() - m_dragOrigin;
        Q_UNUSED(delta);
    }
}

void MdDockWidget::mouseReleaseEvent(QMouseEvent *event)
{
    QWidget::mouseReleaseEvent(event);
    m_dragging = false;
}

void MdDockWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutChildren();
}

void MdDockWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange
        || event->type() == QEvent::FontChange) {
        layoutChildren();
        update();
    }
}

void MdDockWidget::onThemeChanged()
{
    update();
}

} // namespace md
