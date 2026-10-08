// Gallery page 8: Extended FABs — the fifth Actions family.
//
// Same rules as the other component pages: the page never styles anything,
// widgets are banked, and build() is re-entrant. The family-specific note: an
// extended FAB is icon + label on a FAB-grade shadowed container, its width
// is derived from that content (not a token), and the colour sets are *not*
// the FAB's — six sets, none of them surface.

#include "GalleryPages.h"

#include "widgets/MdExtendedFab.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked extended FAB plus the content-local rect the latest build() gave it.
struct ExtendedFabSlot
{
    md::MdExtendedFab *fab = nullptr;
    QRectF rect;
};

/// Gap between neighbouring buttons in a row.
constexpr qreal kRowGap = 24.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 16.0;

} // namespace

class ExtendedFabPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ExtendedFabPage(QWidget *parent = nullptr);
    ~ExtendedFabPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<md::MdExtendedFab *> &fabs);
    void layoutChildren();
    void placeChildren();

    md::MdExtendedFab *fab(const QString &id, const QString &iconName, const QString &label);
    template <typename Configure>
    md::MdExtendedFab *fab(const QString &id, const QString &iconName, const QString &label,
                           Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdExtendedFab *widget = fab(id, iconName, label);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdExtendedFab *> m_bank;
    QVector<ExtendedFabSlot> m_slots;
};

ExtendedFabPage::ExtendedFabPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ExtendedFabPage::~ExtendedFabPage() = default;

QString ExtendedFabPage::title() const
{
    return L("扩展悬浮操作按钮", "Extended FABs");
}

QString ExtendedFabPage::slug() const
{
    return QStringLiteral("Extended FABs");
}

QString ExtendedFabPage::subtitle() const
{
    return L("六种颜色样式、三种尺寸、lowered / raised——图标加文字的悬浮操作，"
             "宽度由内容推导。",
             "Six colour sets, three sizes, lowered / raised — an icon plus text floating "
             "action, with a content-derived width.");
}

