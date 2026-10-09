// Gallery page 31: Navigation rail — `md.comp.navigation-rail.*` (the
// baseline family) and `md.comp.nav-rail-collapsed.*` / `nav-rail-expanded.*`
// / `nav-rail.*` + `nav-rail-item*` (the flexible one).
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. A rail keeps its own token
// width and fills the band's height — the opposite of the bar page's slots —
// which the NavSlot's fill flags express.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * expanded is a **state**, not a variant: the flexible rail has one
//     container with two widths (96 collapsed, content-driven 220–360
//     expanded), and expanding flips every item from the Top pill to the
//     Start pill with a label — the same switch Compose makes by handing its
//     shared item a different style set.
//
//   * the items are the *bar's* item: Compose's `WideNavigationRailItem` and
//     `ShortNavigationBarItem` wrap the same `NavigationItem` composable, so
//     this page's rails are full of `MdNavigationBarItem`s.

#include "GalleryPages.h"

#include "widgets/MdFab.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdNavigationRail.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

#include <cmath>

namespace gallery {

namespace {

/// One banked widget plus the band it asked for, and which sides it fills.
struct NavSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
    bool fillWidth = false;
    bool fillHeight = true;
};

constexpr qreal kRailBandHeight = 420.0;

} // namespace

class NavigationRailPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit NavigationRailPage(QWidget *parent = nullptr);
    ~NavigationRailPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void layoutChildren();
    void placeChildren();

    md::MdNavigationRail *rail(const QString &id);
    md::MdNavigationBarItem *item(const QString &id, const QString &label, const QString &icon);
    md::MdFab *fab(const QString &id, const QString &icon);

    QHash<QString, md::MdNavigationRail *> m_rails;
    QHash<QString, md::MdNavigationBarItem *> m_items;
    QHash<QString, md::MdFab *> m_fabs;
    QVector<NavSlot> m_slots;
};

NavigationRailPage::NavigationRailPage(QWidget *parent)
    : GalleryPage(parent)
{
}

NavigationRailPage::~NavigationRailPage() = default;

QString NavigationRailPage::title() const
{
    return L("导航栏（侧栏）", "Navigation rail");
}

QString NavigationRailPage::slug() const
{
    return QStringLiteral("navigation-rail");
}

QString NavigationRailPage::subtitle() const
{
    return L(
        "侧边导航栏：平板与桌面屏幕上的主目的地纵列。基线家族 "
        "`md.comp.navigation-rail.*` 是 80dp 宽、56x32 药丸的一代，项距 4 / 项内衬 4 / "
        "标题后 8 三行全部是 Compose 的硬编码；flexible 家族（`nav-rail-collapsed` / "
        "`-expanded` / `nav-rail` / `nav-rail-item*`）是 Expressive 的一代：折叠 96dp、"
        "展开 220–360dp **随内容**、modal 三行是抽屉的替代品。规范原文：\"the expanded "
        "navigation rail replaces the navigation drawer\"。项是导航栏的共享 item——"
        "Compose 的 `WideNavigationRailItem` 与 `ShortNavigationBarItem` 本就包着同一个 "
        "`NavigationItem`，折叠是 Top 药丸、展开是带文字的 Start 药丸，展开是**状态**而"
        "不是变体。",
        "The navigation rail: a column of primary destinations for tablet and desktop "
        "screens. The baseline `md.comp.navigation-rail.*` is the 80dp generation — a "
        "56x32 pill, and Compose hard-codes all three spacings (4 between items, 4 of "
        "item padding, 8 after the header). The flexible family (`nav-rail-collapsed` / "
        "`-expanded` / `nav-rail` / `nav-rail-item*`) is the Expressive one: 96dp "
        "collapsed, 220–360dp expanded **driven by the content**, and the modal rows are "
        "the drawer's replacement — the spec's own words are \"the expanded navigation "
        "rail replaces the navigation drawer\". The items are the bar's shared item: "
        "Compose's `WideNavigationRailItem` and `ShortNavigationBarItem` wrap the same "
        "`NavigationItem`, a Top pill collapsed and a Start pill with a label expanded — "
        "expanded is a *state*, not a variant.");
}

