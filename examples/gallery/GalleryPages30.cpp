// Gallery page 30: Navigation bar — `md.comp.navigation-bar.*` (the baseline
// family) and `md.comp.nav-bar.*` + `md.comp.nav-bar-item-{vertical,
// horizontal}.*` (the flexible one), both at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Bands are exactly as tall
// as the bar asks for, so the page never has to know the 80 / 64 arithmetic.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * the export ships **two coexisting families** for one component, at the
//     same version — the tall 80 px baseline and the short 64 px flexible one
//     the spec calls the flexible navigation bar and says *replaces* the
//     baseline. They are two products, not two snapshots of one.
//
//   * the `Centered` arrangement does not inset by a fixed amount: the band
//     shrinks by a *fraction of the bar* that depends on the item count (a
//     fifth per side at three items, zero from seven), so the pills stay a
//     comfortable distance from a floating action button.
//
//   * the item gap is 8 px — Compose's hard-coded `spacedBy(8.dp)`, not the
//     export's published `0px` row, because a gap between items is behaviour.

#include "GalleryPages.h"

#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationBarItem.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

#include <cmath>

namespace gallery {

namespace {

/// One banked widget plus the band it asked for, and which sides of the band
/// it fills: a bar stretches in width and keeps its token height; a rail and
/// a drawer (pages 31 / 32) do the opposite.
struct NavSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
    bool fillWidth = true;
    bool fillHeight = false;
};

} // namespace

class NavigationBarPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit NavigationBarPage(QWidget *parent = nullptr);
    ~NavigationBarPage() override;

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

    md::MdNavigationBar *bar(const QString &id);
    md::MdNavigationBarItem *item(const QString &id, const QString &label, const QString &icon);

    QHash<QString, md::MdNavigationBar *> m_bars;
    QHash<QString, md::MdNavigationBarItem *> m_items;
    QVector<NavSlot> m_slots;
};

NavigationBarPage::NavigationBarPage(QWidget *parent)
    : GalleryPage(parent)
{
}

NavigationBarPage::~NavigationBarPage() = default;

QString NavigationBarPage::title() const
{
    return L("导航栏", "Navigation bar");
}

QString NavigationBarPage::slug() const
{
    return QStringLiteral("navigation-bar");
}

QString NavigationBarPage::subtitle() const
{
    return L(
        "底部导航栏：**两套并存的家族**。基线家族 `md.comp.navigation-bar.*` 是 80dp 高、"
        "64x32 药丸、选中文字 on-surface 的一代；Expressive 的 flexible navigation bar "
        "（`md.comp.nav-bar.*`）是 64dp 高、56x32 药丸、选中文字 secondary 的一代——规范"
        "明说后者**取代**前者，但两套 token 在同一份导出里同时发布（同为 34.0.21），所以"
        "本库用同一个 `variant` 承载两代。material-web 两套都没实现成产品（可用性表 Web "
        "一栏 Unavailable），于是数值来自官方导出、行为来自 Compose 的 `NavigationBar.kt`"
        "（基线）与 `ShortNavigationBar.kt`（flexible）。项是共享的 expressive item：Top "
        "形态（图标在上）与 Start 形态（图标在文字旁，药丸包住两者）是同一类的两种摆法。",
        "The bottom navigation bar: **two coexisting families**. The baseline "
        "`md.comp.navigation-bar.*` is the 80dp generation — a 64x32 pill, the selected "
        "label on-surface. The Expressive flexible navigation bar (`md.comp.nav-bar.*`) "
        "is the 64dp one — 56x32, the selected label secondary — and the spec says it "
        "*replaces* the baseline; yet both token families ship in the same export at "
        "the same version (34.0.21), so this port carries both under one `variant`. "
        "material-web implements neither as a product (the availability table says "
        "Unavailable for Web), so every number comes from the published export and "
        "every behaviour from Compose's `NavigationBar.kt` (baseline) and "
        "`ShortNavigationBar.kt` (flexible). The items are the shared expressive item: "
        "the `Top` arrangement (icon above the label) and the `Start` one (icon beside "
        "it, inside the pill) are two layouts of the same class.");
}

