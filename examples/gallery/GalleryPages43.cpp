// Gallery page 43: Search — `md.comp.search-bar.*` + `md.comp.search-view.*`
// at export 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every bar on this page is
// a live MdSearchBar — the 56 px pill, the leading search icon, the state
// layer and the activation signal are all the real component.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * **the bar is the entry point** — clicking emits `activated()`, the host
//     expands it into the search view;
//   * **three surfaces share one structure** — bar (corner-full), docked view
//     (corner-extra-large) and full-screen view (corner-none) differ only in
//     shape and container height.

#include "GalleryPages.h"

#include "core/MdSearchTokens.h"
#include "widgets/MdSearchBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct SearchSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class SearchPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SearchPage(QWidget *parent = nullptr);
    ~SearchPage() override;

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

    md::MdSearchBar *sb(const QString &id);

    QHash<QString, md::MdSearchBar *> m_bars;
    QVector<SearchSlot> m_slots;
};

SearchPage::SearchPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SearchPage::~SearchPage() = default;

QString SearchPage::title() const
{
    return L("搜索", "Search");
}

QString SearchPage::slug() const
{
    return QStringLiteral("search");
}

QString SearchPage::subtitle() const
{
    return L(
        "搜索栏：56dp 药丸（surface-container-high @ level3、corner-full），24dp 前置搜索"
        "图标、body-large 输入行、尾部动作槽。点击发出 activated() —— 宿主展开成搜索视图。"
        "三个面共享一个结构：栏（corner-full）、停靠视图（corner-extra-large）、全屏视图"
        "（corner-none），只差形状与容器高。Enter 发出 searchRequested(text)。",
        "Search bar: a 56dp pill (surface-container-high @ level3, corner-full) with a "
        "24dp leading search icon, the body-large input line and the trailing action "
        "slot. Clicking emits activated() — the host expands it into the search view. "
        "Three surfaces share one structure: bar (corner-full), docked view "
        "(corner-extra-large) and full-screen view (corner-none), differing only in "
        "shape and container height. Enter emits searchRequested(text).");
}

md::MdSearchBar *SearchPage::sb(const QString &id)
{
    const auto found = m_bars.constFind(id);
    if (found != m_bars.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSearchBar(this);
    created->hide();
    m_bars.insert(id, created);
    return created;
}

void SearchPage::build(GalleryContext &context)
{
    m_slots.clear();

    auto addRow = [&](const QVector<QPair<QString, md::MdSearchBar *>> &bars,
                      qreal width = 360.0) {
        if (bars.isEmpty()) {
            return;
        }
        const qreal pitch = width + 24.0;
        const qreal bandHeight = 56.0;
        const QRectF band = context.band(bandHeight);
        const qreal span = (bars.size() - 1) * pitch + width;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : bars) {
            m_slots.append(SearchSlot{entry.second,
                                      QRectF(x, band.top(), width, bandHeight)});
            x += pitch;
        }
    };

    // --- the bar ---------------------------------------------------------
    context.section(L("搜索栏", "The search bar"));
    context.paragraph(L(
        "56dp 药丸、corner-full、surface-container-high @ level3。前置 24dp 搜索图标，"
        "输入行 body-large。悬停/按压时 on-surface 状态层（0.08 / 0.12）。",
        "A 56dp pill, corner-full, surface-container-high @ level3. A 24dp leading search "
        "icon, the input line body-large. The on-surface state layer shows under hover / "
        "press (0.08 / 0.12)."));
    {
        auto *bar = sb(QStringLiteral("basic"));
        bar->setPlaceholderText(QStringLiteral("Search"));
        addRow({{QStringLiteral("a"), bar}});
        context.detail(L("静止态", "at rest"));
        context.space(16.0);
    }

    // --- the three surfaces ------------------------------------------------
    context.section(L("三个面", "The three surfaces"));
    context.paragraph(L(
        "栏（corner-full）、停靠视图（corner-extra-large、surface-container-low 背景）、"
        "全屏视图（corner-none、72dp 头部）。只差形状与容器高，其余共享。",
        "Bar (corner-full), docked view (corner-extra-large on surface-container-low) "
        "and full-screen view (corner-none with a 72dp header). Only the shape and the "
        "container height differ; everything else is shared."));
    {
        auto *bar = sb(QStringLiteral("surface-bar"));
        bar->setPlaceholderText(QStringLiteral("Search"));
        auto *docked = sb(QStringLiteral("surface-docked"));
        docked->setSurface(md::MdSearchSurface::DockedView);
        docked->setPlaceholderText(QStringLiteral("Search"));
        auto *fullScreen = sb(QStringLiteral("surface-fullscreen"));
        fullScreen->setSurface(md::MdSearchSurface::FullScreenView);
        fullScreen->setPlaceholderText(QStringLiteral("Search"));
        addRow({{QStringLiteral("a"), bar},
                {QStringLiteral("b"), docked},
                {QStringLiteral("c"), fullScreen}},
               220.0);
        context.detail(L("栏 �� 停靠视图 �� 全屏视图", "bar �� docked �� full-screen"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页钉住的 token 事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "下面每一行都由 TestMd3SearchBar 直接断言，动一行就变红。",
        "Every line below is asserted directly by TestMd3SearchBar; moving one turns it "
        "red."));
    context.detail(L("容器 56dp（全屏 72dp）�� 图标 24dp �� 侧距 16dp �� 头像 30dp / 目标 "
                     "48dp �� 状态层 0.08 / 0.12",
                     "the container 56dp (72dp full-screen) �� icons 24dp �� side spaces "
                     "16dp �� the avatar 30dp / target 48dp �� the state layer 0.08 / "
                     "0.12"));
    context.detail(L("三面形状：栏 corner-full �� 停靠 corner-extra-large �� 全屏 "
                     "corner-none �� 停靠/全屏背景 surface-container-low、分隔线 outline",
                     "the three shapes: bar corner-full �� docked corner-extra-large �� "
                     "full-screen corner-none �� the docked/full-screen background "
                     "surface-container-low, the divider outline"));
    context.detail(L("点击发出 activated() �� Enter 发出 searchRequested(text) �� "
                     "surface-tint-layer-color 携带不画（海拔走 MdElevation）",
                     "clicking emits activated() �� Enter emits searchRequested(text) �� "
                     "surface-tint-layer-color carried, not drawn (the elevation paints "
                     "through MdElevation)"));
    context.space(8.0);
}

void SearchPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SearchSlot &slot : m_slots) {
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(),
                           slot.rect.size().toSize());
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
    }
}

void SearchPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SearchPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SearchPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SearchPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSearchPage()
{
    return new SearchPage;
}

} // namespace gallery

#include "GalleryPages43.moc"
