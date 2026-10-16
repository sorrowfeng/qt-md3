// Gallery page 51: Stage 2 Qt extensions — MdSkeleton + MdSteps +
// MdUpload + MdRate + MdFlex.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These are Appendix D
// Qt extensions; the page shows the derived designs as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdSkeleton — Surface-container-highest placeholder + shimmer.
//   * MdSteps    — shape + connecting lines, Progress indicator semantics.
//   * MdUpload   — List rows + Progress indicator + Icon button.
//   * MdRate     — Icon button + selected state.
//   * MdFlex     — MD3 layout flex / spacing-scale semantics.

#include "GalleryPages.h"

#include "widgets/MdFlex.h"
#include "widgets/MdRate.h"
#include "widgets/MdSkeleton.h"
#include "widgets/MdSteps.h"
#include "widgets/MdUpload.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QLabel>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class Stage2DataPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit Stage2DataPage(QWidget *parent = nullptr);
    ~Stage2DataPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void placeChildren();

    md::MdSkeleton *skeleton(const QString &id);
    md::MdSteps *steps(const QString &id);
    md::MdUpload *upload(const QString &id);
    md::MdRate *rate(const QString &id);
    md::MdFlex *flex(const QString &id);

    QHash<QString, md::MdSkeleton *> m_skeletons;
    QHash<QString, md::MdSteps *> m_steps;
    QHash<QString, md::MdUpload *> m_uploads;
    QHash<QString, md::MdRate *> m_rates;
    QHash<QString, md::MdFlex *> m_flexes;
    QVector<Slot> m_slots;
};

Stage2DataPage::Stage2DataPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2DataPage::~Stage2DataPage() = default;

QString Stage2DataPage::title() const
{
    return L("Qt 扩展 · 数据与布局", "Qt extensions · data & layout");
}

QString Stage2DataPage::slug() const
{
    return QStringLiteral("qt-extensions-data");
}

QString Stage2DataPage::subtitle() const
{
    return L(
        "阶段二 Qt 扩展：骨架屏（surface-container-highest + shimmer）、"
        "步骤条（shape + 连接线）、上传列表（List + Progress + Icon button）、"
        "评分（Icon button + 选中态）与 Flex 布局（spacing scale）。",
        "Stage 2 Qt extensions: the skeleton (surface-container-highest + "
        "shimmer), the stepper (shape + connectors), the upload list "
        "(List + Progress + Icon button), the rate control (Icon button + "
        "selected) and the Flex layout (spacing scale).");
}

