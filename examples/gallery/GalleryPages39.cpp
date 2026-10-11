// Gallery page 39: Sliders — `md.comp.slider.*` at export version 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every track on this page is
// a live MdSlider — the vertical-pill handle, the per-state widths, the value
// indicator and the keyboard walk are all the real component.
//
// Three things this page is explicit about because they are easy to misread:
//
//   * **the handle is a vertical pill, not a circle** — 4 px wide, 44 px tall
//     at the base size, narrowing to 2 px under focus and press;
//   * **the colours do not animate** — the track and handle resolve by state
//     and land this frame;
//   * **the inactive track is secondary-container** — the export's row, not
//     the surface-container-highest the material-web theming table claims.

#include "GalleryPages.h"

#include "core/MdSliderTokens.h"
#include "widgets/MdSlider.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct SliderSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class SliderPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SliderPage(QWidget *parent = nullptr);
    ~SliderPage() override;

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

    md::MdSlider *sl(const QString &id);

    QHash<QString, md::MdSlider *> m_sliders;
    QVector<SliderSlot> m_slots;
};

SliderPage::SliderPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SliderPage::~SliderPage() = default;

QString SliderPage::title() const
{
    return L("滑块", "Slider");
}

QString SliderPage::slug() const
{
    return QStringLiteral("slider");
}

QString SliderPage::subtitle() const
{
    return L(
        "滑块：轨道是 corner-full 药丸（16dp），手柄是**竖直药丸**而非圆点 —— 4dp 宽、"
        "44dp 高，聚焦/按压收窄到 2dp，悬停保持 4dp。颜色**不做动画**，轨道与手柄按状态"
        "直解落位。停靠点 4dp、刻度 2dp（弃用的 with-tick-marks 家族）。可选的数值标签"
        "从手柄处缩放展开（duration-short2 + emphasized）。inactive 轨道是 "
        "secondary-container（导出行；material-web 主题表写作 surface-container-highest，"
        "以导出为准）。五档 Expressive 尺寸：16/24/40/56/96dp 轨道高。",
        "Slider: a corner-full pill track (16dp) with a handle that is a **vertical "
        "pill, not a circle** — 4dp wide, 44dp tall, narrowing to 2dp under focus and "
        "press, keeping 4dp on hover. The colours **do not animate**: track and handle "
        "resolve by state and land this frame. Stop indicators 4dp, tick marks 2dp (the "
        "deprecated with-tick-marks family). The optional value label scales in from the "
        "handle (duration-short2 + emphasized). The inactive track is secondary-container "
        "(the export's row; the material-web theming table claims surface-container-"
        "highest — the export wins). Five Expressive sizes: 16/24/40/56/96dp tracks.");
}

