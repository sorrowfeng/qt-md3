#include "MdRibbon.h"

#include "MdTab.h"
#include "MdTabs.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

constexpr qreal kGroupGap = 12.0;
constexpr qreal kBandPad = 8.0;
constexpr qreal kDividerWidth = 1.0;

} // namespace

MdRibbon::MdRibbon(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdRibbon::~MdRibbon() = default;

void MdRibbon::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("ribbon"));

    m_tabStrip = new MdTabs(this);
    // The ribbon's tab strip is the secondary family: square indicator,
    // content in on-surface (the toolbar+tabs composition appendix D names).
    m_tabStrip->setVariant(MdTabsVariant::Secondary);
    m_tabStrip->setObjectName(QStringLiteral("md-ribbon-tabs"));
    connect(m_tabStrip, &MdTabs::currentChanged, this, &MdRibbon::onTabChanged);

    MdStyleBase::connectThemeUpdate(this, &MdRibbon::onThemeChanged);
}

void MdRibbon::setTabs(const QList<MdRibbonTab> &tabs)
{
    m_tabs = tabs;
    if (m_currentIndex >= int(m_tabs.size())) {
        m_currentIndex = int(m_tabs.size()) - 1;
        emit currentIndexChanged(m_currentIndex);
    }
    rebuildTabStrip();
    updateGeometry();
    update();
    emit tabsChanged(m_tabs);
}

void MdRibbon::addTab(const MdRibbonTab &tab)
{
    m_tabs.append(tab);
    rebuildTabStrip();
    if (m_currentIndex < 0) {
        m_currentIndex = 0;
        emit currentIndexChanged(m_currentIndex);
    }
    updateGeometry();
    update();
    emit tabsChanged(m_tabs);
}

void MdRibbon::clearTabs()
{
    if (m_tabs.isEmpty()) {
        return;
    }
    m_tabs.clear();
    m_currentIndex = -1;
    rebuildTabStrip();
    updateGeometry();
    update();
    emit tabsChanged(m_tabs);
}

void MdRibbon::setCurrentIndex(int index)
{
    const int clamped = index < -1 ? -1 : index;
    if (m_currentIndex == clamped) {
        return;
    }
    m_currentIndex = clamped;
    if (m_tabStrip) {
        m_tabStrip->setCurrentIndex(clamped);
    }
    layoutBand();
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdRibbon::setBandHeight(int height)
{
    const int clamped = qBound(48, height, 160);
    if (m_bandHeight == clamped) {
        return;
    }
    m_bandHeight = clamped;
    updateGeometry();
    layoutBand();
    update();
    emit bandHeightChanged(m_bandHeight);
}

MdRibbonTab MdRibbon::tabAt(int index) const
{
    if (index < 0 || index >= int(m_tabs.size())) {
        return MdRibbonTab();
    }
    return m_tabs.at(index);
}

void MdRibbon::addCommand(QWidget *command, int groupIndex)
{
    if (!command) {
        return;
    }
    command->setParent(this);
    while (m_commands.size() <= groupIndex) {
        m_commands.append(QList<QWidget *>());
    }
    m_commands[groupIndex].append(command);
    layoutBand();
}

void MdRibbon::clearCommands()
{
    for (const QList<QWidget *> &group : m_commands) {
        for (QWidget *command : group) {
            if (command->parent() == this) {
                command->setParent(nullptr);
            }
        }
    }
    m_commands.clear();
}

QColor MdRibbon::resolvedBandColor() const
{
    return MdTheme::instance().color(ColorRole::SurfaceContainer);
}

QColor MdRibbon::resolvedDividerColor() const
{
    return MdTheme::instance().color(ColorRole::OutlineVariant);
}

qreal MdRibbon::tabStripHeight() const
{
    return m_tabStrip ? m_tabStrip->sizeHint().height() : 48.0;
}

QSize MdRibbon::sizeHint() const
{
    return QSize(480, int(tabStripHeight() + m_bandHeight));
}

QSize MdRibbon::minimumSizeHint() const
{
    return QSize(200, int(tabStripHeight() + 48));
}

void MdRibbon::rebuildTabStrip()
{
    if (!m_tabStrip) {
        return;
    }
    m_tabStrip->clearTabs();
    for (const MdRibbonTab &tab : m_tabs) {
        auto *item = new MdTab(tab.title);
        m_tabStrip->addTab(item);
    }
    if (m_currentIndex >= 0) {
        m_tabStrip->setCurrentIndex(m_currentIndex);
    }
}

void MdRibbon::layoutBand()
{
    if (!m_tabStrip) {
        return;
    }
    const qreal stripH = tabStripHeight();
    m_tabStrip->setGeometry(QRectF(0, 0, width(), stripH).toRect());

    const qreal bandY = stripH;
    qreal x = kBandPad;
    const qreal bandH = m_bandHeight;
    for (int g = 0; g < m_commands.size(); ++g) {
        const QList<QWidget *> &group = m_commands.at(g);
        if (group.isEmpty()) {
            continue;
        }
        if (g > 0) {
            x += kGroupGap;
        }
        for (QWidget *command : group) {
            const QSize hint = command->sizeHint();
            command->setGeometry(qRound(x),
                                 qRound(bandY + (bandH - hint.height()) * 0.5), hint.width(),
                                 hint.height());
            command->show();
            x += hint.width() + 4;
        }
        x += kGroupGap;
    }
}

void MdRibbon::onTabChanged(int index)
{
    if (m_currentIndex == index) {
        return;
    }
    m_currentIndex = index;
    layoutBand();
    update();
    emit currentIndexChanged(m_currentIndex);
}

void MdRibbon::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal stripH = tabStripHeight();
    const QRectF band(0, stripH, width(), m_bandHeight);
    const QPainterPath path = MdShape::roundedRect(band, ShapeCorner::None);
    painter.fillPath(path, resolvedBandColor());

    // Group dividers — outline-variant hairlines between command groups.
    qreal x = kBandPad;
    for (int g = 0; g < m_commands.size(); ++g) {
        const QList<QWidget *> &group = m_commands.at(g);
        if (group.isEmpty()) {
            continue;
        }
        if (g > 0) {
            painter.fillRect(QRectF(x - kGroupGap * 0.5, band.top() + 8, kDividerWidth,
                                    band.height() - 16),
                             resolvedDividerColor());
        }
        for (QWidget *command : group) {
            const QSize hint = command->sizeHint();
            x += hint.width() + 4;
        }
        x += kGroupGap * 2;
    }
}

void MdRibbon::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutBand();
}

void MdRibbon::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        layoutBand();
        update();
    }
    QWidget::changeEvent(event);
}

void MdRibbon::onThemeChanged()
{
    update();
}

} // namespace md
