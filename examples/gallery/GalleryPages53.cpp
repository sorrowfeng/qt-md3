// Gallery page 53: Stage 2 Qt extensions — MdStatistic + MdPopover +
// MdPopconfirm + MdAutoComplete + MdColorPicker + MdDockWidget / MdDockManager.
//
// Same rules as the other component pages: widgets are banked, build() is
// content-local rects only, placeChildren() runs from resize/show/paint.

#include "GalleryPage.h"
#include "I18n.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdAutoComplete.h"
#include "widgets/MdColorPicker.h"
#include "widgets/MdDockManager.h"
#include "widgets/MdDockWidget.h"
#include "widgets/MdPopconfirm.h"
#include "widgets/MdPopover.h"
#include "widgets/MdStatistic.h"

#include <QtCore/QHash>
#include <QtGui/QPaintEvent>
#include <QtGui/QResizeEvent>
#include <QtGui/QShowEvent>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

namespace gallery {

namespace {

class Stage2EntryPage : public GalleryPage
{
public:
    explicit Stage2EntryPage(QWidget *parent = nullptr);
    ~Stage2EntryPage() override;

    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;

    void build(GalleryContext &context) override;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    md::MdStatistic *statistic(const QString &id);
    md::MdPopover *popover(const QString &id);
    md::MdPopconfirm *popconfirm(const QString &id);
    md::MdAutoComplete *autoComplete(const QString &id);
    md::MdColorPicker *colorPicker(const QString &id);
    md::MdDockManager *dockManager(const QString &id);
    md::MdDockWidget *dockWidget(const QString &id);

    void placeChildren();