md::MdSlider *SliderPage::sl(const QString &id)
{
    const auto found = m_sliders.constFind(id);
    if (found != m_sliders.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSlider(this);
    created->hide();
    m_sliders.insert(id, created);
    return created;
}

void SliderPage::build(GalleryContext &context)
{
    m_slots.clear();

    // Sliders are content-width — place by a fixed 240×48 box.
    auto addRow = [&](const QVector<QPair<QString, md::MdSlider *>> &sliders,
                      qreal width = 240.0) {
        if (sliders.isEmpty()) {
            return;
        }
        const qreal pitch = width + 24.0;
        const qreal bandHeight = 48.0;
        const QRectF band = context.band(bandHeight);
        const qreal span = (sliders.size() - 1) * pitch + width;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : sliders) {
            m_slots.append(SliderSlot{entry.second, QRectF(x, band.top(), width, bandHeight)});
            x += pitch;
        }
    };

    // --- the five Expressive sizes -----------------------------------------------
    context.section(L("五档尺寸", "The five Expressive sizes"));
    context.paragraph(L(
        "尺寸行改轨道高与手柄高，不改手柄宽度：xsmall 16/44、small 24/44、medium 40/44、"
        "large 56/68、xlarge 96/108（轨道/手柄，dp）。轨道端头圆角 8/8/12/16/28。"
        "xsmall 与 small 不发布图标行。",
        "The size rows change the track and handle heights, never the handle width: "
        "xsmall 16/44, small 24/44, medium 40/44, large 56/68, xlarge 96/108 "
        "(track/handle, dp). Track end radii 8/8/12/16/28. xsmall and small publish "
        "no icon rows."));
    {
        auto *xsmall = sl(QStringLiteral("size-xsmall"));
        xsmall->setSliderSize(md::MdSliderSize::XSmall);
        xsmall->setValue(30.0);
        auto *small = sl(QStringLiteral("size-small"));
        small->setSliderSize(md::MdSliderSize::Small);
        small->setValue(40.0);
        auto *medium = sl(QStringLiteral("size-medium"));
        medium->setSliderSize(md::MdSliderSize::Medium);
        medium->setValue(50.0);
        auto *large = sl(QStringLiteral("size-large"));
        large->setSliderSize(md::MdSliderSize::Large);
        large->setValue(60.0);
        auto *xlarge = sl(QStringLiteral("size-xlarge"));
        xlarge->setSliderSize(md::MdSliderSize::XLarge);
        xlarge->setValue(70.0);
        addRow({{QStringLiteral("a"), xsmall},
                {QStringLiteral("b"), small},
                {QStringLiteral("c"), medium},
                {QStringLiteral("d"), large},
                {QStringLiteral("e"), xlarge}},
               180.0);
        context.detail(L("xsmall · small · medium · large · xlarge",
                         "xsmall · small · medium · large · xlarge"));
        context.space(16.0);
    }

    // --- discrete + ticks -----------------------------------------------------------
    context.section(L("离散与刻度", "Discrete, with tick marks"));
    context.paragraph(L(
        "step > 0 时取值吸附到步距；ticks 在每一步画 2dp 刻度点（弃用的 "
        "with-tick-marks 家族，active 侧 on-primary @0.38，inactive 侧 "
        "on-surface-variant @0.38）。轨道两端是 4dp 停靠点。",
        "With step > 0 the value snaps to the step; ticks paint the 2dp tick marks at "
        "every step (the deprecated with-tick-marks family — on-primary @0.38 on the "
        "active side, on-surface-variant @0.38 on the inactive side). The track's ends "
        "carry the 4dp stop indicators."));
    {
        auto *plain = sl(QStringLiteral("discrete-plain"));
        plain->setStep(10.0);
        plain->setValue(50.0);
        auto *ticked = sl(QStringLiteral("discrete-ticks"));
        ticked->setStep(10.0);
        ticked->setTicks(true);
        ticked->setValue(50.0);
        addRow({{QStringLiteral("a"), plain}, {QStringLiteral("b"), ticked}});
        context.detail(L("离散 · 离散 + 刻度", "discrete · discrete + ticks"));
        context.space(16.0);
    }

    // --- range ------------------------------------------------------------------------
    context.section(L("区间：两个手柄", "Range: two handles"));
    context.paragraph(L(
        "range 形式是同一个控件（material-web 的契约）：valueStart 与 valueEnd 各带一个"
        "手柄，激活的那一个接受键盘；Tab 在两者间切换。active 轨道跨越两个手柄之间。",
        "The range form is the same widget (material-web's contract): valueStart and "
        "valueEnd each carry a handle, the active one takes the keyboard; Tab switches "
        "between them. The active track spans the space between the handles."));
    {
        auto *range = sl(QStringLiteral("range-basic"));
        range->setRange(true);
        range->setValueStart(25.0);
        range->setValueEnd(75.0);
        addRow({{QStringLiteral("a"), range}});
        context.detail(L("25 – 75", "25 – 75"));
        context.space(16.0);
    }

    // --- value indicator ----------------------------------------------------------------
    context.section(L("数值标签", "The value label"));
    context.paragraph(L(
        "labeled 时标签从手柄处缩放展开（duration-short2 + emphasized，原点在底部中心），"
        "悬停、聚焦或按压时显示。容器 inverse-surface、文字 inverse-on-surface（label-large）。",
        "With labeled the label scales in from the handle (duration-short2 + emphasized, "
        "origin bottom-centre) while hovered, focused or pressed. The container is "
        "inverse-surface, the text inverse-on-surface (label-large)."));
    {
        auto *labelled = sl(QStringLiteral("label-basic"));
        labelled->setLabeled(true);
        labelled->setValue(40.0);
        auto *labelledStep = sl(QStringLiteral("label-step"));
        labelledStep->setLabeled(true);
        labelledStep->setStep(5.0);
        labelledStep->setValue(65.0);
        addRow({{QStringLiteral("a"), labelled}, {QStringLiteral("b"), labelledStep}});
        context.detail(L("连续 + 标签 · 离散 + 标签（悬停/聚焦/按压可见）",
                         "continuous + label · discrete + label (hover/focus/press)"));
        context.space(16.0);
    }

    // --- disabled -------------------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "禁用按行淡出：active 轨道 0.38、inactive 轨道 0.12、手柄 0.38（on-surface）。"
        "手柄阴影降到 level0。",
        "Disabled fades by row: the active track at 0.38, the inactive track at 0.12, "
        "the handle at 0.38 (on-surface). The handle's shadow drops to level0."));
    {
        auto *off = sl(QStringLiteral("disabled-basic"));
        off->setValue(50.0);
        off->setEnabled(false);
        addRow({{QStringLiteral("a"), off}});
        context.detail(L("禁用 50%", "disabled at 50%"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页钉住的 token 事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "下面每一行都由 TestMd3Slider 直接断言，动一行就变红。",
        "Every line below is asserted directly by TestMd3Slider; moving one turns it "
        "red."));
    context.detail(L("手柄宽 4dp，hover 4dp、focus/pressed 2dp、disabled 4dp · 状态层 40dp · "
                     "停靠点 4dp（trailing-space 4dp）· 刻度 2dp · 标签下间距 12dp、最小 28dp",
                     "the handle is 4dp wide — hover 4dp, focus/pressed 2dp, disabled 4dp — "
                     "the state layer 40dp · stop indicators 4dp (trailing-space 4dp) · "
                     "tick marks 2dp · the label sits 12dp above, minimum 28dp"));
    context.detail(L("尺寸行：轨道 16/24/40/56/96dp、手柄 44/44/44/68/108dp、端头圆角 "
                     "8/8/12/16/28dp、图标 0/0/24/24/32dp",
                     "the size rows: tracks 16/24/40/56/96dp, handles 44/44/44/68/108dp, "
                     "end radii 8/8/12/16/28dp, icons 0/0/24/24/32dp"));
    context.detail(L("颜色按状态直解（无动画）：active 轨道 primary、inactive 轨道 "
                     "secondary-container（导出行，非主题表的 surface-container-highest）、"
                     "手柄 primary；禁用 on-surface @0.38 / @0.12 / @0.38",
                     "colours resolve by state (no animation): the active track primary, "
                     "the inactive track secondary-container (the export's row, not the "
                     "theming table's surface-container-highest), the handle primary; "
                     "disabled on-surface @0.38 / @0.12 / @0.38"));
    context.detail(L("值标签：inverse-surface 容器 + inverse-on-surface 文字（label-large），"
                     "duration-short2 + emphasized 从手柄缩放展开 · 手柄阴影 level1 · "
                     "disabled level0",
                     "the value label: inverse-surface container + inverse-on-surface text "
                     "(label-large), scaling in from the handle on duration-short2 + "
                     "emphasized · the handle's shadow level1 · level0 disabled"));
    context.space(8.0);
}

void SliderPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SliderSlot &slot : m_slots) {
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

void SliderPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SliderPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SliderPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SliderPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSliderPage()
{
    return new SliderPage;
}

} // namespace gallery

#include "GalleryPages39.moc"
