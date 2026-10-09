// Gallery page 28: App bars — `md.comp.app-bar.*` and
// `md.comp.bottom-app-bar.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every band below is exactly
// as tall as the bar's own `currentHeight()`, so the page never has to know
// the 64 / 112 / 136 / 152 arithmetic — and, more usefully, a collapsed bar's
// band shrinks with it, which is what makes the two-row collapse legible.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * a *single-row* bar's `heightOffsetLimit` is its whole height, so a small
//     bar collapses by sliding off, not by losing a row. Only a two-row bar
//     keeps a 64 px icon row on screen — `adjustHeightOffsetLimit` runs on the
//     text row alone.
//
//   * the centre slot is demonstrated with an `MdButton`, not a search field.
//     The published `search.*` rows (56 px, corner-full, surface-container ->
//     surface-container-highest, 8 px leading/trailing space) all exist in the
//     token set this port resolves, but the search *field* is a text-field
//     component and this library has no text field yet. The slot's geometry is
//     real here; the field is not. Recorded in docs/porting-todo.md.

#include "GalleryPages.h"

#include "core/MdAppBarScrollBehavior.h"
#include "widgets/MdBottomAppBar.h"
#include "widgets/MdButton.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdTopAppBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

/// One banked bar plus the band it asked for. Both sides of the bar family use
/// the same slot record because both are placed identically.
struct BarSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

/// `L()` for a row whose two copies may both be absent.
///
/// The tables below carry an optional subtitle, so their entries hold either a
/// Chinese/English pair or a pair of nulls. Passing a null straight through
/// would be a `QString::fromUtf8(nullptr)`, and falling back to the Chinese
/// copy in English mode would render tofu rather than nothing — which is
/// exactly the bug the screenshot pass caught on this page's subtitle rows.
QString copyOrEmpty(const char *zh, const char *en)
{
    return (zh == nullptr || en == nullptr) ? QString() : L(zh, en);
}

} // namespace

class AppBarPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit AppBarPage(QWidget *parent = nullptr);
    ~AppBarPage() override;

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

    /// The banked top bar for `id`, created once and reconfigured afterwards.
    md::MdTopAppBar *topBar(const QString &id);
    /// The behaviour banked alongside `id`. `settleAt` drives the bar to a
    /// collapsed fraction *after* the bar has written its own limit, which is
    /// the only order that works: the limit is what the offset is coerced
    /// against.
    md::MdAppBarScrollBehavior *behavior(const QString &id);
    /// The banked icon button for `id`.
    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdFab *fab(const QString &id, const QString &icon);
    md::MdBottomAppBar *bottomBar(const QString &id);

    QHash<QString, md::MdTopAppBar *> m_topBars;
    QHash<QString, md::MdAppBarScrollBehavior *> m_behaviors;
    QHash<QString, md::MdIconButton *> m_iconButtons;
    QHash<QString, md::MdFab *> m_fabs;
    QHash<QString, md::MdBottomAppBar *> m_bottomBars;
    /// The stand-in that sits in the search bar's centre slot. Not a text
    /// field: see the section that introduces it.
    md::MdButton *m_searchStand = nullptr;
    QVector<BarSlot> m_slots;
};

AppBarPage::AppBarPage(QWidget *parent)
    : GalleryPage(parent)
{
}

AppBarPage::~AppBarPage() = default;

QString AppBarPage::title() const
{
    return L("应用栏", "App bars");
}

QString AppBarPage::slug() const
{
    return QStringLiteral("app-bar");
}

