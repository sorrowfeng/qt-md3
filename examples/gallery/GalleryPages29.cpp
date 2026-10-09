// Gallery page 29: Toolbars — `md.comp.toolbar.docked.*`,
// `md.comp.toolbar.floating.*` and the `standard` / `vibrant` colour sets.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Bands are exactly as tall as
// the widget asks for, so the page never has to know the 64 / 80 arithmetic —
// and, more usefully, a band shrinks with a collapsing docked toolbar, which is
// what makes the whole-height collapse legible.
//
// Four things this page is explicit about because they are easy to misread:
//
//   * the docked toolbar is not a "bottom app bar with a smaller height". Its
//     row is centred with a *token* spacing of 32 px and there is no
//     arrangement property, because the published component has none.
//
//   * a docked toolbar's collapse takes its **whole** height
//     (`heightOffsetLimit = -64`), unlike a two-row top app bar which keeps its
//     icon row. The half-collapsed band below is the evidence.
//
//   * a floating toolbar's pill shortens with `expandedProgress` while the
//     widget's own bounds stay put, its leading / trailing slots are absent at
//     progress 0 and its content is hidden once the pill no longer contains it
//     — so the fully collapsed band is *blank*, which is correct and is what
//     the paragraph under it is about.
//
//   * the action button beside a floating toolbar is asked to take the FAB
//     token's 56 or 80 px, but an `MdFab` publishes one fixed size, so it stays
//     56 px centred in the 80 px box. The toolbar's geometry is right; the FAB
//     family has no size set yet. Recorded in docs/porting-todo.md.

#include "GalleryPages.h"

#include "widgets/MdDockedToolbar.h"
#include "widgets/MdFab.h"
#include "widgets/MdFloatingToolbar.h"
#include "widgets/MdIconButton.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

#include <cmath>

namespace gallery {

namespace {

/// One banked widget plus the band it asked for.
struct ToolbarSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class ToolbarPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ToolbarPage(QWidget *parent = nullptr);
    ~ToolbarPage() override;

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

    md::MdDockedToolbar *docked(const QString &id);
    md::MdFloatingToolbar *floating(const QString &id);
    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdFab *fab(const QString &id, const QString &icon);

