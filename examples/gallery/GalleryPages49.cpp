// Gallery page 49: Stage 2 shell & input extensions — MdWindow + MdFileDialog
// + MdInputNumber + MdScaffold.
//
// Same rules as the other component pages: widgets are banked, build() is
// re-entrant, and the page never styles anything. These are Appendix D
// Qt extensions; the page shows the derived designs as real live widgets.
//
// MD3 semantics these anchor to, from the Appendix D table:
//   * MdWindow     — Surface + Top app bar; title-bar buttons are IconButtons.
//   * MdFileDialog — Dialog + Navigation drawer (left) + List + Text field.
//   * MdInputNumber — Text field + Icon button steppers.
//   * MdScaffold   — App bar + Navigation + FAB + body page skeleton.

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdFab.h"
#include "widgets/MdFileDialog.h"
#include "widgets/MdInputNumber.h"
#include "widgets/MdScaffold.h"
#include "widgets/MdTopAppBar.h"
#include "widgets/MdWindow.h"

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

class Stage2ShellPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit Stage2ShellPage(QWidget *parent = nullptr);
    ~Stage2ShellPage() override;

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

    md::MdWindow *window(const QString &id);
    md::MdInputNumber *inputNumber(const QString &id);
    md::MdFileDialog *fileDialog(const QString &id);
    md::MdScaffold *scaffold(const QString &id);

    QHash<QString, md::MdWindow *> m_windows;
    QHash<QString, md::MdInputNumber *> m_inputNumbers;
    QHash<QString, md::MdFileDialog *> m_fileDialogs;
    QHash<QString, md::MdScaffold *> m_scaffolds;
    QVector<Slot> m_slots;
};

Stage2ShellPage::Stage2ShellPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2ShellPage::~Stage2ShellPage() = default;

QString Stage2ShellPage::title() const
{
    return L("Qt 扩展 · 壳体与输入", "Qt extensions · shell & input");
}

QString Stage2ShellPage::slug() const
{
    return QStringLiteral("qt-extensions-shell");
}

QString Stage2ShellPage::subtitle() const
{
    return L(
        "阶段二 Qt 扩展：无边框主窗口（Surface + Top app bar + Icon "
        "button 标题栏）、文件对话框（Dialog + Drawer + List + Text "
        "field）、数字输入（Text field + Icon button 步进）与页面骨架"
        "（App bar + Navigation + FAB + body）。",
        "Stage 2 Qt extensions: the frameless main window (Surface + Top "
        "app bar + Icon button title bar), the file dialog (Dialog + "
        "drawer + list + text field), the numeric input (Text field + "
        "icon-button steppers) and the page scaffold (App bar + "
        "Navigation + FAB + body).");
}

