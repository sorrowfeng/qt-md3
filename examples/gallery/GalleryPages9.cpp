// Gallery page 9: FAB menus — the sixth Actions family.
//
// Same rules as the other component pages: the page never styles anything,
// widgets are banked, and build() is re-entrant. The family-specific note:
// FAB menu is an M3 Expressive component — an anchor FAB, a close button
// that shares its top trailing corner, and up to six staggered list items;
// the token export publishes no motion rows, so the reveal is the Compose
// convention (spatial spring + 40 ms stagger), recorded in porting-todo.

#include "GalleryPages.h"

#include "widgets/MdFabMenu.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked FAB menu plus the content-local rect the latest build() gave it.
struct FabMenuSlot
{
    md::MdFabMenu *menu = nullptr;
    QRectF rect;
};

constexpr qreal kStackGap = 24.0;

} // namespace

class FabMenuPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit FabMenuPage(QWidget *parent = nullptr);
    ~FabMenuPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layoutChildren();
    void placeChildren();

    md::MdFabMenu *menu(const QString &id, const QString &anchorIcon);
    template <typename Configure>
    md::MdFabMenu *menu(const QString &id, const QString &anchorIcon, Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdFabMenu *widget = menu(id, anchorIcon);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdFabMenu *> m_bank;
    QVector<FabMenuSlot> m_slots;
};

FabMenuPage::FabMenuPage(QWidget *parent)
    : GalleryPage(parent)
{
}

FabMenuPage::~FabMenuPage() = default;

QString FabMenuPage::title() const
{
    return L("悬浮操作菜单", "FAB menus");
}

QString FabMenuPage::slug() const
{
    return QStringLiteral("FAB menus");
}

QString FabMenuPage::subtitle() const
{
    return L("M3 Expressive 新组件——锚定 FAB、共享顶角位的关闭按钮、最多六项的"
             "阶梯式展开列表。",
             "A new M3 Expressive component — an anchor FAB, a close button sharing its top "
             "trailing corner, and up to six list items staggering in.");
}

md::MdFabMenu *FabMenuPage::menu(const QString &id, const QString &anchorIcon)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFabMenu(anchorIcon, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void FabMenuPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- three colour groups -----------------------------------------------------
    context.section(L("三种颜色组 —— md.comp.fab-menu.<variant>",
                      "Three colour groups — md.comp.fab-menu.<variant>"));
    context.paragraph(L(
        "每个颜色组发布两份：关闭按钮用纯色（56 px、icon 20、corner-full、海拔 L3→悬停 L4），"
        "列表项用 container 色（56 px 高、icon 24、corner-full、海拔恒 L0）。点击锚点 FAB 展开，"
        "点击关闭按钮收回。",
        "Each colour group is published twice: the close button takes the pure colour (56 px, "
        "icon 20, corner-full, elevation L3 → hovered L4), the list items the container colour "
        "(56 px tall, icon 24, corner-full, elevation L0 throughout). Tap the anchor to expand, "
        "the close button to collapse."));
    {
        const QVector<QPair<QString, md::FabMenuVariant>> specs = {
            {QStringLiteral("Primary"), md::FabMenuVariant::Primary},
            {QStringLiteral("Secondary"), md::FabMenuVariant::Secondary},
            {QStringLiteral("Tertiary"), md::FabMenuVariant::Tertiary},
        };
        for (const auto &spec : specs) {
            auto *m = menu(QStringLiteral("variant-") + md::fabMenuVariantName(spec.second),
                           QStringLiteral("add"), [&spec](md::MdFabMenu *widget) {
                               widget->setVariant(spec.second);
                               widget->addItem(QStringLiteral("edit"), L("编辑", "Edit"));
                               widget->addItem(QStringLiteral("favorite"),
                                               L("收藏", "Favorite"));
                               widget->addItem(QStringLiteral("delete"), L("删除", "Delete"));
                           });
            const QSize hint = m->sizeHint();
            const QRectF bandRect = context.band(hint.height());
            m_slots.append(FabMenuSlot{
                m, QRectF(bandRect.left(), bandRect.top(), hint.width(), hint.height())});
        }
        context.space(kStackGap);
    }

    // --- up to six items ---------------------------------------------------------
    context.section(L("最多六个列表项", "Up to six list items"));
    context.paragraph(L(
        "规范页写明菜单最多六项；列表项间距 4 px，关闭按钮与首项之间 8 px。展开动画从 FAB 的"
        "top-trailing 角向下阶梯浮现——令牌导出没有动效行，这里用 Compose M3 Expressive 的 "
        "spatial spring 加每项 40 ms 的 stagger（记录在 porting-todo）。",
        "The spec page caps the menu at six items; items are 4 px apart, 8 px below the close "
        "button. The open animation staggers down from the FAB's top trailing edge — the token "
        "export publishes no motion rows, so this reveal uses the Compose M3 Expressive spatial "
        "spring with a 40 ms per-item stagger (recorded in porting-todo)."));
    {
        auto *m = menu(QStringLiteral("six-items"), QStringLiteral("add"), [](md::MdFabMenu *w) {
            w->setVariant(md::FabMenuVariant::Secondary);
            w->addItem(QStringLiteral("edit"), L("编辑", "Edit"));
            w->addItem(QStringLiteral("chat"), L("消息", "Message"));
            w->addItem(QStringLiteral("mail"), L("邮件", "Mail"));
            w->addItem(QStringLiteral("calendar_today"), L("日程", "Event"));
            w->addItem(QStringLiteral("location_on"), L("位置", "Place"));
            w->addItem(QStringLiteral("phone"), L("电话", "Call"));
        });
        m->setExpanded(true);
        const QSize hint = m->sizeHint();
        const QRectF bandRect = context.band(hint.height());
        m_slots.append(
            FabMenuSlot{m, QRectF(bandRect.left(), bandRect.top(), hint.width(), hint.height())});
        context.space(kStackGap);
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-fab-menu{,-<variant>"
        "-container,-<variant>-close-button}.scss"));
    context.detail(QStringLiteral(
        "close    56 x 56 · icon 20 · corner-full · L3 resting / L4 hovered"));
    context.detail(QStringLiteral(
        "item     56 tall · icon 24 · 24 / 8 / 24 rhythm · corner-full · L0 in every state · "
        "title-medium label"));
    context.detail(QStringLiteral(
        "gaps     close-button.between-space 8 · menu-item.between-space 4 · up to six items"));
    context.detail(QStringLiteral(
        "motion   no rows in the export — reveal = Compose SpatialDefault spring + 40 ms/item "
        "stagger (recorded, not a token fact)"));
    context.detail(QStringLiteral(
        "disabled container on-surface @ 0.12 · content on-surface @ 0.38 · L0 (spec table; the "
        "export publishes no disabled rows)"));
    context.space(8.0);
    context.detail(L("TestMd3FabMenu 将两个元素的令牌行、三色组、间距与规范填入的 disabled 行"
                     "逐字段锁定。",
                     "TestMd3FabMenu pins both elements' token rows, all three colour groups, "
                     "the spacing and the spec-filled disabled row field by field."));
}

void FabMenuPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const FabMenuSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.menu->geometry() != target) {
            slot.menu->setGeometry(target);
        }
    }
}

void FabMenuPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void FabMenuPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void FabMenuPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void FabMenuPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createFabMenuPage()
{
    return new FabMenuPage;
}

} // namespace gallery

#include "GalleryPages9.moc"
