// Gallery page 42: Text fields — `md.comp.{filled,outlined}-text-field.*`
// at export 34.0.21.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Every field on this page is
// a live MdTextField — the floating label, the active indicator's state spine
// and the error overlay are all the real component.
//
// Two things this page is explicit about because they are easy to misread:
//
//   * **the label floats** — body-large in the input line at rest, body-small
//     on the container's top edge once populated or focused;
//   * **the indicator is the state's spine** — 1 px on-surface-variant
//     resting, on-surface hover, 2 px primary focus, error when errored.

#include "GalleryPages.h"

#include "core/MdTextFieldTokens.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QSizePolicy>

namespace gallery {

namespace {

struct TextFieldSlot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

} // namespace

class TextFieldPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit TextFieldPage(QWidget *parent = nullptr);
    ~TextFieldPage() override;

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

    md::MdTextField *tf(const QString &id);
    md::MdTextArea *area(const QString &id);

    QHash<QString, md::MdTextField *> m_fields;
    QHash<QString, md::MdTextArea *> m_areas;
    QVector<TextFieldSlot> m_slots;
};

TextFieldPage::TextFieldPage(QWidget *parent)
    : GalleryPage(parent)
{
}

TextFieldPage::~TextFieldPage() = default;

QString TextFieldPage::title() const
{
    return L("文本框", "Text field");
}

QString TextFieldPage::slug() const
{
    return QStringLiteral("text-field");
}

QString TextFieldPage::subtitle() const
{
    return L(
        "文本框：56dp 容器（filled 是 surface-container-highest + corner-extra-small-top，"
        "outlined 是 1dp 描边 + corner-small）。标签**浮动**：静止时 body-large 在输入行，"
        "填充或聚焦后升到顶部 body-small。活动指示条是状态脊柱 —— 1dp on-surface-variant"
        "静止、on-surface 悬停、2dp primary 聚焦、error 错误。导出的两套（filled / "
        "outlined）共享一个结构。",
        "Text field: a 56dp container (filled is surface-container-highest with "
        "corner-extra-small-top, outlined is a 1dp outline with corner-small). The label "
        "**floats**: body-large in the input line at rest, rising to body-small on the top "
        "edge once populated or focused. The active indicator is the state's spine — 1dp "
        "on-surface-variant resting, on-surface hover, 2dp primary focus, error when "
        "errored. The export's two sets (filled / outlined) share one structure.");
}

