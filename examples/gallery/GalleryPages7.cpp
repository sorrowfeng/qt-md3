// Gallery page 7: FABs — the fourth Actions family.
//
// Same rules as the other component pages: the page never styles anything,
// widgets are banked, and build() is re-entrant. The family-specific note: a
// FAB is icon-only by token arithmetic (the label form is the extended FAB, a
// separate component), and it is the one Actions family that casts a shadow —
// every colour set publishes a container.elevation.

#include "GalleryPages.h"

#include "widgets/MdFab.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked FAB plus the content-local rect the latest build() gave it.
struct FabSlot
{
    md::MdFab *fab = nullptr;
    QRectF rect;
};

/// Gap between neighbouring FABs in a row.
constexpr qreal kRowGap = 24.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 16.0;

} // namespace

class FabPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit FabPage(QWidget *parent = nullptr);
    ~FabPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<md::MdFab *> &fabs);
    void layoutChildren();
    void placeChildren();

    md::MdFab *fab(const QString &id, const QString &iconName);
    template <typename Configure>
    md::MdFab *fab(const QString &id, const QString &iconName, Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdFab *widget = fab(id, iconName);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdFab *> m_bank;
    QVector<FabSlot> m_slots;
};

FabPage::FabPage(QWidget *parent)
    : GalleryPage(parent)
{
}

FabPage::~FabPage() = default;

QString FabPage::title() const
{
    return L("悬浮操作按钮", "FABs");
}

QString FabPage::slug() const
{
    return QStringLiteral("FABs");
}

QString FabPage::subtitle() const
{
    return L("四种颜色样式、三种尺寸、lowered / raised 两种海拔——屏幕上最重要的"
             "那个操作，带真实投影。",
             "Four colour sets, three sizes, lowered / raised elevations — the most important "
             "action on the screen, with a real shadow.");
}