md::MdWindow *Stage2ShellPage::window(const QString &id)
{
    const auto found = m_windows.constFind(id);
    if (found != m_windows.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdWindow(this);
    m_windows.insert(id, created);
    return created;
}

md::MdInputNumber *Stage2ShellPage::inputNumber(const QString &id)
{
    const auto found = m_inputNumbers.constFind(id);
    if (found != m_inputNumbers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdInputNumber(this);
    m_inputNumbers.insert(id, created);
    return created;
}

md::MdFileDialog *Stage2ShellPage::fileDialog(const QString &id)
{
    const auto found = m_fileDialogs.constFind(id);
    if (found != m_fileDialogs.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdFileDialog(this);
    m_fileDialogs.insert(id, created);
    return created;
}

md::MdScaffold *Stage2ShellPage::scaffold(const QString &id)
{
    const auto found = m_scaffolds.constFind(id);
    if (found != m_scaffolds.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdScaffold(this);
    m_scaffolds.insert(id, created);
    return created;
}

void Stage2ShellPage::build(GalleryContext &context)
{
    m_slots.clear();

    // --- MdWindow ------------------------------------------------------------
    context.section(L("MdWindow — 无边框主窗口",
                      "MdWindow — frameless main window"));
    context.paragraph(L(
        "Surface 背景 + 64px 标题栏（title-large / on-surface）。标题栏按钮是"
        "真正的 MdIconButton（standard / xsmall）。浮动时 extra-large 圆角 + "
        "outline-variant 细线；最大化时直角填满。",
        "Surface background + a 64 px title bar (title-large / on-surface). "
        "The title-bar buttons are real MdIconButtons (standard / xsmall). "
        "Floating windows take the extra-large corner and an outline-variant "
        "hairline; maximised windows go square and fill."));
    {
        auto *frame = window(QStringLiteral("win-demo"));
        frame->setTitle(L("qt-md3 画廊", "qt-md3 gallery"));
        frame->setIconName(QStringLiteral("dashboard"));
        auto *body = new QLabel(
            L("中央区域：页面内容放在这里。", "Central widget: the page body lives here."),
            frame);
        body->setWordWrap(true);
        body->setAlignment(Qt::AlignCenter);
        frame->setCentralWidget(body);
        frame->setTitleBarHeight(56);
        const QRectF band = context.band(200.0);
        m_slots.append({frame, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 200.0)});
    }
    context.detail(L(
        "Surface + app-bar 标题栏 + icon-button chrome + extra-large 圆角",
        "Surface + app-bar title bar + icon-button chrome + extra-large corner"));

    context.space(16.0);

    // --- MdInputNumber -------------------------------------------------------
    context.section(L("MdInputNumber — Text field + Icon button 步进",
                      "MdInputNumber — Text field + Icon button steppers"));
    context.paragraph(L(
        "MdTextField 作为锚点（filled / outlined），后缀槽放一对 xsmall "
        "icon button（arrow_drop_up / arrow_drop_down）。value 钳制在 "
        "[min, max]；前缀/后缀/小数位只影响格式化。",
        "An MdTextField anchor (filled / outlined) with a pair of xsmall "
        "icon buttons (arrow_drop_up / arrow_drop_down) in the trailing "
        "slot. value clamps to [min, max]; prefix / suffix / decimals only "
        "affect formatting."));
    {
        auto *plain = inputNumber(QStringLiteral("in-plain"));
        plain->setLabelText(L("数量", "Quantity"));
        plain->setValue(3);
        plain->setRange(0, 99);
        const QRectF band = context.band(80.0);
        m_slots.append({plain, QRectF(band.left(), band.top(),
                                      qMin(240.0, band.width() - 16.0), 72.0)});

        auto *money = inputNumber(QStringLiteral("in-money"));
        money->setVariant(md::MdTextFieldVariant::Outlined);
        money->setLabelText(L("价格", "Price"));
        money->setDecimals(2);
        money->setPrefix(QStringLiteral("$"));
        money->setSuffix(QStringLiteral(" USD"));
        money->setValue(19.5);
        money->setSingleStep(0.5);
        money->setRange(0.0, 1000.0);
        m_slots.append({money, QRectF(band.left() + qMin(280.0, band.width() - 16.0), band.top(),
                                      qMin(280.0, band.width() - 16.0), 72.0)});
    }
    context.detail(L(
        "filled / outlined 路 prefix + decimals 路 step 路 clamp",
        "filled / outlined 路 prefix + decimals 路 step 路 clamp"));

    context.space(16.0);

    // --- MdScaffold ----------------------------------------------------------
    context.section(L("MdScaffold — 页面骨架",
                      "MdScaffold — page skeleton"));
    context.paragraph(L(
        "组合 App bar + Navigation（rail / bar）+ FAB + body。骨架只画 "
        "surface 背景并摆放槽位；像素全部由 Stage 1 组件自己画。",
        "Composes App bar + Navigation (rail / bar) + FAB + body. The "
        "scaffold only paints the surface background and places the slots; "
        "every pixel of the slots themselves is Stage 1's."));
    {
        auto *frame = scaffold(QStringLiteral("scaf-demo"));
        auto *appBar = new md::MdTopAppBar(frame);
        appBar->setTitle(L("收件箱", "Inbox"));
        auto *nav = new QLabel(L("导航栏", "Nav"), frame);
        nav->setAlignment(Qt::AlignCenter);
        nav->setMinimumWidth(80);
        auto *body = new QLabel(L("正文区域", "Body"), frame);
        body->setAlignment(Qt::AlignCenter);
        auto *fab = new md::MdFab(frame);
        fab->setIconName(QStringLiteral("add"));
        frame->setAppBar(appBar);
        frame->setNavigation(nav, md::MdNavPlacement::Start);
        frame->setBody(body);
        frame->setFab(fab);
        frame->setFabPlacement(md::MdFabPlacement::EndFloat);
        const QRectF band = context.band(240.0);
        m_slots.append({frame, QRectF(band.left(), band.top(),
                                      qMin(520.0, band.width() - 16.0), 240.0)});
    }
    context.detail(L(
        "app-bar 路 start rail 路 body 路 EndFloat FAB",
        "app-bar 路 start rail 路 body 路 EndFloat FAB"));

    context.space(16.0);

    // --- MdFileDialog --------------------------------------------------------
    context.section(L("MdFileDialog — Dialog + Drawer + List + Text field",
                      "MdFileDialog — Dialog + Drawer + List + Text field"));
    context.paragraph(L(
        "Dialog chrome（surface-container-high / level 3 / extra-large）+ 左侧"
        "位置栏（drawer 语义）+ 文件列表（List 行）+ 路径/文件名 Text field + "
        "Filled/Text 按钮。",
        "Dialog chrome (surface-container-high / level 3 / extra-large) + a "
        "left location rail (drawer semantics) + a file list (List rows) + "
        "path / name text fields + Filled / Text buttons."));
    {
        auto *picker = fileDialog(QStringLiteral("fd-demo"));
        picker->setTitle(L("打开文件", "Open file"));
        const QRectF band = context.band(320.0);
        m_slots.append({picker, QRectF(band.left(), band.top(),
                                      qMin(640.0, band.width() - 16.0), 320.0)});
    }
    context.detail(L(
        "dialog tokens 路 drawer 列 路 list 行 56 路 text field 路 actions",
        "dialog tokens 路 drawer column 路 56 px list rows 路 text fields "
        "路 actions"));
}

void Stage2ShellPage::placeChildren()
{
    for (const Slot &slot : m_slots) {
        if (slot.widget != nullptr) {
            slot.widget->setGeometry(slot.rect.toRect());
        }
    }
}

void Stage2ShellPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2ShellPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2ShellPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createStage2ShellPage()
{
    return new Stage2ShellPage;
}

} // namespace gallery

#include "GalleryPages49.moc"
