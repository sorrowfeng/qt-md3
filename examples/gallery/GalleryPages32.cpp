// Gallery page 32: Navigation drawer — `md.comp.navigation-drawer.*`, the
// family's one file with two container row groups (modal / standard).
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. A drawer keeps its 360 px
// token width and fills the band's height — the rail page's slot shape.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * the drawer's item is **not** the shared expressive item, and that is
//     the source's own decision: the pill IS the item — a full-width row
//     whose container colour is the selected state — so Compose keeps
//     `NavigationDrawerItem` independent, and so does this port.
//
//   * the scrim is carried and not painted: a child widget cannot cover its
//     parent, so the colour and opacity live on the tokens for a host
//     overlay.

#include "GalleryPages.h"

#include "widgets/MdNavigationDrawer.h"
#include "widgets/MdNavigationDrawerItem.h"

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

} // namespace

class NavigationDrawerPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit NavigationDrawerPage(QWidget *parent = nullptr);
    ~NavigationDrawerPage() override;

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

    md::MdNavigationDrawer *drawer(const QString &id);
    md::MdNavigationDrawerItem *item(const QString &id, const QString &label, const QString &icon);

    QHash<QString, md::MdNavigationDrawer *> m_drawers;
    QHash<QString, md::MdNavigationDrawerItem *> m_items;
    QVector<NavSlot> m_slots;
};

NavigationDrawerPage::NavigationDrawerPage(QWidget *parent)
    : GalleryPage(parent)
{
}

NavigationDrawerPage::~NavigationDrawerPage() = default;

QString NavigationDrawerPage::title() const
{
    return L("导航抽屉", "Navigation drawer");
}

QString NavigationDrawerPage::slug() const
{
    return QStringLiteral("navigation-drawer");
}

QString NavigationDrawerPage::subtitle() const
{
    return L(
        "导航抽屉：从侧边滑入的目的地纵列。这一族只有**一份** token 文件 "
        "（`md.comp.navigation-drawer.*`，64 行）：standard 与 modal 是同一个文件里的两"
        "个容器行组——standard 是 level0 的 surface、modal 是 level1 的 "
        "surface-container-low、从 scrim 上滑入。Expressive 的替代品是展开的导航栏（上一"
        "页），抽屉按已发布的样子移植。项是**独立的** `MdNavigationDrawerItem`——药丸就"
        "是项本身：一条全宽、最低 56dp 的行，选中时容器色就是 secondary-container，没有"
        "宽度动画；颜色表与导航栏三处不同，badge 挂在尾端。",
        "The navigation drawer: a column of destinations sliding in from the edge. This "
        "family has **one** token file (`md.comp.navigation-drawer.*`, 64 rows): standard "
        "and modal are two container row groups in it — standard is level0 surface, modal "
        "is level1 surface-container-low arriving over a scrim. The Expressive "
        "replacement is the expanded navigation rail (the previous page); the drawer is "
        "ported as published. The item is **not** the shared expressive one but an "
        "independent `MdNavigationDrawerItem` — the pill IS the item, a full-width "
        "56dp-minimum row whose container colour is the selected state, with no width "
        "animation. Its colour table diverges from the bar's in three recorded places, "
        "and a badge hangs at the trailing edge.");
}

