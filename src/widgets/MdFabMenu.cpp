#include "MdFabMenu.h"

#include "MdFabMenuItem.h"

#include "core/MdFabTokens.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "widgets/MdFab.h"

#include <cmath>
#include <QtWidgets/QLayout>

namespace md {

namespace {

// The stagger and slide come from the Compose M3 Expressive menu motion
// convention — the token export publishes no motion rows for this family
// (recorded in docs/porting-todo.md). 40 ms between items, a 24 px settle
// from the close button's anchor downward, one shared spatial spring.
constexpr qreal kItemStaggerSeconds = 0.04;
constexpr qreal kItemSlideDistance = 24.0;
constexpr qreal kMaxItems = 6.0;

// Close glyph for the close button; the spec page shows the standard close
// icon and publishes no name token.
const char *kCloseIconName = "close";

FabVariant anchorVariantFor(FabMenuVariant variant)
{
    switch (variant) {
    case FabMenuVariant::Secondary: return FabVariant::Secondary;
    case FabMenuVariant::Tertiary: return FabVariant::Tertiary;
    case FabMenuVariant::Primary:
    case FabMenuVariant::Count: break;
    }
    return FabVariant::Primary;
}

} // namespace

MdFabMenu::MdFabMenu(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdFabMenu::MdFabMenu(const QString &anchorIconName, QWidget *parent)
    : QWidget(parent)
{
    init();
    setAnchorIconName(anchorIconName);
}

MdFabMenu::~MdFabMenu() = default;

void MdFabMenu::init()
{
    // The anchor FAB — a normal MdFab in the menu's pure colour.
    m_anchor = new MdFab(QString(), this);
    m_anchor->setVariant(anchorVariantFor(m_variant));
    m_anchor->setFabSize(FabSize::Medium);
    m_anchor->show();
    connect(m_anchor, &QAbstractButton::clicked, this, &MdFabMenu::onAnchorClicked);

    // The close button — same place, same size ladder, close glyph.
    m_closeButton = new MdFabMenuItem(FabMenuElement::CloseButton, m_variant,
                                      QString::fromUtf8(kCloseIconName), QString(), this);
    connect(m_closeButton, &QAbstractButton::clicked, this, &MdFabMenu::onCloseClicked);
    m_closeButton->setReveal(0.0);
    m_closeButton->hide();

    m_revealAnimation.setDuration(MdMotion::spring(MotionSpring::SpatialDefault)
                                      .settlingDurationMs());
    connect(&m_revealAnimation, &QVariantAnimation::valueChanged, this,
            &MdFabMenu::onRevealChanged);
}

void MdFabMenu::setVariant(FabMenuVariant variant)
{
    if (m_variant == variant) {
        return;
    }
    m_variant = variant;
    m_anchor->setVariant(anchorVariantFor(variant));
    m_closeButton->setVariant(variant);
    for (MdFabMenuItem *item : m_items) {
        item->setVariant(variant);
    }
    relayout();
    emit variantChanged(m_variant);
}

QString MdFabMenu::anchorIconName() const
{
    return m_anchor->iconName();
}

void MdFabMenu::setAnchorIconName(const QString &iconName)
{
    if (m_anchor->iconName() == iconName) {
        return;
    }
    m_anchor->setIconName(iconName);
    relayout();
    emit anchorIconNameChanged(iconName);
}

void MdFabMenu::addItem(const QString &iconName, const QString &label)
{
    auto *item = new MdFabMenuItem(FabMenuElement::ListItem, m_variant, iconName, label, this);
    connect(item, &QAbstractButton::clicked, this, [this, item] {
        const int index = m_items.indexOf(item);
        emit itemActivated(index, item->text());
    });
    m_items.append(item);
    item->setReveal(m_expanded ? 1.0 : 0.0);
    item->setVisible(m_expanded);
    relayout();
}

int MdFabMenu::itemCount() const
{
    return m_items.size();
}

MdFabMenuItem *MdFabMenu::itemAt(int index) const
{
    return (index >= 0 && index < m_items.size()) ? m_items.at(index) : nullptr;
}

void MdFabMenu::clearItems()
{
    for (MdFabMenuItem *item : m_items) {
        item->deleteLater();
    }
    m_items.clear();
    relayout();
}

qreal MdFabMenu::expandedContentHeight() const
{
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(m_variant);
    qreal height = tokens.closeButton.containerHeight;
    if (!m_items.isEmpty()) {
        height += tokens.closeButtonBetweenSpace;
        height += m_items.size() * tokens.listItem.containerHeight;
        height += (m_items.size() - 1) * tokens.menuItemBetweenSpace;
    }
    return height;
}

qreal MdFabMenu::menuWidth() const
{
    // Children are right-aligned inside a shared focus-indicator margin; the
    // width is the widest child *hint* (which already carries that margin on
    // both sides) — no extra room needed beyond one margin on each edge.
    qreal width = 0.0;
    width = qMax<qreal>(width, m_anchor->sizeHint().width());
    width = qMax<qreal>(width, m_closeButton->sizeHint().width());
    for (const MdFabMenuItem *item : m_items) {
        width = qMax<qreal>(width, item->sizeHint().width());
    }
    return width;
}

QSize MdFabMenu::sizeHint() const
{
    // Always the expanded size, so a layout that reserved room for the open
    // menu keeps its geometry while the menu animates. The vertical ledger:
    // one focus margin at the top, the close button (its own hint carries
    // its margins), the close-to-items gap, the items with their gaps, and
    // one focus margin at the bottom.
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(m_variant);
    const qreal inset = 7.5;
    const qreal height = inset + m_closeButton->sizeHint().height()
                         + (m_items.isEmpty()
                                ? 0.0
                                : tokens.closeButtonBetweenSpace
                                      + m_items.size() * tokens.listItem.containerHeight
                                      + (m_items.size() - 1) * tokens.menuItemBetweenSpace)
                         + inset;
    return QSize(int(std::ceil(menuWidth())), int(std::ceil(height)));
}

QSize MdFabMenu::minimumSizeHint() const
{
    return sizeHint();
}

bool MdFabMenu::isAnimating() const
{
    return m_revealAnimation.state() == QAbstractAnimation::Running;
}

void MdFabMenu::setExpanded(bool expanded)
{
    if (m_expanded == expanded) {
        return;
    }
    m_expanded = expanded;
    if (m_items.isEmpty()) {
        // Nothing to stagger: the close button still crossfades with the FAB.
        m_reveal = expanded ? 1.0 : 0.0;
        m_closeButton->setReveal(m_reveal);
        m_anchor->setVisible(!expanded);
        relayout();
        emit expandedChanged(m_expanded);
        return;
    }

    m_revealAnimation.stop();
    m_revealAnimation.setStartValue(m_reveal);
    m_revealAnimation.setEndValue(expanded ? 1.0 : 0.0);
    m_revealAnimation.start();
    emit expandedChanged(m_expanded);
}

void MdFabMenu::onRevealChanged(const QVariant &value)
{
    m_reveal = value.toReal();
    relayout();
}

void MdFabMenu::onAnchorClicked()
{
    setExpanded(true);
}

void MdFabMenu::onCloseClicked()
{
    setExpanded(false);
}

void MdFabMenu::relayout()
{
    const MdFabMenuTokens tokens = MdFabMenuTokens::resolve(m_variant);
    const qreal inset = 7.5; // the shared focus-indicator margin
    const qreal width = menuWidth();

    // The close button shares the FAB's top trailing corner as its anchor:
    // both sit at the top, trailing-aligned, exactly overlapping. Children
    // are placed by *hint* (which carries the focus margin), so their
    // containers land inside the shared margin.
    const qreal right = width - inset;
    const QSize anchorHint = m_anchor->sizeHint();
    const QSize closeHint = m_closeButton->sizeHint();
    m_anchor->setGeometry(int(right - anchorHint.width()), int(inset), anchorHint.width(),
                          anchorHint.height());
    m_closeButton->setGeometry(int(right - closeHint.width()), int(inset), closeHint.width(),
                               closeHint.height());

    // Items trail below the close button, right-aligned, staggered by their
    // own reveal: each item fades in while settling `kItemSlideDistance`
    // downward from the close button's anchor. Reveal timing: item i starts
    // at i * stagger and runs for the remainder of the shared progress.
    const qreal revealSpan = qMax<qreal>(1.0, 1.0 + (m_items.size() - 1) * kItemStaggerSeconds);
    const qreal settleSeconds = m_revealAnimation.duration() / 1000.0;
    // The first item's container top: close button bottom + gap. Item
    // widgets are placed by hint too, so their widget origin sits one focus
    // margin above their container top.
    qreal itemContainerTop = inset + closeHint.height() + tokens.closeButtonBetweenSpace;
    for (int i = 0; i < m_items.size(); ++i) {
        MdFabMenuItem *item = m_items.at(i);
        const QSize hint = item->sizeHint();
        const qreal itemLeft = right - hint.width();
        const qreal restY = itemContainerTop - inset;

        const qreal localStart = i * kItemStaggerSeconds / revealSpan;
        const qreal localSpan = 1.0 / revealSpan;
        const qreal local =
            qBound<qreal>(0.0, (m_reveal - localStart) / localSpan, 1.0);
        // The slide runs on the shared spatial spring over the animation's
        // own timeline; the fade tracks the same progress.
        const qreal eased =
            qBound<qreal>(0.0,
                          MdMotion::spring(MotionSpring::SpatialDefault).valueAt(
                              local * settleSeconds),
                          1.0);
        const qreal offsetY = (1.0 - eased) * kItemSlideDistance;

        item->setReveal(eased);
        item->setGeometry(int(itemLeft), int(restY + offsetY), hint.width(), hint.height());
        itemContainerTop += tokens.listItem.containerHeight + tokens.menuItemBetweenSpace;
    }

    // Crossfade the anchor against the close button.
    m_anchor->setVisible(m_reveal < 0.5);
    m_closeButton->setReveal(m_reveal);
    m_closeButton->setVisible(m_reveal > 0.0);
}

void MdFabMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdFabMenu::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    relayout();
}

} // namespace md
