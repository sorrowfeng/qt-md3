#include "MdBackTop.h"

#include "widgets/MdFab.h"

#include <QtGui/QHideEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QScrollBar>

namespace md {

/// FAB small: 40 px container (`md.comp.fab.small.container.size`).
static constexpr int kFabSize = 40;

MdBackTop::MdBackTop(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdBackTop::~MdBackTop()
{
    // Disconnect children / signal links before members die.
    unhookScrollArea();
    if (m_fab) {
        disconnect(m_fab, nullptr, this, nullptr);
    }
}

void MdBackTop::init()
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setAccessibleName(QStringLiteral("back to top"));
    setAttribute(Qt::WA_TransparentForMouseEvents, true);

    // A real Stage 1 FAB so the ripple / state layer / focus ring cannot drift.
    m_fab = new MdFab(QStringLiteral("arrow_upward"), this);
    m_fab->setFabSize(FabSize::Small);
    m_fab->setVariant(FabVariant::Surface);
    m_fab->setAccessibleName(QStringLiteral("back to top"));
    // The host is transparent for mouse; the FAB itself takes the hits.
    m_fab->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    connect(m_fab, &MdFab::clicked, this, &MdBackTop::onFabClicked);
    m_fab->hide();

    layoutFab();
}

QString MdBackTop::iconName() const
{
    return m_fab ? m_fab->iconName() : QString();
}

void MdBackTop::setIconName(const QString &name)
{
    if (m_fab) {
        m_fab->setIconName(name);
    }
    emit iconNameChanged(name);
}

void MdBackTop::setVisibilityThreshold(int pixels)
{
    const int clamped = qMax(0, pixels);
    if (m_threshold == clamped) {
        return;
    }
    m_threshold = clamped;
    refresh();
    emit visibilityThresholdChanged(m_threshold);
}

bool MdBackTop::isButtonVisible() const
{
    return m_visible;
}

void MdBackTop::setScrollArea(QAbstractScrollArea *area)
{
    if (m_scrollArea == area) {
        return;
    }
    unhookScrollArea();
    m_scrollArea = area;
    hookScrollArea();
    refresh();
    emit scrollAreaChanged(m_scrollArea);
}

QSize MdBackTop::sizeHint() const
{
    return QSize(kFabSize, kFabSize);
}

QSize MdBackTop::minimumSizeHint() const
{
    return sizeHint();
}

void MdBackTop::scrollToTop()
{
    if (m_scrollArea && m_scrollArea->verticalScrollBar()) {
        m_scrollArea->verticalScrollBar()->setValue(0);
    }
    refresh();
    emit clicked();
}

void MdBackTop::refresh()
{
    const int value = (m_scrollArea && m_scrollArea->verticalScrollBar())
            ? m_scrollArea->verticalScrollBar()->value()
            : 0;
    setButtonVisible(value >= m_threshold);
}

void MdBackTop::layoutFab()
{
    if (m_fab) {
        m_fab->setGeometry(rect());
    }
}

void MdBackTop::setButtonVisible(bool visible)
{
    if (m_visible == visible) {
        return;
    }
    m_visible = visible;
    if (m_fab) {
        m_fab->setVisible(visible);
    }
    // Host stays "visible" so geometry works; the FAB child carries the
    // actual show/hide (the FAB small variant is the visual).
    emit buttonVisibleChanged(m_visible);
}

void MdBackTop::hookScrollArea()
{
    if (!m_scrollArea) {
        return;
    }
    m_scrollConn = connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
                           this, &MdBackTop::onScrollChanged);
}

void MdBackTop::unhookScrollArea()
{
    if (m_scrollConn) {
        disconnect(m_scrollConn);
        m_scrollConn = QMetaObject::Connection();
    }
    m_scrollArea = nullptr;
}

void MdBackTop::onFabClicked()
{
    scrollToTop();
}

void MdBackTop::onScrollChanged()
{
    refresh();
}

void MdBackTop::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutFab();
}

void MdBackTop::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refresh();
}

void MdBackTop::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange) {
        layoutFab();
    }
}

void MdBackTop::onThemeChanged()
{
    update();
}

} // namespace md
