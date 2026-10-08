// Gallery page 19: Snackbars — the fourth Communication family,
// `md.comp.snackbar.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. The family-specific note:
// one published set, no variants — the inverse colour roles make the snackbar
// readable on any surface — and both interactive elements (action, dismiss)
// paint their own published hover / focus / pressed rows.

#include "GalleryPages.h"

#include "widgets/MdSnackbar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// One banked snackbar plus the content-local rect the latest build() gave it.
struct SnackbarSlot
{
    md::MdSnackbar *snackbar = nullptr;
    QRectF rect;
};

constexpr qreal kStackGap = 16.0;

} // namespace

class SnackbarPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit SnackbarPage(QWidget *parent = nullptr);
    ~SnackbarPage() override;

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
    md::MdSnackbar *snackbar(const QString &id, const QString &message,
                             const std::function<void(md::MdSnackbar *)> &configure = {});

    QHash<QString, md::MdSnackbar *> m_bank;
    QVector<SnackbarSlot> m_slots;
};

SnackbarPage::SnackbarPage(QWidget *parent)
    : GalleryPage(parent)
{
}

SnackbarPage::~SnackbarPage() = default;

QString SnackbarPage::title() const
{
    return L("Snackbar", "Snackbars");
}

QString SnackbarPage::slug() const
{
    return QStringLiteral("snackbars");
}

QString SnackbarPage::subtitle() const
{
    return L("md.comp.snackbar：inverse-surface 容器（level 3 阴影、extra-small "
             "圆角）上放 body-medium 的 supporting text（inverse-on-surface）、"
             "label-large 的 action（inverse-primary）与 24 px 的 close 图标"
             "（inverse-on-surface）。导出为 action 与 icon 各发布 hover / focus "
             "/ pressed 状态行——这是通信族里第一个把已发布状态行全部画出来的"
             "家族。布局移植自 Compose 的 OneRowSnackbar / NewLineButtonSnackbar："
             "单行高 max(48, 内容)、换行首行顶在 30 px、双行最小高 68；new-line "
             "布局把 action 换到消息下方的行尾。带 action 的 snackbar 不自行"
             "消失（宿主时长规则：Short 4 s / Long 10 s / Indefinite 永不）。",
             "md.comp.snackbar: a body-medium supporting text (inverse-on-surface), a "
             "label-large action (inverse-primary) and a 24 px close icon (inverse-on-"
             "surface) on an inverse-surface container (level 3 shadow, extra-small "
             "corner). The export publishes hover / focus / pressed rows for BOTH the "
             "action and the icon — the first Communication family that paints every "
             "published state row. The layouts port Compose's OneRowSnackbar / "
             "NewLineButtonSnackbar: single-line height max(48, content), wrapped "
             "first line at 30 px, 68 px two-line minimum; the new-line layout moves "
             "the action below the message. An actionable snackbar never self-dismisses "
             "(host durations: Short 4 s / Long 10 s / Indefinite never).");
}

