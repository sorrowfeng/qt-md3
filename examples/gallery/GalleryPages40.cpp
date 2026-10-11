// Gallery page 40: Time pickers — `md.comp.time-picker.*` at export 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every dial on this page is
// a live MdTimePicker — the 256 px dial, the 48 px selector handle, the face
// / period switching and the keyboard walk are all the real component.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * **the dial snaps** — hours to 1, minutes to 5-minute slots (Compose's
//     contract); the handle is a 48 px circle around an 8 px centre on a
//     2 px track, all primary;
//   * **the export publishes no disabled rows** — the selectors fall back to
//     on-surface at 0.38.

#include "GalleryPages.h"

#include "core/MdTimePickerTokens.h"
#include "widgets/MdTimePicker.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct TimePickerSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class TimePickerPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit TimePickerPage(QWidget *parent = nullptr);
    ~TimePickerPage() override;

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

    md::MdTimePicker *tp(const QString &id);

    QHash<QString, md::MdTimePicker *> m_pickers;
    QVector<TimePickerSlot> m_slots;
};

TimePickerPage::TimePickerPage(QWidget *parent)
    : GalleryPage(parent)
{
}

TimePickerPage::~TimePickerPage() = default;

QString TimePickerPage::title() const
{
    return L("时间选择器", "Time picker");
}

QString TimePickerPage::slug() const
{
    return QStringLiteral("time-picker");
}

QString TimePickerPage::subtitle() const
{
    return L(
        "时间选择器：256dp 表盘（corner-full、surface-container-highest），48dp 选择手柄"
        "绕 8dp 中心点在 2dp 轨道上，全部 primary。时间选择器 96×80（24h 单框 114dp），"
        "选中 primary-container；AM/PM 选择器 216×38（outlined corner-small），选中 "
        "tertiary-container。表盘**吸附**：小时到 1，分钟到 5 分钟槽。容器 "
        "surface-container-high @ level3、corner-extra-large。导出不发布禁用行 —— 选择器"
        "回退到 on-surface @0.38。",
        "Time picker: a 256dp dial (corner-full, surface-container-highest) with a 48dp "
        "selector handle around an 8dp centre on a 2dp track, all primary. Time selectors "
        "96×80 (114dp in 24h mode), selected primary-container; the AM/PM period selector "
        "216×38 (outlined corner-small), selected tertiary-container. The dial **snaps**: "
        "hours to 1, minutes to 5-minute slots. The container is surface-container-high "
        "@ level3 with corner-extra-large. The export publishes no disabled rows — the "
        "selectors fall back to on-surface @0.38.");
}

md::MdTimePicker *TimePickerPage::tp(const QString &id)
{
    const auto found = m_pickers.constFind(id);
    if (found != m_pickers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTimePicker(this);
    created->hide();
    m_pickers.insert(id, created);
    return created;
}

void TimePickerPage::build(GalleryContext &context)
{
    m_slots.clear();

    auto addPicker = [&](const QString &id, qreal width = 320.0) {
        auto *picker = tp(id);
        const QSize hint = picker->sizeHint();
        const QRectF band = context.band(hint.height());
        m_slots.append(TimePickerSlot{
            picker, QRectF(band.left() + (band.width() - width) / 2.0, band.top(), width,
                           hint.height())});
    };

    // --- hours face ---------------------------------------------------------
    context.section(L("小时面", "The hours face"));
    context.paragraph(L(
        "点击小时选择器切到小时面；表盘手柄跟随指针，吸附到最近的小时。选中的数字是 "
        "on-primary（在 primary 手柄上），未选中是 on-surface。",
        "Clicking the hours selector switches to the hours face; the handle follows the "
        "pointer and snaps to the nearest hour. The selected number is on-primary (on the "
        "primary handle), the unselected ones on-surface."));
    addPicker(QStringLiteral("hours"));
    context.detail(L("12h 模式，10:30", "12h mode, 10:30"));
    context.space(16.0);

    // --- minutes face ---------------------------------------------------------
    context.section(L("分钟面", "The minutes face"));
    context.paragraph(L(
        "分钟面吸附到 5 分钟槽（Compose 的契约）。Tab 在两个面之间切换，方向键步进。",
        "The minutes face snaps to 5-minute slots (Compose's contract). Tab switches "
        "between the faces, the arrow keys step."));
    auto *minutes = tp(QStringLiteral("minutes"));
    minutes->setFace(md::MdTimePickerFace::Minutes);
    minutes->setTime(QTime(10, 30));
    addPicker(QStringLiteral("minutes"));
    context.detail(L("分钟面，10:30", "the minutes face, 10:30"));
    context.space(16.0);

    // --- 24h mode ---------------------------------------------------------
    context.section(L("24 小时制", "24-hour mode"));
    context.paragraph(L(
        "24h 模式用一个 114dp 的时间选择器（导出行 time-selector.24h-vertical.container."
        "width），不带 AM/PM 选择器。",
        "24h mode uses a single 114dp time selector (the export's "
        "time-selector.24h-vertical.container.width row) and no period selector."));
    auto *h24 = tp(QStringLiteral("h24"));
    h24->set24h(true);
    h24->setTime(QTime(18, 45));
    addPicker(QStringLiteral("h24"));
    context.detail(L("24h 模式，18:45", "24h mode, 18:45"));
    context.space(16.0);

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页钉住的 token 事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "下面每一行都由 TestMd3TimePicker 直接断言，动一行就变红。",
        "Every line below is asserted directly by TestMd3TimePicker; moving one turns it "
        "red."));
    context.detail(L("表盘 256dp · 选择手柄 48dp · 中心 8dp · 轨道 2dp（全 primary）· "
                     "时间选择器 96×80 / 24h 114×80 · AM/PM 216×38（outlined）",
                     "the dial 256dp · the selector handle 48dp · the centre 8dp · the "
                     "track 2dp (all primary) · time selectors 96×80 / 24h 114×80 · the "
                     "period selector 216×38 (outlined)"));
    context.detail(L("容器 surface-container-high @ level3、corner-extra-large · 表盘 "
                     "surface-container-highest · 选中 primary-container / "
                     "tertiary-container",
                     "the container surface-container-high @ level3, corner-extra-large · "
                     "the dial surface-container-highest · selected primary-container / "
                     "tertiary-container"));
    context.detail(L("表盘吸附：小时到 1、分钟到 5 分钟槽 · 导出不发布禁用行（回退 "
                     "on-surface @0.38）· 弃用行携带不读（clock-dial.color.ignore / "
                     "shape.ignore、surface-tint-layer-color）",
                     "the dial snaps: hours to 1, minutes to 5-minute slots · the export "
                     "publishes no disabled rows (falling back to on-surface @0.38) · "
                     "deprecated rows carried, not read (clock-dial.color.ignore / "
                     "shape.ignore, surface-tint-layer-color)"));
    context.space(8.0);
}

void TimePickerPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const TimePickerSlot &slot : m_slots) {
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

void TimePickerPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void TimePickerPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void TimePickerPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void TimePickerPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createTimePickerPage()
{
    return new TimePickerPage;
}

} // namespace gallery

#include "GalleryPages40.moc"
