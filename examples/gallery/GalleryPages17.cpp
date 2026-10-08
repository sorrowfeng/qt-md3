// Gallery page 17: Progress indicators — linear and circular, the merged
// `md.comp.progress-indicator.*` family.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// no interaction at all (the export publishes no state rows), determinate
// values transition over the published durations, and the indeterminate
// cycles are the MDC-heritage keyframes material-web ships.

#include "GalleryPages.h"

#include "widgets/MdProgressIndicator.h"

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
struct ProgressSlot
{
    md::MdProgressIndicator *indicator = nullptr;
    QRectF rect;
};

constexpr qreal kStackGap = 20.0;
constexpr qreal kIndicatorWidth = 240.0;

} // namespace

class ProgressIndicatorPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ProgressIndicatorPage(QWidget *parent = nullptr);
    ~ProgressIndicatorPage() override;

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
    md::MdProgressIndicator *indicator(const QString &id, md::ProgressIndicatorShape shape,
                                       const std::function<void(md::MdProgressIndicator *)>
                                           &configure = {});

    QHash<QString, md::MdProgressIndicator *> m_bank;
    QVector<ProgressSlot> m_slots;
};

ProgressIndicatorPage::ProgressIndicatorPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ProgressIndicatorPage::~ProgressIndicatorPage() = default;

QString ProgressIndicatorPage::title() const
{
    return L("进度指示器", "Progress indicators");
}

QString ProgressIndicatorPage::slug() const
{
    return QStringLiteral("progress-indicators");
}

QString ProgressIndicatorPage::subtitle() const
{
    return L("md.comp.progress-indicator：合并后的进度指示器族（34.0.21 起废弃了"
             "线性/圆形两套旧令牌）。无交互——导出没有发布任何状态行。确定值沿"
             "发布的时长过渡（线性 250 ms / 圆形 500 ms）；不定态就是 material-web "
             "的 MDC 遗产关键帧——线性双条 2 s、圆形三层旋转组合。",
             "md.comp.progress-indicator: the merged family (the two per-shape token sets "
             "are deprecated as of 34.0.21). Not interactive — the export publishes no state "
             "rows. Determinate values transition over the published durations (250 ms linear "
             "/ 500 ms circular); the indeterminate cycles are material-web's MDC-heritage "
             "keyframes — the linear two-bar 2 s cycle and the circular three composed "
             "rotations.");
}