QString AppBarPage::subtitle() const
{
    return L(
        "md.comp.app-bar：五种尺寸布局（small 64 / medium 112 / large 152 / "
        "medium-flexible 112·136 / large-flexible 120·152）加一组共用 token，"
        "下接 md.comp.bottom-app-bar（80dp）。规范表里列了七项，但只有五个布局："
        "\"居中\"是 small 的一种配置（同一套 token，只换对齐），而搜索应用栏是"
        "把 bar 的**中间**换成搜索框的配置。交互不是状态层——这一族根本没有 hover / "
        "press / focus 行——而是滚动：容器色 surface → surface-container，两行栏"
        "额外从 level0 升到 level2，并把自己的文字行折掉。",
        "md.comp.app-bar: the five size layouts (small 64 / medium 112 / large 152 / "
        "medium-flexible 112 and 136 / large-flexible 120 and 152) over one common "
        "token set, plus md.comp.bottom-app-bar (80dp). The spec's table lists seven "
        "entries but there are only five layouts: \"center aligned\" is a "
        "*configuration* of small — same tokens, different alignment — and the "
        "search app bar is the configuration that swaps the bar's *centre* for a "
        "search field. Interaction is not a state layer (this family publishes no "
        "hover, press or focus rows at all) but scrolling: the container goes "
        "surface -> surface-container, a two-row bar additionally rises from level0 "
        "to level2, and it collapses its own text row.");
}

