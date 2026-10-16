// Gallery page 54: Stage 2 Qt extensions — MdDescriptions + MdTimeline +
// MdImage + MdWatermark + MdTransfer + MdMentions + MdBackTop.
//
// Same rules as the other component pages: widgets are banked, build() is
// content-local rects only, placeChildren() runs from resize/show/paint.

#include "GalleryPage.h"
#include "I18n.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdBackTop.h"
#include "widgets/MdDescriptions.h"
#include "widgets/MdFab.h"
#include "widgets/MdImage.h"
#include "widgets/MdMentions.h"
#include "widgets/MdTimeline.h"
#include "widgets/MdTransfer.h"
#include "widgets/MdWatermark.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QPixmap>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

QPixmap demoPixmap()
{
    QPixmap px(160, 120);
    px.fill(QColor(0x88, 0x44, 0x22));
    QPainter p(&px);
    p.fillRect(20, 20, 60, 40, QColor(0xff, 0xdd, 0xaa));
    p.fillRect(90, 50, 40, 40, QColor(0x44, 0x88, 0x66));
    return px;
}

class Stage2DetailPage : public GalleryPage
{
public:
    explicit Stage2DetailPage(QWidget *parent = nullptr);
    ~Stage2DetailPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;

    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    struct Slot
    {
        QWidget *widget = nullptr;
        QRectF rect;
    };

    md::MdDescriptions *descriptions(const QString &id);
    md::MdTimeline *timeline(const QString &id);
    md::MdImage *image(const QString &id);
    md::MdWatermark *watermark(const QString &id);
    md::MdTransfer *transfer(const QString &id);
    md::MdMentions *mentions(const QString &id);
    md::MdBackTop *backTop(const QString &id);

    void placeChildren();

    QHash<QString, md::MdDescriptions *> m_descriptions;
    QHash<QString, md::MdTimeline *> m_timelines;
    QHash<QString, md::MdImage *> m_images;
    QHash<QString, md::MdWatermark *> m_watermarks;
    QHash<QString, md::MdTransfer *> m_transfers;
    QHash<QString, md::MdMentions *> m_mentions;
    QHash<QString, md::MdBackTop *> m_backTops;
    QList<Slot> m_slots;
};

Stage2DetailPage::Stage2DetailPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2DetailPage::~Stage2DetailPage() = default;

QString Stage2DetailPage::title() const
{
    return QStringLiteral("Stage 2 · Detail & Overlay");
}

QString Stage2DetailPage::slug() const
{
    return QStringLiteral("qt-extensions-detail");
}

QString Stage2DetailPage::subtitle() const
{
    return L("MdDescriptions / MdTimeline / MdImage / MdWatermark / "
             "MdTransfer / MdMentions / MdBackTop",
             "MdDescriptions / MdTimeline / MdImage / MdWatermark / "
             "MdTransfer / MdMentions / MdBackTop");
}

