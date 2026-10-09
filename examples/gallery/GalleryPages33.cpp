// Gallery page 33: Tabs — `md.comp.primary-navigation-tab.*` and
// `md.comp.secondary-navigation-tab.*`, both at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Bands are exactly as tall
// as the row asks for.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * the two families differ in **indicator geometry**, not just numbers —
//     the primary's 3 px indicator is rounded on its top corners and animates
//     to the selected tab's *content* width; the secondary's 2 px one is a
//     square that spans the whole tab;
//   * the indicator **centres** in the tab — Compose's scrollable row does
//     this explicitly, its fixed row omits the step, and Flutter's M3
//     defaults (generated from the same token database) centre both. The
//     centring wins; the divergence is recorded in porting-todo.md;
//   * the export publishes no disabled rows and no motion rows — the indicator
//     rides the spatial default spring and the content colours cross-fade on
//     the effects ones, both Compose behaviour.

#include "GalleryPages.h"

#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

#include <cmath>

namespace gallery {

namespace {

struct TabsSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
    bool fillWidth = true;
    bool fillHeight = false;
};

} // namespace

class TabsPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit TabsPage(QWidget *parent = nullptr);
    ~TabsPage() override;

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

    md::MdTabs *row(const QString &id);
    md::MdTab *tab(const QString &id, const QString &label, const QString &icon);

    QHash<QString, md::MdTabs *> m_rows;
    QHash<QString, md::MdTab *> m_tabs;
    QVector<TabsSlot> m_slots;
};

TabsPage::TabsPage(QWidget *parent)
    : GalleryPage(parent)
{
}

TabsPage::~TabsPage() = default;

QString TabsPage::title() const
{
    return L("标签页", "Tabs");
}

QString TabsPage::slug() const
{
    return QStringLiteral("tabs");
}

QString TabsPage::subtitle() const
{
    return L(
        "标签页：**primary 与 secondary 两族**。primary 的指示器 3dp、顶部两角圆、宽度动画"
        "到选中项的**内容宽**；secondary 的指示器 2dp、方角、铺满整个标签。两族的高度都是 "
        "48dp（图标加文字 64dp），文字都是 title-small。material-web 两族都没实现成产品，"
        "数值来自官方导出、行为来自 Compose 的 `TabRow.kt`——指示器居中这一条以 Flutter 的 "
        "M3 defaults（同一 token 库生成）为准，因为 Compose 的 fixed 行漏了这一步，"
        "分歧记录在 porting-todo.md。",
        "Tabs: **primary and secondary families**. The primary indicator is 3dp, rounded on "
        "its top corners, and animates to the selected tab's *content* width; the "
        "secondary's is 2dp, square, and spans the whole tab. Both rows are 48dp tall (64dp "
        "with icon and label) and both label in title-small. material-web implements "
        "neither family as a product, so every number comes from the published export and "
        "every behaviour from Compose's `TabRow.kt` — except the indicator's centring, "
        "which follows Flutter's M3 defaults (generated from the same token database) "
        "because Compose's fixed row omits that step; the divergence is recorded in "
        "porting-todo.md.");
}