    QHash<QString, md::MdStatistic *> m_statistics;
    QHash<QString, md::MdPopover *> m_popovers;
    QHash<QString, md::MdPopconfirm *> m_popconfirms;
    QHash<QString, md::MdAutoComplete *> m_autoCompletes;
    QHash<QString, md::MdColorPicker *> m_colorPickers;
    QHash<QString, md::MdDockManager *> m_dockManagers;
    QHash<QString, md::MdDockWidget *> m_dockWidgets;
    QHash<QString, QWidget *> m_slots;
};

Stage2EntryPage::Stage2EntryPage(QWidget *parent)
    : GalleryPage(parent)
{
}

Stage2EntryPage::~Stage2EntryPage() = default;

QString Stage2EntryPage::title() const
{
    return QStringLiteral("Stage 2 · Entry & Overlay");
}

QString Stage2EntryPage::slug() const
{
    return QStringLiteral("qt-extensions-entry");
}

QString Stage2EntryPage::subtitle() const
{
    return L("MdStatistic / MdPopover / MdPopconfirm / MdAutoComplete / "
             "MdColorPicker / MdDock",
             "MdStatistic / MdPopover / MdPopconfirm / MdAutoComplete / "
             "MdColorPicker / MdDock");
}

md::MdStatistic *Stage2EntryPage::statistic(const QString &id)
{
    const auto found = m_statistics.constFind(id);
    if (found != m_statistics.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdStatistic(this);
    created->setVisible(false);
    m_statistics.insert(id, created);
    return created;
}

md::MdPopover *Stage2EntryPage::popover(const QString &id)
{
    const auto found = m_popovers.constFind(id);
    if (found != m_popovers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdPopover(this);
    created->setVisible(false);
    m_popovers.insert(id, created);
    return created;
}

md::MdPopconfirm *Stage2EntryPage::popconfirm(const QString &id)
{
    const auto found = m_popconfirms.constFind(id);
    if (found != m_popconfirms.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdPopconfirm(this);
    created->setVisible(false);
    m_popconfirms.insert(id, created);
    return created;
}

md::MdAutoComplete *Stage2EntryPage::autoComplete(const QString &id)
{
    const auto found = m_autoCompletes.constFind(id);
    if (found != m_autoCompletes.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdAutoComplete(this);
    created->setVisible(false);
    m_autoCompletes.insert(id, created);
    return created;
}

md::MdColorPicker *Stage2EntryPage::colorPicker(const QString &id)
{
    const auto found = m_colorPickers.constFind(id);
    if (found != m_colorPickers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdColorPicker(this);
    created->setVisible(false);
    m_colorPickers.insert(id, created);
    return created;
}

md::MdDockManager *Stage2EntryPage::dockManager(const QString &id)
{
    const auto found = m_dockManagers.constFind(id);
    if (found != m_dockManagers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDockManager(this);
    created->setVisible(false);
    m_dockManagers.insert(id, created);
    return created;
}

md::MdDockWidget *Stage2EntryPage::dockWidget(const QString &id)
{
    const auto found = m_dockWidgets.constFind(id);
    if (found != m_dockWidgets.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDockWidget(QStringLiteral("Panel"), this);
    created->setVisible(false);
    m_dockWidgets.insert(id, created);
    return created;
}

void Stage2EntryPage::build(GalleryContext &context)
{
    context.section(L("MdStatistic — Display / Headline 数值展示",
                      "MdStatistic — Display / Headline value"));
    context.paragraph(L(
        "Display/Headline 字号承载数值，LabelLarge 标题，前缀/后缀/趋势用语义色："
        "上升 primary，下降 error。所有字号来自 MdTypeScale。",
        "Display/Headline carries the number, LabelLarge the title, and prefix/"
        "suffix/trend use semantic colour: primary up, error down. Every size "
        "comes from MdTypeScale."));

    {
        const QRectF band = context.band(88.0);
        auto *s = statistic(QStringLiteral("stat-demo"));
        s->setTitle(L("总收入", "Total revenue"));
        s->setPrefix(QStringLiteral("$"));
        s->setSuffix(QStringLiteral("M"));
        s->setValue(42.5);
        s->setPrecision(1);
        s->setTrend(md::MdStatisticTrend::Up);
        s->setTrendText(QStringLiteral("+8%"));
        s->setVisible(true);
        m_slots.insert(QStringLiteral("stat-demo"), s);
        Q_UNUSED(band);
    }
    context.detail(L("DisplaySmall + LabelLarge 标题 + primary 趋势",
                     "DisplaySmall + LabelLarge title + primary trend"));
    context.space(16.0);

    context.section(L("MdPopover / MdPopconfirm — Menu / Dialog 语义浮层",
                      "MdPopover / MdPopconfirm — Menu / Dialog overlay"));
    context.paragraph(L(
        "Popover 是 Menu 弹层的“内容版”：surface-container、corner-extra-small，"
        "宿主任意子控件。Popconfirm 是带确认/取消动作的 Popover。",
        "Popover is the content reading of the Menu popup: surface-container, "
        "corner-extra-small, arbitrary child. Popconfirm is a popover with "
        "confirm / cancel actions."));

    {
        const QRectF band = context.band(64.0);
        // MdPopover is a Qt::Popup top-level — it must never be setVisible(true)
        // in the page bank or it floats over every other gallery page. Demonstrate
        // it from a trigger button instead.
        auto *trigger = new QPushButton(L("打开 Popover", "Open popover"), this);
        trigger->setObjectName(QStringLiteral("pop-demo"));
        auto *p = popover(QStringLiteral("pop-demo"));
        p->setTitle(L("预览", "Preview"));
        auto *inner = new QWidget(p);
        inner->setMinimumSize(160, 40);
        p->setContentWidget(inner);
        connect(trigger, &QPushButton::clicked, p, [p, trigger] {
            p->popup(QRect(trigger->mapToGlobal(QPoint(0, 0)), trigger->size()));
        });
        trigger->setVisible(true);
        m_slots.insert(QStringLiteral("pop-demo"), trigger);
        Q_UNUSED(band);
    }
    {
        const QRectF band = context.band(64.0);
        auto *trigger = new QPushButton(L("打开 Popconfirm", "Open popconfirm"), this);
        trigger->setObjectName(QStringLiteral("confirm-demo"));
        auto *c = popconfirm(QStringLiteral("confirm-demo"));
        c->setTitle(L("删除", "Delete"));
        c->setText(L("确定要删除吗？", "Delete this?"));
        c->setConfirmText(L("删除", "Delete"));
        c->setCancelText(L("取消", "Cancel"));
        connect(trigger, &QPushButton::clicked, c, [c, trigger] {
            c->open(QRect(trigger->mapToGlobal(QPoint(0, 0)), trigger->size()));
        });
        trigger->setVisible(true);
        m_slots.insert(QStringLiteral("confirm-demo"), trigger);
        Q_UNUSED(band);
    }
    context.detail(L("surface-container + corner-extra-small + fast spatial",
                     "surface-container + corner-extra-small + fast spatial"));
    context.space(16.0);

    context.section(L("MdAutoComplete — Text field + exposed dropdown",
                      "MdAutoComplete — Text field + exposed dropdown"));
    context.paragraph(L(
        "MdTextField + MdMenu 弹层，Contains / StartsWith / Custom 三种过滤，"
        "Down/Up 走建议，Enter 提交，Esc 关闭。",
        "MdTextField + MdMenu popup, Contains / StartsWith / Custom filters, "
        "Down/Up walk suggestions, Enter commits, Esc closes."));

    {
        const QRectF band = context.band(56.0);
        auto *ac = autoComplete(QStringLiteral("ac-demo"));
        ac->setLabelText(L("城市", "City"));
        ac->setPlaceholderText(L("输入城市名", "Type a city"));
        ac->setItems(QStringList() << QStringLiteral("Tokyo") << QStringLiteral("Toronto")
                                   << QStringLiteral("Osaka") << QStringLiteral("Berlin"));
        ac->setVisible(true);
        m_slots.insert(QStringLiteral("ac-demo"), ac);
        Q_UNUSED(band);
    }
    context.detail(L("MdTextField Outlined + MdMenu popup", "MdTextField Outlined + MdMenu popup"));
    context.space(16.0);

    context.section(L("MdColorPicker — Dialog + Slider(HSV) + Text field(Hex)",
                      "MdColorPicker — Dialog + Slider(HSV) + Text field(Hex)"));
    context.paragraph(L(
        "MD3 无色板规范，由三个 MdSlider（H/S/V）+ Hex MdTextField + "
        "Full 形状 swatch 组成，轮廓 outline-variant。",
        "MD3 has no colour-swatch spec; three MdSliders (H/S/V) + a Hex "
        "MdTextField + a Full-shape swatch with an outline-variant ring."));

    {
        const QRectF band = context.band(180.0);
        auto *cp = colorPicker(QStringLiteral("cp-demo"));
        cp->setColor(QColor(30, 144, 255));
        cp->setVisible(true);
        m_slots.insert(QStringLiteral("cp-demo"), cp);
        Q_UNUSED(band);
    }
    context.detail(L("HSV sliders + hex field + Full swatch",
                     "HSV sliders + hex field + Full swatch"));
    context.space(16.0);

    context.section(L("MdDockWidget / MdDockManager — Side sheet + surface",
                      "MdDockWidget / MdDockManager — Side sheet + surface"));
    context.paragraph(L(
        "Side sheet 语义的可停靠面板 + 管理器。拖拽时目标区域用 "
        "secondary-container 形状高亮引导。",
        "A side-sheet-semantic dockable panel plus the manager. While dragging, "
        "the target area lights up with a secondary-container shape highlight."));

    {
        const QRectF band = context.band(200.0);
        auto *dm = dockManager(QStringLiteral("dm-demo"));
        auto *center = new QWidget(dm);
        center->setMinimumSize(120, 120);
        dm->setCenterWidget(center);
        auto *left = dockWidget(QStringLiteral("dw-left"));
        left->setTitle(L("资源管理器", "Explorer"));
        dm->addDockWidget(left, md::MdDockArea::Left);
        dm->showGuide(md::MdDockArea::Right);
        dm->setVisible(true);
        m_slots.insert(QStringLiteral("dm-demo"), dm);
        Q_UNUSED(band);
    }
    context.detail(L("Left dock + secondary-container guide",
                     "Left dock + secondary-container guide"));
}

void Stage2EntryPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    const qreal left = content.left();
    qreal y = buildOriginY();

    auto place = [&](const QString &id, qreal w, qreal h) {
        auto it = m_slots.find(id);
        if (it == m_slots.end() || !it.value()) {
            return;
        }
        it.value()->setGeometry(QRect(qRound(left), qRound(y), qRound(w), qRound(h)));
        y += h + 12.0;
    };

    place(QStringLiteral("stat-demo"), qMin(320.0, content.width()), 88.0);
    place(QStringLiteral("pop-demo"), qMin(280.0, content.width()), 64.0);
    place(QStringLiteral("confirm-demo"), qMin(280.0, content.width()), 64.0);
    place(QStringLiteral("ac-demo"), qMin(320.0, content.width()), 56.0);
    place(QStringLiteral("cp-demo"), qMin(360.0, content.width()), 180.0);
    place(QStringLiteral("dm-demo"), qMin(480.0, content.width()), 200.0);
}

void Stage2EntryPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    placeChildren();
}

void Stage2EntryPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    placeChildren();
}

void Stage2EntryPage::paintEvent(QPaintEvent *event)
{
    placeChildren();
    GalleryPage::paintEvent(event);
}

} // namespace

GalleryPage *createStage2EntryPage()
{
    return new Stage2EntryPage;
}

} // namespace gallery
