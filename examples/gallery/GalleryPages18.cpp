// Gallery page 18: Loading indicators — the Expressive
// `md.comp.loading-indicator.*` family.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// the export is token-only (material-web ships no web component), the
// behaviour port is Compose M3 Expressive's LoadingIndicator, and there is
// no interaction at all (the export publishes no state rows).

#include "GalleryPages.h"

#include "widgets/MdLoadingIndicator.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// One banked indicator plus the content-local rect the latest build() gave it.
struct LoadingSlot
{
    md::MdLoadingIndicator *indicator = nullptr;
    QRectF rect;
};

constexpr qreal kStackGap = 20.0;
constexpr qreal kIndicatorSize = 48.0;

} // namespace

class LoadingIndicatorPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit LoadingIndicatorPage(QWidget *parent = nullptr);
    ~LoadingIndicatorPage() override;

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

    /// Banked construction; configuration runs only on first creation.
    md::MdLoadingIndicator *indicator(const QString &id, md::LoadingIndicatorVariant variant,
                                      const std::function<void(md::MdLoadingIndicator *)>
                                          &configure = {});

    QHash<QString, md::MdLoadingIndicator *> m_bank;
    QVector<LoadingSlot> m_slots;
};

LoadingIndicatorPage::LoadingIndicatorPage(QWidget *parent)
    : GalleryPage(parent)
{
}

LoadingIndicatorPage::~LoadingIndicatorPage() = default;

QString LoadingIndicatorPage::title() const
{
    return L("加载指示器", "Loading indicators");
}

QString LoadingIndicatorPage::slug() const
{
    return QStringLiteral("loading-indicators");
}

QString LoadingIndicatorPage::subtitle() const
{
    return L("md.comp.loading-indicator：M3 Expressive 的形状变形加载指示器。"
             "导出只有令牌行（material-web 没有网页组件实现），行为以 Compose M3 "
             "Expressive 的 LoadingIndicator 为移植源：七个 Material 形状在 650 ms "
             "网格上循环变形（弹簧 damping 0.6 / stiffness 200），整体叠加每变形"
             "一次 90° 的步进旋转与 4666 ms 的线性自旋；确定值模式在圆形→软爆形"
             "之间按进度变形并逆时针扫过半圈。无交互——导出没有发布任何状态行。",
             "md.comp.loading-indicator: the M3 Expressive shape-morphing loading indicator. "
             "The export is token-only (material-web ships no web component), so the behaviour "
             "port is Compose M3 Expressive's LoadingIndicator: seven Material shapes morph in "
             "a loop on a 650 ms grid (spring damping 0.6 / stiffness 200), with a 90° step "
             "per morph on top of a 4666 ms linear spin; the determinate mode morphs circle → "
             "soft-burst by progress and sweeps a counter-clockwise half turn. Not "
             "interactive — the export publishes no state rows.");
}