md::MdTopAppBar *AppBarPage::topBar(const QString &id)
{
    const auto found = m_topBars.constFind(id);
    if (found != m_topBars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTopAppBar(this);
    m_topBars.insert(id, created);
    return created;
}

md::MdAppBarScrollBehavior *AppBarPage::behavior(const QString &id)
{
    const auto found = m_behaviors.constFind(id);
    if (found != m_behaviors.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdAppBarScrollBehavior(md::MdAppBarScrollMode::ExitUntilCollapsed, this);
    m_behaviors.insert(id, created);
    return created;
}

md::MdIconButton *AppBarPage::iconButton(const QString &id, const QString &icon)
{
    const auto found = m_iconButtons.constFind(id);
    if (found != m_iconButtons.constEnd()) {
        return found.value();
    }
    // A standard (containerless) icon button is what an app bar's nav and
    // action slots conventionally hold. Parented to the page, hidden until a
    // bar adopts it, so it never flashes at (0, 0).
    auto *created = new md::MdIconButton(icon, this);
    created->hide();
    m_iconButtons.insert(id, created);
    return created;
}

md::MdFab *AppBarPage::fab(const QString &id, const QString &icon)
{
    const auto found = m_fabs.constFind(id);
    if (found != m_fabs.constEnd()) {
        return found.value();
    }
    // The default is already the docked size: FabSize::Medium is the 56 dp FAB.
    auto *created = new md::MdFab(icon, this);
    created->hide();
    m_fabs.insert(id, created);
    return created;
}

md::MdBottomAppBar *AppBarPage::bottomBar(const QString &id)
{
    const auto found = m_bottomBars.constFind(id);
    if (found != m_bottomBars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdBottomAppBar(this);
    m_bottomBars.insert(id, created);
    return created;
}

void AppBarPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- the five size layouts -------------------------------------------------
    context.section(L("五种尺寸布局（七条高度）", "The five size layouts, seven heights"));
    context.paragraph(L(
        "自上而下：small 64、medium 112、large 152、medium-flexible 112、"
        "medium-flexible 带副标题 136、large-flexible 120、large-flexible 带副标题 "
        "152。medium 与 large 这两条基线在规范里**作为设计已废弃**——原文写着 "
        "\"No subtitle support on the legacy app bar\"，并指向各自的 flexible 替代"
        "——但它们仍是已发布的 token 组，所以这里照画。flexible 的两种高度由是否"
        "有副标题决定，这也是它们相对基线唯一的结构差别：标题更大"
        "（headline-medium / display-small），并且标题贴底而非居中。",
        "Top to bottom: small 64, medium 112, large 152, medium-flexible 112, "
        "medium-flexible with a subtitle 136, large-flexible 120 and large-flexible "
        "with a subtitle 152. The medium and large baselines are *deprecated as "
        "designs* — the spec says so outright, \"No subtitle support on the legacy "
        "app bar\", and points at their flexible replacements — but they are still "
        "published token sets, so they are still drawn here. A flexible bar's two "
        "heights are decided by whether it has a subtitle, and that is also its only "
        "structural difference from the baseline: a larger title (headline-medium / "
        "display-small) sitting on the bottom edge rather than in the middle."));
    {
        struct VariantRow
        {
            const char *id;
            md::MdAppBarVariant variant;
            const char *titleZh;
            const char *titleEn;
            const char *subZh;
            const char *subEn;
        };
        const QVector<VariantRow> rows = {
            {"v-small", md::MdAppBarVariant::Small, "Small · 64dp", "Small · 64dp", nullptr, nullptr},
            {"v-medium", md::MdAppBarVariant::Medium, "Medium · 112dp", "Medium · 112dp", nullptr,
             nullptr},
            {"v-large", md::MdAppBarVariant::Large, "Large · 152dp", "Large · 152dp", nullptr,
             nullptr},
            {"v-mflex", md::MdAppBarVariant::MediumFlexible, "Medium flexible · 112dp",
             "Medium flexible · 112dp", nullptr, nullptr},
            {"v-mflex-sub", md::MdAppBarVariant::MediumFlexible, "Medium flexible · 136dp",
             "Medium flexible · 136dp", "带副标题", "With a subtitle"},
            {"v-lflex", md::MdAppBarVariant::LargeFlexible, "Large flexible · 120dp",
             "Large flexible · 120dp", nullptr, nullptr},
            {"v-lflex-sub", md::MdAppBarVariant::LargeFlexible, "Large flexible · 152dp",
             "Large flexible · 152dp", "带副标题", "With a subtitle"},
        };

        for (const VariantRow &row : rows) {
            const QString id = QString::fromUtf8(row.id);
            md::MdTopAppBar *bar = topBar(id);
            bar->setVariant(row.variant);
            bar->setAlignment(md::MdAppBarAlignment::Leading);
            bar->setTitle(L(row.titleZh, row.titleEn));
            bar->setSubtitle(copyOrEmpty(row.subZh, row.subEn));
            bar->setNavigationWidget(iconButton(id + QStringLiteral("-nav"), QStringLiteral("menu")));
            bar->clearActionWidgets();
            bar->addActionWidget(
                iconButton(id + QStringLiteral("-a1"), QStringLiteral("favorite")));
            bar->addActionWidget(
                iconButton(id + QStringLiteral("-a2"), QStringLiteral("more_vert")));
            m_slots.append(BarSlot{bar, context.band(bar->currentHeight())});
            context.space(12.0);
        }
    }

    // --- the two alignments -----------------------------------------------------
    context.section(L("两种对齐：居中不是第六个布局", "Two alignments: centred is not a sixth layout"));
    context.paragraph(L(
        "规范把 \"Center-aligned\" 单列一行，但正文写的是 \"Use centered-text "
        "configuration\"——同一套 small token，只换对齐方式，所以这里是一个 "
        "`alignment` 属性而不是第二个类。居中时标题先对整个 bar 的宽度居中，"
        "再被 leading/action 两组槽推进去：它绝不会压到导航按钮或动作按钮下面。",
        "The spec lists \"Center-aligned\" as its own row, but its body text says "
        "\"Use centered-text configuration\" — the same small token set with a "
        "different alignment, which is why this port has an `alignment` property "
        "rather than a second class. Centred, the title is centred against the whole "
        "bar's width *first* and then pushed in by the leading and action slots, so "
        "it can never end up underneath the navigation or action buttons."));
    {
        const QVector<QPair<QString, md::MdAppBarAlignment>> rows = {
            {QStringLiteral("align-leading"), md::MdAppBarAlignment::Leading},
            {QStringLiteral("align-center"), md::MdAppBarAlignment::Center},
        };
        for (const auto &row : rows) {
            md::MdTopAppBar *bar = topBar(row.first);
            bar->setVariant(md::MdAppBarVariant::Small);
            bar->setAlignment(row.second);
            bar->setTitle(L("居中对齐的标题", "A centred title"));
            bar->setSubtitle(QString());
            bar->setNavigationWidget(
                iconButton(row.first + QStringLiteral("-nav"), QStringLiteral("arrow_back")));
            bar->clearActionWidgets();
            bar->addActionWidget(
                iconButton(row.first + QStringLiteral("-a1"), QStringLiteral("notifications")));
            m_slots.append(BarSlot{bar, context.band(bar->currentHeight())});
            context.space(12.0);
        }
    }

    // --- the two-row collapse ---------------------------------------------------
    context.section(L("两行折叠：medium 与 large", "The two-row collapse: medium and large"));
    context.paragraph(L(
        "两行栏只折自己的**文字行**，第一行恒为 64dp 且一直留在屏幕上——"
        "`adjustHeightOffsetLimit` 只跑在文字行上。于是 medium 从 112 折到 64（经过 "
        "88），large 从 152 折到 64（经过 108）。折叠过程中两个标题交叉淡入淡出："
        "标题行的小标题取 `cubic-bezier(.8, 0, .8, .15)` 的 `TopTitleAlphaEasing`，"
        "展开标题取 `1 - collapsedFraction`；两者是同一段文字，只是字阶不同"
        "（title-large 对 headline-small / headline-medium）。",
        "A two-row bar collapses only its *text row*; the first row is always 64dp and "
        "never leaves the screen, because `adjustHeightOffsetLimit` runs on the text "
        "row alone. So medium goes 112 -> 64 through 88, and large goes 152 -> 64 "
        "through 108. Across the collapse the two titles cross-fade: the small title "
        "in the leading row takes `TopTitleAlphaEasing`, `cubic-bezier(.8, 0, .8, "
        ".15)`, while the expanded title takes `1 - collapsedFraction`. They are the "
        "same string in two type scales (title-large against headline-small or "
        "headline-medium)."));
    {
        struct CollapseRow
        {
            const char *id;
            md::MdAppBarVariant variant;
            qreal fraction;
            const char *labelZh;
            const char *labelEn;
        };
        const QVector<CollapseRow> rows = {
            {"col-m0", md::MdAppBarVariant::Medium, 0.0, "Medium · 展开 112dp", "Medium · expanded 112dp"},
            {"col-m5", md::MdAppBarVariant::Medium, 0.5, "Medium · 折到一半 88dp", "Medium · half collapsed 88dp"},
            {"col-m1", md::MdAppBarVariant::Medium, 1.0, "Medium · 折完 64dp", "Medium · collapsed 64dp"},
            {"col-l5", md::MdAppBarVariant::Large, 0.5, "Large · 折到一半 108dp", "Large · half collapsed 108dp"},
            {"col-l1", md::MdAppBarVariant::Large, 1.0, "Large · 折完 64dp", "Large · collapsed 64dp"},
        };
        for (const CollapseRow &row : rows) {
            const QString id = QString::fromUtf8(row.id);
            md::MdTopAppBar *bar = topBar(id);
            bar->setVariant(row.variant);
            bar->setAlignment(md::MdAppBarAlignment::Leading);
            bar->setTitle(L(row.labelZh, row.labelEn));
            bar->setSubtitle(QString());
            bar->setNavigationWidget(
                iconButton(id + QStringLiteral("-nav"), QStringLiteral("menu")));
            bar->clearActionWidgets();
            bar->addActionWidget(
                iconButton(id + QStringLiteral("-a1"), QStringLiteral("edit")));
            bar->addActionWidget(
                iconButton(id + QStringLiteral("-a2"), QStringLiteral("share")));

            // The bar always writes the limit from its own collapsible row, so
            // the offset can only be set once the behaviour is attached.
            md::MdAppBarScrollBehavior *scroll = behavior(id);
            bar->setScrollBehavior(scroll);
            scroll->setHeightOffset(row.fraction * scroll->heightOffsetLimit());

            m_slots.append(BarSlot{bar, context.band(bar->currentHeight())});
            context.space(12.0);
        }
    }

    // --- the scroll colour -------------------------------------------------------
    context.section(L("滚动配色：surface → surface-container", "The scroll colour: surface to surface-container"));
    context.paragraph(L(
        "单行栏的规则是**阶跃**而非渐变：一旦 `overlappedFraction > 0.01` 就整体取"
        "滚动后的颜色，那个 0 → 1 的过渡交给 `animateColorAsState(target, "
        "DefaultEffects)` 的弹簧去补。两行栏则不同，它按 `collapsedFraction` **连续**"
        "变化——\"changes at the same rate the app bar expands or collapse\"——"
        "上面那一节的 bars 已经在做这件事。这里的过渡分数先过 "
        "`FastOutLinearInEasing`（`cubic-bezier(0.4, 0, 1, 1)`），再在 **Oklab** "
        "里插值；在 sRGB 里插值会得到不同的中点。",
        "A single-row bar's rule is a *step*, not a ramp: the moment "
        "`overlappedFraction > 0.01` it takes the scrolled colour outright, and the "
        "0 -> 1 transition is what `animateColorAsState(target, DefaultEffects)`'s "
        "spring fills in. A two-row bar is different — it changes *continuously* "
        "with `collapsedFraction` (\"changes at the same rate the app bar expands or "
        "collapse\"), which the bars in the section above are already doing. Here the "
        "transition fraction is run through `FastOutLinearInEasing`, "
        "`cubic-bezier(0.4, 0, 1, 1)`, and the colour is interpolated in **Oklab**; "
        "interpolating in sRGB would land on a different midpoint."));
    {
        // At rest: no behaviour at all, so `containerColor()` is the plain
        // surface role.
        md::MdTopAppBar *rest = topBar(QStringLiteral("scroll-rest"));
        rest->setVariant(md::MdAppBarVariant::Small);
        rest->setTitle(L("静止 · surface", "At rest · surface"));
        rest->setNavigationWidget(
            iconButton(QStringLiteral("scroll-rest-nav"), QStringLiteral("menu")));
        rest->clearActionWidgets();
        m_slots.append(BarSlot{rest, context.band(rest->currentHeight())});
        context.space(12.0);

        // Scrolled: a real behaviour, driven to overlappedFraction == 1.
        md::MdTopAppBar *scrolled = topBar(QStringLiteral("scroll-on"));
        scrolled->setVariant(md::MdAppBarVariant::Small);
        scrolled->setTitle(L("已滚动 · surface-container", "Scrolled · surface-container"));
        scrolled->setNavigationWidget(
            iconButton(QStringLiteral("scroll-on-nav"), QStringLiteral("menu")));
        scrolled->clearActionWidgets();
        md::MdAppBarScrollBehavior *scroll = behavior(QStringLiteral("scroll-on"));
        scrolled->setScrollBehavior(scroll);
        scroll->setScrollingContentAtStart(false);
        // |contentOffset| >= the limit is the scrolled-to-the-top condition;
        // the colour then ramps to 1 on the spring, which is why the screenshot
        // harness has to let the event loop settle before sampling.
        scroll->setContentOffset(-scroll->heightOffsetLimit());
        m_slots.append(BarSlot{scrolled, context.band(scrolled->currentHeight())});
    }

    // --- the centre slot ---------------------------------------------------------
    context.section(L("搜索应用栏就是 centre 槽", "The search app bar is the centre slot"));
    context.paragraph(L(
        "把中间的标题换成搜索框就是搜索应用栏——不是第六个布局，而是 small 的一种"
        "配置。槽里的控件接管标题的位置与空间，标题与副标题随即不再绘制。已发布的 "
        "search.* 行全部在解析器里（高 56、corner-full、"
        "`search.label.color on-surface-variant`、表面 `surface-container`、滚动后 "
        "`surface-container-highest`、左右各 8px 内边距）。**但搜索框本身不在本族**"
        "：它是一个文本框，而本库还没有文本框组件，所以这里用一个 MdButton 站在"
        "槽里演示几何，字段留到 ★ Text field。已记录在 docs/porting-todo.md。",
        "Swapping the centre for a search field *is* the search app bar — not a sixth "
        "layout but a configuration of small. A widget in the slot takes the title's "
        "place and space, and the title and subtitle stop being drawn. Every "
        "published `search.*` row is already in the resolver (height 56, "
        "corner-full, `search.label.color` on-surface-variant, container "
        "surface-container rising to surface-container-highest once scrolled, 8px of "
        "leading and trailing space). **The field itself is not this family's**: it "
        "is a text field, and this library has no text field yet, so an `MdButton` "
        "stands in the slot here purely to show the geometry. The field is left to "
        "\u2605 Text field, and recorded in docs/porting-todo.md."));
    {
        md::MdTopAppBar *bar = topBar(QStringLiteral("search"));
        bar->setVariant(md::MdAppBarVariant::Small);
        bar->setTitle(L("这一行不会被绘制", "This title is never drawn"));
        bar->setNavigationWidget(
            iconButton(QStringLiteral("search-nav"), QStringLiteral("menu")));
        bar->clearActionWidgets();
        bar->addActionWidget(
            iconButton(QStringLiteral("search-a1"), QStringLiteral("person")));

        // build() runs twice (once measuring, once painting), so the stand-in
        // has to be banked like every other child on this page — a fresh
        // `setCenterWidget` on the second pass would strand the first one as a
        // hidden child of the page.
        if (m_searchStand == nullptr) {
            m_searchStand = new md::MdButton(L("搜索", "Search"), this);
            m_searchStand->setVariant(md::ButtonVariant::Tonal);
            m_searchStand->setButtonSize(md::ButtonSize::Small);
            m_searchStand->hide();
        }
        bar->setCenterWidget(m_searchStand);

        m_slots.append(BarSlot{bar, context.band(bar->currentHeight())});
    }

    // --- the bottom app bar -------------------------------------------------------
    context.section(L("底部应用栏", "The bottom app bar"));
    context.paragraph(L(
        "`md.comp.bottom-app-bar` 只有一行：高 80、`container.color surface-container`、"
        "`container.elevation level2`、直角。内容内边距是 `start 4 / top 4 / end 4` "
        "——**没有下边距**，这正是内容带贴着容器底边的原因：一行内容会比真正的垂直"
        "居中还低约 2px。Docked FAB 从**右**上角落位（左右 16、上 12），并且自带 "
        "12dp 的内边距余量。有 FAB 时的 72dp 高度与 `surface-tint-layer` 都已废弃"
        "（\"design updated to use a single height for all configurations\" / "
        "\"opacity based surfaces to tonal surfaces\"），本页一律用 80dp。",
        "`md.comp.bottom-app-bar` is a single row: 80 tall, `container.color` "
        "surface-container, `container.elevation` level2, square. Its content padding "
        "is `start 4 / top 4 / end 4` — there is **no bottom value**, which is exactly "
        "why the content band runs to the container's bottom edge: a single line of "
        "content sits about 2px below a true vertical centre. A docked FAB takes the "
        "trailing top corner (16 from the side, 12 from the top) and brings its own "
        "12dp padding allowance. The 72dp with-FAB height and the "
        "`surface-tint-layer` are both deprecated (\"design updated to use a single "
        "height for all configurations\" and \"opacity based surfaces to tonal "
        "surfaces\"), so this page uses 80dp throughout."));
    {
        struct BottomRow
        {
            const char *id;
            md::MdBottomAppBar::Arrangement arrangement;
            bool withFab;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<BottomRow> rows = {
            {"bottom-start", md::MdBottomAppBar::Arrangement::Start, false, "Start，无 FAB",
             "Start, no FAB"},
            {"bottom-center", md::MdBottomAppBar::Arrangement::Center, false, "Center，无 FAB",
             "Center, no FAB"},
            {"bottom-fab", md::MdBottomAppBar::Arrangement::SpaceBetween, true,
             "SpaceBetween + 停靠 FAB", "SpaceBetween + a docked FAB"},
        };
        for (const BottomRow &row : rows) {
            const QString id = QString::fromUtf8(row.id);
            md::MdBottomAppBar *bar = bottomBar(id);
            bar->setArrangement(row.arrangement);

            const QStringList icons = {QStringLiteral("favorite"), QStringLiteral("search"),
                                       QStringLiteral("settings")};
            for (int i = 0; i < icons.size(); ++i) {
                md::MdIconButton *button =
                    iconButton(id + QStringLiteral("-a%1").arg(i), icons.at(i));
                // Added to the bar exactly once — `addWidget` on a widget that is
                // already in the list would duplicate it.
                if (!bar->widgets().contains(button)) {
                    bar->addWidget(button);
                }
            }

            if (row.withFab) {
                md::MdFab *docked = fab(id + QStringLiteral("-fab"), QStringLiteral("add"));
                if (bar->floatingActionButton() != docked) {
                    bar->setFloatingActionButton(docked);
                }
            }

            context.detail(L(row.noteZh, row.noteEn));
            m_slots.append(BarSlot{bar, context.band(bar->sizeHint().height())});
            context.space(12.0);
        }
    }

    // --- token facts -------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-app-bar.scss (+ "
        "-app-bar-small / -medium / -large / -medium-flexible / -large-flexible / "
        "-bottom-app-bar, 34.0.21) + Compose M3 AppBar.kt / AppBarDsl.kt / "
        "tokens/AppBar*Tokens.kt + m3.material.io/components/app-bars"));
    context.detail(QStringLiteral(
        "common      avatar 32 · icon-button-space 0 · icon 24 · leading-space 4 · "
        "trailing-space 4 · search.leading/trailing-space 8 · container.color "
        "surface · container.elevation level0 · container.shape corner-none · "
        "leading-icon on-surface · title on-surface · subtitle/trailing-icon/"
        "search.label on-surface-variant"));
    context.detail(QStringLiteral(
        "heights     small 64 (title-large, subtitle label-medium, search 56 "
        "corner-full body-large) · medium 112 (headline-small) · large 152 "
        "(headline-medium) · medium-flexible 112/136 (headline-medium) · "
        "large-flexible 120/152 (display-small) · bottom-app-bar 80"));
    context.detail(QStringLiteral(
        "on scroll   container.color surface-container · container.elevation "
        "level2 · search.container.color surface-container -> "
        "search.on-scroll.container.color surface-container-highest · "
        "step at overlappedFraction > 0.01 on one row, continuous "
        "collapsedFraction on two"));
    context.detail(L(
        "内边距      横向容器内边距就是已发布的 leading-space（4），所以标题的 "
        "12dp inset = 16 - 4，而\"离边 16\"是 4 + 图标按钮自身的 12 —— 16 本身"
        "**不是**一条令牌行。medium 标题底距 24、large 28，同样都是代码常量。",
        "padding     the horizontal container padding *is* the published "
        "leading-space (4), so the title's 12dp inset is 16 - 4, and \"16 from the "
        "edge\" is 4 plus the 12 an icon button brings itself — the 16 is **not** a "
        "token row. The medium title's 24dp bottom padding and the large one's 28 "
        "are likewise code constants."));
    context.detail(L(
        "落位        槽位摆的是**容器**，不是 widget。能显示焦点环的组件都会在自身"
        "矩形内留出 7.5dp 的余量（Qt 会把子控件裁到自己的矩形里，环画不出去），"
        "所以 MdIconButton 是 55×55 的 widget 包着 40×40 的容器。按 sizeHint 落位"
        "会让导航画在离边 11.5 而非 4、标题挤到 63 而非 48、动作按钮间距 55 而非 "
        "40。`MdChildBox` 是共用答案（规则由 MdButtonGroup 确立）。副作用是两个"
        "相邻 widget 的矩形会重叠 15dp —— 那层余量是透明的，记载在 "
        "docs/porting-todo.md。",
        "placement   the slots place **containers**, not widgets. A component that "
        "can show a focus indicator reserves 7.5dp inside its own rect (Qt clips a "
        "child to its rect, so the ring cannot be drawn outside it), which makes "
        "MdIconButton a 55x55 widget around a 40x40 container. Laying out by "
        "sizeHint draws the navigation 11.5 from the edge instead of 4, pushes the "
        "title to 63 instead of 48, and spaces the action buttons 55 apart instead "
        "of 40. `MdChildBox` is the shared answer, on the rule MdButtonGroup "
        "established. The consequence — two neighbouring widget rects overlap by "
        "15dp — is transparent overhang, recorded in docs/porting-todo.md."));
    context.detail(L(
        "divergences (1) 搜索框不在本族，本页用 MdButton 顶替（★ Text field）；"
        "(2) 16dp 边距不是令牌行；(3) 底栏的 level2 阴影**承载但不绘制**——底栏"
        "的阴影落在自身矩形之上，原位组件会被 Qt 裁掉，与卡片族的处理一致；"
        "(4) 嵌套滚动的两钩子协议在本库里由 `consumeScroll` 单入口归约，非 Pinned "
        "模式会像 Compose 一样**过度消费**整个 delta，这是刻意的；"
        "(5) 底栏内容带没有下边距，是导出本身如此。全部记录在 "
        "docs/porting-todo.md。",
        "divergences (1) the search field is not this family's, so an MdButton stands "
        "in (\u2605 Text field); (2) the 16dp edge distance is not a token row; "
        "(3) the bottom bar's level2 elevation is *carried but not painted* — its "
        "shadow falls above the bar's own rect, where Qt clips an in-place widget, "
        "the same call the card family made; (4) Compose's two-hook nested-scroll "
        "protocol is reduced here to the single `consumeScroll` entry point, and the "
        "non-pinned modes over-consume the whole delta exactly as Compose does — that "
        "is deliberate; (5) the bottom bar's content band has no bottom padding "
        "because the export does not. All five are recorded in docs/porting-todo.md."));
    context.space(8.0);
    context.detail(L(
        "TestMd3AppBar 将两族 token 表、七条高度、折叠行取 small 高度、12dp inset 的"
        "推导、两行与单行的几何、导航槽把标题推开的规则、居中标题的夹取、两族配色"
        "过渡表、Oklab 插值端点、三种滚动模式的消费行为、settle 的吸附门槛、"
        "滚动条镜像与两族渲染冒烟逐字段锁定。",
        "TestMd3AppBar pins both token tables, the seven heights, the collapsed row "
        "taking the small height, the derivation of the 12dp inset, the two-row and "
        "single-row geometry, the rule that the navigation slot pushes the title "
        "past itself, the centring clamp, both colour-transition tables, the Oklab "
        "lerp endpoints, the consumption behaviour of all three scroll modes, the "
        "settle snap threshold, the scroll-bar mirroring and both render smokes "
        "field by field."));
}

void AppBarPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const BarSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           QSize(int(std::lround(slot.rect.width())),
                                 int(std::lround(slot.rect.height()))));
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void AppBarPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void AppBarPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void AppBarPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void AppBarPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createAppBarPage()
{
    return new AppBarPage;
}

} // namespace gallery

#include "GalleryPages28.moc"