md::MdTextField *TextFieldPage::tf(const QString &id)
{
    const auto found = m_fields.constFind(id);
    if (found != m_fields.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTextField(this);
    created->hide();
    m_fields.insert(id, created);
    return created;
}

/// Banked: build() is re-entrant, and creating a MdTextArea on every pass
/// would leave the previous instance painting beneath (the menu page's rule).
md::MdTextArea *TextFieldPage::area(const QString &id)
{
    const auto found = m_areas.constFind(id);
    if (found != m_areas.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdTextArea(this);
    created->hide();
    m_areas.insert(id, created);
    return created;
}

void TextFieldPage::build(GalleryContext &context)
{
    m_slots.clear();

    auto addRow = [&](const QVector<QPair<QString, md::MdTextField *>> &fields,
                      qreal width = 280.0) {
        if (fields.isEmpty()) {
            return;
        }
        const qreal pitch = width + 24.0;
        const qreal bandHeight = 76.0;
        const QRectF band = context.band(bandHeight);
        const qreal span = (fields.size() - 1) * pitch + width;
        qreal x = band.left() + qMax<qreal>(0.0, (band.width() - span) / 2.0);
        for (const auto &entry : fields) {
            m_slots.append(TextFieldSlot{entry.second,
                                         QRectF(x, band.top(), width, bandHeight)});
            x += pitch;
        }
    };

    // --- filled vs outlined ------------------------------------------------
    context.section(L("两套：filled 与 outlined", "The two sets: filled and outlined"));
    context.paragraph(L(
        "filled 是 surface-container-highest 容器 + corner-extra-small-top（4px 只圆顶部）；"
        "outlined 是透明容器 + 1dp 描边 + corner-small（四角 4px）。",
        "Filled is a surface-container-highest container with corner-extra-small-top "
        "(4px top corners only); outlined is a transparent container with a 1dp outline "
        "and corner-small (4px all round)."));
    {
        auto *filled = tf(QStringLiteral("filled"));
        filled->setLabelText(QStringLiteral("Filled"));
        auto *outlined = tf(QStringLiteral("outlined"));
        outlined->setVariant(md::MdTextFieldVariant::Outlined);
        outlined->setLabelText(QStringLiteral("Outlined"));
        addRow({{QStringLiteral("a"), filled}, {QStringLiteral("b"), outlined}});
        context.detail(L("filled · outlined", "filled · outlined"));
        context.space(16.0);
    }

    // --- the floating label ------------------------------------------------
    context.section(L("浮动标签", "The floating label"));
    context.paragraph(L(
        "静止时标签在输入行（body-large）；一旦输入文字或聚焦，升到容器顶部（body-small）。"
        "聚焦时标签变 primary。",
        "At rest the label sits in the input line (body-large); once the field is "
        "populated or focused it rises to the container's top edge (body-small). Under "
        "focus the label turns primary."));
    {
        auto *rest = tf(QStringLiteral("rest"));
        rest->setLabelText(QStringLiteral("Resting"));
        auto *filledField = tf(QStringLiteral("populated"));
        filledField->setLabelText(QStringLiteral("Populated"));
        filledField->setText(QStringLiteral("hello"));
        addRow({{QStringLiteral("a"), rest}, {QStringLiteral("b"), filledField}});
        context.detail(L("静止 · 已填充", "resting · populated"));
        context.space(16.0);
    }

    // --- error ------------------------------------------------------------
    context.section(L("错误态", "The error state"));
    context.paragraph(L(
        "error 属性把指示条、标签、辅助文字和尾图标切到 error 色（导出的 error 覆盖族）。",
        "The error property swaps the indicator, label, supporting text and trailing icon "
        "to the error colour (the export's error overlay family)."));
    {
        auto *err = tf(QStringLiteral("error"));
        err->setLabelText(QStringLiteral("Email"));
        err->setSupportingText(QStringLiteral("Enter a valid email"));
        err->setError(true);
        addRow({{QStringLiteral("a"), err}});
        context.detail(L("错误态 + 辅助文字", "error + supporting text"));
        context.space(16.0);
    }

    // --- multiline: MdTextArea ---------------------------------------------
    context.section(L("多行：MdTextArea", "Multiline: MdTextArea"));
    context.paragraph(L(
        "同一套 token 服务两个形态（导出无 text-area 命名空间）。标签同样浮动；容器包住"
        "整个多行正文，指示条在最后一行之下。rows 是正文行数（默认 2）。",
        "One token set serves both forms (the export publishes no text-area namespace)."
        " The label floats the same way; the container wraps the whole multiline body"
        " with the indicator under the last line. rows is the body's line count (default"
        " 2)."));

    {
        auto *shortArea = area(QStringLiteral("short-area"));
        shortArea->setLabelText(QStringLiteral("Short"));
        shortArea->setRows(2);
        auto *tallArea = area(QStringLiteral("tall-area"));
        tallArea->setLabelText(QStringLiteral("Tall"));
        tallArea->setSupportingText(QStringLiteral("rows = 6"));
        tallArea->setRows(6);
        const QRectF band = context.band(200.0);
        m_slots.append(TextFieldSlot{shortArea, QRectF(band.left(), band.top(), 280.0, 96.0)});
        m_slots.append(TextFieldSlot{tallArea, QRectF(band.left() + 304.0, band.top(), 280.0, 200.0)});
        context.detail(L("rows = 2 · rows = 6", "rows = 2 · rows = 6"));
        context.space(16.0);
    }

    // --- disabled ---------------------------------------------------------
    context.section(L("禁用", "Disabled"));
    context.paragraph(L(
        "禁用按行淡出：容器 0.04、指示条 0.38、文字与图标 0.38。",
        "Disabled fades by row: the container at 0.04, the indicator at 0.38, the text "
        "and icons at 0.38."));
    {
        auto *off = tf(QStringLiteral("disabled"));
        off->setLabelText(QStringLiteral("Disabled"));
        off->setText(QStringLiteral("hello"));
        off->setEnabled(false);
        addRow({{QStringLiteral("a"), off}});
        context.detail(L("禁用 + 已填充", "disabled + populated"));
        context.space(16.0);
    }

    // --- what this page is pinned to -----------------------------------------
    context.section(L("本页钉住的 token 事实", "Token facts this page is pinned to"));
    context.paragraph(L(
        "下面每一行都由 TestMd3TextField 直接断言，动一行就变红。",
        "Every line below is asserted directly by TestMd3TextField; moving one turns it "
        "red."));
    context.detail(L("容器 56dp · 图标 24dp · 指示条 1/1/2/1dp（静止/悬停/聚焦/禁用）· "
                     "容器圆角 4dp（filled 只圆顶部）· 禁用容器 0.04、内容 0.38",
                     "the container 56dp · icons 24dp · the indicator 1/1/2/1dp "
                     "(rest/hover/focus/disabled) · the container radius 4dp (filled top "
                     "only) · disabled container 0.04, content 0.38"));
    context.detail(L("颜色按状态直解：指示条 on-surface-variant · on-surface · primary · "
                     "标签聚焦 primary、error 全家 error",
                     "colours resolve by state: the indicator on-surface-variant · "
                     "on-surface · primary · the label primary under focus, the error "
                     "family all error"));
    context.detail(L("标签浮动：body-large 静止 → body-small 填充/聚焦 · 两套共享一个"
                     "结构 · 导出的 focus-active-indicator-thickness 读系统 3dp 行，"
                     "指示条本身用 2dp 的 focus 行",
                     "the label floats: body-large resting → body-small populated/focused "
                     "· the two sets share one structure · the export's "
                     "focus-active-indicator-thickness reads the system 3dp row while the "
                     "indicator itself uses the 2dp focus row"));
    context.space(8.0);
}

void TextFieldPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const TextFieldSlot &slot : m_slots) {
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

void TextFieldPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void TextFieldPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void TextFieldPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void TextFieldPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createTextFieldPage()
{
    return new TextFieldPage;
}

} // namespace gallery

#include "GalleryPages42.moc"
