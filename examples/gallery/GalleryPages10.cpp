// Gallery page 10: Split buttons — the sixth Actions family.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// the two halves are one control with two press targets — colours are the
// button family's, the metric rows (including the morphing inner corners) are
// the split button's own.

#include "GalleryPages.h"

#include "widgets/MdSplitButton.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>

namespace gallery {

namespace {

/// One banked split button plus the content-local rect the latest build() gave it.
struct SplitSlot
{
    md::MdSplitButton *button = nullptr;
    QRectF rect;
};

/// Gap between neighbouring split buttons in a row.
constexpr qreal kRowGap = 24.0;
/// Gap between stacked rows inside one section.
constexpr qreal kStackGap = 16.0;

} // namespace

class SplitButtonPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SplitButtonPage(QWidget *parent = nullptr);
    ~SplitButtonPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void layFlow(GalleryContext &context, const QVector<md::MdSplitButton *> &buttons);
    void layoutChildren();
    void placeChildren();

    md::MdSplitButton *split(const QString &id, const QString &label);
    template <typename Configure>
    md::MdSplitButton *split(const QString &id, const QString &label, Configure configure)
    {
        const bool created = !m_bank.contains(id);
        md::MdSplitButton *widget = split(id, label);
        if (created) {
            configure(widget);
        }
        return widget;
    }

    QHash<QString, md::MdSplitButton *> m_bank;
    QVector<SplitSlot> m_slots;
};

SplitButtonPage::SplitButtonPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SplitButtonPage::~SplitButtonPage() = default;

QString SplitButtonPage::title() const
{
    return L("拆分按钮", "Split buttons");
}

QString SplitButtonPage::slug() const
{
    return QStringLiteral("Split buttons");
}

QString SplitButtonPage::subtitle() const
{
    return L("一颗药丸切成两半、中间留 2 px：左边执行主操作，右边下拉更多选项。"
             "内角在悬停 / 按压时会变圆——这是这个族独有的形状形变。",
             "One pill cut in two with a 2 px gap: the leading half runs the action, the "
             "trailing half opens more options. The facing corners round off on hover / press "
             "— a shape morph unique to this family.");
}

md::MdSplitButton *SplitButtonPage::split(const QString &id, const QString &label)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSplitButton(label, this);
    created->show();
    m_bank.insert(id, created);
    return created;
}

void SplitButtonPage::layFlow(GalleryContext &context, const QVector<md::MdSplitButton *> &buttons)
{
    const qreal available = context.width();

    QVector<md::MdSplitButton *> row;
    qreal rowWidth = 0.0;
    qreal rowHeight = 0.0;

    const auto flush = [&] {
        if (row.isEmpty()) {
            return;
        }
        const QRectF band = context.band(rowHeight);
        qreal x = band.left();
        for (md::MdSplitButton *button : row) {
            const QSize hint = button->sizeHint();
            m_slots.append(SplitSlot{
                button,
                QRectF(x, band.top() + (rowHeight - hint.height()) / 2.0, hint.width(),
                       hint.height())});
            x += hint.width() + kRowGap;
        }
        context.space(kStackGap);
        row.clear();
        rowWidth = 0.0;
        rowHeight = 0.0;
    };

    for (md::MdSplitButton *button : buttons) {
        const QSize hint = button->sizeHint();
        const qreal needed =
            row.isEmpty() ? qreal(hint.width()) : rowWidth + kRowGap + hint.width();
        if (!row.isEmpty() && needed > available) {
            flush();
        }
        if (!row.isEmpty()) {
            rowWidth += kRowGap;
        }
        row.append(button);
        rowWidth += hint.width();
        rowHeight = qMax(rowHeight, qreal(hint.height()));
    }
    flush();
}

void SplitButtonPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- five colour sets ---------------------------------------------------
    context.section(L("五种颜色样式 —— 借自按钮族", "Five colour sets — borrowed from the buttons"));
    context.paragraph(L(
        "规范页写明“拆分按钮使用与标准按钮相同的配色方案”，所以颜色行来自 md.comp.button.*，"
        "本族只发布度量行。选中不变色、只加状态层——这一点和开关按钮不同。",
        "The spec page: \"Split buttons use the same color schemes as standard buttons\", so "
        "the colour rows come from md.comp.button.* and this family publishes metric rows "
        "only. Selection never changes the colour — just a state layer, unlike toggle "
        "buttons."));
    {
        const QVector<QPair<QString, md::ButtonVariant>> specs = {
            {QStringLiteral("Elevated"), md::ButtonVariant::Elevated},
            {QStringLiteral("Filled"), md::ButtonVariant::Filled},
            {QStringLiteral("Tonal"), md::ButtonVariant::Tonal},
            {QStringLiteral("Outlined"), md::ButtonVariant::Outlined},
            {QStringLiteral("Text"), md::ButtonVariant::Text},
        };
        QVector<md::MdSplitButton *> row;
        for (const auto &spec : specs) {
            row.append(split(QStringLiteral("variant-") + spec.first,
                             L("操作", "Action"), [&spec](md::MdSplitButton *widget) {
                                 widget->setVariant(spec.second);
                                 widget->setTrailingIcon(QStringLiteral("expand_more"));
                             }));
        }
        layFlow(context, row);
    }

    // --- five sizes -----------------------------------------------------------
    context.section(L("五种尺寸 —— 内角形变", "Five sizes — the inner-corner morph"));
    context.paragraph(L(
        "md.comp.split-button.<size>：32 / 40 / 56 / 96 / 136 px，外角恒为完整药丸，"
        "内角（两半相向的角）从 4 px 起步，悬停 / 按压时弹到 8 / 12 / 20 px——"
        "把指针悬停上去看两半的相向角变圆。",
        "md.comp.split-button.<size>: 32 / 40 / 56 / 96 / 136 px. The outer corners stay a "
        "full pill; the facing corners start at 4 px and spring to 8 / 12 / 20 px on hover / "
        "press — hover one half and watch its facing corners round off."));
    {
        QVector<md::MdSplitButton *> row;
        for (int i = 0; i < int(md::SplitButtonSize::Count); ++i) {
            const auto size = md::SplitButtonSize(i);
            row.append(split(QStringLiteral("size-") + md::splitButtonSizeName(size),
                             L("操作", "Action"), [size](md::MdSplitButton *widget) {
                                 widget->setVariant(md::ButtonVariant::Tonal);
                                 widget->setSplitSize(size);
                                 widget->setTrailingIcon(QStringLiteral("expand_more"));
                             }));
        }
        layFlow(context, row);
    }

    // --- trailing selected ----------------------------------------------------
    context.section(L("trailing 选中 —— 菜单打开态", "Trailing selected — the open-menu state"));
    context.paragraph(L(
        "trailing 选中时，它的相向角变成字面意义的 50%——两半之间的缝隙被完全"
        "圆角封住。菜单本体是宿主的职责：点开时把 trailingSelected 置真、关闭时置回。",
        "While the trailing half is selected its facing corners go to the literal 50% — the "
        "gap between the halves seals itself with fully rounded corners. The menu itself is "
        "the host's job: set trailingSelected while it is open and clear it when it closes."));
    {
        auto *selected = split(QStringLiteral("selected"), L("已展开", "Open"),
                               [](md::MdSplitButton *widget) {
                                   widget->setVariant(md::ButtonVariant::Filled);
                                   widget->setTrailingSelected(true);
                                   widget->setTrailingIcon(QStringLiteral("expand_more"));
                               });
        auto *resting = split(QStringLiteral("unselected"), L("已收起", "Closed"),
                              [](md::MdSplitButton *widget) {
                                  widget->setVariant(md::ButtonVariant::Filled);
                                  widget->setTrailingIcon(QStringLiteral("expand_more"));
                              });
        layFlow(context, {selected, resting});
    }

    // --- disabled ---------------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "禁用行走按钮族的同一行：on-surface 容器 0.12、文字 / 图标 0.38、无投影。"
        "soft-disabled 仍可聚焦，便于工具栏场景。",
        "The disabled row is the button family's: on-surface container at 0.12, text / icon "
        "at 0.38, no shadow. A soft-disabled control stays keyboard-focusable for toolbar "
        "use."));
    {
        QVector<md::MdSplitButton *> row;
        for (int i = 0; i < int(md::ButtonVariant::Count); ++i) {
            const auto variant = md::ButtonVariant(i);
            row.append(split(QStringLiteral("off-") + QString::number(i), L("操作", "Action"),
                             [variant](md::MdSplitButton *widget) {
                                 widget->setVariant(variant);
                                 widget->setTrailingIcon(QStringLiteral("expand_more"));
                                 widget->setSoftDisabled(true);
                             }));
        }
        layFlow(context, row);
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts ------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-split-button-<size>.scss"));
    context.detail(QStringLiteral(
        "sizes    xsmall 32 · small 40 · medium 56 · large 96 · xlarge 136   gap 2   "
        "icons 22 / 22 / 26 / 38 / 50"));
    context.detail(QStringLiteral(
        "corners  outer corner-full (height/2) · inner rest 4/4/4/8/12 · hovered+pressed "
        "8/12/12/20/20 · trailing selected 50%"));
    context.detail(QStringLiteral(
        "colours  none published for this family — the button family's rows come across "
        "(spec page: \"the same color schemes as standard buttons\"); selection adds a "
        "state layer only"));
    context.detail(QStringLiteral(
        "motion   no rows published — the corner morph runs on the button family's "
        "spring-fast-spatial (1400 / 0.9), the Compose Expressive choice"));
    context.space(8.0);
    context.detail(L("TestMd3SplitButton 将五行尺寸表、借用的颜色行、内外角几何与每个半区"
                     "的涟漪 / 焦点规则逐字段锁定。",
                     "TestMd3SplitButton pins the five-row size table, the borrowed colour "
                     "rows, the inner/outer corner geometry and the per-half ripple / focus "
                     "rules field by field."));
}

void SplitButtonPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SplitSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void SplitButtonPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SplitButtonPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SplitButtonPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SplitButtonPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSplitButtonPage()
{
    return new SplitButtonPage;
}

} // namespace gallery

#include "GalleryPages10.moc"