md::MdSnackbar *SnackbarPage::snackbar(const QString &id, const QString &message,
                                       const std::function<void(md::MdSnackbar *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSnackbar(message, this);
    created->show();
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

void SnackbarPage::build(GalleryContext &context)
{
    m_slots.clear();
    const qreal contentWidth = context.width();

    // --- one row, message only ------------------------------------------------
    context.section(L("单行 —— 只有消息", "One row — message only"));
    context.paragraph(L(
        "md.comp.snackbar：48 px 的 inverse-surface 容器（corner-extra-small，"
        "level 3 阴影），消息以 body-medium 的 inverse-on-surface 垂直居中；"
        "无 dismiss 图标时文本尾部预留 8 px 的额外间距。",
        "md.comp.snackbar: the 48 px inverse-surface container (corner-extra-small, level 3 "
        "shadow) with the message centred in body-medium inverse-on-surface; without a "
        "dismiss icon the text keeps an 8 px extra end spacing."));
    {
        const qreal width = qMin<qreal>(360.0, contentWidth);
        md::MdSnackbar *plain = snackbar(QStringLiteral("plain"),
                                         L("已归档", "Message archived"));
        const QRectF band = context.band(plain->heightForWidth(int(width)));
        m_slots.append(SnackbarSlot{plain, QRectF(band.topLeft(), QSizeF(width, band.height()))});
    }

    // --- one row with action and dismiss ---------------------------------------
    context.section(L("单行 —— action 与 dismiss", "One row — action and dismiss"));
    context.paragraph(L(
        "action 是 label-large 的 inverse-primary 文本（命中区即按钮 chrome，"
        "标签居中其中），dismiss 是 24 px 的 inverse-on-surface close 图标"
        "（40 px 的 icon-button chrome 贴右缘）。两者都从本导出自己的状态行取"
        "hover / focus / pressed 层与涟漪色。",
        "The action is a label-large inverse-primary label (the hit region plays the button "
        "chrome, the label centred inside); the dismiss is a 24 px inverse-on-surface close "
        "icon in its 40 px icon-button chrome, flush right. Both take their hover / focus / "
        "pressed layers and ripple colours from this export's own state rows."));
    {
        const qreal width = qMin<qreal>(480.0, contentWidth);
        md::MdSnackbar *full = snackbar(QStringLiteral("full"),
                                        L("照片已删除", "Photo deleted"), [](md::MdSnackbar *w) {
                                            w->setActionLabel(L("撤销", "Undo"));
                                            w->setHasDismissAction(true);
                                        });
        const QRectF band = context.band(full->heightForWidth(int(width)));
        m_slots.append(SnackbarSlot{full, QRectF(band.topLeft(), QSizeF(width, band.height()))});
    }

    // --- one row, wrapped --------------------------------------------------------
    context.section(L("单行 —— 换行", "One row — wrapped"));
    context.paragraph(L(
        "消息放不下时首行顶在 30 px（HeightToFirstLine），容器长到双行的 "
        "68 px 最小高；文本宽度预算让位给 action 与 dismiss。",
        "When the message wraps, the first line sits 30 px from the top (HeightToFirstLine) "
        "and the container grows to the 68 px two-line minimum; the text width budget yields "
        "to the action and the dismiss."));
    {
        const qreal width = qMin<qreal>(420.0, contentWidth);
        md::MdSnackbar *wrapped = snackbar(
            QStringLiteral("wrapped"),
            L("文件已移动到归档文件夹，原位置的快捷方式已更新为新的路径。",
              "The file moved to the archive folder and the shortcut in its old place now "
              "points at the new path."));
        const QRectF band = context.band(wrapped->heightForWidth(int(width)));
        m_slots.append(
            SnackbarSlot{wrapped, QRectF(band.topLeft(), QSizeF(width, band.height()))});
    }

    // --- new line -----------------------------------------------------------------
    context.section(L("action 换行", "Action on a new line"));
    context.paragraph(L(
        "actionOnNewLine 把 action 放到消息下方的行尾（Compose 推荐用于长"
        "动作文本）：文本框上下 14 px，action 行距容器底 4 px、无 dismiss 时"
        "距右缘 8 px。",
        "actionOnNewLine puts the action below the message, right-aligned (Compose's "
        "recommendation for long action text): the text box keeps 14 px vertical padding and "
        "the action row sits 4 px above the container's bottom, 8 px in from the right edge "
        "without a dismiss icon."));
    {
        const qreal width = qMin<qreal>(480.0, contentWidth);
        md::MdSnackbar *newLine = snackbar(QStringLiteral("newline"),
                                           L("已从云端同步 128 个文件。", "Synced 128 files."),
                                           [](md::MdSnackbar *w) {
                                               w->setActionLabel(L("查看更改", "Review changes"));
                                               w->setActionOnNewLine(true);
                                               w->setHasDismissAction(true);
                                           });
        const QRectF band = context.band(newLine->heightForWidth(int(width)));
        m_slots.append(SnackbarSlot{newLine, QRectF(band.topLeft(), QSizeF(width, band.height()))});
    }

    Q_ASSERT(m_slots.size() == m_bank.size());

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-snackbar.scss (34.0.21) "
        "+ Compose M3 Snackbar.kt / SnackbarHost.kt"));
    context.detail(QStringLiteral(
        "export      container inverse-surface · level 3 · shadow · corner-extra-small · "
        "single-line 48 px · two-lines 68 px"));
    context.detail(QStringLiteral(
        "export      supporting-text inverse-on-surface body-medium · icon 24 px "
        "inverse-on-surface · action inverse-primary label-large"));
    context.detail(QStringLiteral(
        "state rows  action + icon: hover 0.08 · focus 0.12 · pressed 0.12 (the ripple "
        "colour follows the pressed row)"));
    context.detail(QStringLiteral(
        "compose     container 600 max · start 16 · text-end extra 8 · vertical 14 · "
        "first line 30 · action bottom 4 · host margin 12"));
    context.detail(QStringLiteral(
        "host        Short 4000 ms · Long 10000 ms · Indefinite never · Auto = Indefinite "
        "with an action · fade+scale on effects-fast / spatial-fast springs"));
    context.space(8.0);
    context.detail(L("TestMd3Snackbar 将导出行、覆盖解析、两张状态行表、时长规则、"
                     "两种布局几何、指针与键盘激活契约以及宿主队列/过渡逐字段锁定。",
                     "TestMd3Snackbar pins the export rows, the override parsing, both state "
                     "row tables, the duration rules, both layout geometries, the pointer "
                     "and keyboard activation contracts and the host queue/transition field "
                     "by field."));
}

void SnackbarPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const SnackbarSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.snackbar->geometry() != target) {
            slot.snackbar->setGeometry(target);
        }
    }
}

void SnackbarPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void SnackbarPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void SnackbarPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void SnackbarPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createSnackbarPage()
{
    return new SnackbarPage;
}

} // namespace gallery

#include "GalleryPages19.moc"
