// Gallery page 27: List — `md.comp.list.*` (the container) and
// `md.comp.list.list-item.*` (the items).
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Lists are real MdList
// widgets placed in bands; every band is as tall as the list's own sizeHint,
// so the page never has to know the 56 / 72 / 88 arithmetic.
//
// One thing the page is explicit about, because it is easy to misread as a
// bug: a list item's container colour *is* the surface, so a resting item's
// shape is invisible on a surface-coloured list — in this port and in
// material-web alike. The corner ladder is only visible where the container
// differs from the surface, which is why the demos below lean on the selected,
// dragged and disabled states.

#include "GalleryPages.h"

#include "widgets/MdList.h"
#include "widgets/MdListItem.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

namespace gallery {

namespace {

struct ListSlot
{
    md::MdList *list = nullptr;
    QRectF rect;
};

} // namespace

class ListPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ListPage(QWidget *parent = nullptr);
    ~ListPage() override;

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

    /// The banked list for `id`, created empty on first use. build() runs twice
    /// (once measuring, once painting), so everything must be created once and
    /// only reconfigured afterwards.
    md::MdList *list(const QString &id);
    /// The banked item for `id` inside `owner`. The item is added to the list
    /// exactly once, on creation.
    md::MdListItem *item(const QString &id, md::MdList *owner, const QString &headline,
                         bool interactive = true);

    QHash<QString, md::MdList *> m_lists;
    QHash<QString, md::MdListItem *> m_items;
    QVector<ListSlot> m_slots;
};

ListPage::ListPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ListPage::~ListPage() = default;

QString ListPage::title() const
{
    return L("列表", "List");
}

QString ListPage::slug() const
{
    return QStringLiteral("list");
}

QString ListPage::subtitle() const
{
    return L(
        "md.comp.list：列表容器（container.color / .shape、上下各 8px 内边距、"
        "分段列表 2px 间隙）加上 md.comp.list.list-item.* 的条目族。行数是**推导**"
        "出来的，不能设置：只有标题=1 行，有 overline 或 supporting=2 行，两者都有"
        "或 supporting 折行=3 行，容器高度取 56/72/88 的令牌下限。风格是视觉选择"
        "而非行为选择——Standard 恒为直角，Expressive 随状态形变（静止 extra-"
        "small 4、悬停 medium 12、聚焦/按压/选中/拖拽 large 16）。",
        "md.comp.list: the container (container.color / .shape, the 8px vertical "
        "padding, the 2px segmented gap) plus the md.comp.list.list-item.* family. "
        "The line count is *derived*, never set: 1 line with only a headline, 2 "
        "with an overline or a supporting text, 3 with both or with a supporting "
        "text that wraps, and the container height takes the token floor of "
        "56/72/88. The style is a visual choice rather than a behaviour one — "
        "Standard is square in every state, Expressive morphs with the state "
        "(extra-small 4 at rest, medium 12 hovered, large 16 focused / pressed / "
        "selected / dragged).");
}