md::MdExtendedFab *ExtendedFabPage::fab(const QString &id, const QString &iconName,
                                        const QString &label)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdExtendedFab(iconName, label, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void ExtendedFabPage::layFlow(GalleryContext &context, const QVector<md::MdExtendedFab *> &fabs)
{
    const qreal available = context.width();

    QVector<md::MdExtendedFab *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdExtendedFab *fab : row) {
            const QSize hint = fab->sizeHint();
            m_slots.append(ExtendedFabSlot{
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

    for (md::MdExtendedFab *fab : fabs) {
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

void ExtendedFabPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- six colour sets -------------------------------------------------------
    context.section(L("六种颜色样式 —— md.comp.extended-fab.<variant>",
                      "Six colour sets — md.comp.extended-fab.<variant>"));
    context.paragraph(L(
        "与 FAB 族的四色不同：这里没有 surface，而是 primary / secondary / tertiary 加上三个"
        " container 变体。纯色组用 on-* 角色，container 组统一用 on-*-container。悬停看状态"
        "层，按住看涟漪——涟漪色就是 pressed 行的 state-layer 颜色。",
        "Unlike the FAB family's four sets: no surface here, but primary / secondary / tertiary "
        "plus three container sets. The colour groups use the on-* roles, the container groups "
        "on-*-container. Hover for the state layer, hold for the ripple — the ripple colour is "
        "the pressed row's state-layer colour."));
    {
        const QVector<QPair<QString, md::ExtendedFabVariant>> specs = {
            {QStringLiteral("Primary"), md::ExtendedFabVariant::Primary},
            {QStringLiteral("Secondary"), md::ExtendedFabVariant::Secondary},
            {QStringLiteral("Tertiary"), md::ExtendedFabVariant::Tertiary},
            {QStringLiteral("Primary Container"), md::ExtendedFabVariant::PrimaryContainer},
            {QStringLiteral("Secondary Container"), md::ExtendedFabVariant::SecondaryContainer},
            {QStringLiteral("Tertiary Container"), md::ExtendedFabVariant::TertiaryContainer},
        };
        QVector<md::MdExtendedFab *> row;
        for (const auto &spec : specs) {
            row.append(fab(QStringLiteral("variant-") + md::extendedFabVariantName(spec.second),
                           QStringLiteral("add"), L("创建", "Create"),
                           [&spec](md::MdExtendedFab *widget) {
                               widget->setVariant(spec.second);
                               widget->setFabSize(md::ExtendedFabSize::Small);
                           }));
        }
        layFlow(context, row);
    }

    // --- three sizes ------------------------------------------------------------
    context.section(L("三种尺寸", "Three sizes"));
    context.paragraph(L(
        "md.comp.extended-fab.<size>：56 / 80 / 96 px 高，圆角 corner-large / "
        "corner-large-increased / corner-extra-large（16 / 20 / 28 px），图标 24 / 28 / 36，"
        "文字 title-medium / title-large / headline-small。宽度不是令牌——由 "
        "leading + 图标 + 间距 + 文字 + trailing 推导。",
        "md.comp.extended-fab.<size>: 56 / 80 / 96 px tall, corners corner-large / "
        "corner-large-increased / corner-extra-large (16 / 20 / 28 px), icons 24 / 28 / 36, "
        "labels title-medium / title-large / headline-small. Width is not a token — it is "
        "leading + icon + gap + label + trailing, derived."));
    {
        const QVector<QPair<QString, md::ExtendedFabSize>> labels = {
            {L("撰写", "Compose"), md::ExtendedFabSize::Small},
            {L("撰写", "Compose"), md::ExtendedFabSize::Medium},
            {L("撰写", "Compose"), md::ExtendedFabSize::Large},
        };
        QVector<md::MdExtendedFab *> row;
        for (const auto &label : labels) {
            const auto size = label.second;
            row.append(fab(QStringLiteral("size-") + md::extendedFabSizeName(size),
                           QStringLiteral("edit"), label.first,
                           [size](md::MdExtendedFab *widget) {
                               widget->setVariant(md::ExtendedFabVariant::SecondaryContainer);
                               widget->setFabSize(size);
                           }));
        }
        layFlow(context, row);
    }

    // --- lowered / raised ---------------------------------------------------------
    context.section(L("lowered 与 raised 海拔", "Lowered and raised elevations"));
    context.paragraph(L(
        "海拔行与 FAB 族逐字相同：raised 停在 level3、悬停 level4；lowered 停在 level1、"
        "悬停 level2。与 FAB 族不同——这里没有 lowered 容器色，lowered 只改海拔。",
        "The elevation rows match the FAB family word for word: raised rests at level3 and "
        "hovers at level4; lowered rests at level1 and hovers at level2. Unlike the FAB family "
        "there is no lowered container colour — lowered changes elevation only."));
    {
        auto *raised = fab(QStringLiteral("elev-raised"), QStringLiteral("favorite"),
                           L("收藏", "Favorite"), [](md::MdExtendedFab *widget) {
                               widget->setVariant(md::ExtendedFabVariant::TertiaryContainer);
                               widget->setFabSize(md::ExtendedFabSize::Small);
                           });
        auto *lowered = fab(QStringLiteral("elev-lowered"), QStringLiteral("favorite"),
                            L("收藏", "Favorite"), [](md::MdExtendedFab *widget) {
                                widget->setVariant(md::ExtendedFabVariant::TertiaryContainer);
                                widget->setFabSize(md::ExtendedFabSize::Small);
                                widget->setLowered(true);
                            });
        layFlow(context, {raised, lowered});
    }

    // --- disabled -----------------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "令牌导出没有发布 disabled 行——禁用值取自规范页的状态表：on-surface 容器 0.12、"
        "图标与文字 0.38、海拔 level0（不投影）。这与按钮族共用同一行。",
        "The token export publishes no disabled rows — the disabled values come from the spec "
        "page's state table: on-surface container at 0.12, icon and label at 0.38, elevation "
        "level0 (no shadow). It is the same disabled row every button family shows."));
    {
        const QVector<QPair<QString, md::ExtendedFabVariant>> specs = {
            {QStringLiteral("Primary"), md::ExtendedFabVariant::Primary},
            {QStringLiteral("Primary Container"), md::ExtendedFabVariant::PrimaryContainer},
            {QStringLiteral("Tertiary Container"), md::ExtendedFabVariant::TertiaryContainer},
        };
        QVector<md::MdExtendedFab *> row;
        for (const auto &spec : specs) {
            row.append(fab(QStringLiteral("off-") + md::extendedFabVariantName(spec.second),
                           QStringLiteral("delete"), L("删除", "Delete"),
                           [&spec](md::MdExtendedFab *widget) {
                               widget->setVariant(spec.second);
                               widget->setFabSize(md::ExtendedFabSize::Small);
                               widget->setEnabled(false);
                           }));
        }
        layFlow(context, row);
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-extended-fab{,-<size>,"
        "-<variant>}.scss"));
    context.detail(QStringLiteral(
        "sizes    small 56 · medium 80 · large 96  icons  24 · 28 · 36  shapes  16 / 20 / 28"));
    context.detail(QStringLiteral(
        "labels   title-medium · title-large · headline-small   width  leading + icon + gap + "
        "label + trailing (derived, not a token)"));
    context.detail(QStringLiteral(
        "raised   enabled L3 · hovered L4 · focused L3 · pressed L3   lowered  L1 · L2 · L1 · "
        "L1   no lowered container colour"));
    context.detail(QStringLiteral(
        "press    no pressed shape — the ripple is the press response; ripple colour = pressed "
        "row's state-layer role (on-* for the colour sets, on-*-container for the container "
        "sets)"));
    context.detail(QStringLiteral(
        "disabled container on-surface @ 0.12 · icon+label on-surface @ 0.38 · L0 (spec table; "
        "the export publishes no disabled rows)"));
    context.space(8.0);
    context.detail(L("TestMd3ExtendedFab 将尺寸表、六种颜色组、lowered 海拔行与规范填入的 "
                     "disabled 行逐字段锁定。",
                     "TestMd3ExtendedFab pins the size table, all six colour sets, the lowered "
                     "elevation rows and the spec-filled disabled row field by field."));
}

void ExtendedFabPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ExtendedFabSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.fab->geometry() != target) {
            slot.fab->setGeometry(target);
        }
    }
}

void ExtendedFabPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ExtendedFabPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ExtendedFabPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ExtendedFabPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createExtendedFabPage()
{
    return new ExtendedFabPage;
}

} // namespace gallery

#include "GalleryPages8.moc"