md::MdNavigationDrawer *NavigationDrawerPage::drawer(const QString &id)
{
    const auto found = m_drawers.constFind(id);
    if (found != m_drawers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationDrawer(this);
    created->hide();
    m_drawers.insert(id, created);
    return created;
}

md::MdNavigationDrawerItem *NavigationDrawerPage::item(const QString &id, const QString &label,
                                                       const QString &icon)
{
    const auto found = m_items.constFind(id);
    if (found != m_items.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationDrawerItem(label, icon, this);
    created->hide();
    m_items.insert(id, created);
    return created;
}

void NavigationDrawerPage::build(GalleryContext &context)
{
    m_slots.clear();

    struct Destination
    {
        const char *id;
        const char *zh;
        const char *en;
        const char *icon;
        const char *badge;
    };
    const QVector<Destination> destinations = {
        {"inbox", "收件箱", "Inbox", "mail", "24"},
        {"drafts", "草稿", "Drafts", "edit", ""},
        {"starred", "星标", "Starred", "star", ""},
        {"archive", "归档", "Archive", "menu", ""},
    };
    auto addDestinationColumn = [&](md::MdNavigationDrawer *sheet, const QString &prefix) {
        sheet->clearItems();
        for (const Destination &destination : destinations) {
            md::MdNavigationDrawerItem *destinationItem =
                item(prefix + QStringLiteral("-") + QString::fromUtf8(destination.id),
                     L(destination.zh, destination.en), destination.icon);
            destinationItem->setBadge(QString::fromUtf8(destination.badge));
            sheet->addItem(destinationItem);
        }
    };

    // --- the standard drawer -------------------------------------------------
    context.section(L("Standard：360 宽的停靠面板", "Standard: the 360dp docked sheet"));
    context.paragraph(L(
        "standard 行组：level0 的 surface、corner-large-end（**尾端**一对圆角，起始边是"
        "方的，RTL 下镜像到另一对）。360dp 里装的是 336dp 的药丸——外部的 12dp 来自 "
        "Compose 的 `ItemPadding`（水平 12，硬编码）。项的内容行也是硬编码：左 16、图标 "
        "24、图标文字间 12、右 24；文字 label-large，选中时用 prominent（本库的 "
        "Emphasized 切）。第一项带 badge：24 的角标文字挂在尾端 24dp 内衬处，用 "
        "`large-badge-label-*` 行（label-large 基线字重）。",
        "The standard rows: level0 surface, corner-large-end — the **end** pair rounds "
        "and the leading edge stays square, mirrored in RTL. Inside the 360dp sits a "
        "336dp pill: the external 12dp comes from Compose's `ItemPadding` (horizontal 12, "
        "hard-coded). The item's content row is hard-coded too — 16 leading, a 24 icon, "
        "12 to the label, 24 trailing — with the label label-large, prominent "
        "(this port's Emphasized cut) when selected. The first item carries a badge: the "
        "24 text hangs at the trailing inset and paints from the `large-badge-label-*` "
        "rows (label-large at the baseline weight)."));
    {
        md::MdNavigationDrawer *sheet = drawer(QStringLiteral("drawer-standard"));
        sheet->setVariant(md::MdNavigationDrawerVariant::Standard);
        sheet->setHeadline(L("邮件", "Mail"));
        sheet->setDividerVisible(true);
        addDestinationColumn(sheet, QStringLiteral("drawer-standard"));
        sheet->setCurrentIndex(0);
        m_slots.append(NavSlot{sheet, context.band(qreal(sheet->sizeHint().height()))});
        context.detail(L("Standard · surface / level0 · headline + 分隔线 · 首项带 badge 24",
                         "Standard · surface / level0 · headline + divider · first item with badge 24"));
        context.space(16.0);
    }

    // --- the modal drawer -----------------------------------------------------
    context.section(L("Modal：surface-container-low 与 scrim",
                      "Modal: surface-container-low and the scrim"));
    context.paragraph(L(
        "modal 行组把容器换成 level1 的 surface-container-low，并附两句 scrim 行——"
        "`scrim-color: neutral-variant20`、`scrim-opacity: 0.4`。这两行是**承载不画**"
        "的：scrim 盖在抽屉的父级上，子控件画不到父级上面，所以颜色和不透明度放在 token"
        "上（`scrimColor()` / `scrimOpacity()`）供宿主搭浮层；导出写的 neutral-variant20 "
        "与本库 Scrim 角色解析出的 neutral0 之间的分歧已记录（本库没有逐组件的颜色行）。"
        "modal 的 level1 阴影同理：落在子面板矩形之外，承载不画，与导航栏的 level2 同"
        "例。下一行的面板就是 modal 行组画的——注意容器的色差。",
        "The modal rows swap the container to level1 surface-container-low and add the "
        "two scrim rows — `scrim-color: neutral-variant20`, `scrim-opacity: 0.4`. Both "
        "are **carried, not painted**: the scrim covers the drawer's *parent*, which a "
        "child widget cannot paint over, so the colour and opacity live on the tokens "
        "(`scrimColor()` / `scrimOpacity()`) for a host overlay; the divergence between "
        "the export's neutral-variant20 and the library's Scrim role (which resolves "
        "from neutral0) is recorded — this library has no per-component colour rows. The "
        "modal level1 shadow is the same story: it falls outside a child sheet's rect, "
        "so it is carried, like the bar's level2. The sheet below is painted from the "
        "modal group — note the container's tint."));
    {
        md::MdNavigationDrawer *sheet = drawer(QStringLiteral("drawer-modal"));
        sheet->setVariant(md::MdNavigationDrawerVariant::Modal);
        sheet->setHeadline(L("邮件", "Mail"));
        sheet->setDividerVisible(false);
        addDestinationColumn(sheet, QStringLiteral("drawer-modal"));
        sheet->setCurrentIndex(2);
        m_slots.append(NavSlot{sheet, context.band(qreal(sheet->sizeHint().height()))});
        context.detail(L("Modal · surface-container-low / level1 · scrim 承载在 token 上",
                         "Modal · surface-container-low / level1 · the scrim carried on the tokens"));
        context.space(8.0);
    }

    // --- what this page is pinned to ----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Navigation 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Navigation; moving one turns it "
        "red."));
    context.detail(L("容器: 360 宽 / corner-large-end（尾端一对，RTL 镜像）/ standard surface "
                     "level0 · modal surface-container-low level1",
                     "container: 360 wide / corner-large-end (the end pair, mirrored in RTL) / "
                     "standard surface level0 · modal surface-container-low level1"));
    context.detail(L("项: 药丸就是项 · 最低 56 / 全宽（360 - 2x12）/ corner-full / 选中容器色 "
                     "secondary-container / 无宽度动画",
                     "item: the pill IS the item · 56 minimum / full width (360 - 2x12) / "
                     "corner-full / selected container secondary-container / no width "
                     "animation"));
    context.detail(L("颜色表与导航栏三处不同: active 全 on-secondary-container · inactive "
                     "pressed 的 state-layer 是 on-secondary-container 特例 · 文字 label-large",
                     "the colour table diverges from the bar's in three places: every active "
                     "row on-secondary-container · the inactive pressed state layer is the "
                     "on-secondary-container special case · the label label-large"));
    context.detail(L("内容行硬编码: 左 16 / 图标 24 / 间 12 / 右 24 · ItemPadding 12（336 = "
                     "360 - 24 的来源）· headline→内容 12（Compose sample 间距，非 token 行）",
                     "the content row is hard-coded: 16 leading / 24 icon / 12 gap / 24 "
                     "trailing · ItemPadding 12 (where 336 = 360 - 24 comes from) · "
                     "headline-to-content 12 (Compose sample spacing, not a token row)"));
    context.detail(L("承载不画: scrim（neutral-variant20@0.4，导出色与库 Scrim 角色的分歧已"
                     "记录）· modal level1 · large-badge 行（badge 以文本实现）",
                     "carried, not painted: the scrim (neutral-variant20@0.4; the export's "
                     "colour vs the library's Scrim role recorded) · modal level1 · the "
                     "large-badge rows (the badge is a text)"));
    context.space(8.0);
}

void NavigationDrawerPage::placeChildren()
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

void NavigationDrawerPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void NavigationDrawerPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void NavigationDrawerPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void NavigationDrawerPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createNavigationDrawerPage()
{
    return new NavigationDrawerPage;
}

} // namespace gallery

#include "GalleryPages32.moc"