md::MdNavigationBar *NavigationBarPage::bar(const QString &id)
{
    const auto found = m_bars.constFind(id);
    if (found != m_bars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationBar(this);
    created->hide();
    m_bars.insert(id, created);
    return created;
}

md::MdNavigationBarItem *NavigationBarPage::item(const QString &id, const QString &label,
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

void NavigationBarPage::build(GalleryContext &context)
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
        {"home", "首页", "Home", "home"},
        {"search", "搜索", "Search", "search"},
        {"mail", "邮件", "Mail", "mail"},
        {"settings", "设置", "Settings", "settings"},
    };
    auto addDestinationRow = [&](md::MdNavigationBar *bar, const QString &prefix) {
        bar->clearItems();
        for (const Destination &destination : destinations) {
            bar->addItem(item(prefix + QStringLiteral("-") + QString::fromUtf8(destination.id),
                              L(destination.zh, destination.en), destination.icon));
        }
    };

    // --- the two families ----------------------------------------------------
    context.section(L("两套家族并存：80dp 的基线与 64dp 的 flexible",
                      "Two families at once: the 80dp baseline and the 64dp flexible one"));
    context.paragraph(L(
        "上面一条是基线家族：80dp 高、64x32 的药丸、选中的文字用 on-surface。下面一条是 "
        "flexible 家族：64dp 高、56x32 的药丸、选中的文字用 secondary——它比基线矮 16dp，"
        "视觉上明显更紧凑。两套 token 在同一份导出里同时发布（同为 34.0.21），规范的话语"
        "是 flexible 取代基线，但没有把基线从导出里删掉；Compose 也是两个类并存（"
        "`NavigationBar` 与 `ShortNavigationBar`）。所以本库不选边：一个 `variant` 属性，"
        "两套数值各自成表，测试把两套都钉死。",
        "The upper bar is the baseline family: 80dp tall, a 64x32 pill, the selected "
        "label on-surface. The lower one is the flexible family: 64dp tall, 56x32, the "
        "selected label secondary — 16dp shorter and visibly tighter. Both token "
        "families ship in the same export at the same version (34.0.21); the spec's "
        "words are that the flexible bar replaces the baseline, but the baseline was "
        "not removed, and Compose keeps both classes too (`NavigationBar` and "
        "`ShortNavigationBar`). So this port does not pick a side: one `variant` "
        "property, two resolved token tables, and tests that pin both."));
    {
        struct FamilyRow
        {
            const char *id;
            md::MdNavigationBarVariant variant;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<FamilyRow> rows = {
            {"bar-baseline", md::MdNavigationBarVariant::Baseline,
             "Baseline · container.height 80 · 药丸 64x32", "Baseline · container.height 80 · pill 64x32"},
            {"bar-flexible", md::MdNavigationBarVariant::Flexible,
             "Flexible · container.height 64 · 药丸 56x32", "Flexible · container.height 64 · pill 56x32"},
        };
        for (const FamilyRow &row : rows) {
            md::MdNavigationBar *rowBar = bar(QString::fromUtf8(row.id));
            rowBar->setVariant(row.variant);
            rowBar->setArrangement(md::MdNavigationBarArrangement::EqualWeight);
            addDestinationRow(rowBar, QString::fromUtf8(row.id));
            rowBar->setCurrentIndex(0);
            m_slots.append(NavSlot{rowBar, context.band(rowBar->sizeHint().height())});
            context.detail(L(row.noteZh, row.noteEn));
            context.space(16.0);
        }
    }

    // --- EqualWeight and Centered ---------------------------------------------
    context.section(L("EqualWeight 与 Centered：带宽按项数收缩",
                      "EqualWeight and Centered: the band shrinks with the item count"));
    context.paragraph(L(
        "默认的 EqualWeight 把整条宽度除以项数（项间再留 8dp）。Centered 是 flexible 家族"
        "的另一种排布：可用带宽按一个**随项数收缩的比例**从两侧收进——3 项时每侧让出五分"
        "之一、4 项八分之一、5 项十分之一、7 项起为 0——给旁边可能存在的浮动动作按钮留出"
        "空隙。Compose 的公式是 `((100 - 10 * (count + 3)) / 2) / 100`，这里是它的 Qt 版："
        "同一个 4 项的 flexible 栏，上面 EqualWeight 占满，下面 Centered 两侧各收进一成。",
        "The default EqualWeight divides the bar's width by the item count (8dp between "
        "neighbours). Centered is the flexible family's other arrangement: the usable "
        "band is inset from both sides by a *fraction that shrinks as items are added* — "
        "a fifth per side at three items, an eighth at four, a tenth at five, zero from "
        "seven — leaving room for a floating action button that may sit beside the bar. "
        "Compose's formula is `((100 - 10 * (count + 3)) / 2) / 100`; the rows below are "
        "the same four-item flexible bar, EqualWeight first and Centered second, inset a "
        "tenth per side."));
    {
        struct ArrangementRow
        {
            const char *id;
            md::MdNavigationBarArrangement arrangement;
        };
        const QVector<ArrangementRow> rows = {
            {"arr-equal", md::MdNavigationBarArrangement::EqualWeight},
            {"arr-centered", md::MdNavigationBarArrangement::Centered},
        };
        for (const ArrangementRow &row : rows) {
            md::MdNavigationBar *rowBar = bar(QString::fromUtf8(row.id));
            rowBar->setVariant(md::MdNavigationBarVariant::Flexible);
            rowBar->setArrangement(row.arrangement);
            addDestinationRow(rowBar, QString::fromUtf8(row.id));
            rowBar->setCurrentIndex(1);
            m_slots.append(NavSlot{rowBar, context.band(rowBar->sizeHint().height())});
            context.space(16.0);
        }
    }

    // --- the Start arrangement ------------------------------------------------
    context.section(L("Top 与 Start：图标在上，或与文字同排",
                      "Top and Start: the icon above, or beside the label"));
    context.paragraph(L(
        "共享 item 的两种形态。Top 是导航栏的常态：图标在上、文字在下，选中的药丸只在"
        "图标后面张开（宽度弹簧，从 0 到 56，高度不动）。Start 是 flexible 家族的横向形态："
        "图标与文字同排，**药丸包住两者**——所以它的宽是 16 + 图标 + 4 + 文字 + 16，高是 "
        "40，文字用图标的颜色（on-secondary-container），因为文字坐在药丸里面。下面一条"
        "把四个项全部摆成 Start。",
        "The shared item's two arrangements. Top is the bar's normal one: icon above the "
        "label, the selected pill opening behind the icon only — a width spring from 0 "
        "to 56, the height fixed. Start is the flexible family's horizontal one: icon "
        "and label side by side and the **pill wraps both** — 16 + icon + 4 + label + 16 "
        "wide, 40 tall, with the label painted in the icon's colour "
        "(on-secondary-container), because the label sits inside the pill. The row below "
        "puts all four items in the Start arrangement."));
    {
        md::MdNavigationBar *rowBar = bar(QStringLiteral("bar-start"));
        rowBar->setVariant(md::MdNavigationBarVariant::Flexible);
        rowBar->setArrangement(md::MdNavigationBarArrangement::EqualWeight);
        // The bar owns its items' arrangement — the `itemLayout` property is
        // what `applyToItem` pushes — so the switch happens here, not per item.
        rowBar->setItemLayout(md::MdNavigationItemIconPosition::Start);
        rowBar->clearItems();
        for (const Destination &destination : destinations) {
            rowBar->addItem(item(QStringLiteral("bar-start-") + QString::fromUtf8(destination.id),
                                 L(destination.zh, destination.en), destination.icon));
        }
        rowBar->setCurrentIndex(0);
        m_slots.append(NavSlot{rowBar, context.band(rowBar->sizeHint().height())});
        context.space(8.0);
    }

    // --- what this page is pinned to ----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Navigation 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Navigation; moving one turns it "
        "red."));
    context.detail(L("baseline: 高 80 / 药丸 64x32 / 项间 8 / level2（承载不画）· 选中文字 "
                     "on-surface · 80 是导出与 Compose 行为体一致的数（其 token 文件里的 64 "
                     "挂着 TODO）",
                     "baseline: 80 tall / pill 64x32 / 8 between items / level2 (carried, "
                     "unpainted) · selected label on-surface · the 80 is what the export "
                     "and Compose's behaviour body agree on (the 64 in its token file sits "
                     "under a TODO)"));
    context.detail(L("flexible: 高 64 / 药丸 56x32 / 选中文字 secondary · Start：药丸 40 高、"
                     "左右 16、图标文字间 4 · 文字随图标色",
                     "flexible: 64 tall / pill 56x32 / selected label secondary · Start: pill "
                     "40 tall, 16 a side, 4 between icon and label · label rides the icon's "
                     "colour"));
    context.detail(L("项间 8 是 Compose 的 spacedBy(8.dp)——导出的 item.between-space 是 0px，"
                     "但项间距是行为，行为赢",
                     "the 8 between items is Compose's spacedBy(8.dp) — the export's "
                     "item.between-space reads 0px, but a gap between items is behaviour, "
                     "and behaviour wins"));
    context.detail(L("Centered 带宽比例：(100 - 10 * (count + 3)) / 2 / 100——3 项每侧 1/5、"
                     "4 项 1/8、5 项 1/10、7 项起 0",
                     "the Centered band fraction: (100 - 10 * (count + 3)) / 2 / 100 — a "
                     "fifth per side at 3 items, an eighth at 4, a tenth at 5, zero from 7"));
    context.detail(L("药丸宽度弹簧：从 0 到满宽，FastSpatial，高度不动；:focus-visible 内缩焦点环",
                     "the pill's width spring: 0 to full width on FastSpatial, height fixed; "
                     "the :focus-visible ring draws inward"));
    context.space(8.0);
}

void NavigationBarPage::placeChildren()
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

void NavigationBarPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void NavigationBarPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void NavigationBarPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void NavigationBarPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createNavigationBarPage()
{
    return new NavigationBarPage;
}

} // namespace gallery

#include "GalleryPages30.moc"