md::MdNavigationRail *NavigationRailPage::rail(const QString &id)
{
    const auto found = m_rails.constFind(id);
    if (found != m_rails.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationRail(this);
    created->hide();
    m_rails.insert(id, created);
    return created;
}

md::MdNavigationBarItem *NavigationRailPage::item(const QString &id, const QString &label,
                                                  const QString &icon)
{
    const auto found = m_items.constFind(id);
    if (found != m_items.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationBarItem(label, icon, this);
    created->hide();
    m_items.insert(id, created);
    return created;
}

md::MdFab *NavigationRailPage::fab(const QString &id, const QString &icon)
{
    const auto found = m_fabs.constFind(id);
    if (found != m_fabs.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFab(icon, this);
    created->hide();
    m_fabs.insert(id, created);
    return created;
}

void NavigationRailPage::build(GalleryContext &context)
{
    m_slots.clear();

    struct Destination
    {
        const char *id;
        const char *zh;
        const char *en;
        const char *icon;
    };
    const QVector<Destination> destinations = {
        {"inbox", "收件", "Inbox", "mail"},
        {"drafts", "草稿", "Drafts", "edit"},
        {"star", "星标", "Starred", "star"},
        {"trash", "已删除", "Trash", "delete"},
    };
    auto addDestinationColumn = [&](md::MdNavigationRail *column, const QString &prefix) {
        column->clearItems();
        for (const Destination &destination : destinations) {
            column->addItem(
                item(prefix + QStringLiteral("-") + QString::fromUtf8(destination.id),
                     L(destination.zh, destination.en), destination.icon));
        }
    };

    // --- the two families, collapsed ------------------------------------------
    context.section(L("两个家族，折叠态", "The two families, collapsed"));
    context.paragraph(L(
        "左边是基线家族：80dp 宽、56x32 药丸、无文字——它的药丸有 "
        "`no-label-active-indicator-height: 56px` 这一行，所以无文字的药丸是 **56x56 的"
        "方块**而不是 56x32 的横条。右边是 flexible 家族折叠态：96dp 宽、顶部 44dp 内衬、"
        "项距 4。两条都带一个 FAB 头部（rail 的 header 是外来控件，内容归调用方）。基线的"
        "三行间距——项距 4、项内衬 4、标题后 8——导出里一行都没有，全是 Compose 的 "
        "`NavigationRail.kt` 硬编码，已按行为数字记录。",
        "On the left, the baseline family: 80dp wide, a 56x32 pill, no labels — and its "
        "no-label pill has a published row (`no-label-active-indicator-height: 56px`), so "
        "a label-less pill is a **56 x 56 square**, not a 56x32 strip. On the right, the "
        "flexible family collapsed: 96dp wide, 44dp of top inset, 4 between items. Both "
        "carry a FAB header — a rail's header is a foreign widget whose content belongs "
        "to the caller. The baseline's three spacings — 4 between items, 4 of item "
        "padding, 8 after the header — have no rows in the export at all; they are "
        "Compose's `NavigationRail.kt` hard-codes, recorded as the behaviour's numbers."));
    {
        struct FamilyRow
        {
            const char *id;
            md::MdNavigationRailVariant variant;
            qreal x;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<FamilyRow> rows = {
            {"rail-baseline", md::MdNavigationRailVariant::Baseline, 0.0,
             "Baseline · 80dp · 药丸 56x56（无文字方块）", "Baseline · 80dp · pill 56x56 (label-less square)"},
            {"rail-flexible", md::MdNavigationRailVariant::Flexible, 200.0,
             "Flexible 折叠 · 96dp · 顶部内衬 44", "Flexible collapsed · 96dp · top inset 44"},
        };
        // One shared band: the two rails sit side by side inside it.
        const QRectF band = context.band(kRailBandHeight);
        for (const FamilyRow &row : rows) {
            md::MdNavigationRail *column = rail(QString::fromUtf8(row.id));
            column->setVariant(row.variant);
            column->setModal(false);
            column->setExpanded(false);
            column->setHeader(fab(QString::fromUtf8(row.id) + QStringLiteral("-fab"),
                                  QStringLiteral("add")));
            addDestinationColumn(column, QString::fromUtf8(row.id));
            column->setCurrentIndex(0);
            m_slots.append(NavSlot{column, band.translated(row.x, 0.0)});
        }
        for (const FamilyRow &row : rows) {
            context.detail(L(row.noteZh, row.noteEn));
        }
        context.space(16.0);
    }

    // --- expanded ---------------------------------------------------------------
    context.section(L("展开：宽度随内容，项随状态变形",
                      "Expanded: the width follows the content, the items change shape"));
    context.paragraph(L(
        "同一个 flexible 栏的展开态：宽度不再是 token 的 96，而是**内容的**——最宽项的药丸"
        "加上 20dp 的尾部余量，再夹进发布出的 220–360 上下界。展开同时翻转每一项：Top 药丸"
        "换成带文字的 Start 药丸（alwaysShowLabel 打开、文字用 label-large、图标文字间是 8 "
        "而折叠态是 4），项距从折叠的 4 换成展开行自己的 between-item-space 0。这是"
        "**状态**翻转而不是插值——Compose 把展开交给一个布尔，不给中间形态。",
        "The same flexible rail expanded: the width is no longer the token's 96 but the "
        "**content's** — the widest item's pill plus 20dp of trailing room, clamped into "
        "the published 220–360 bounds. Expanding also flips every item: the Top pill "
        "becomes a Start pill with its label (alwaysShowLabel on, the label label-large, "
        "8 between icon and label where the collapsed one reads 4), and the gap between "
        "items changes from the collapsed 4 to the expanded family's own between-item-"
        "space, 0. The flip is a **state** switch, not an interpolation — Compose hands "
        "it a boolean and draws no in-between."));
    {
        md::MdNavigationRail *column = rail(QStringLiteral("rail-expanded"));
        column->setVariant(md::MdNavigationRailVariant::Flexible);
        column->setModal(false);
        column->setHeader(fab(QStringLiteral("rail-expanded-fab"), QStringLiteral("add")));
        addDestinationColumn(column, QStringLiteral("rail-expanded"));
        column->setCurrentIndex(0);
        column->setExpanded(true);
        const QRectF band = context.band(kRailBandHeight);
        m_slots.append(NavSlot{column, band.translated(120.0, 0.0)});
        context.detail(L("Flexible 展开 · 220–360 之间随内容", "Flexible expanded · content-driven between 220 and 360"));
        context.space(16.0);
    }

    // --- modal -------------------------------------------------------------------
    context.section(L("Modal：抽屉的替代品", "Modal: the drawer's replacement"));
    context.paragraph(L(
        "modal 三行属于 flexible 家族：surface-container 的容器、level2、corner-large、"
        "从 scrim 上滑入，宽度弹簧换用 FastSpatial。规范把它定位成 navigation drawer 的"
        "Expressive 替代——所以下一页的抽屉会再遇到一次这套行。scrim 本身是宿主的浮层："
        "子控件画不到父级上面，颜色和不透明度在 token 上承载（`scrimColor()` / "
        "`scrimOpacity()`），已记进 docs/porting-todo.md。",
        "The modal rows belong to the flexible family: a surface-container container at "
        "level2 with a corner-large shape, arriving over a scrim, its width spring "
        "switched to FastSpatial. The spec positions it as the navigation drawer's "
        "Expressive replacement — the drawer page meets these rows again. The scrim "
        "itself is the host's overlay: a child widget cannot paint over its parent, so "
        "the colour and opacity are carried on the tokens (`scrimColor()` / "
        "`scrimOpacity()`), recorded in docs/porting-todo.md."));
    {
        md::MdNavigationRail *column = rail(QStringLiteral("rail-modal"));
        column->setVariant(md::MdNavigationRailVariant::Flexible);
        column->setModal(true);
        column->setHeader(nullptr);
        addDestinationColumn(column, QStringLiteral("rail-modal"));
        column->setCurrentIndex(0);
        column->setExpanded(true);
        const QRectF band = context.band(kRailBandHeight);
        m_slots.append(NavSlot{column, band.translated(120.0, 0.0)});
        context.detail(L("Flexible modal · surface-container / level2 / corner-large",
                         "Flexible modal · surface-container / level2 / corner-large"));
        context.space(8.0);
    }

    // --- what this page is pinned to ----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Navigation 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Navigation; moving one turns it "
        "red."));
    context.detail(L("baseline: 80 宽 / 药丸 56x32（无文字 56x56）/ 项距 4 · 项内衬 4 · 标题后 8"
                     "（三行全是 Compose 硬编码）",
                     "baseline: 80 wide / pill 56x32 (56x56 label-less) / 4 between items / 4 "
                     "of item padding / 8 after the header (all three Compose hard-codes)"));
    context.detail(L("flexible 折叠: 96 宽（narrow-container-width 80 已发布但 Compose 不读）· "
                     "顶部 44 · 项距 4",
                     "flexible collapsed: 96 wide (narrow-container-width 80 is published and "
                     "unread) · top inset 44 · 4 between items"));
    context.detail(L("flexible 展开: 220–360 随内容（最宽项 + 20 尾部余量）· 项距 0 · Start 药丸"
                     "高 56 · 图标文字间 8 · 文字 label-large",
                     "flexible expanded: 220-360 content-driven (widest item + 20 trailing) / "
                     "0 between items / Start pill 56 tall / 8 between icon and label / "
                     "label-large"));
    context.detail(L("展开是状态不是变体；baseline 不发布展开行，setExpanded(true) 被拒绝",
                     "expanded is a state, not a variant; the baseline publishes no expanded "
                     "rows and refuses setExpanded(true)"));
    context.detail(L("modal: surface-container / level2 / corner-large / FastSpatial——抽屉的替代品",
                     "modal: surface-container / level2 / corner-large / FastSpatial — the "
                     "drawer's replacement"));
    context.space(8.0);
}

void NavigationRailPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const NavSlot &slot : m_slots) {
        const QSize hint = slot.widget->sizeHint();
        const QSize size(slot.fillWidth ? int(std::lround(slot.rect.width())) : hint.width(),
                         slot.fillHeight ? int(std::lround(slot.rect.height())) : hint.height());
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(), size);
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void NavigationRailPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void NavigationRailPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void NavigationRailPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void NavigationRailPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createNavigationRailPage()
{
    return new NavigationRailPage;
}

} // namespace gallery

#include "GalleryPages31.moc"