    QHash<QString, md::MdDockedToolbar *> m_docked;
    QHash<QString, md::MdFloatingToolbar *> m_floating;
    QHash<QString, md::MdIconButton *> m_iconButtons;
    QHash<QString, md::MdFab *> m_fabs;
    QVector<ToolbarSlot> m_slots;
};

ToolbarPage::ToolbarPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ToolbarPage::~ToolbarPage() = default;

QString ToolbarPage::title() const
{
    return L("工具栏", "Toolbars");
}

QString ToolbarPage::slug() const
{
    return QStringLiteral("toolbar");
}

QString ToolbarPage::subtitle() const
{
    return L(
        "md.comp.toolbar：Expressive 的两个变体。**停靠**工具栏是单行、corner-none、"
        "surface-container 的 64dp 容器，内容左右各留 16dp、项与项之间至少 32dp 且整体"
        "居中——它是底部应用栏的替代品（规范原文：bottom app bar \"is no longer "
        "recommended… should be replaced with the docked toolbar\"）。**浮动**工具栏是"
        "16dp 内衬、corner-full、level3 的 pill，带 standard / vibrant 两套配色与横向 / "
        "纵向两种排布，并可在旁边挂一个动作按钮。material-web 两个都没实现（可用性表里 "
        "Web 一栏是 Unavailable），所以这一族的数值全部来自官方导出，行为来自 Compose。",
        "md.comp.toolbar: the two Expressive variants. The **docked** toolbar is a "
        "single 64dp row, corner-none and surface-container, its content padded 16dp in "
        "from each end and centred with at least 32dp between items — it is the "
        "replacement for the bottom app bar (the spec's own words: the bottom app bar "
        "\"is no longer recommended… should be replaced with the docked toolbar\"). The "
        "**floating** toolbar is a corner-full, level3 pill with 8dp of content padding, "
        "two colour schemes (standard and vibrant), two layouts (horizontal and "
        "vertical) and an optional action button beside it. material-web implements "
        "neither — the availability table says Unavailable for Web — so every number "
        "here comes from the published export and every behaviour from Compose.");
}

md::MdDockedToolbar *ToolbarPage::docked(const QString &id)
{
    const auto found = m_docked.constFind(id);
    if (found != m_docked.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDockedToolbar(this);
    created->hide();
    m_docked.insert(id, created);
    return created;
}

md::MdFloatingToolbar *ToolbarPage::floating(const QString &id)
{
    const auto found = m_floating.constFind(id);
    if (found != m_floating.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFloatingToolbar(this);
    created->hide();
    m_floating.insert(id, created);
    return created;
}

md::MdIconButton *ToolbarPage::iconButton(const QString &id, const QString &icon)
{
    const auto found = m_iconButtons.constFind(id);
    if (found != m_iconButtons.constEnd()) {
        return found.value();
    }
    // A standard (containerless) icon button is what a toolbar's slots
    // conventionally hold. Parented to the page and hidden until a toolbar
    // adopts it, so it never flashes at (0, 0).
    auto *created = new md::MdIconButton(icon, this);
    created->hide();
    m_iconButtons.insert(id, created);
    return created;
}

md::MdFab *ToolbarPage::fab(const QString &id, const QString &icon)
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

void ToolbarPage::build(GalleryContext &context)
{
    m_slots.clear();

    const QStringList glyphs = {QStringLiteral("edit"), QStringLiteral("share"),
                                QStringLiteral("delete")};

    // --- the docked toolbar -------------------------------------------------
    context.section(L("停靠工具栏：整条高度可折叠", "The docked toolbar: the whole height collapses"));
    context.paragraph(L(
        "上面一条是展开的 64dp。下面一条把 `heightOffset` 推到 −32，于是它的**整条**高度"
        "只剩 32——不是折掉一行，而是整条滑走，因为 Compose 在 `BottomAppBarLayout` 里写的是 "
        "`heightOffsetLimit = -placeable.height`。同一个布局也用 `Arrangement.spacedBy("
        "32, CenterHorizontally)` 摆这一行：项与项之间是 token 的 32dp，整体居中，所以这个"
        "组件没有 `arrangement` 属性——已发布的组件没有。",
        "The upper row is the 64dp toolbar expanded. The lower one has its "
        "`heightOffset` pushed to -32, so its *entire* height is down to 32 — not one "
        "row folded away but the whole bar sliding off, because Compose writes "
        "`heightOffsetLimit = -placeable.height` in `BottomAppBarLayout`. That same "
        "layout arranges the row with `Arrangement.spacedBy(32, CenterHorizontally)`: "
        "the spacing is the token's 32dp and the run is centred, which is why this "
        "component has no `arrangement` property — the published component has none."));
    {
        const QVector<QPair<QString, qreal>> rows = {
            {QStringLiteral("dock-full"), 0.0},
            {QStringLiteral("dock-half"), -32.0},
        };
        for (const auto &row : rows) {
            md::MdDockedToolbar *bar = docked(row.first);
            bar->clearWidgets();
            for (int i = 0; i < glyphs.size(); ++i) {
                bar->addWidget(
                    iconButton(row.first + QStringLiteral("-i%1").arg(i), glyphs.at(i)));
            }
            bar->setHeightOffset(row.second);
            // The band tracks the bar's *current* height, so the half-collapsed
            // row is visibly half the height of the other.
            m_slots.append(ToolbarSlot{bar, context.band(bar->currentHeight())});
            context.space(16.0);
        }
    }

    // --- the floating toolbar, both orientations ----------------------------
    context.section(L("浮动工具栏：横向与纵向，standard 与 vibrant",
                      "The floating toolbar: horizontal and vertical, standard and vibrant"));
    context.paragraph(L(
        "浮动工具栏是一个 pill：corner-full、内容四周各 8dp（8 + 一个 48dp 图标按钮 + 8 "
        "正好是 64dp 的容器高）、表面用 level3 抬高。相邻两项之间还有 4dp——这条 "
        "`container.between-space` 是本族**唯一一条本库读、Compose 不读**的行：Compose 把项"
        "的排列交给调用方的 `Row`，所以上游一个间距都不出；本库的控件自己摆子控件，于是把"
        "这 4dp 用上。要注意本页用的是 40dp 的图标按钮（本库的 Small）而不是 48dp：工具栏"
        "**没有** item 尺寸行（导出的 36 + 75 行逐行核对过），项宽完全由子控件决定，而本库 "
        "`MdIconButton` 的尺寸阶梯是 Expressive 的 32 / 40 / 56 / 96 / 136，没有 48。所以"
        "每条 pill 都比规范算式窄 8dp——这是子控件的事实，不是布局的错。导出给的横向 / 纵"
        "向两组行只差一个数：横向的外部边距 16dp、纵向 24dp，纵向比横向更靠里；除此之外两者"
        "是精确的转置，所以这里是同一个类上的一个 `orientation`，而不是两个类。配色同理："
        "Standard 是 surface-container 上的 on-surface-variant，Vibrant 是 primary-container "
        "上的 on-primary-container，且**未选中项**用的就是容器的那一对颜色。",
        "A floating toolbar is a pill: corner-full, 8dp of padding on all four sides "
        "(8 + a 48dp icon button + 8 is exactly the 64dp container height) and raised "
        "to level3. Neighbouring items are a further 4dp apart — `container.between-space` "
        "is the one row in this family that this port reads and Compose does not: "
        "upstream hands the arrangement of a toolbar's items to the caller's `Row`, so it "
        "ships no spacing at all, while this widget places its own children and therefore "
        "uses the 4dp. Note that this page uses a 40px icon button (the library's Small) "
        "rather than a 48px one: a toolbar has **no** item-size row (all 36 + 75 exported "
        "rows were checked), so the item width is entirely the child component's business, "
        "and this library's `MdIconButton` ladder is the Expressive 32 / 40 / 56 / 96 / 136 "
        "— there is no 48. Every pill here is therefore 8px narrower per item than the "
        "spec's arithmetic; that is a fact about the child, not a layout error. The "
        "export's horizontal and vertical rows differ by one number — the external edge "
        "offset, 16dp for a horizontal toolbar and 24dp for a vertical one, so a vertical "
        "toolbar sits further in — and are otherwise an exact transpose, which is why this "
        "port expresses them as an `orientation` on one class rather than two classes. The "
        "colour schemes work the same way: Standard is on-surface-variant on "
        "surface-container, Vibrant is on-primary-container on primary-container, and the "
        "*unselected* items are the ones painted in the container's own pair."));
    {
        struct FloatingRow
        {
            const char *id;
            md::MdToolbarOrientation orientation;
            md::MdToolbarColorScheme scheme;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<FloatingRow> rows = {
            {"float-h-std", md::MdToolbarOrientation::Horizontal,
             md::MdToolbarColorScheme::Standard, "横向 · Standard", "Horizontal · standard"},
            {"float-h-vib", md::MdToolbarOrientation::Horizontal,
             md::MdToolbarColorScheme::Vibrant, "横向 · Vibrant", "Horizontal · vibrant"},
            {"float-v-std", md::MdToolbarOrientation::Vertical,
             md::MdToolbarColorScheme::Standard, "纵向 · Standard", "Vertical · standard"},
        };
        for (const FloatingRow &row : rows) {
            const QString id = QString::fromUtf8(row.id);
            md::MdFloatingToolbar *bar = floating(id);
            bar->setOrientation(row.orientation);
            bar->setColorScheme(row.scheme);
            bar->setLeadingWidget(nullptr);
            bar->setTrailingWidget(nullptr);
            bar->setFab(nullptr);
            bar->clearContentWidgets();
            for (int i = 0; i < glyphs.size(); ++i) {
                bar->addContentWidget(
                    iconButton(id + QStringLiteral("-i%1").arg(i), glyphs.at(i)));
            }
            bar->setExpanded(true);
            bar->setExpansionProgress(1.0);
            m_slots.append(ToolbarSlot{bar, context.band(bar->sizeHint().height())});
            context.detail(L(row.noteZh, row.noteEn));
            context.space(16.0);
        }
    }

    // --- expanded and collapsed ---------------------------------------------
    context.section(L("展开与折叠：pill 会缩，外框不动",
                      "Expanded and collapsed: the pill shortens, the bounds do not"));
    context.paragraph(L(
        "同一个浮动工具栏的三个进度。0.5 时 pill 只有一半长，因为它按 "
        "`maxIntrinsicWidth * expandedProgress` 量——但**控件自己的矩形没有变**，槽位也不"
        "回流，缩短的只是 pill。进度 0 时 leading 与 trailing 整个从布局里消失（Compose "
        "用 `AnimatedVisibility(visible = expandedState)` 包着它们），content 随之在原来的"
        "带里重新居中；而 pill 此时已经没有长度，里面的东西也就都不见了——所以最后那一条"
        "是空的。Compose 靠 `graphicsLayer { clip = true }` 把内容裁到 pill 上；Qt 的子控件"
        "只按控件矩形裁剪，所以这里是把「已经跑到 pill 外面」的子控件直接隐藏，是同一件事"
        "的离散版本，已记进 docs/porting-todo.md。",
        "Three progresses of the same floating toolbar. At 0.5 the pill is half as long, "
        "because it is measured at `maxIntrinsicWidth * expandedProgress` — but the "
        "widget's own rectangle has not moved and the slots have not reflowed; only the "
        "pill shortened. At progress 0 the leading and trailing slots leave the layout "
        "entirely (Compose wraps them in `AnimatedVisibility(visible = expandedState)`), "
        "so the content re-centres in the band it had — and the pill has no length left, "
        "so nothing inside it shows either, which is why the last row is empty. Compose "
        "clips the content to the pill with `graphicsLayer { clip = true }`; Qt clips a "
        "child to its widget rectangle, so this port hides a child that has left the "
        "pill, which is the same reveal discretised. Recorded in docs/porting-todo.md."));
    {
        const QVector<QPair<QString, qreal>> rows = {
            {QStringLiteral("expand-1"), 1.0},
            {QStringLiteral("expand-05"), 0.5},
            {QStringLiteral("expand-0"), 0.0},
        };
        for (const auto &row : rows) {
            md::MdFloatingToolbar *bar = floating(row.first);
            bar->setOrientation(md::MdToolbarOrientation::Horizontal);
            bar->setColorScheme(md::MdToolbarColorScheme::Standard);
            bar->clearContentWidgets();
            for (int i = 0; i < glyphs.size(); ++i) {
                bar->addContentWidget(
                    iconButton(row.first + QStringLiteral("-i%1").arg(i), glyphs.at(i)));
            }
            bar->setLeadingWidget(iconButton(row.first + QStringLiteral("-lead"),
                                             QStringLiteral("arrow_back")));
            bar->setTrailingWidget(iconButton(row.first + QStringLiteral("-trail"),
                                              QStringLiteral("more_vert")));
            bar->setFab(nullptr);
            bar->setExpansionProgress(row.second);
            // The band uses the *expanded* height, which is what the widget's own
            // bounds are; only the pill inside gets shorter.
            m_slots.append(ToolbarSlot{bar, context.band(bar->sizeHint().height())});
            context.detail(L("expandedProgress = 1.0 / 0.5 / 0.0", "expandedProgress = 1.0 / 0.5 / 0.0"));
            context.space(14.0);
        }
    }

    // --- the action button ---------------------------------------------------
    context.section(L("旁边的动作按钮：展开 56dp，折叠 80dp",
                      "The action button beside it: 56dp expanded, 80dp collapsed"));
    context.paragraph(L(
        "带动作按钮时，控件自己的矩形就不再等于 pill 了：它按 "
        "`toolbarMaxWidth + ToolbarToFabGap + 56` 变宽，pill 只占剩下的一段，所以折叠时"
        "pill 的残骸会和 80dp 的按钮重叠 16dp——这是 Compose 的算式，不是画错了。按钮"
        "**随着工具栏收缩而变大**：56dp 配 level1 与 corner-large，80dp 配 level2 与 "
        "corner-large-increased。要说明的一点：这个库的 `MdFab` 只发布一个尺寸，所以它在 "
        "80dp 的框里仍然是 56dp 的圆——工具栏的几何是对的，缺的是 FAB 那一族的尺寸集，"
        "已记进 docs/porting-todo.md。",
        "With an action button the widget's rectangle is no longer the pill: it widens to "
        "`toolbarMaxWidth + ToolbarToFabGap + 56` and the pill occupies only the rest, "
        "which is why a collapsed pill's ruins overlap the 80dp button by 16dp — that is "
        "Compose's arithmetic, not a drawing mistake. All three rows below are fully "
        "expanded, where the action button is the 56dp one, so the 80dp box is not "
        "visible on this page; what pins it is "
        "`TestMd3Toolbar::floatingFabStripAndSizeSets`, which asserts the collapsed box "
        "is `(48, 0, 80, 80)` and that its leading edge really is inside the pill's "
        "trailing anchor. The button *grows as the toolbar shrinks*: 56dp with level1 "
        "and corner-large, 80dp with level2 and corner-large-increased. One caveat: this "
        "library's `MdFab` publishes a single size, so it stays a 56dp circle inside the "
        "80dp box — and that is why the growth would not be visible even in a collapsed "
        "row. The toolbar's geometry is right; what is missing is the FAB family's size "
        "set, recorded in docs/porting-todo.md."));
    {
        struct FabRow
        {
            const char *id;
            md::MdToolbarOrientation orientation;
            md::MdToolbarFabPosition position;
            const char *noteZh;
            const char *noteEn;
        };
        const QVector<FabRow> rows = {
            {"fab-end", md::MdToolbarOrientation::Horizontal, md::MdToolbarFabPosition::End,
             "横向 · 按钮在尾端（默认）", "Horizontal · button at the end (the default)"},
            {"fab-start", md::MdToolbarOrientation::Horizontal, md::MdToolbarFabPosition::Start,
             "横向 · 按钮在首端", "Horizontal · button at the start"},
            {"fab-bottom", md::MdToolbarOrientation::Vertical, md::MdToolbarFabPosition::End,
             "纵向 · 按钮在下方", "Vertical · button at the bottom"},
        };
        for (const FabRow &row : rows) {
            const QString id = QString::fromUtf8(row.id);
            md::MdFloatingToolbar *bar = floating(id);
            bar->setOrientation(row.orientation);
            bar->setColorScheme(md::MdToolbarColorScheme::Vibrant);
            bar->setLeadingWidget(nullptr);
            bar->setTrailingWidget(nullptr);
            bar->clearContentWidgets();
            for (int i = 0; i < 2; ++i) {
                bar->addContentWidget(
                    iconButton(id + QStringLiteral("-i%1").arg(i), glyphs.at(i)));
            }
            bar->setFab(fab(id + QStringLiteral("-fab"), QStringLiteral("add")));
            bar->setFabPosition(row.position);
            bar->setExpansionProgress(1.0);
            m_slots.append(ToolbarSlot{bar, context.band(bar->sizeHint().height())});
            context.detail(L(row.noteZh, row.noteEn));
            context.space(14.0);
        }
    }

    // --- what this page is pinned to ----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Toolbar 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Toolbar; moving one turns it "
        "red."));
    context.detail(L("docked: 64 / leading 16 / trailing 16 / max-spacing 32 / min-spacing 4 / "
                     "corner-none / surface-container（min-spacing 已发布但 Compose 从不读）",
                     "docked: 64 / leading 16 / trailing 16 / max-spacing 32 / min-spacing 4 / "
                     "corner-none / surface-container (min-spacing is published and never "
                     "read by Compose)"));
    context.detail(L("floating: 横向高 64 · 纵向宽 64 · 内容内衬 8 · between-space 4（本库读、"
                     "Compose 不读）· 外部边距 16（横）/ 24（纵）· level3 · corner-full",
                     "floating: 64 tall horizontally, 64 wide vertically / content padding 8 / "
                     "between-space 4 (read here, never in Compose) / external offset 16 "
                     "horizontal, 24 vertical / level3 / corner-full"));
    context.detail(L("floating FAB: between-space 8 · 展开 56（图标 24，corner-large，level1）· "
                     "折叠 80（图标 28，corner-large-increased，level2）",
                     "floating FAB: between-space 8 / expanded 56 (icon 24, corner-large, "
                     "level1) / collapsed 80 (icon 28, corner-large-increased, level2)"));
    context.detail(L("standard: 未选中 on-surface-variant，选中 secondary-container 上的 "
                     "on-secondary-container；vibrant 正好相反",
                     "standard: unselected on-surface-variant, selected on-secondary-container "
                     "on a secondary-container pill; vibrant is the exact inverse"));
    context.detail(L("工具栏没有 item 尺寸行（docked 36 + floating 75 行逐行核对）：项宽由子控件"
                     "决定，本库的 MdIconButton 阶梯是 32/40/56/96/136，没有 48，所以每条 pill "
                     "每项比规范算式窄 8dp",
                     "a toolbar has no item-size row (all 36 docked + 75 floating rows were "
                     "checked): the item width is the child's, and this library's MdIconButton "
                     "ladder is 32/40/56/96/136 with no 48, so every pill is 8dp narrower per "
                     "item than the spec's arithmetic"));
    context.detail(L("两种尺寸都走「摆容器不摆控件」：一个 40dp 的图标按钮是 55dp 的控件，"
                     "按 sizeHint 摆会每项多出 15dp",
                     "both sizes place containers, never widgets: a 40dp icon button is a 55dp "
                     "widget, so placing by sizeHint would add 15dp per item"));
    context.space(8.0);
}

void ToolbarPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ToolbarSlot &slot : m_slots) {
        // A toolbar that expands horizontally fills the band it was given; one
        // that hugs its content keeps its own size. Getting this wrong is
        // visible rather than subtle: a stretched floating toolbar keeps its
        // pill at the leading edge (the pill's length is the content's, not the
        // widget's) but pushes an end-positioned action button — and its whole
        // `End` arithmetic — out to the far edge of the band.
        const bool stretches =
            slot.widget->sizePolicy().horizontalPolicy() == QSizePolicy::Expanding;
        const QSize hint = slot.widget->sizeHint();
        const QSize size(stretches ? int(std::lround(slot.rect.width())) : hint.width(),
                         stretches ? int(std::lround(slot.rect.height())) : hint.height());
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(), size);
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void ToolbarPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ToolbarPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ToolbarPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ToolbarPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createToolbarPage()
{
    return new ToolbarPage;
}

} // namespace gallery

#include "GalleryPages29.moc"
