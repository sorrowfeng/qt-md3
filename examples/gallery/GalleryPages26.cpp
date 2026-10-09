// Gallery page 26: Divider — `md.comp.divider.*`, the two-row token export.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The snapshots are
// fixed-size MdDivider instances placed in bands, so the four inset modes
// and both orientations are directly comparable.

#include "GalleryPages.h"

#include "widgets/MdDivider.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

namespace gallery {

namespace {

struct DividerSlot
{
    md::MdDivider *divider = nullptr;
    QRectF rect;
};

} // namespace

class DividerPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit DividerPage(QWidget *parent = nullptr);
    ~DividerPage() override;

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

    md::MdDivider *divider(const QString &id, Qt::Orientation orientation);

    QHash<QString, md::MdDivider *> m_bank;
    QVector<DividerSlot> m_slots;
};

DividerPage::DividerPage(QWidget *parent)
    : GalleryPage(parent)
{
}

DividerPage::~DividerPage() = default;

QString DividerPage::title() const
{
    return L("Divider", "Divider");
}

QString DividerPage::slug() const
{
    return QStringLiteral("divider");
}

QString DividerPage::subtitle() const
{
    return L(
        "md.comp.divider：全库最小的令牌表——仅 thickness 1px 与 outline-variant "
        "两行，无任何状态行（非交互是契约）。本页展示四种 inset 模式（full-"
        "width / 起始 16 / 末端 16 / 两侧各 16，start-end 是逻辑 inline "
        "边、RTL 镜像）、Compose 的垂直形态，以及 thickness / 颜色参数覆盖。"
        "线以设备像素对齐绘制，1px 保持一个锐利的物理像素。",
        "md.comp.divider: the smallest token table in the library — exactly two "
        "rows, thickness 1px and outline-variant, with no state rows at all "
        "(non-interactivity is the contract). This page shows the four inset "
        "modes (full-width / start 16 / end 16 / both, with start-end as "
        "logical inline edges mirrored in RTL), Compose's vertical form, and "
        "the thickness / color parameter overrides. The line is painted "
        "device-pixel aligned, so 1px stays one crisp physical pixel.");
}

md::MdDivider *DividerPage::divider(const QString &id, Qt::Orientation orientation)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDivider(orientation, this);
    m_bank.insert(id, created);
    return created;
}

void DividerPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- the four inset modes ------------------------------------------------
    context.section(L("四种 inset 模式", "The four inset modes"));
    context.paragraph(L(
        "自上而下：full-width（100%）、spec 的 inset（起始边 16、末端 0）、"
        "material-web 的 [inset-end]（仅末端 16）、middle-inset（两侧各 16）。"
        "同一根 1px outline-variant 细线，唯一的变量是内缩。",
        "Top to bottom: full-width (100%), the spec's inset (start margin 16, end 0), "
        "material-web's [inset-end] (trailing 16 only) and middle-inset (both 16). "
        "The same 1px outline-variant hairline; the only variable is the inset."));
    {
        const QStringList ids = {
            QStringLiteral("full"),
            QStringLiteral("start"),
            QStringLiteral("end"),
            QStringLiteral("both"),
        };
        const md::MdDivider::InsetMode modes[] = {
            md::MdDivider::InsetMode::None,
            md::MdDivider::InsetMode::Start,
            md::MdDivider::InsetMode::End,
            md::MdDivider::InsetMode::Both,
        };
        for (int i = 0; i < ids.size(); ++i) {
            md::MdDivider *created = divider(ids.at(i), Qt::Horizontal);
            created->setInsetMode(modes[i]);
            m_slots.append(DividerSlot{created, context.band(1.0)});
            context.space(14.0);
        }
    }

    // --- vertical --------------------------------------------------------------
    context.section(L("垂直形态（Compose VerticalDivider）", "Vertical (Compose's VerticalDivider)"));
    context.paragraph(L(
        "material-web 无垂直形态（CSS 布局不需要它）；Compose 发布了 "
        "VerticalDivider——fillMaxHeight().width(thickness)，内缩同理换到"
        "上下边。三根垂直线展示 full / start / both。",
        "material-web has no vertical form (CSS layout does not need one); Compose "
        "publishes VerticalDivider — fillMaxHeight().width(thickness), with the inset "
        "moving to the top/bottom edges the same way. Three vertical lines show "
        "full / start / both."));
    {
        const md::MdDivider::InsetMode modes[] = {
            md::MdDivider::InsetMode::None,
            md::MdDivider::InsetMode::Start,
            md::MdDivider::InsetMode::Both,
        };
        const QRectF band = context.band(48.0);
        const qreal slotWidth = band.width() / 3.0;
        for (int i = 0; i < 3; ++i) {
            md::MdDivider *created = divider(QStringLiteral("v%1").arg(i), Qt::Vertical);
            created->setInsetMode(modes[i]);
            const QRectF cell(band.left() + slotWidth * i, band.top(), slotWidth, 48.0);
            // Centre the 1px column inside each third of the band.
            m_slots.append(DividerSlot{created, QRectF(cell.center().x() - 0.5,
                                                       cell.top(), 1.0, 48.0)});
        }
    }

    // --- parameter overrides ------------------------------------------------------
    context.section(L("参数覆盖（thickness / color）", "Parameter overrides (thickness / color)"));
    context.paragraph(L(
        "Compose 把 thickness 与 color 作为参数暴露（废弃的 Divider 甚至接受"
        "与密度无关的 hairline）。这里 4px 粗度配自定义颜色，对照上方默认"
        "令牌线。",
        "Compose exposes thickness and color as parameters (its deprecated Divider "
        "even accepts a density-independent hairline). The 4px line below with a "
        "custom colour contrasts with the default token lines above."));
    {
        md::MdDivider *thick = divider(QStringLiteral("thick"), Qt::Horizontal);
        thick->setThickness(4.0);
        thick->setCustomColor(QColor(0x7C, 0x4D, 0xFF));
        m_slots.append(DividerSlot{thick, context.band(4.0)});
    }

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-divider.scss "
        "(34.0.21) + divider/internal/_divider.scss + Compose M3 Divider.kt "
        "(HorizontalDivider / VerticalDivider / DividerDefaults)"));
    context.detail(QStringLiteral(
        "export      thickness 1px · color outline-variant — the whole table; no "
        "state rows, no shape rows (non-interactivity is the contract)"));
    context.detail(QStringLiteral(
        "specs       full-width 100% · inset left 16dp right 0 · middle-inset 16dp "
        "both · space to supporting-text 4dp · right/bottom margin 8dp (usage "
        "spacing, recorded)"));
    context.detail(QStringLiteral(
        "divergence  material-web [inset] pads BOTH edges while the spec's \"inset\" "
        "is start-only — InsetMode covers all four combinations; the 16px inset is "
        "hardcoded in material-web scss, tokenised here as md.comp.divider.inset"));
    context.space(8.0);
    context.detail(L("TestMd3Divider 将 token 表、非交互契约、四种 inset 几何、"
                     "垂直形态、RTL inline 镜像、thickness/color 参数覆盖、"
                     "override 解析与设备像素对齐渲染逐字段锁定。",
                     "TestMd3Divider pins the token table, the non-interactivity "
                     "contract, the four inset geometries, the vertical form, the RTL "
                     "inline mirroring, the thickness/color overrides, the override "
                     "parsing and the device-pixel-aligned rendering field by field."));
}

void DividerPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const DividerSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.divider->geometry() != target) {
            slot.divider->setGeometry(target);
        }
    }
}

void DividerPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void DividerPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void DividerPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void DividerPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createDividerPage()
{
    return new DividerPage;
}

} // namespace gallery

#include "GalleryPages26.moc"
