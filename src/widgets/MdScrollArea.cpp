#include "MdScrollArea.h"

#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"
#include "styles/MdScrollBarStyle.h"

#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QWidget>

namespace md {

MdScrollArea::MdScrollArea(QWidget *parent)
    : QScrollArea(parent)
{
    init();
}

MdScrollArea::~MdScrollArea() = default;

void MdScrollArea::init()
{
    // MD3 surfaces have no inset frame; the content sits on the surface.
    setFrameShape(QFrame::NoFrame);
    setFocusPolicy(Qt::StrongFocus);
    setAttribute(Qt::WA_OpaquePaintEvent, false);

    // Compose MdScrollBar in place of the stock bars. QScrollArea installs
    // the replacement bars through setVerticalScrollBar().
    m_vBar = new MdScrollBar(Qt::Vertical, this);
    m_hBar = new MdScrollBar(Qt::Horizontal, this);
    setVerticalScrollBar(m_vBar);
    setHorizontalScrollBar(m_hBar);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    connect(m_vBar, &MdScrollBar::liftedChanged, this, &MdScrollArea::onBarLiftedChanged);
    connect(m_hBar, &MdScrollBar::liftedChanged, this, &MdScrollArea::onBarLiftedChanged);

    // The viewport is a child widget: its Paint event is where the surface
    // tone belongs (the scroll area's own paint only covers the frame area).
    if (QWidget *vp = viewport()) {
        vp->installEventFilter(this);
        vp->setAttribute(Qt::WA_OpaquePaintEvent, false);
    }

    MdStyleBase::connectThemeUpdate(this, &MdScrollArea::onThemeChanged);

    // Make sure the first paint is never the platform scrollbar.
    MdScrollBarStyle::shared();
    applyChrome();
}

void MdScrollArea::applyChrome()
{
    if (m_vBar != nullptr) {
        m_vBar->setThickness(m_barThickness);
        m_vBar->setVisibleMode(m_barVisibleMode);
    }
    if (m_hBar != nullptr) {
        m_hBar->setThickness(m_barThickness);
        m_hBar->setVisibleMode(m_barVisibleMode);
    }
    update();
}

int MdScrollArea::barThickness() const
{
    return m_barThickness;
}

void MdScrollArea::setBarThickness(int thickness)
{
    const int clamped = qBound(4, thickness, 24);
    if (m_barThickness == clamped) {
        return;
    }
    m_barThickness = clamped;
    applyChrome();
    emit barThicknessChanged(m_barThickness);
}

MdScrollBarVisibility MdScrollArea::barVisibleMode() const
{
    return m_barVisibleMode;
}

void MdScrollArea::setBarVisibleMode(MdScrollBarVisibility mode)
{
    if (m_barVisibleMode == mode) {
        return;
    }
    m_barVisibleMode = mode;
    applyChrome();
    emit barVisibleModeChanged(m_barVisibleMode);
}

bool MdScrollArea::isAnyBarLifted() const
{
    return m_anyLifted;
}

void MdScrollArea::onBarLiftedChanged()
{
    syncLifted();
}

void MdScrollArea::syncLifted()
{
    const bool lifted = (m_vBar != nullptr && m_vBar->isLifted())
                        || (m_hBar != nullptr && m_hBar->isLifted());
    if (m_anyLifted == lifted) {
        return;
    }
    m_anyLifted = lifted;
    emit anyBarLiftedChanged(m_anyLifted);
}

bool MdScrollArea::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == viewport() && event->type() == QEvent::Paint) {
        // Fill the surface *under* the viewport's own painting so a content
        // widget smaller than the viewport still shows the surface tone.
        QPainter painter(viewport());
        painter.fillRect(viewport()->rect(), MdTheme::instance().color(ColorRole::Surface));
    }
    return QScrollArea::eventFilter(watched, event);
}

void MdScrollArea::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
}

void MdScrollArea::showEvent(QShowEvent *event)
{
    QScrollArea::showEvent(event);
    applyChrome();
}

void MdScrollArea::paintEvent(QPaintEvent *event)
{
    QScrollArea::paintEvent(event);
}

void MdScrollArea::onThemeChanged()
{
    if (viewport() != nullptr) {
        viewport()->update();
    }
    update();
}

} // namespace md