md::MdTabs *TabsPage::row(const QString &id)
{
    const auto found = m_rows.constFind(id);
    if (found != m_rows.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTabs(this);
    created->hide();
    m_rows.insert(id, created);
    return created;
}

md::MdTab *TabsPage::tab(const QString &id, const QString &label, const QString &icon)
{
    const auto found = m_tabs.constFind(id);
    if (found != m_tabs.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTab(label, this);
    if (!icon.isEmpty()) {
        created->setIconName(icon);
    }
    created->hide();
    m_tabs.insert(id, created);
    return created;
}

void TabsPage::build(GalleryContext &context)
{
    m_slots.clear();

    // The bundled classic set has these; the send/label glyph gap taught the
    // app-bars page to pick from the known-good list.
    struct Destination
    {
        const char *id;
        const char *zh;
        const char *en;
        const char *icon;
    };
    const QVector<Destination> destinations = {
        {"mail", "邮件", "Mail", "mail"},
        {"edit", "草稿", "Drafts", "edit"},
        {"star", "收藏", "Starred", "star"},
        {"delete", "已删", "Trash", "delete"},
    };
    auto addDestinationRow = [&](md::MdTabs *row, const QString &prefix, bool withIcons) {
        row->clearTabs();
        for (const Destination &destination : destinations) {
            md::MdTab *destinationTab = tab(prefix + QStringLiteral("-") + QString::fromUtf8(destination.id),
                                            L(destination.zh, destination.en),
                                            withIcons ? destination.icon : QString());
            row->addTab(destinationTab);
        }
    };

    // --- the two families ------------------------------------------------------
    context.section(L("两族指示器：primary 圆顶 3dp，secondary 方角 2dp",
                      "Two indicator shapes: the primary's rounded 3dp, the secondary's square 2dp"));
    context.paragraph(L(
        "同一条 48dp 的行，指示器是两族最直白的区别。primary 的指示器 3dp 高、顶部两角 "
        "3dp 圆（导出行 `active-indicator.shape: 3px 3px 0px 0px`），宽度动画到选中标签的"
        "**内容宽**——文字宽出 2×16 内边距，下限 24dp 的触控目标。secondary 的指示器 2dp "
        "高、没有 shape 行所以是方角、宽度是**整个标签**。选中内容的颜色也不同：primary "
        "族用 primary，secondary 族用 on-surface。",
        "The same 48dp row, and the indicator is where the two families part ways most "
        "visibly. The primary's is 3dp tall, rounded 3dp on its top corners (the export's "
        "`active-indicator.shape: 3px 3px 0px 0px`), and animates to the selected label's "
        "**content width** — the text minus its 2 x 16 padding, floored at the 24dp touch "
        "target. The secondary's is 2dp tall, has no shape row so it is square, and spans "
        "**the whole tab**. The selected content colour differs too: primary for the "
        "primary family, on-surface for the secondary."));
    {
        struct FamilyRow
        {
            const char *id;
            md::MdTabsVariant variant;
            bool withIcons;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<FamilyRow> rows = {
            {"row-primary", md::MdTabsVariant::Primary, true,
             "Primary · 指示器 3dp 顶部圆角 · 宽度 = 内容宽", "Primary · indicator 3dp rounded on top · width = content width"},
            {"row-secondary", md::MdTabsVariant::Secondary, true,
             "Secondary · 指示器 2dp 方角 · 宽度 = 整个标签", "Secondary · indicator 2dp square · width = the whole tab"},
        };
        for (const FamilyRow &family : rows) {
            md::MdTabs *rowTabs = row(QString::fromUtf8(family.id));
            rowTabs->setVariant(family.variant);
            addDestinationRow(rowTabs, QString::fromUtf8(family.id), family.withIcons);
            rowTabs->setCurrentIndex(0);
            m_slots.append(TabsSlot{rowTabs, context.band(rowTabs->sizeHint().height())});
            context.detail(L(family.noteZh, family.noteEn));
            context.space(16.0);
        }
    }

    // --- content heights ---------------------------------------------------------
    context.section(L("内容高度：纯文字 48dp，图标加文字 64dp",
                      "Content heights: 48dp text-only, 64dp with icon and label"));
    context.paragraph(L(
        "行高由最高的标签决定。纯文字与纯图标的标签占 `container.height` 的 48dp；图标在"
        "上、文字在下的标签占 `with-icon-and-label-text.container.height` 的 64dp。这里"
        "一行的四个标签都带图标加文字，行自动长到 64。Compose 的 `Tab.kt` 把这一行硬编码"
        "成 72dp——它的 token 文件和导出都写着 64——三票对一票，本库取 64，分歧记录在案。",
        "The row height follows its tallest tab. Text-only and icon-only tabs take the "
        "`container.height` 48dp; icon-above-label tabs take the "
        "`with-icon-and-label-text.container.height` 64dp. Every tab in the row below "
        "carries both, so the row grows to 64. Compose's `Tab.kt` hard-codes 72dp for "
        "this — its own token file and the export both say 64 — and three sources beat "
        "one hard-coded number, so this port takes 64 and records the divergence."));
    {
        md::MdTabs *rowTabs = row(QStringLiteral("row-icon-label"));
        rowTabs->setVariant(md::MdTabsVariant::Primary);
        addDestinationRow(rowTabs, QStringLiteral("row-icon-label"), true);
        rowTabs->setCurrentIndex(2);
        m_slots.append(TabsSlot{rowTabs, context.band(rowTabs->sizeHint().height())});
        context.space(16.0);
    }

    // --- scrollable ---------------------------------------------------------------
    context.section(L("可滚动：52dp 边距、90dp 下限、选中自动滚向中心",
                      "Scrollable: 52dp edge padding, a 90dp minimum, and auto-centre on selection"));
    context.paragraph(L(
        "Fixed 布局把整条宽度均分给标签——这是规范页描述的唯一形态。Scrollable 是 Compose "
        "补的第二形态：标签取自己的内容宽（下限 90dp），整行内容离两端各留 52dp，选中"
        "变化时整行在空间弹簧上滚向把选中项带到视口中心的位置（夹在内容范围内）。下面"
        "一条是六个标签的可滚动行，选中最后一个：它被滚向中心而不是留在行尾。滚轮也"
        "可以直接滚动，瞬时落位不动画。",
        "The Fixed layout divides the row evenly — the only shape the spec page describes. "
        "Scrollable is the second shape Compose ships: tabs take their content width from "
        "the 90dp minimum up, the run keeps a 52dp padding from each edge, and a selection "
        "change scrolls the run on the spatial spring until the selected tab is near the "
        "viewport's centre (clamped to the content). The row below is six tabs with the "
        "last one selected: it is scrolled towards the centre, not left at the end. The "
        "wheel scrolls too, landing instantly."));
    {
        md::MdTabs *rowTabs = row(QStringLiteral("row-scrollable"));
        rowTabs->setVariant(md::MdTabsVariant::Primary);
        rowTabs->setLayout(md::MdTabsLayout::Scrollable);
        rowTabs->clearTabs();
        const QVector<Destination> many = {
            {"mail", "邮件", "Mail", "mail"},
            {"edit", "草稿", "Drafts", "edit"},
            {"star", "收藏", "Starred", "star"},
            {"delete", "已删", "Trash", "delete"},
            {"menu", "更多", "More", "menu"},
            {"home", "主页", "Home", "home"},
        };
        for (const Destination &destination : many) {
            rowTabs->addTab(tab(QStringLiteral("row-scrollable-") + QString::fromUtf8(destination.id),
                                L(destination.zh, destination.en), destination.icon));
        }
        rowTabs->setCurrentIndex(0);
        m_slots.append(TabsSlot{rowTabs, context.band(rowTabs->sizeHint().height())});
        // The auto-centre runs on the event loop; selecting the last tab here
        // lets the screenshot catch it settled.
        rowTabs->setCurrentIndex(5);
        context.space(16.0);
    }

    // --- the leading icon ------------------------------------------------------------
    context.section(L("图标在侧：LeadingIconTab 的 48dp 行",
                      "Icon beside the label: LeadingIconTab's 48dp row"));
    context.paragraph(L(
        "Compose 的 `LeadingIconTab` 是第三个内容形态：图标与文字同排、中间 8dp、两侧各 "
        "16dp 内边距，高度与纯文字一样是 48dp。它的内容宽（primary 指示器的动画目标）"
        "相应是图标 + 8 + 文字。下面一行把四个标签全部摆成图标在侧。",
        "Compose's `LeadingIconTab` is the third content shape: icon and label side by "
        "side, 8dp between them, 16dp of padding each side, and the same 48dp height as "
        "text-only. Its content width — what the primary indicator animates to — is "
        "accordingly icon + 8 + label. The row below puts all four tabs in that shape."));
    {
        md::MdTabs *rowTabs = row(QStringLiteral("row-leading"));
        rowTabs->setVariant(md::MdTabsVariant::Primary);
        addDestinationRow(rowTabs, QStringLiteral("row-leading"), true);
        for (const Destination &destination : destinations) {
            md::MdTab *destinationTab =
                tab(QStringLiteral("row-leading-") + QString::fromUtf8(destination.id),
                    L(destination.zh, destination.en), destination.icon);
            destinationTab->setIconPosition(md::MdTabIconPosition::Start);
        }
        rowTabs->setCurrentIndex(1);
        m_slots.append(TabsSlot{rowTabs, context.band(rowTabs->sizeHint().height())});
        context.space(16.0);
    }

    // --- what this page is pinned to ----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Tabs 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Tabs; moving one turns it red."));
    context.detail(L("primary: 指示器 3dp、shape 3px 3px 0px 0px、宽度 = max(min(内容, "
                     "标签宽) − 32, 24) · 选中内容 primary · 未选中悬停/聚焦层 on-surface、"
                     "**按下层 primary**（一行特例）",
                     "primary: indicator 3dp, shape 3px 3px 0px 0px, width = "
                     "max(min(content, tab width) - 32, 24) · selected content primary · "
                     "inactive hover/focus layer on-surface, the **pressed layer primary** "
                     "(a one-row special case)"));
    context.detail(L("secondary: 指示器 2dp 方角、宽度 = 整个标签 · 选中内容 on-surface · "
                     "两侧状态层共用 on-surface",
                     "secondary: indicator 2dp square, width = the whole tab · selected "
                     "content on-surface · one shared on-surface state-layer table"));
    context.detail(L("两族指示器都**居中**于标签：Compose 的 scrollable 行显式居中、fixed 行"
                     "漏了这一步；Flutter 的 M3 defaults（同一 token 库）两处都居中——居中赢，"
                     "分歧记录在 porting-todo.md",
                     "both families **centre** the indicator in the tab: Compose's scrollable "
                     "row does it explicitly and its fixed row omits the step; Flutter's M3 "
                     "defaults (same token database) centre both — the centring wins, the "
                     "divergence is recorded in porting-todo.md"));
    context.detail(L("无 motion 行：指示器的 offset/width 走空间默认弹簧，内容色过渡进 "
                     "EffectsDefault、出 EffectsFast（Compose 的 TabTransition）",
                     "no motion rows: the indicator's offset/width ride the spatial default "
                     "spring, the content colours fade in on EffectsDefault and out on "
                     "EffectsFast (Compose's TabTransition)"));
    context.detail(L("divider 行已弃用但仍发布、仍绘制（上游默认也画）· 两族都无 disabled 行"
                     "——内容按系统 0.38 透明度",
                     "the divider rows are deprecated but published and still painted "
                     "(upstream paints one too) · neither family publishes disabled rows — "
                     "content at the system 0.38 alpha"));
    context.space(8.0);
}

void TabsPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const TabsSlot &slot : m_slots) {
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

void TabsPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void TabsPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void TabsPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void TabsPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createTabsPage()
{
    return new TabsPage;
}

} // namespace gallery

#include "GalleryPages33.moc"