md::MdProgressIndicator *ProgressIndicatorPage::indicator(
    const QString &id, md::ProgressIndicatorShape shape,
    const std::function<void(md::MdProgressIndicator *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdProgressIndicator(shape, this);
    created->show();
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

void ProgressIndicatorPage::build(GalleryContext &context)
{
    m_slots.clear();
    const QSize linearSize(int(kIndicatorWidth), 4);
    const QSize circleSize(40, 40);

    // --- linear determinate ---------------------------------------------------------
    context.section(L("线性 —— 确定值", "Linear — determinate"));
    context.paragraph(L(
        "md.comp.progress-indicator.linear：4 px 高、primary 活动指示器、"
        "secondary-container 轨道，双 corner-full。值变化沿 250 ms "
        "cubic-bezier(0.4, 0, 0.6, 1) 过渡。终点带着 4 px 间隙与 4 px 圆形"
        "停止指示器（trailing-space 0）；值到 1 时它们让位给整条指示器。",
        "md.comp.progress-indicator.linear: 4 px tall, primary active indicator on a "
        "secondary-container track, both corner-full. A value change transitions over 250 ms "
        "cubic-bezier(0.4, 0, 0.6, 1). The end carries a 4 px gap and the 4 px round stop "
        "indicator (trailing-space 0); at value 1 they give way to the full indicator."));
    {
        const QRectF band = context.band(4.0 * 3.0 + kStackGap * 2.0);
        const auto rows = {0.25, 0.6, 1.0};
        int row = 0;
        for (const qreal value : rows) {
            const QString id = QStringLiteral("l-%1").arg(row);
            md::MdProgressIndicator *bar = indicator(
                id, md::ProgressIndicatorShape::Linear, [value](md::MdProgressIndicator *w) {
                    w->setValue(value);
                });
            m_slots.append(ProgressSlot{
                bar, QRectF(band.left(), band.top() + row * (4.0 + kStackGap),
                            linearSize.width(), linearSize.height())});
            ++row;
        }
    }

    // --- linear buffer -----------------------------------------------------------------
    context.section(L("线性 —— 缓冲", "Linear — buffer"));
    context.paragraph(L(
        "buffer 大于 0 时轨道缩放到缓冲比例（scaleX 契约），缩过的轨道之外是"
        "周期 250 ms 的滚动圆点——点直径 2 px（轨道高度的一半）、间距 2.5 倍"
        "直径。这是 material-web 的点阵遮罩方案。",
        "With a buffer above 0 the track scales to the buffer fraction (the scaleX contract), "
        "and past the scaled track run dots scrolling on a 250 ms cycle — 2 px diameter (half "
        "the track height) at 2.5-diameter spacing. That is material-web's dot-mask scheme."));
    {
        const QRectF band = context.band(4.0);
        md::MdProgressIndicator *bar = indicator(
            QStringLiteral("l-buffer"), md::ProgressIndicatorShape::Linear,
            [](md::MdProgressIndicator *w) {
                w->setValue(0.4);
                w->setBuffer(0.7);
            });
        m_slots.append(ProgressSlot{bar, QRectF(band.topLeft(), linearSize)});
    }

    // --- linear indeterminate ------------------------------------------------------------
    context.section(L("线性 —— 不定态", "Linear — indeterminate"));
    context.paragraph(L(
        "不定态保留整条轨道，两条 primary 条在上面跑 MDC 的 2 s 关键帧——"
        "平移 0→200.611 %、缩放 0.08→0.661479→0.08，每段各自的贝塞尔。"
        "fourColor 打开后颜色按废弃四色集循环（primary → primary-container → "
        "tertiary → tertiary-container，4 s 一轮，颜色平滑插值）。",
        "The indeterminate mode keeps the full track and runs two primary bars over the MDC "
        "2 s keyframes — translate 0→200.611 %, scale 0.08→0.661479→0.08, each segment with "
        "its own bezier. fourColor cycles the deprecated four-color set instead (primary → "
        "primary-container → tertiary → tertiary-container, a 4 s cycle, colours interpolated "
        "smoothly)."));
    {
        const QRectF band = context.band(4.0 * 2.0 + kStackGap);
        md::MdProgressIndicator *spinning = indicator(
            QStringLiteral("l-spin"), md::ProgressIndicatorShape::Linear,
            [](md::MdProgressIndicator *w) { w->setIndeterminate(true); });
        m_slots.append(ProgressSlot{spinning, QRectF(band.topLeft(), linearSize)});

        md::MdProgressIndicator *rainbow = indicator(
            QStringLiteral("l-rainbow"), md::ProgressIndicatorShape::Linear,
            [](md::MdProgressIndicator *w) {
                w->setIndeterminate(true);
                w->setFourColor(true);
            });
        m_slots.append(ProgressSlot{
            rainbow, QRectF(band.left(), band.top() + 4.0 + kStackGap, linearSize.width(),
                            linearSize.height())});
    }

    // --- circular ----------------------------------------------------------------------------
    context.section(L("圆形", "Circular"));
    context.paragraph(L(
        "md.comp.progress-indicator.circular：40 px、描边 4 px、圆帽。确定值"
        "从 12 点钟顺时针扫过（500 ms 过渡）；不定态是三层旋转的组合——"
        "1333 ms 的弧扩张（265°↔130°）、4 倍周期的分组步进 135°、再加一层"
        "ARCTIME×360/306 ms 的线性自旋，右半环延迟半个周期。",
        "md.comp.progress-indicator.circular: 40 px, 4 px stroke, round caps. Determinate "
        "sweeps clockwise from 12 o'clock (500 ms transition); the indeterminate mode is "
        "three composed rotations — the 1333 ms arc expansion (265°↔130°), the eased 135° "
        "group steps over 4× that period, and a linear spin of ARCTIME×360/306 ms, the right "
        "half delayed half a period."));
    {
        const QRectF band = context.band(40.0);
        const auto columns = {0.3, 0.75, -1.0, -2.0};
        int column = 0;
        for (const qreal value : columns) {
            const QString id = QStringLiteral("c-%1").arg(column);
            md::MdProgressIndicator *ring = indicator(
                id, md::ProgressIndicatorShape::Circular,
                [value](md::MdProgressIndicator *w) {
                    if (value >= 0.0) {
                        w->setValue(value);
                    } else {
                        w->setIndeterminate(true);
                        w->setFourColor(value < -1.0);
                    }
                });
            m_slots.append(ProgressSlot{
                ring, QRectF(band.left() + column * (40.0 + kStackGap * 2.0), band.top(),
                             circleSize.width(), circleSize.height())});
            ++column;
        }
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ----------------------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-progress-indicator{,-linear,"
        "-circular}.scss + internal/_{linear,circular}-progress.scss (MDC-heritage keyframes)"));
    context.detail(QStringLiteral(
        "merged set   linear 4 px · circular 40 px · gap 4 px · stop 4 px + trailing 0 · "
        "primary / primary / secondary-container · all corner-full · no state rows at all"));
    context.detail(QStringLiteral(
        "deprecated    the two per-shape sets (four-color rows' only source) and the thick.* "
        "variant rows — transcribed, never exposed as an API"));
    context.detail(QStringLiteral(
        "indeterminate linear: the 2 s two-bar MDC keyframes · circular: expand 1333 ms "
        "(265°↔130°) + group ×135° over 5332 ms + linear spin 1567.06 ms · four-color 4 s / "
        "5332 ms with per-segment easing"));
    context.detail(QStringLiteral(
        "gap           the Expressive wave rows (amplitude 3 / 1.6 px, wavelength 40 / 15 px, "
        "with-wave 10 px / 48 px) are carried but not rendered — the family's registered gap, "
        "Compose's WavyProgressIndicator is the porting source"));
    context.space(8.0);
    context.detail(L("TestMd3ProgressIndicator 将合并导出的行、废弃行的转录、关键帧"
                     "纯函数、确定值过渡、缓冲契约与非交互契约逐字段锁定。",
                     "TestMd3ProgressIndicator pins the merged rows, the deprecated "
                     "transcriptions, the keyframe pure functions, the determinate "
                     "transitions, the buffer contract and the non-interactivity contract "
                     "field by field."));
}

void ProgressIndicatorPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const ProgressSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.indicator->geometry() != target) {
            slot.indicator->setGeometry(target);
        }
    }
}

void ProgressIndicatorPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ProgressIndicatorPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ProgressIndicatorPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ProgressIndicatorPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createProgressIndicatorPage()
{
    return new ProgressIndicatorPage;
}

} // namespace gallery

#include "GalleryPages17.moc"