md::MdFab *FabPage::fab(const QString &id, const QString &iconName)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFab(iconName, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void FabPage::layFlow(GalleryContext &context, const QVector<md::MdFab *> &fabs)
{
    const qreal available = context.width();

    QVector<md::MdFab *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdFab *fab : row) {
            const QSize hint = fab->sizeHint();
            m_slots.append(FabSlot{
                fab,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (md::MdFab *fab : fabs) {
        const QSize hint = fab->sizeHint();
        const qreal needed =
            row.isEmpty() ? qreal(hint.width()) : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(fab);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void FabPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- four colour sets -------------------------------------------------------
    context.section(L("四种颜色样式 —— md.comp.fab.<variant>",
                      "Four colour sets — md.comp.fab.<variant>"));
    context.paragraph(L(
        "surface 停在 surface-container-high 上、图标是 primary；primary / secondary / "
        "tertiary 分别填满各自的颜色、图标用对应的 on-* 角色。悬停看状态层，按住看涟漪"
        "——涟漪色就是 pressed 行的 state-layer 颜色。",
        "Surface rests on surface-container-high with a primary icon; primary / secondary / "
        "tertiary fill their own colour with the matching on-* icon. Hover for the state layer, "
        "hold for the ripple — the ripple colour is the pressed row's state-layer colour."));
    {
        const QVector<QPair<QString, md::FabVariant>> specs = {
            {QStringLiteral("Surface"), md::FabVariant::Surface},
            {QStringLiteral("Primary"), md::FabVariant::Primary},
            {QStringLiteral("Secondary"), md::FabVariant::Secondary},
            {QStringLiteral("Tertiary"), md::FabVariant::Tertiary},
        };
        QVector<md::MdFab *> row;
        for (const auto &spec : specs) {
            row.append(fab(QStringLiteral("variant-") + md::fabVariantName(spec.second),
                           QStringLiteral("add"), [&spec](md::MdFab *widget) {
                               widget->setVariant(spec.second);
                               widget->setFabSize(md::FabSize::Small);
                           }));
        }
        layFlow(context, row);
    }

    // --- three sizes ------------------------------------------------------------
    context.section(L("三种尺寸", "Three sizes"));
    context.paragraph(L(
        "md.comp.fab.<size>：40 / 56 / 96 px，圆角 corner-medium / corner-large / "
        "corner-extra-large（12 / 16 / 28 px），图标 24 / 24 / 36。形状在按压时"
        "不变——这个族没有发布按压形状令牌，按压响应就是涟漪本身。",
        "md.comp.fab.<size>: 40 / 56 / 96 px, corners corner-medium / corner-large / "
        "corner-extra-large (12 / 16 / 28 px), icons 24 / 24 / 36. The shape never morphs on "
        "press — this family publishes no pressed shape, so the press response is the ripple "
        "alone."));
    {
        QVector<md::MdFab *> row;
        for (int i = 0; i < int(md::FabSize::Count); ++i) {
            const auto size = md::FabSize(i);
            row.append(fab(QStringLiteral("size-") + md::fabSizeName(size),
                            QStringLiteral("edit"), [size](md::MdFab *widget) {
                                widget->setVariant(md::FabVariant::Primary);
                                widget->setFabSize(size);
                            }));
        }
        layFlow(context, row);
    }

    // --- lowered / raised ---------------------------------------------------------
    context.section(L("lowered 与 raised 海拔", "Lowered and raised elevations"));
    context.paragraph(L(
        "raised 停在 level3、悬停升到 level4；lowered 停在 level1、悬停 level2——"
        "投影是真实的，海拔每差一级都能看出来。surface 变体的 lowered 形态还换了容器色："
        "surface-container-low。",
        "Raised rests at level3 and climbs to level4 on hover; lowered rests at level1 and "
        "hovers at level2 — the shadow is real, and every level difference is visible. A "
        "lowered surface FAB also swaps its container to surface-container-low."));
    {
        auto *raisedSurface = fab(QStringLiteral("elev-surface"), QStringLiteral("search"),
                                  [](md::MdFab *widget) {
                                      widget->setVariant(md::FabVariant::Surface);
                                      widget->setFabSize(md::FabSize::Small);
                                  });
        auto *loweredSurface = fab(QStringLiteral("elev-surface-low"), QStringLiteral("search"),
                                   [](md::MdFab *widget) {
                                       widget->setVariant(md::FabVariant::Surface);
                                       widget->setFabSize(md::FabSize::Small);
                                       widget->setLowered(true);
                                   });
        auto *raisedPrimary = fab(QStringLiteral("elev-primary"), QStringLiteral("favorite"),
                                  [](md::MdFab *widget) {
                                      widget->setVariant(md::FabVariant::Primary);
                                      widget->setFabSize(md::FabSize::Small);
                                  });
        auto *loweredPrimary = fab(QStringLiteral("elev-primary-low"), QStringLiteral("favorite"),
                                   [](md::MdFab *widget) {
                                       widget->setVariant(md::FabVariant::Primary);
                                       widget->setFabSize(md::FabSize::Small);
                                       widget->setLowered(true);
                                   });
        layFlow(context, {raisedSurface, loweredSurface, raisedPrimary, loweredPrimary});
    }

    // --- disabled -----------------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "令牌导出没有发布 disabled 行——禁用值取自规范页的状态表：on-surface 容器 0.12、"
        "图标 0.38、海拔 level0（不投影）。这与按钮族共用同一行。",
        "The token export publishes no disabled rows — the disabled values come from the spec "
        "page's state table: on-surface container at 0.12, icon at 0.38, elevation level0 (no "
        "shadow). It is the same disabled row every button family shows."));
    {
        QVector<md::MdFab *> row;
        for (int i = 0; i < int(md::FabVariant::Count); ++i) {
            const auto variant = md::FabVariant(i);
            row.append(fab(QStringLiteral("off-") + md::fabVariantName(variant),
                            QStringLiteral("delete"), [variant](md::MdFab *widget) {
                                widget->setVariant(variant);
                                widget->setFabSize(md::FabSize::Small);
                                widget->setEnabled(false);
                            }));
        }
        layFlow(context, row);
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-fab{,-<variant>,"
        "-<size>}.scss"));
    context.detail(QStringLiteral(
        "sizes    small 40 · medium 56 · large 96  icons  24 · 24 · 36  shapes  12 / 16 / 28"));
    context.detail(QStringLiteral(
        "raised   enabled L3 · hovered L4 · focused L3 · pressed L3   lowered  L1 · L2 · L1 · "
        "L1"));
    context.detail(QStringLiteral(
        "press    no pressed shape — the ripple is the press response; ripple colour = pressed "
        "row's state-layer role (surface/primary/secondary/tertiary on-* or primary)"));
    context.detail(QStringLiteral(
        "disabled container on-surface @ 0.12 · icon on-surface @ 0.38 · L0 (spec table; the "
        "export publishes no disabled rows)"));
    context.space(8.0);
    context.detail(L("TestMd3Fab 将尺寸表、四种颜色组、lowered 海拔行与规范填入的 disabled 行"
                     "逐字段锁定。",
                     "TestMd3Fab pins the size table, all four colour sets, the lowered "
                     "elevation rows and the spec-filled disabled row field by field."));
}

void FabPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const FabSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.fab->geometry() != target) {
            slot.fab->setGeometry(target);
        }
    }
}

void FabPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void FabPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void FabPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void FabPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createFabPage()
{
    return new FabPage;
}

} // namespace gallery

#include "GalleryPages7.moc"
