// Gallery page 22: Dialogs — the Containment family's basic dialog,
// `md.comp.dialog.*`.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. Family-specific notes:
// a dialog is a modal surface — the static snapshots show the container with
// its content, and the live demo raises a real MdDialogHost overlay across
// the whole page (scrim, centring, Escape / outside-click dismissal).

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdDialog.h"
#include "widgets/MdDialogHost.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <functional>

namespace gallery {

namespace {

/// One banked dialog plus the content-local rect the latest build() gave it.
struct DialogSlot
{
    md::MdDialog *dialog = nullptr;
    QRectF rect;
};

/// One banked demo button plus its rect.
struct ButtonSlot
{
    md::MdButton *button = nullptr;
    QRectF rect;
};

} // namespace

class DialogPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit DialogPage(QWidget *parent = nullptr);
    ~DialogPage() override;

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
    md::MdDialog *dialog(const QString &id,
                         const std::function<void(md::MdDialog *)> &configure = {});
    md::MdButton *button(const QString &id, const QString &text,
                         const std::function<void(md::MdButton *)> &configure = {});

    QHash<QString, md::MdDialog *> m_bank;
    QVector<DialogSlot> m_slots;
    QHash<QString, md::MdButton *> m_buttonBank;
    QVector<ButtonSlot> m_buttonSlots;
    md::MdDialogHost *m_demoHost = nullptr;
};

DialogPage::DialogPage(QWidget *parent)
    : GalleryPage(parent)
{
}

DialogPage::~DialogPage() = default;

QString DialogPage::title() const
{
    return L("Dialog", "Dialogs");
}

QString DialogPage::slug() const
{
    return QStringLiteral("dialogs");
}

QString DialogPage::subtitle() const
{
    return L(
        "md.comp.dialog：surface-container-high 容器静置 level3、corner-extra-large，"
        "标题 headline-small（on-surface）、支持文本 body-medium（on-surface-variant）、"
        "可选 24 px 图标（secondary）、操作按钮 label-large（primary）。Compose 的"
        "槽位布局：24 px 容器内边距、图标下 16、标题下 16、文本下 24，操作行右对齐、"
        "间距 8，宽度夹在 280..560。按钮是真正的 Text 按钮（导出的操作行就是文本按钮"
        "自己的 token），涟漪与 :focus-visible 免费。宿主负责 scrim（black 0.32）、"
        "居中、Escape 与外点关闭——onDismissRequest 语义。",
        "md.comp.dialog: a surface-container-high container at level3 with the "
        "extra-large corner, headline-small title (on-surface), body-medium supporting "
        "text (on-surface-variant), an optional 24 px icon (secondary) and label-large "
        "actions (primary). Compose's slot layout: 24 px container padding, icon bottom "
        "16, title bottom 16, text bottom 24, end-aligned action rows at 8 px spacing, "
        "width clamped into 280..560. The buttons are real Text buttons — the export's "
        "action rows ARE the text button's tokens — so ripple and :focus-visible come "
        "free. The host owns the scrim (black 0.32), centring, and the Escape / "
        "outside-click onDismissRequest flow.");
}