md::MdDescriptions *Stage2DetailPage::descriptions(const QString &id)
{
    const auto found = m_descriptions.constFind(id);
    if (found != m_descriptions.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDescriptions(this);
    created->setVisible(false);
    m_descriptions.insert(id, created);
    return created;
}

md::MdTimeline *Stage2DetailPage::timeline(const QString &id)
{
    const auto found = m_timelines.constFind(id);
    if (found != m_timelines.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTimeline(this);
    created->setVisible(false);
    m_timelines.insert(id, created);
    return created;
}

md::MdImage *Stage2DetailPage::image(const QString &id)
{
    const auto found = m_images.constFind(id);
    if (found != m_images.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdImage(this);
    created->setVisible(false);
    m_images.insert(id, created);
    return created;
}

md::MdWatermark *Stage2DetailPage::watermark(const QString &id)
{
    const auto found = m_watermarks.constFind(id);
    if (found != m_watermarks.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdWatermark(this);
    created->setVisible(false);
    m_watermarks.insert(id, created);
    return created;
}

md::MdTransfer *Stage2DetailPage::transfer(const QString &id)
{
    const auto found = m_transfers.constFind(id);
    if (found != m_transfers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTransfer(this);
    created->setVisible(false);
    m_transfers.insert(id, created);
    return created;
}

md::MdMentions *Stage2DetailPage::mentions(const QString &id)
{
    const auto found = m_mentions.constFind(id);
    if (found != m_mentions.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdMentions(this);
    created->setVisible(false);
    m_mentions.insert(id, created);
    return created;
}

md::MdBackTop *Stage2DetailPage::backTop(const QString &id)
{
    const auto found = m_backTops.constFind(id);
    if (found != m_backTops.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdBackTop(this);
    created->setVisible(false);
    m_backTops.insert(id, created);
    return created;
}

void Stage2DetailPage::build(GalleryContext &context)
{
    m_slots.clear();

    context.section(L("MdDescriptions — 键值列表",
                      "MdDescriptions — key / value list"));
    context.paragraph(L(
        "LabelMedium / on-surface-variant 对 BodyMedium / on-surface，"
        "纵向或横向，可选 outline-variant 分隔。",
        "LabelMedium on on-surface-variant against BodyMedium on on-surface. "
        "Vertical or horizontal stacking, optional outline-variant rules."));
    {
        const QRectF band = context.band(140.0);
        auto *d = descriptions(QStringLiteral("desc-demo"));
        d->setBordered(true);
        d->setItems(QList<md::MdDescriptionItem>()
                    << md::MdDescriptionItem(L("姓名", "Name"),
                                             QStringLiteral("Ada Lovelace"))
                    << md::MdDescriptionItem(L("角色", "Role"),
                                             QStringLiteral("Engineer"))
                    << md::MdDescriptionItem(L("语言", "Language"),
                                             QStringLiteral("C++ / Qt")));
        d->setVisible(true);
        m_slots.append({d, QRectF(band.left(), band.top(),
                                  qMin(360.0, band.width() - 16.0), 132.0)});
    }
    context.detail(L("LabelMedium vs BodyMedium", "LabelMedium vs BodyMedium"));
    context.space(16.0);

    context.section(L("MdTimeline — List + 连线", "MdTimeline — list + connector"));
    context.paragraph(L(
        "列表行 + 圆点 + 连线。Default/Active/Complete/Error 映射 "
        "outline-variant / primary / error。",
        "List rows joined by dots and a connector. Default / Active / "
        "Complete / Error map to outline-variant / primary / error."));
    {
        const QRectF band = context.band(220.0);
        auto *t = timeline(QStringLiteral("tl-demo"));
        t->setItems(QList<md::MdTimelineItem>()
                    << md::MdTimelineItem(L("创建草稿", "Draft created"),
                                          QStringLiteral("Monday"),
                                          QStringLiteral("09:00"),
                                          md::MdTimelineStatus::Complete)
                    << md::MdTimelineItem(L("评审中", "In review"),
                                          QStringLiteral("Tuesday"),
                                          QStringLiteral("14:00"),
                                          md::MdTimelineStatus::Active)
                    << md::MdTimelineItem(L("已合并", "Merged"),
                                          QString(),
                                          QStringLiteral("Wed"),
                                          md::MdTimelineStatus::Default));
        t->setVisible(true);
        m_slots.append({t, QRectF(band.left(), band.top(),
                                  qMin(420.0, band.width() - 16.0), 212.0)});
    }
    context.detail(L("TitleMedium + BodyMedium + status dot",
                     "TitleMedium + BodyMedium + status dot"));
    context.space(16.0);

    context.section(L("MdImage — 图块 + 全屏预览",
                      "MdImage — tile + full-screen preview"));
    context.paragraph(L(
        "ShapeCorner 裁切、surface-container-lowest 垫底；点击进入全屏 "
        "scrim 预览，滚轮缩放，Esc 退出。",
        "ShapeCorner clip over surface-container-lowest. Click opens a "
        "full-screen scrim preview; wheel zooms, Esc closes."));
    {
        const QRectF band = context.band(150.0);
        auto *img = image(QStringLiteral("img-demo"));
        img->setPixmap(demoPixmap());
        img->setAltText(L("演示图片", "Demo image"));
        img->setVisible(true);
        m_slots.append({img, QRectF(band.left(), band.top(),
                                    qMin(240.0, band.width() - 16.0), 142.0)});
    }
    context.detail(L("Cover + ShapeCorner::Medium + scrim preview",
                     "Cover + ShapeCorner::Medium + scrim preview"));
    context.space(16.0);

    context.section(L("MdWatermark — 低透明度平铺",
                      "MdWatermark — low-alpha tile"));
    context.paragraph(L(
        "on-surface @ state-layer hover (0.08) 倾斜平铺，"
        "鼠标穿透，不挡下层交互。",
        "on-surface at the state-layer hover opacity (0.08), tiled at an "
        "angle. Mouse-transparent so the content underneath stays live."));
    {
        const QRectF band = context.band(120.0);
        auto *wm = watermark(QStringLiteral("wm-demo"));
        wm->setText(L("内部草稿", "DRAFT"));
        wm->setVisible(true);
        m_slots.append({wm, QRectF(band.left(), band.top(),
                                   qMin(320.0, band.width() - 16.0), 112.0)});
    }
    context.space(16.0);

    context.section(L("MdTransfer — 双列表穿梭", "MdTransfer — dual-list shuttle"));
    context.paragraph(L(
        "List × 2 + Text 按钮。选中行 secondary-container，"
        "方向键走行，Space 切换选中。",
        "Two List panels plus Text buttons. Selected rows take "
        "secondary-container; arrows walk, Space toggles."));
    {
        const QRectF band = context.band(240.0);
        auto *tr = transfer(QStringLiteral("tr-demo"));
        tr->setSourceTitle(L("可选", "Available"));
        tr->setTargetTitle(L("已选", "Chosen"));
        tr->setSourceItems(QList<md::MdTransferItem>()
                           << md::MdTransferItem(QStringLiteral("a"),
                                                 QStringLiteral("Alpha"))
                           << md::MdTransferItem(QStringLiteral("b"),
                                                 QStringLiteral("Beta"))
                           << md::MdTransferItem(QStringLiteral("c"),
                                                 QStringLiteral("Gamma")));
        tr->setTargetItems(QList<md::MdTransferItem>()
                           << md::MdTransferItem(QStringLiteral("d"),
                                                 QStringLiteral("Delta")));
        tr->setVisible(true);
        m_slots.append({tr, QRectF(band.left(), band.top(),
                                   qMin(520.0, band.width() - 16.0), 232.0)});
    }
    context.space(16.0);

    context.section(L("MdMentions — Text field + Menu",
                      "MdMentions — text field + menu"));
    context.paragraph(L(
        "输入 @ 弹出 MdMenu 候选，Enter 提交为 @Name。"
        "Esc 关闭弹层，保留已输入文本。",
        "Typing @ opens an MdMenu of candidates; Enter commits @Name. "
        "Esc closes the popup and keeps the typed text."));
    {
        const QRectF band = context.band(64.0);
        auto *mn = mentions(QStringLiteral("mn-demo"));
        mn->setLabelText(L("消息", "Message"));
        mn->setPlaceholderText(L("输入 @ 提及某人…", "Type @ to mention…"));
        mn->setItems(QStringList() << QStringLiteral("Ada")
                                   << QStringLiteral("Alan")
                                   << QStringLiteral("Grace"));
        mn->setVisible(true);
        m_slots.append({mn, QRectF(band.left(), band.top(),
                                   qMin(360.0, band.width() - 16.0), 56.0)});
    }
    context.space(16.0);

    context.section(L("MdBackTop — FAB small 回顶",
                      "MdBackTop — small FAB back-to-top"));
    context.paragraph(L(
        "滚动超过阈值后显示 small FAB（40 px，corner-medium），"
        "点击回到顶部。",
        "A small FAB (40 px, corner-medium) appears past the scroll "
        "threshold and returns the surface to the top."));
    {
        const QRectF band = context.band(56.0);
        auto *bt = backTop(QStringLiteral("bt-demo"));
        bt->fab()->show();
        bt->setVisible(true);
        m_slots.append({bt, QRectF(band.left(), band.top(), 40.0, 40.0)});
    }
    context.detail(L("md.comp.fab.small.* + arrow_upward",
                     "md.comp.fab.small.* + arrow_upward"));
    context.space(16.0);
}

void Stage2DetailPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(
                QRect(qRound(content.left() + slot.rect.left()),
                      qRound(content.top() + slot.rect.top()),
                      qRound(slot.rect.width()),
                      qRound(slot.rect.height())));
        }
    }
}

void Stage2DetailPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2DetailPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2DetailPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

} // namespace

GalleryPage *createStage2DetailPage()
{
    return new Stage2DetailPage;
}

} // namespace gallery
