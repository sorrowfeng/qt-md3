// Gallery page 41: Date pickers — `md.comp.date-picker-modal.*` at 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every calendar on this page
// is a live MdDatePicker — the 40 px date cells, today's outline, the range
// fill and the year face are all the real component.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * **the grid is Monday-first** — the weekday row starts on Monday and the
//     day→slot mapping is the picker's own `dateAtCell` contract, the same
//     helper the hit test uses;
//   * **the export publishes no disabled rows** — the date / year tables fall
//     back to on-surface at 0.38.

#include "GalleryPages.h"

#include "core/MdDatePickerTokens.h"
#include "widgets/MdDatePicker.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct DatePickerSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class DatePickerPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit DatePickerPage(QWidget *parent = nullptr);
    ~DatePickerPage() override;

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

    md::MdDatePicker *dp(const QString &id);

    QHash<QString, md::MdDatePicker *> m_pickers;
    QVector<DatePickerSlot> m_slots;
};

DatePickerPage::DatePickerPage(QWidget *parent)
    : GalleryPage(parent)
{
}

DatePickerPage::~DatePickerPage() = default;

QString DatePickerPage::title() const
{
    return L("日期选择器", "Date picker");
}

QString DatePickerPage::slug() const
{
    return QStringLiteral("date-picker");
}

QString DatePickerPage::subtitle() const
{
    return L(
        "日期选择器：360×524 容器（surface-container-high @ level3、corner-extra-large），"
        "120dp 头部（headline-large + label-large），40dp 日期格（corner-full）。选中 "
        "primary / on-primary；今天是 1dp primary 描边 + primary 文字；区间形式用 "
        "secondary-container 填充。周行从**周一开始**，日→槽映射是 dateAtCell 契约"
        "（与命中测试同一函数）。导出不发布禁用行 —— 回退到 on-surface @0.38。",
        "Date picker: a 360×524 container (surface-container-high @ level3, "
        "corner-extra-large), a 120dp header (headline-large + label-large) and 40dp date "
        "cells (corner-full). Selected primary / on-primary; today carries a 1dp primary "
        "outline with a primary label; the range form fills with secondary-container. The "
        "weekday row starts on **Monday** and the day→slot mapping is the dateAtCell "
        "contract (the same helper the hit test uses). The export publishes no disabled "
        "rows — falling back to on-surface @0.38.");
}

md::MdDatePicker *DatePickerPage::dp(const QString &id)
{
    const auto found = m_pickers.constFind(id);
    if (found != m_pickers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDatePicker(this);
    created->hide();
    m_pickers.insert(id, created);
    return created;
}

void DatePickerPage::build(GalleryContext &context)
{
    m_slots.clear();

    auto addPicker = [&](const QString &id, qreal width = 380.0) {
        auto *picker = dp(id);
        const QSize hint = picker->sizeHint();
        const QRectF band = context.band(hint.height());
        m_slots.append(DatePickerSlot{
            picker, QRectF(band.left() + (band.width() - width) / 2.0, band.top(), width,
                           hint.height())});
    };

    // --- the calendar -------------------------------------------------------
    context.section(L("日历", "The calendar"));
    context.paragraph(L(
        "选中的日期填 primary / on-primary；今天是 1dp primary 描边。点击头部的副标题"
        "文字切到年份面。方向键走日期（左右一天、上下一周），PageUp/PageDown 翻月。",
        "The selected date fills primary / on-primary; today carries the 1dp primary "
        "outline. Clicking the header's supporting text switches to the year face. The "
        "arrow keys walk the dates (left/right a day, up/down a week), PageUp/PageDown "
        "turn the month."));
    auto *basic = dp(QStringLiteral("basic"));
    basic->setSelectedDate(QDate(2026, 6, 15));
    basic->setDisplayedMonth(QDate(2026, 6, 1));
    addPicker(QStringLiteral("basic"));
    context.detail(L("2026 年 6 月 15 日", "June 15, 2026"));
    context.space(16.0);

    // --- range ------------------------------------------------------------
    context.section(L("区间选择", "Range selection"));
    context.paragraph(L(
        "range 形式用 secondary-container 填充区间内的日期（label 是 "
        "on-secondary-container），端点是 primary 的 40dp 药丸。",
        "The range form fills the in-range days with secondary-container (the label is "
        "on-secondary-container); the endpoints carry the primary 40dp pill."));
    auto *range = dp(QStringLiteral("range"));
    range->setRange(true);
    range->setRangeStart(QDate(2026, 6, 10));
    range->setRangeEnd(QDate(2026, 6, 20));
    range->setSelectedDate(QDate(2026, 6, 15));
    range->setDisplayedMonth(QDate(2026, 6, 1));
    addPicker(QStringLiteral("range"));
    context.detail(L("2026 年 6 月 10 日 �� 20 日", "June 10 �� 20, 2026"));
    context.space(16.0);

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页钉住的 token 事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "下面每一行都由 TestMd3DatePicker 直接断言，动一行就变红。",
        "Every line below is asserted directly by TestMd3DatePicker; moving one turns it "
        "red."));
    context.detail(L("容器 360×524 �� 头部 120dp �� 日期格 40dp �� 年份格 72×36dp �� "
                     "今天描边 1dp �� 区间指示 40dp",
                     "the container 360×524 �� the header 120dp �� date cells 40dp �� "
                     "year cells 72×36dp �� the today outline 1dp �� the range indicator "
                     "40dp"));
    context.detail(L("颜色按状态直解：选中 primary / on-primary、今天 primary、未选中 "
                     "on-surface、区间 secondary-container / on-secondary-container",
                     "colours resolve by state: selected primary / on-primary, today "
                     "primary, unselected on-surface, the range secondary-container / "
                     "on-secondary-container"));
    context.detail(L("周行从周一开始 �� 日→槽映射是 dateAtCell 契约 �� 弃用行携带不读"
                     "（*-state-layer-opcaity 拼写行、container-surface-tint-layer-color）"
                     "�� docked 变体是另一个导出，未移植",
                     "the weekday row starts on Monday �� the day→slot mapping is the "
                     "dateAtCell contract �� deprecated rows carried, not read (the "
                     "*-state-layer-opcaity typo rows, container-surface-tint-layer-"
                     "color) �� the docked variant is a separate export, not ported"));
    context.space(8.0);
}

void DatePickerPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const DatePickerSlot &slot : m_slots) {
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

void DatePickerPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void DatePickerPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void DatePickerPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void DatePickerPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createDatePickerPage()
{
    return new DatePickerPage;
}

} // namespace gallery

#include "GalleryPages41.moc"