md::MdLoadingIndicator *LoadingIndicatorPage::indicator(
    const QString &id, md::LoadingIndicatorVariant variant,
    const std::function<void(md::MdLoadingIndicator *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdLoadingIndicator(variant, this);
    created->show();
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

void LoadingIndicatorPage::build(GalleryContext &context)
{
    m_slots.clear();
    const QSize indicatorSize{int(kIndicatorSize), int(kIndicatorSize)};

    // --- plain — indeterminate -------------------------------------------------
    context.section(L("朴素 —— 不定态", "Plain — indeterminate"));
    context.paragraph(L(
        "md.comp.loading-indicator：48 px 容器内 38 px 的活动指示器（"
        "ActiveIndicatorScale = 38/48），primary 色。形状按 软爆形 → 9 曲奇 → "
        "五边形 → 药丸 → 阳光 → 4 曲奇 → 椭圆 → 软爆形 循环变形，每次 650 ms "
        "的弹簧变形自带回弹（过冲到约 1.08）；旋转 = 形变内 90° + 每次完成步进 "
        "90° + 4666 ms 线性自旋。",
        "md.comp.loading-indicator: a 38 px active indicator inside the 48 px container "
        "(ActiveIndicatorScale = 38/48), in primary. The shapes morph soft-burst → 9-cookie "
        "→ pentagon → pill → sunny → 4-cookie → oval → soft-burst, one 650 ms spring morph "
        "each, with the spring's own bounce (overshooting to ≈1.08); rotation = 90° in-morph "
        "+ a 90° step per completed morph + the 4666 ms linear spin."));
    {
        const QRectF band = context.band(kIndicatorSize);
        md::MdLoadingIndicator *spin =
            indicator(QStringLiteral("p-spin"), md::LoadingIndicatorVariant::Plain,
                      [](md::MdLoadingIndicator *w) { w->setIndeterminate(true); });
        m_slots.append(LoadingSlot{spin, QRectF(band.topLeft(), indicatorSize)});
    }

    // --- plain — determinate ----------------------------------------------------
    context.section(L("朴素 —— 确定值", "Plain — determinate"));
    context.paragraph(L(
        "确定值模式在 旋转 18° 的圆形 → 软爆形 之间按进度变形（两条序列，一档"
        "变形），同时逆时针扫过半圈——这正是 Compose 用它做下拉刷新指示器的"
        "方式。这里摆的是固定进度快照。",
        "The determinate mode morphs between the circle rotated 18° and the soft burst by "
        "progress (one morph in the open sequence) while sweeping a counter-clockwise half "
        "turn — how Compose uses it as the pull-to-refresh indicator. These are fixed "
        "progress snapshots."));
    {
        const QRectF band = context.band(kIndicatorSize);
        const auto columns = {0.0, 0.25, 0.5, 0.75, 1.0};
        int column = 0;
        for (const qreal progress : columns) {
            const QString id = QStringLiteral("p-d-%1").arg(column);
            md::MdLoadingIndicator *frozen =
                indicator(id, md::LoadingIndicatorVariant::Plain,
                          [progress](md::MdLoadingIndicator *w) {
                              w->setIndeterminate(false);
                              w->setProgress(progress);
                          });
            m_slots.append(LoadingSlot{
                frozen, QRectF(band.left() + column * (kIndicatorSize + kStackGap), band.top(),
                               indicatorSize.width(), indicatorSize.height())});
            ++column;
        }
    }

    // --- contained -----------------------------------------------------------------
    context.section(L("容器变体", "Contained"));
    context.paragraph(L(
        "contained 变体把形状放进 primary-container 的 corner-full 圆盘，形状换"
        "用 on-primary-container——导出里 deprecated 的 container.color 行"
        "（secondary-container）就是被这套独立配色取代的，本移植转录但从不读取。",
        "The contained variant puts the shape inside the primary-container corner-full disc "
        "and switches to on-primary-container — the export's deprecated container.color row "
        "(secondary-container) is what this distinct colour mapping superseded; transcribed "
        "but never read."));
    {
        const QRectF band = context.band(kIndicatorSize);
        md::MdLoadingIndicator *containedSpin =
            indicator(QStringLiteral("c-spin"), md::LoadingIndicatorVariant::Contained,
                      [](md::MdLoadingIndicator *w) { w->setIndeterminate(true); });
        m_slots.append(LoadingSlot{containedSpin, QRectF(band.topLeft(), indicatorSize)});

        md::MdLoadingIndicator *containedFrozen =
            indicator(QStringLiteral("c-d"), md::LoadingIndicatorVariant::Contained,
                      [](md::MdLoadingIndicator *w) {
                          w->setIndeterminate(false);
                          w->setProgress(0.6);
                      });
        m_slots.append(LoadingSlot{
            containedFrozen,
            QRectF(band.left() + kIndicatorSize + kStackGap, band.top(), indicatorSize.width(),
                   indicatorSize.height())});
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-loading-indicator.scss "
        "(34.0.21) + Compose M3 LoadingIndicator.kt / MaterialShapes.kt / graphics-shapes"));
    context.detail(QStringLiteral(
        "export      active-indicator 38 px · container 48×48 corner-full · primary / "
        "on-primary-container on primary-container · no state rows at all"));
    context.detail(QStringLiteral(
        "deprecated  container.color (secondary-container) — superseded by the contained "
        "variant's distinct mapping; transcribed, never read"));
    context.detail(QStringLiteral(
        "animation   morph grid 650 ms · spring dampingRatio 0.6 stiffness 200 (closed form, "
        "bounce kept) · quarter-turn step per morph · global spin 4666 ms linear · determinate "
        "sweep −180°"));
    context.detail(QStringLiteral(
        "gap         the morph interpolates radially (all sequence shapes are star-convex) "
        "instead of graphics-shapes' feature-matched cubics — the port's registered "
        "divergence, docs/porting-todo.md"));
    context.space(8.0);
    context.detail(L("TestMd3LoadingIndicator 将导出行、废弃行转录、形状引擎归一化、"
                     "morph 端点契约、弹簧与帧纯函数、动画状态与非交互契约逐字段锁定。",
                     "TestMd3LoadingIndicator pins the export rows, the deprecated "
                     "transcription, the shape engine's normalization, the morph endpoint "
                     "contracts, the spring and frame pure functions, the animation state and "
                     "the non-interactivity contract field by field."));
}

void LoadingIndicatorPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const LoadingSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.indicator->geometry() != target) {
            slot.indicator->setGeometry(target);
        }
    }
}

void LoadingIndicatorPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void LoadingIndicatorPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void LoadingIndicatorPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void LoadingIndicatorPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createLoadingIndicatorPage()
{
    return new LoadingIndicatorPage;
}

} // namespace gallery

#include "GalleryPages18.moc"