md::MdSkeleton *Stage2DataPage::skeleton(const QString &id)
{
    const auto found = m_skeletons.constFind(id);
    if (found != m_skeletons.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSkeleton(this);
    m_skeletons.insert(id, created);
    return created;
}

md::MdSteps *Stage2DataPage::steps(const QString &id)
{
    const auto found = m_steps.constFind(id);
    if (found != m_steps.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSteps(this);
    m_steps.insert(id, created);
    return created;
}

md::MdUpload *Stage2DataPage::upload(const QString &id)
{
    const auto found = m_uploads.constFind(id);
    if (found != m_uploads.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdUpload(this);
    m_uploads.insert(id, created);
    return created;
}

md::MdRate *Stage2DataPage::rate(const QString &id)
{
    const auto found = m_rates.constFind(id);
    if (found != m_rates.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdRate(this);
    m_rates.insert(id, created);
    return created;
}

md::MdFlex *Stage2DataPage::flex(const QString &id)
{
    const auto found = m_flexes.constFind(id);
    if (found != m_flexes.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFlex(this);
    m_flexes.insert(id, created);
    return created;
}

void Stage2DataPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- MdSkeleton ---------------------------------------------------------
    context.section(L("MdSkeleton — Surface-container-highest 占位 + shimmer",
                      "MdSkeleton — Surface-container-highest placeholder + shimmer"));
    context.paragraph(L(
        "加载占位块：surface-container-highest 底色，shimmer 高光沿对角线"
        "扫过（相位驱动，不是 QEasingCurve）。支持 rect / circle / text-line"
        " 三种轮廓。",
        "Loading placeholder blocks on surface-container-highest, with a "
        "shimmer highlight sweeping the diagonal (phase-driven, never a "
        "QEasingCurve). Rect / circle / text-line silhouettes."));
    {
        auto *text = skeleton(QStringLiteral("sk-text"));
        text->setShape(md::MdSkeletonShape::TextLine);
        text->setLines(3);
        const QRectF band = context.band(72.0);
        m_slots.append({text, QRectF(band.left(), band.top(),
                                     qMin(420.0, band.width() - 16.0), 64.0)});

        auto *row = skeleton(QStringLiteral("sk-row"));
        row->setShape(md::MdSkeletonShape::Circle);
        row->setShimmering(true);
        const QRectF band2 = context.band(64.0);
        m_slots.append({row, QRectF(band2.left(), band2.top(), 56.0, 56.0)});

        auto *block = skeleton(QStringLiteral("sk-block"));
        block->setShape(md::MdSkeletonShape::Rect);
        const QRectF band3 = context.band(96.0);
        m_slots.append({block, QRectF(band3.left() + 72.0, band3.top(),
                                      180.0, 88.0)});
    }
    context.detail(L("surface-container-highest + surface-container-high shimmer",
                     "surface-container-highest + surface-container-high shimmer"));

    context.space(16.0);

    // --- MdSteps ------------------------------------------------------------
    context.section(L("MdSteps — shape + 连接线（Progress 语义）",
                      "MdSteps — shape + connectors (Progress semantics)"));
    context.paragraph(L(
        "步骤指示：完成步 primary 圆盘 + check，当前步 primary-container，"
        "未到步 surface-container-highest。连接线在完成段切到 primary。",
        "Step indicators: completed discs in primary with a check, the "
        "current disc in primary-container, upcoming discs in "
        "surface-container-highest. Connectors switch to primary once the "
        "step before them is complete."));
    {
        auto *wizard = steps(QStringLiteral("st-demo"));
        wizard->setStepCount(4);
        wizard->setCurrentStep(1);
        wizard->setLabels(QStringList()
                          << L("购物车", "Cart") << L("配送", "Ship")
                          << L("支付", "Pay") << L("完成", "Done"));
        const QRectF band = context.band(96.0);
        m_slots.append({wizard, QRectF(band.left(), band.top(),
                                       qMin(520.0, band.width() - 16.0), 88.0)});

        auto *vertical = steps(QStringLiteral("st-vert"));
        vertical->setStepCount(3);
        vertical->setCurrentStep(2);
        vertical->setOrientation(md::MdStepsOrientation::Vertical);
        vertical->setLabels(QStringList()
                            << L("上传", "Upload") << L("处理", "Process")
                            << L("发布", "Publish"));
        const QRectF band2 = context.band(140.0);
        m_slots.append({vertical, QRectF(band2.left(), band2.top(), 220.0, 132.0)});
    }
    context.detail(L("primary / primary-container / surface-container-highest",
                     "primary / primary-container / surface-container-highest"));

    context.space(16.0);

    // --- MdUpload -----------------------------------------------------------
    context.section(L("MdUpload — List + Progress indicator + Icon button",
                      "MdUpload — List + Progress indicator + Icon button"));
    context.paragraph(L(
        "文件队列：List 行语义（56px 行高、outline-variant 分隔线），"
        "传输中显示真正的 MdProgressIndicator，行尾是 MdIconButton 关闭按钮。",
        "File queue: List row semantics (56 px rows, outline-variant "
        "dividers), a real MdProgressIndicator while transferring, and a "
        "trailing MdIconButton to remove the entry."));
    {
        auto *queue = upload(QStringLiteral("up-demo"));
        queue->clearFiles();
        queue->addFile(QStringLiteral("design-system.fig"), 2048);
        queue->addFile(QStringLiteral("release-notes.md"), 128);
        queue->addFile(QStringLiteral("demo-reel.mp4"), 8192);
        queue->setFileProgress(0, 0.62, true);
        queue->setFileProgress(1, 1.0, false);
        queue->setFileProgress(2, 0.15, true);
        const QRectF band = context.band(176.0);
        m_slots.append({queue, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 168.0)});
    }
    context.detail(L("List 56px + Progress + IconButton",
                     "List 56px + Progress + IconButton"));

    context.space(16.0);

    // --- MdRate -------------------------------------------------------------
    context.section(L("MdRate — Icon button + 选中态",
                      "MdRate — Icon button + selected state"));
    context.paragraph(L(
        "评分：星形（或心形）Material Symbols 字形，选中态 primary 实心，"
        "未选 on-surface-variant 描边。支持半星、悬停预览与键盘操作。",
        "Rating: star (or heart) Material Symbols glyphs, filled primary when "
        "selected, outlined on-surface-variant otherwise. Half steps, hover "
        "preview and keyboard walk."));
    {
        auto *stars = rate(QStringLiteral("rt-demo"));
        stars->setValue(3.0);
        stars->setMax(5);
        stars->setHalfSteps(true);
        const QRectF band = context.band(56.0);
        m_slots.append({stars, QRectF(band.left(), band.top(), 240.0, 48.0)});

        auto *hearts = rate(QStringLiteral("rt-heart"));
        hearts->setGlyph(md::MdRateGlyph::Heart);
        hearts->setValue(4.0);
        hearts->setReadOnly(true);
        const QRectF band2 = context.band(56.0);
        m_slots.append({hearts, QRectF(band2.left(), band2.top(), 240.0, 48.0)});
    }
    context.detail(L("primary selected + on-surface-variant unselected",
                     "primary selected + on-surface-variant unselected"));

    context.space(16.0);

    // --- MdFlex -------------------------------------------------------------
    context.section(L("MdFlex — MD3 layout flex 语义",
                      "MdFlex — MD3 layout flex semantics"));
    context.paragraph(L(
        "布局容器：direction / justify / align / gap（spacing scale）/ wrap。"
        "间距步进来自 MD3 的 4dp 间距标度（0 / 4 / 8 / 12 / 16 / 24 / 32）。",
        "Layout container: direction / justify / align / gap (spacing scale) "
        "/ wrap. The gap steps are the MD3 4 dp spacing scale "
        "(0 / 4 / 8 / 12 / 16 / 24 / 32)."));
    {
        auto *row = flex(QStringLiteral("fx-row"));
        row->setDirection(md::MdFlexDirection::Row);
        row->setGap(md::MdFlexSpacing::Large);
        row->setAlign(md::MdFlexAlign::Center);
        if (row->findChildren<QWidget *>().isEmpty()) {
            for (int i = 0; i < 3; ++i) {
                auto *chip = new QLabel(QString::number(i + 1), row);
                chip->setAlignment(Qt::AlignCenter);
                chip->setFixedSize(48, 32);
                chip->setFont(GalleryPage::fontFor(md::TypeStyle::LabelLarge));
            }
        }
        const QRectF band = context.band(56.0);
        m_slots.append({row, QRectF(band.left(), band.top(),
                                    qMin(520.0, band.width() - 16.0), 48.0)});

        auto *wrap = flex(QStringLiteral("fx-wrap"));
        wrap->setDirection(md::MdFlexDirection::Row);
        wrap->setWrap(true);
        wrap->setGap(md::MdFlexSpacing::Medium);
        if (wrap->findChildren<QWidget *>().isEmpty()) {
            for (int i = 0; i < 5; ++i) {
                auto *box = new QLabel(QString::number(i + 1), wrap);
                box->setAlignment(Qt::AlignCenter);
                box->setFixedSize(64, 28);
                box->setFont(GalleryPage::fontFor(md::TypeStyle::LabelMedium));
            }
        }
        const QRectF band2 = context.band(72.0);
        m_slots.append({wrap, QRectF(band2.left(), band2.top(),
                                     qMin(320.0, band2.width() - 16.0), 64.0)});
    }
    context.detail(L("spacing scale 0 / 4 / 8 / 12 / 16 / 24 / 32",
                     "spacing scale 0 / 4 / 8 / 12 / 16 / 24 / 32"));
}

void Stage2DataPage::placeChildren()
{
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(slot.rect.toRect());
        }
    }
}

void Stage2DataPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2DataPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2DataPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createStage2DataPage()
{
    return new Stage2DataPage;
}

} // namespace gallery

#include "GalleryPages51.moc"
