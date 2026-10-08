// Gallery page 25: Carousel — `md.comp.carousel-item.*`, the multi-browse
// keyline model.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The static snapshots are
// fixed-size MdCarousel instances placed in bands at two scroll offsets, so
// the keyline interpolation's resize-as-it-scrolls deformation is visible
// between them.

#include "GalleryPages.h"

#include "widgets/MdCarousel.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>

namespace gallery {

namespace {

constexpr int kCarouselHeight = 180;

QWidget *makeItemContent(const QColor &color, const QString &label, QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setAutoFillBackground(true);
    QPalette palette = frame->palette();
    palette.setColor(QPalette::Window, color);
    frame->setPalette(palette);
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(12, 12, 12, 12);
    auto *text = new QLabel(label, frame);
    text->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    layout->addWidget(text);
    return frame;
}

} // namespace

class CarouselPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit CarouselPage(QWidget *parent = nullptr);
    ~CarouselPage() override;

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

    md::MdCarousel *carousel(const QString &id, qreal scrollOffset);

    QHash<QString, md::MdCarousel *> m_bank;
    struct CarouselSlot
    {
        md::MdCarousel *carousel = nullptr;
        QRectF rect;
    };
    QVector<CarouselSlot> m_slots;
};

CarouselPage::CarouselPage(QWidget *parent)
    : GalleryPage(parent)
{
}

CarouselPage::~CarouselPage() = default;

QString CarouselPage::title() const
{
    return L("Carousel", "Carousel");
}

QString CarouselPage::slug() const
{
    return QStringLiteral("carousel");
}

QString CarouselPage::subtitle() const
{
    return L(
        "md.comp.carousel-item：multi-browse 键线模型——开头一个大项，其后中、小项，"
        "项随滚动在槽位间重塑宽度。容器 surface、level0（hover level1）、corner-"
        "extra-large（28）恒定圆角；项间距 8、小项 40–56、锚 10。数学是 Compose M3 "
        "carousel 包的移植：Arrangement 以最小代价解出大项尺寸（中项 = (大+小)/2，"
        "±10% flex 吸收余量），键线按 unadjusted 轴插值出每项的可见宽度与位置，"
        "拖拽松手 snap 到最近的项边界。",
        "md.comp.carousel-item: the multi-browse keyline model — a large item at the "
        "start, medium and small items behind it, each item resizing as it scrolls "
        "through the slots. Surface container, level0 (level1 on hover), the constant "
        "corner-extra-large (28) radius; 8 px spacing, 40-56 px small items, 10 px "
        "anchors. The math is the port of the Compose M3 carousel package: the "
        "Arrangement solves the large size at the lowest cost (medium = (large + "
        "small)/2, the +/-10% flex absorbing the rest), the keylines interpolate each "
        "item's visible width and position along the unadjusted axis, and a drag "
        "settles to the nearest item boundary.");
}

md::MdCarousel *CarouselPage::carousel(const QString &id, qreal scrollOffset)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdCarousel(this);
    const QColor tones[] = {
        QColor(0xD0, 0xBC, 0xFF), QColor(0xFF, 0xD8, 0xE4), QColor(0xB9, 0xF2, 0xCA),
        QColor(0xE8, 0xDE, 0xF5), QColor(0xFD, 0xE4, 0xB2), QColor(0xD5, 0xE3, 0xFF),
    };
    for (int i = 0; i < 6; ++i) {
        created->addItem(makeItemContent(tones[i], QStringLiteral("Item %1").arg(i + 1),
                                         created));
    }
    created->scrollTo(scrollOffset);
    m_bank.insert(id, created);
    return created;
}

void CarouselPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- at rest -----------------------------------------------------------------
    context.section(L("静置形态（scroll offset 0）", "At rest (scroll offset 0)"));
    context.paragraph(L(
        "第一屏按 Arrangement 解出的槽位排布：本宽度下代价函数选出一大一小"
        "（大项 + 8 间距 + 小项恰好填满容器，中项行放不下被放弃——这正是 "
        "findLowestCostArrangement 的判定，中项槽位在滚动中出现）。大项左缘贴"
        "容器起点，小项沿锚线部分裁出右缘。",
        "The first screen arranges the slots the Arrangement solved: at this width the "
        "cost function picks one large and one small (large + 8 spacing + small fills "
        "the container exactly; the medium row cannot fit and loses the cost "
        "comparison — findLowestCostArrangement's verdict, with the medium slot "
        "appearing while scrolling). The large item's edge sits flush with the start; "
        "the small item rides the anchor, partially cut off."));
    {
        md::MdCarousel *rest = carousel(QStringLiteral("rest"), 0.0);
        const QRectF band = context.band(kCarouselHeight);
        m_slots.append(CarouselSlot{rest, band});
    }

    // --- mid-scroll ----------------------------------------------------------------
    context.section(L("滚动中（键线插值重塑项宽）", "Mid-scroll (keyline interpolation)"));
    context.paragraph(L(
        "滚过 210 px：大项沿键线曲线缩小、小项放大上移——项的可见宽度是相邻键线"
        "按 unadjusted 轴插值的结果，这是 carousel 区别于横向列表的标志性形变。",
        "Scrolled 210 px: the large item shrinks along the keyline curve, the small "
        "grows toward it — each item's visible width is the interpolation of its two "
        "surrounding keylines along the unadjusted axis, the signature deformation "
        "that separates a carousel from a horizontal list."));
    {
        md::MdCarousel *mid = carousel(QStringLiteral("mid"), 210.0);
        const QRectF band = context.band(kCarouselHeight);
        m_slots.append(CarouselSlot{mid, band});
    }

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-carousel-item.scss "
        "(34.0.21) + Compose M3 carousel package Carousel.kt / Strategy.kt / "
        "Keylines.kt / KeylineList.kt / Arrangement.kt (no web component published)"));
    context.detail(QStringLiteral(
        "export      container surface level0 corner-extra-large (28) · hover level1 · "
        "with-outline 1 px outline · state layers on-surface 0.08/0.12/0.12 · disabled "
        "0.38 container / 0.12 outline · focus indicator secondary (recorded)"));
    context.detail(QStringLiteral(
        "compose     small 40-56 (large/3 clamped) · anchor 10 · medium flex 0.1 · "
        "medium-large diff threshold 0.85 (recorded) · snap = Pager page boundaries"));
    context.detail(QStringLiteral(
        "specs       item spacing 8 (Compose defaults 0 — spec wins, divergence "
        "recorded) · leading/trailing padding 16 (the caller's margin) · cross padding "
        "8 · corner radius 28"));
    context.space(8.0);
    context.detail(L("TestMd3Carousel 将 token 行、覆盖解析、Arrangement 数学"
                     "（fit/lowest-cost/理想拟合）、multiBrowseKeylineList（对齐 "
                     "Compose 自测的 380/186/8 期望值）、项插值、最大滚动、σ 分段、"
                     "widget 布局/拖拽 snap 与渲染烟测逐字段锁定。",
                     "TestMd3Carousel pins the token rows, the override parsing, the "
                     "Arrangement math (fit / lowest-cost / the ideal fit), "
                     "multiBrowseKeylineList (against Compose's own 380/186/8 test "
                     "expectations), the item interpolation, the max scroll, the sigma "
                     "piecewise ranges, the widget layout/drag snap and the render smoke "
                     "field by field."));
}

void CarouselPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const CarouselSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.carousel->geometry() != target) {
            slot.carousel->setGeometry(target);
        }
    }
}

void CarouselPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void CarouselPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void CarouselPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void CarouselPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createCarouselPage()
{
    return new CarouselPage;
}

} // namespace gallery

#include "GalleryPages25.moc"