md::MdList *ListPage::list(const QString &id)
{
    const auto found = m_lists.constFind(id);
    if (found != m_lists.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdList(this);
    m_lists.insert(id, created);
    return created;
}

md::MdListItem *ListPage::item(const QString &id, md::MdList *owner, const QString &headline,
                               bool interactive)
{
    const auto found = m_items.constFind(id);
    if (found != m_items.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdListItem(headline, owner);
    created->setInteractive(interactive);
    owner->addItem(created);
    m_items.insert(id, created);
    return created;
}

void ListPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- Standard vs Expressive ---------------------------------------------
    context.section(L("两种风格：Standard 与 Expressive", "The two styles: Standard and Expressive"));
    context.paragraph(L(
        "两列同内容、同状态，唯一变量是风格。每列第二项为选中态，因为选中态的容器"
        "是 secondary-container——只有容器色与页面表面不同时，形状才看得见。"
        "Standard 选中项仍是直角；Expressive 选中项取 large（16dp）。",
        "Two lists, same content and same states, with the style as the only "
        "variable. The second item of each is selected, because the selected "
        "container is secondary-container — a shape is only visible when the "
        "container differs from the page surface. The Standard item stays square; "
        "the Expressive one takes corner-large (16dp)."));
    {
        md::MdList *standard = list(QStringLiteral("standard"));
        md::MdListItem *a = item(QStringLiteral("std-1"), standard, L("收件箱", "Inbox"));
        a->setLeadingIcon(QStringLiteral("home"));
        a->setTrailingIcon(QStringLiteral("expand_more"));
        md::MdListItem *b = item(QStringLiteral("std-2"), standard, L("已加星标", "Starred"));
        b->setLeadingIcon(QStringLiteral("favorite"));
        b->setTrailingSupporting(QStringLiteral("12"));
        b->setSelected(true);
        md::MdListItem *c = item(QStringLiteral("std-3"), standard, L("废纸篓", "Trash"));
        c->setLeadingIcon(QStringLiteral("delete"));
        m_slots.append(ListSlot{standard, context.band(standard->sizeHint().height())});

        context.space(16.0);

        md::MdList *expressive = list(QStringLiteral("expressive"));
        md::MdListItem *d = item(QStringLiteral("exp-1"), expressive, L("收件箱", "Inbox"));
        d->setLeadingIcon(QStringLiteral("home"));
        d->setTrailingIcon(QStringLiteral("expand_more"));
        md::MdListItem *e = item(QStringLiteral("exp-2"), expressive, L("已加星标", "Starred"));
        e->setLeadingIcon(QStringLiteral("favorite"));
        e->setTrailingSupporting(QStringLiteral("12"));
        e->setSelected(true);
        md::MdListItem *f = item(QStringLiteral("exp-3"), expressive, L("废纸篓", "Trash"));
        f->setLeadingIcon(QStringLiteral("delete"));
        for (md::MdListItem *each : {d, e, f}) {
            each->setVariant(md::MdListVariant::Expressive);
        }
        m_slots.append(ListSlot{expressive, context.band(expressive->sizeHint().height())});
    }

    // --- the line count -------------------------------------------------------
    context.section(L("行数是推导出来的（56 / 72 / 88）", "The line count is derived (56 / 72 / 88)"));
    context.paragraph(L(
        "自上而下：只有标题=1 行（56dp）；加了 supporting=2 行（72dp）；再加 "
        "overline=3 行（88dp）。令牌高度是下限而非上限——supporting 折行时条目会"
        "长过 88dp，而折行本身就把条目判成 3 行。",
        "Top to bottom: headline only = 1 line (56dp); adding a supporting text = 2 "
        "lines (72dp); adding an overline as well = 3 lines (88dp). The token "
        "heights are floors, not caps — a wrapping supporting text grows the item "
        "past 88dp, and wrapping is itself what makes it a three-line item."));
    {
        const QStringList ids = {QStringLiteral("row-1"), QStringLiteral("row-2"),
                                 QStringLiteral("row-3")};
        const QStringList headlines = {L("一行：只有标题", "One line: headline only"),
                                       L("两行：加 supporting", "Two lines: + supporting"),
                                       L("三行：再加 overline", "Three lines: + overline")};
        const QStringList supportings = {QString(), QStringLiteral("supporting 正文"),
                                         QStringLiteral("supporting 正文")};
        const QStringList overlines = {QString(), QString(), QStringLiteral("OVERLINE")};
        md::MdList *rows = list(QStringLiteral("lines"));
        for (int i = 0; i < ids.size(); ++i) {
            md::MdListItem *row = item(ids.at(i), rows, headlines.at(i));
            row->setSupporting(supportings.at(i));
            row->setOverline(overlines.at(i));
            row->setLeadingIcon(QStringLiteral("calendar_today"));
        }
        m_slots.append(ListSlot{rows, context.band(rows->sizeHint().height())});
    }

    // --- the slots -------------------------------------------------------------
    context.section(L("槽位：avatar / image / 图标", "Slots: avatar / image / icons"));
    context.paragraph(L(
        "leading 槽可以放头像（40dp、corner-full、primary-container 底）、图像"
        "（56×56）或图标（24dp，Expressive 为 20dp）；trailing 槽可放图标、"
        "label-small 的元信息文字或任意控件。Expressive 下图像槽取 small 圆角，"
        "Standard 下是直角——`leading-image.shape` 与 `.expressive.shape` 的区别"
        "在这里是看得见的。",
        "The leading slot holds an avatar (40dp, corner-full, on a primary-container "
        "disc), an image (56x56) or an icon (24dp, 20dp in the Expressive style); "
        "the trailing slot holds an icon, label-small meta text, or any widget. In "
        "the Expressive style the image slot takes the small corner where Standard "
        "leaves it square — the `leading-image.shape` versus `.expressive.shape` "
        "difference is visible here."));
    {
        // `slots` is a Qt keyword macro, so the local is named `slotList`.
        md::MdList *slotList = list(QStringLiteral("slots"));
        md::MdListItem *avatar =
            item(QStringLiteral("slot-1"), slotList, L("林小满", "Avery Chen"));
        avatar->setLeadingKind(md::MdListItem::LeadingKind::Avatar);
        avatar->setAvatarLabel(QStringLiteral("林"));
        avatar->setSupporting(L("在线", "Available"));
        avatar->setTrailingSupporting(L("12 分钟", "12 min"));

        md::MdListItem *image =
            item(QStringLiteral("slot-2"), slotList, L("图像槽 56×56", "Image slot 56x56"));
        image->setLeadingKind(md::MdListItem::LeadingKind::Image);
        image->setSupporting(L("Expressive：small 圆角", "Expressive: corner-small"));

        md::MdListItem *icons =
            item(QStringLiteral("slot-3"), slotList, L("图标 + 元信息", "Icon + meta"));
        icons->setLeadingIcon(QStringLiteral("chat"));
        icons->setTrailingIcon(QStringLiteral("expand_more"));

        for (md::MdListItem *each : {avatar, image, icons}) {
            each->setVariant(md::MdListVariant::Expressive);
        }
        m_slots.append(ListSlot{slotList, context.band(slotList->sizeHint().height())});
    }

    // --- segmented -------------------------------------------------------------
    context.section(L("分段列表（segmented）", "Segmented lists"));
    context.paragraph(L(
        "分段列表把条目间距换成 2px，并把 index/count 交给每个条目：首个条目的"
        "**上**对角、末个条目的**下**对角改用列表自身的 container.shape"
        "（corner-large），单个条目四条角全改。这里的中间两项选中，2px 间隙在"
        "两个 secondary-container 之间直接可读。",
        "A segmented list swaps the item spacing for 2px and hands each item its "
        "index/count: the first item's *top* corner pair and the last item's "
        "*bottom* pair take the list's own container.shape (corner-large), while a "
        "lone item takes all four. The middle two items here are selected, so the "
        "2px gap reads directly between two secondary-containers."));
    {
        md::MdList *segmented = list(QStringLiteral("segmented"));
        segmented->setSegmented(true);
        segmented->setSelectionMode(md::MdList::SelectionMode::Multiple);
        const QStringList headlines = {L("日", "Day"), L("周", "Week"), L("月", "Month"),
                                       L("年", "Year")};
        for (int i = 0; i < headlines.size(); ++i) {
            md::MdListItem *each =
                item(QStringLiteral("seg-%1").arg(i), segmented, headlines.at(i));
            each->setVariant(md::MdListVariant::Expressive);
            each->setSelected(i == 1 || i == 2);
        }
        m_slots.append(ListSlot{segmented, context.band(segmented->sizeHint().height())});
    }

    // --- the interaction states ------------------------------------------------
    context.section(L("交互态与形状阶梯", "The interaction states and the shape ladder"));
    context.paragraph(L(
        "自上而下：选中（secondary-container、large）、拖拽（on-surface 0.16 状态层、"
        "large）、选中且禁用（on-surface 以 0.38 覆在 selected 容器上）、禁用"
        "（容器不变，仅内容淡出 0.38）。静止与悬停的条目在这里看不见形状——容"
        "器色就是页面表面色，material-web 亦然；这正是上面各页演示都靠选中/拖拽/"
        "禁用态的原因。`dragged.container.elevation`（level4）本页不画阴影：条目"
        "的容器就是它整个矩形，阴影会被 Qt 裁掉，Compose 是靠浮层渲染拖拽项的。",
        "Top to bottom: selected (secondary-container, corner-large), dragged (an "
        "on-surface state layer at 0.16, corner-large), selected and disabled "
        "(on-surface at 0.38 over the selected container) and disabled (container "
        "unchanged, content faded to 0.38). A resting or hovered item shows no "
        "shape here — its container colour *is* the page surface, exactly as in "
        "material-web, which is why every demo above leans on the selected, "
        "dragged and disabled states. `dragged.container.elevation` (level4) casts "
        "no shadow on this page: a list item's container is its whole rect, so the "
        "shadow would be clipped away by Qt — Compose renders a dragged item in an "
        "overlay instead."));
    {
        md::MdList *states = list(QStringLiteral("states"));
        const QStringList headlines = {L("选中", "Selected"), L("拖拽中", "Dragging"),
                                       L("选中且禁用", "Selected, disabled"),
                                       L("禁用", "Disabled")};
        QVector<md::MdListItem *> rows;
        for (int i = 0; i < headlines.size(); ++i) {
            md::MdListItem *each =
                item(QStringLiteral("state-%1").arg(i), states, headlines.at(i));
            each->setVariant(md::MdListVariant::Expressive);
            each->setLeadingIcon(QStringLiteral("done"));
            rows.append(each);
        }
        rows.at(0)->setSelected(true);
        rows.at(1)->setDragged(true);
        rows.at(2)->setSelected(true);
        rows.at(2)->setEnabled(false);
        rows.at(3)->setEnabled(false);
        m_slots.append(ListSlot{states, context.band(states->sizeHint().height())});
    }

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-list.scss (34.0.21) "
        "+ list/internal/_list.scss + list/internal/listitem/_list-item.scss "
        "+ Compose M3 ListItem.kt / ListItemDefaults.kt / tokens/ListTokens.kt"));
    context.detail(QStringLiteral(
        "export      one/two/three-line 56/72/88 · top/bottom-space 10 · "
        "leading/trailing-space 16 · between-space 12 · segmented.gap 2 · "
        "leading-icon 24 (expressive 20) · avatar 40 corner-full "
        "primary-container · image 56x56 · video 100x56, large 114x64"));
    context.detail(QStringLiteral(
        "shapes      container.shape corner-none · container.expressive.shape "
        "corner-extra-small · hovered corner-medium · focused/pressed/selected/"
        "dragged corner-large · disabled corner-extra-small · "
        "selected.disabled corner-large · list container.shape corner-large"));
    context.detail(QStringLiteral(
        "colours     container surface · label body-large on-surface · leading/"
        "trailing icon on-surface-variant · overline label-small · supporting "
        "body-medium · trailing-supporting label-small · selected container "
        "secondary-container · state layers on-surface 0.08/0.12/0.12/0.16 · "
        "disabled 0.38 (state layer 0.1, selected container 0.38)"));
    context.detail(QStringLiteral(
        "specs       content centred until 88dp, top-aligned from there · leading "
        "icon always top-aligned 8dp (12dp at 88dp+) · label left 16 · leading "
        "left 16 · trailing right 24 (token 16 wins) · 48dp targets · "
        "\"unselected 4 inner / 16 outer, selected 16 all round\""));
    context.detail(L(
        "divergences (1) between-space 12（令牌）而非 material-web SCSS 的 16；"
        "(2) trailing 右内边距取令牌 16 而非 spec 的 24；(3) 8px 容器内边距不是"
        "令牌行，来自 _list.scss；(4) 不画拖拽阴影（需浮层宿主）；(5) 焦点环跟随"
        "容器圆角，而 material-web 硬编码 8px。全部记录在 docs/porting-todo.md。",
        "divergences (1) between-space 12 (the token) rather than the 16 hardcoded "
        "in material-web's SCSS; (2) the trailing right padding is the token's 16, "
        "not the spec's 24; (3) the 8px container padding is not a token row — it "
        "comes from _list.scss; (4) no dragged shadow is painted (it needs an "
        "overlay host); (5) the focus ring follows the container's corners where "
        "material-web hardcodes 8px. All five are recorded in docs/porting-todo.md."));
    context.space(8.0);
    context.detail(L(
        "TestMd3List 将条目与容器的 token 表、行数规则、三种高度、Standard/Expressive "
        "的形状阶梯、88dp 对齐规则、分段外角、交互状态层与选中配色、列表选择模式、"
        "键盘导航（含绕回）与渲染冒烟逐字段锁定。",
        "TestMd3List pins the item and container token tables, the line-count rule, "
        "the three heights, the Standard/Expressive shape ladder, the 88dp "
        "alignment rules, the segmented outer corners, the interaction state "
        "layers and selection colours, the list selection modes, the keyboard "
        "navigation (wrapping included) and the render smoke field by field."));
}

void ListPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ListSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.list->geometry() != target) {
            slot.list->setGeometry(target);
        }
    }
}

void ListPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ListPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ListPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ListPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createListPage()
{
    return new ListPage;
}

} // namespace gallery

#include "GalleryPages27.moc"
