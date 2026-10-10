// Gallery page 38: Menus — `md.comp.menu.*` at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The embedded surface is a
// real MdMenu (shown as a plain child, never popped up), so its rows are the
// live component — the hover state layer, the press ripple and the disabled
// fade are all real.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * **the focus ring is inward** — the export's focus.indicator rows point
//     at the system *inner* offset, so a keyboard focus paints the secondary
//     ring inside the row, unlike every button family;
//   * **the state layer reads the unselected side for both selections** — the
//     export publishes one state-layer family;
//   * **the open animation hides the real children** — while the scale+alpha
//     flight runs, the surface paints them through Compose's graphicsLayer
//     transform; this page shows the settled state.

#include "GalleryPages.h"

#include "widgets/MdMenu.h"

#include "I18n.h"

#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct MenuSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class MenuPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit MenuPage(QWidget *parent = nullptr);
    ~MenuPage() override;

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

    md::MdMenu *surfaceMenu();
    md::MdMenuItem *rowItem(int index);

    md::MdMenu *m_menu = nullptr;
    QVector<md::MdMenuItem *> m_rows;
    QVector<MenuSlot> m_slots;
};

MenuPage::MenuPage(QWidget *parent)
    : GalleryPage(parent)
{
}

MenuPage::~MenuPage() = default;

/// The embedded surface is banked: build() is re-entrant, and recreating the
/// menu on every pass would leave the previous instance painting beneath.
md::MdMenu *MenuPage::surfaceMenu()
{
    if (m_menu != nullptr) {
        return m_menu;
    }
    // MdMenu is hard-wired as a Qt::Popup; embedding it as a plain child (the
    // way this page shows the settled surface) strips the flag.
    m_menu = new md::MdMenu(this);
    m_menu->setWindowFlags(Qt::Widget);
    m_menu->addItem(L("第一项", "First item"));
    m_menu->addItem(L("第二项（选中）", "Second item (selected)"));
    m_menu->items().at(1)->setSelected(true);
    m_menu->addDivider();
    m_menu->addItem(L("第三项", "Third item"));
    m_menu->addItem(L("不可用的第四项", "A disabled fourth item"));
    m_menu->items().at(3)->setEnabled(false);
    m_menu->show();
    return m_menu;
}

/// The three standalone rows are banked the same way.
md::MdMenuItem *MenuPage::rowItem(int index)
{
    if (index < m_rows.size()) {
        return m_rows.at(index);
    }
    const QVector<md::MdMenuItem *> fresh = {
        [this] {
            auto *item = new md::MdMenuItem(L("带前导图标", "With a leading icon"), this);
            item->setLeadingIconName(QStringLiteral("delete"));
            return item;
        }(),
        [this] {
            auto *item = new md::MdMenuItem(L("有下级", "Has a submenu"), this);
            item->setTrailingIconName(QStringLiteral("arrow_forward"));
            return item;
        }(),
        [this] {
            auto *item = new md::MdMenuItem(L("禁用", "Disabled"), this);
            item->setEnabled(false);
            return item;
        }(),
    };
    for (md::MdMenuItem *item : fresh) {
        item->show();
        m_rows.append(item);
    }
    return m_rows.at(index);
}

QString MenuPage::title() const
{
    return L("菜单", "Menu");
}

QString MenuPage::slug() const
{
    return QStringLiteral("menu");
}

QString MenuPage::subtitle() const
{
    return L(
        "菜单：surface-container 的 corner-extra-small 小圆角表面、level2 阴影、上下各 8dp "
        "内边距；行高 48dp、水平内边距 12dp、宽度夹在 112–280dp 之间。选中的行铺 "
        "secondary-container、内容 on-secondary-container；分隔符是 1dp 的 surface-variant，"
        "左右各缩进 12dp、上下各留 2dp。焦点环**朝内**——导出的 focus.indicator 行指向系统"
        "内偏移，键盘聚焦把 secondary 的环画在行内，与所有按钮族相反。打开动画是围绕锚角"
        "的 scale 0.8→1.0 + alpha 0→1。",
        "Menu: a surface-container corner-extra-small surface at level 2 with 8dp of "
        "vertical padding; 48dp rows, 12dp horizontal padding, the width clamped between "
        "112 and 280dp. The selected row fills secondary-container with "
        "on-secondary-container content; a divider is a 1dp surface-variant rule inset "
        "12dp on each side with 2dp above and below. The focus ring draws **inward** — "
        "the export's focus.indicator rows point at the system inner offset, so a "
        "keyboard focus paints the secondary ring inside the row, opposite to every "
        "button family. Opening animates scale 0.8→1.0 and alpha 0→1 around the anchor "
        "corner.");
}

void MenuPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- the surface ------------------------------------------------------------
    context.section(L("表面与行", "The surface and its rows"));
    context.paragraph(L(
        "一整块真实的 MdMenu（以普通子控件嵌入，不弹窗）：第二行选中——secondary-container "
        "的行块、on-secondary-container 的文字；第二、三行之间是 1dp 的 surface-variant "
        "分隔符，左右各缩进 12dp；最后一行禁用——on-surface @0.38。行是活的：悬停有 "
        "on-surface @0.08 的状态层，按下有 on-surface 的涟漪。",
        "A whole real MdMenu (embedded as a plain child, never popped up): the second row "
        "is selected — a secondary-container block with on-secondary-container text; "
        "between the second and third rows a 1dp surface-variant divider inset 12dp on "
        "each side; the last row is disabled — on-surface @0.38. The rows are live: "
        "hovering paints the on-surface @0.08 state layer, pressing ripples on-surface."));
    {
        md::MdMenu *menu = surfaceMenu();
        const QSize menuSize = menu->size();
        const QRectF band = context.band(menuSize.height());
        m_slots.append(MenuSlot{menu, QRectF(band.topLeft(), QSizeF(menuSize))});
        context.space(16.0);
    }

    // --- the rows themselves -------------------------------------------------------
    context.section(L("行自身的三种装束", "Three costumes of a row"));
    context.paragraph(L(
        "前导图标 24dp、与文字隔 8dp；级联菜单项的后箭头读 cascading-indicator 行"
        "（on-surface-variant）——它就是普通的 trailing-icon 槽；禁用行的文字与图标淡到 "
        "on-surface @0.38。图标不做任何交互抬升——导出的 hover/focus/pressed 图标行都是"
        "同一个颜色。",
        "A 24dp leading icon spaced 8dp from the text; a submenu's trailing arrow reads "
        "the cascading-indicator row (on-surface-variant) — it is the plain trailing-icon "
        "slot; a disabled row fades its text and icons to on-surface @0.38. Icons get no "
        "interaction lift at all — the export's hover/focus/pressed icon rows carry the "
        "same colour."));
    {
        const qreal pitch = 248.0;
        const QRectF band = context.band(48.0);
        qreal x = band.left();
        for (int index = 0; index < 3; ++index) {
            md::MdMenuItem *item = rowItem(index);
            item->resize(item->sizeHint());
            m_slots.append(MenuSlot{item, QRectF(x, band.top(), qreal(item->width()), 48.0)});
            x += pitch;
        }
        context.detail(L("带图标 · 级联箭头 · 禁用", "with icon · cascading arrow · disabled"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "以下每一条都由 TestMd3Menu 直接断言，改一处就会红。",
        "Every line below is asserted directly by TestMd3Menu; moving one turns it red."));
    context.detail(L("表面 surface-container · level2 阴影 · corner-extra-small · 上下内边距 "
                     "8dp · 行高 48dp · 水平内边距 12dp · 宽 112–280dp · 图标 24dp · 图文距 "
                     "8dp · label-large",
                     "a surface-container surface · level-2 shadow · corner-extra-small · "
                     "8dp vertical padding · 48dp rows · 12dp horizontal padding · width "
                     "112–280dp · 24dp icons · 8dp icon-text spacing · label-large"));
    context.detail(L("选中行：secondary-container 容器 + on-secondary-container 内容（导出只在"
                     " Enabled 发布选中行）· 禁用+选中回退到未选中的禁用行（容器透明）",
                     "the selected row: a secondary-container container with "
                     "on-secondary-container content (the export publishes selected rows at "
                     "Enabled only) · disabled+selected falls back to the unselected "
                     "disabled rows (a transparent container)"));
    context.detail(L("状态层读未选中一侧（导出只发布一族，on-surface；Compose 对选中行同样"
                     "施加指示层）· 悬停 0.08 / 焦点 0.12 / 按压 0.12，按下的层就是涟漪",
                     "the state layer reads the unselected side (the export publishes one "
                     "family, on-surface; Compose applies the same indication to selected "
                     "rows) · hover 0.08 / focus 0.12 / pressed 0.12, the pressed layer "
                     "being the ripple"));
    context.detail(L("分隔符 1dp surface-variant、左右 12dp、上下 2dp · 级联指示器 "
                     "on-surface-variant · 焦点环朝内：secondary、内偏移 3dp、粗细 3dp，只在"
                     "键盘焦点出现（Tab/Backtab/快捷键）",
                     "a 1dp surface-variant divider, 12dp inset each side, 2dp above and "
                     "below · the cascading indicator on-surface-variant · the focus ring "
                     "draws inward: secondary, inner offset 3dp, thickness 3dp, keyboard "
                     "focus only (Tab/Backtab/shortcuts)"));
    context.detail(L("打开动画：scale 0.8→1.0（快速空间弹簧）+ alpha 0→1（快速效果弹簧），"
                     "绕锚角（Below = 左上角）；飞行中真实子控件隐藏、由表面经变换渲染 · "
                     "Expressive 菜单族（Standard/Vibrant/Segmented）是独立令牌集，未移植",
                     "the open animation: scale 0.8→1.0 (fast spatial) + alpha 0→1 (fast "
                     "effects) around the anchor corner (Below = top-start); the real "
                     "children hide for the flight and render through the surface's "
                     "transform · the Expressive menu families (Standard/Vibrant/"
                     "Segmented) are separate token sets, not ported"));
    context.space(8.0);
}

void MenuPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const MenuSlot &slot : m_slots) {
        const QSize size = slot.widget->size();
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(), size);
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void MenuPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void MenuPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void MenuPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void MenuPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createMenuPage()
{
    return new MenuPage;
}

} // namespace gallery

#include "GalleryPages38.moc"