md::MdDialog *DialogPage::dialog(const QString &id,
                                 const std::function<void(md::MdDialog *)> &configure)
{
    const auto found = m_bank.constFind(id);
    if (found != m_bank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDialog(this);
    if (configure) {
        configure(created);
    }
    m_bank.insert(id, created);
    return created;
}

md::MdButton *DialogPage::button(const QString &id, const QString &text,
                                 const std::function<void(md::MdButton *)> &configure)
{
    const auto found = m_buttonBank.constFind(id);
    if (found != m_buttonBank.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdButton(text, this);
    if (configure) {
        configure(created);
    }
    m_buttonBank.insert(id, created);
    return created;
}

void DialogPage::build(GalleryContext &context)
{
    m_slots.clear();
    m_buttonSlots.clear();
    const qreal contentWidth = context.width();

    // --- basic dialog with everything -------------------------------------------
    context.section(L("基本对话框", "Basic dialog"));
    context.paragraph(L(
        "图标、标题、支持文本与两个操作齐全的形态：图标 24 px 居中（下 16），"
        "标题 headline-small（下 16），支持文本 body-medium 自动换行（下 24），"
        "操作行右对齐——dismiss 在左、confirm 在右。",
        "The icon, title, supporting text and two actions in one shape: the 24 px icon "
        "centred (bottom 16), the headline-small title (bottom 16), the body-medium "
        "supporting text wrapping (bottom 24), and the end-aligned action row — dismiss "
        "left of confirm."));
    {
        md::MdDialog *full = dialog(QStringLiteral("full"), [](md::MdDialog *w) {
            w->setTitle(L("重置所有设置？", "Reset settings?"));
            w->setText(L(
                "此操作会把全部设置恢复到默认值，且无法撤销。请确认你要继续。",
                "This resets every setting to its default and cannot be undone. "
                "Confirm to continue."));
            auto *confirm = new md::MdButton(w);
            confirm->setText(L("接受", "Accept"));
            confirm->setVariant(md::ButtonVariant::Text);
            w->setConfirmButton(confirm);
            auto *dismiss = new md::MdButton(w);
            dismiss->setText(L("取消", "Cancel"));
            dismiss->setVariant(md::ButtonVariant::Text);
            w->setDismissButton(dismiss);
        });
        const qreal width = qMin<qreal>(360.0, contentWidth);
        const QRectF band = context.band(qreal(full->heightForWidth(int(width))));
        m_slots.append(DialogSlot{
            full, QRectF(band.topLeft(), QSizeF(width, full->heightForWidth(int(width))))});
    }

    // --- text-only dialog ---------------------------------------------------------
    context.section(L("仅文本", "Text only"));
    context.paragraph(L(
        "没有图标与标题的形态：支持文本从容器顶部内边距直接开始，单个操作按钮"
        "仍右对齐。",
        "No icon and no title: the supporting text starts right at the container's top "
        "padding, and the single action button stays end-aligned."));
    {
        md::MdDialog *textOnly = dialog(QStringLiteral("text-only"), [](md::MdDialog *w) {
            w->setText(L(
                "这是一条较长的提示文本——没有标题时它就是全部内容，宽度受限时"
                "自动换行，容器随之变高。",
                "A longer supporting text with no title — it is the whole content here, "
                "wrapping when the width runs out and the container grows with it."));
            auto *confirm = new md::MdButton(w);
            confirm->setText(L("知道了", "Got it"));
            confirm->setVariant(md::ButtonVariant::Text);
            w->setConfirmButton(confirm);
        });
        const qreal width = qMin<qreal>(280.0, contentWidth);
        const QRectF band = context.band(qreal(textOnly->heightForWidth(int(width))));
        m_slots.append(DialogSlot{
            textOnly,
            QRectF(band.topLeft(), QSizeF(width, textOnly->heightForWidth(int(width))))});
    }

    // --- live demo ------------------------------------------------------------------
    context.section(L("活动演示", "Live demo"));
    context.paragraph(L(
        "点击按钮弹出真正的对话框宿主：scrim（black 0.32）盖住整页、对话框居中，"
        "Escape 或点击 scrim 外侧触发 onDismissRequest。",
        "The button raises a real dialog host: the scrim (black 0.32) covers the page, "
        "the dialog sits centred, and Escape or a scrim click runs onDismissRequest."));
    {
        md::MdButton *demo = button(QStringLiteral("demo"),
                                    L("显示对话框", "Show dialog"));
        if (!m_demoHost) {
            m_demoHost = new md::MdDialogHost(this);
            m_demoHost->dialog()->setTitle(L("重置所有设置？", "Reset settings?"));
            m_demoHost->dialog()->setText(L(
                "此操作会把全部设置恢复到默认值，且无法撤销。请确认你要继续。",
                "This resets every setting to its default and cannot be undone. "
                "Confirm to continue."));
            auto *confirm = new md::MdButton(m_demoHost->dialog());
            confirm->setText(L("接受", "Accept"));
            confirm->setVariant(md::ButtonVariant::Text);
            m_demoHost->dialog()->setConfirmButton(confirm);
            auto *dismiss = new md::MdButton(m_demoHost->dialog());
            dismiss->setText(L("取消", "Cancel"));
            dismiss->setVariant(md::ButtonVariant::Text);
            m_demoHost->dialog()->setDismissButton(dismiss);
            connect(demo, &md::MdButton::clicked, m_demoHost, &md::MdDialogHost::show);
        }
        const qreal buttonWidth = qreal(demo->sizeHint().width());
        const qreal buttonHeight = qreal(demo->sizeHint().height());
        const QRectF band = context.band(buttonHeight);
        m_buttonSlots.append(
            ButtonSlot{demo, QRectF(band.topLeft(), QSizeF(buttonWidth, buttonHeight))});
    }

    // --- token facts -----------------------------------------------------------------
    context.section(L("本页锁定的令牌事实", "Token facts this page is pinned to"));
    context.detail(QStringLiteral(
        "src: material-web tokens/versions/latest/sass/_md-comp-dialog.scss (34.0.21) + "
        "Compose M3 AlertDialog.kt (no web component published)"));
    context.detail(QStringLiteral(
        "export      container surface-container-high level3 corner-extra-large · headline "
        "headline-small on-surface · text body-medium on-surface-variant · icon 24 "
        "secondary · action label-large primary (hover 0.08 / focus 0.12 / pressed 0.12)"));
    context.detail(QStringLiteral(
        "compose     24 px padding · icon bottom 16 · title bottom 16 · text bottom 24 · "
        "rows end-aligned 8 px · width 280..560 · scrim black 0.32 · fade only"));
    context.space(8.0);
    context.detail(L("TestMd3Dialog 将 token 行表、操作状态行、覆盖解析、布局几何"
                     "（含图标居中与操作换行）、宽度夹取、指针与键盘契约、宿主"
                     "淡入淡出与居中几何逐字段锁定。",
                     "TestMd3Dialog pins the token rows, the action state rows, the "
                     "override parsing, the layout geometry (icon centring and action "
                     "wrapping included), the width bounds, the pointer and keyboard "
                     "contracts and the host fade/centring field by field."));
}

void DialogPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const DialogSlot &slot : m_slots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.dialog->geometry() != target) {
            slot.dialog->setGeometry(target);
        }
    }
    for (const ButtonSlot &slot : m_buttonSlots) {
        const QRect target((content.topLeft() + slot.rect.topLeft()).toPoint(),
                           slot.rect.size().toSize());
        if (slot.button->geometry() != target) {
            slot.button->setGeometry(target);
        }
    }
}

void DialogPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void DialogPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void DialogPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void DialogPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createDialogPage()
{
    return new DialogPage;
}

} // namespace gallery

#include "GalleryPages22.moc"
