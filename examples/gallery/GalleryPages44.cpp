// Gallery page 44: showcase - Mail / Inbox.
//
// A product screen assembled from already-ported families, not a component
// grid: top app bar with search + badged avatar, a search bar, filter chips,
// an inbox list, a compose FAB and a bottom navigation bar. Same rules as
// every other page: widgets are banked, build() is re-entrant, and the page
// never styles anything - every colour, radius and type size comes from the
// theme at paint time.

#include "GalleryPages.h"

#include "widgets/MdBadgedBox.h"
#include "widgets/MdBadge.h"
#include "widgets/MdChip.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdTopAppBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

/// One banked widget plus the rect it occupies inside the screen band.
struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

/// The screen band's height: app bar + search + chips + six mail rows +
/// the navigation bar, with the FAB overlapping the lower-right corner.
constexpr qreal kScreenHeight = 780.0;
constexpr qreal kMailRowHeight = 76.0;

struct MailEntry
{
    const char *id;
    const char *senderZh;
    const char *senderEn;
    const char *subjectZh;
    const char *subjectEn;
    const char *previewZh;
    const char *previewEn;
    const char *timeZh;
    const char *timeEn;
    const char *avatar;
    bool unread;
};

const MailEntry kMails[] = {
    {"m0", "林小满", "Avery Chen", "周会纪要", "Standup notes",
     "后端接口的边界情况是…", "Edge cases on the API surface are…",
     "10:24", "10:24", "林", true},
    {"m1", "设计组", "Design Guild", "新色板已发布", "New palette shipped",
     "动态提取的次级色已合入主干。", "The derived secondary is merged to main.",
     "09:12", "09:12", "设", true},
    {"m2", "Jordan Lee", "Jordan Lee", "Friday offsite", "Friday offsite",
     "Meeting point is the lobby at 09:30.", "Meeting point is the lobby at 09:30.",
     "昨天", "Yesterday", "J", false},
    {"m3", "成员通知", "Membership", "年费已扣费", "Annual fee charged",
     "发票号码已附在本邮件。", "The invoice number is attached to this mail.",
     "周三", "Wed", "会", false},
    {"m4", "Build Bot", "Build Bot", "Pipeline green", "Pipeline green",
     "qt-md3-example 通过全部 54 项测试。", "qt-md3-example passed all 54 tests.",
     "周二", "Tue", "B", false},
    {"m5", "M3 Spec", "M3 Spec", "Token export 34.0.21", "Token export 34.0.21",
     "flexible navigation bar 替代基线家族。", "The flexible navigation bar replaces the baseline family.",
     "周一", "Mon", "M", false},
};

} // namespace

class ShowcaseMailPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ShowcaseMailPage(QWidget *parent = nullptr);
    ~ShowcaseMailPage() override;

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

    md::MdTopAppBar *appBar();
    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdBadgedBox *avatarBox();
    md::MdSearchBar *searchBar();
    md::MdChip *chip(const QString &id, const QString &label);
    md::MdList *mailList();
    md::MdListItem *mailItem(const QString &id);
    md::MdFab *composeFab();
    md::MdNavigationBar *navBar();
    md::MdNavigationBarItem *navItem(const QString &id, const QString &label,
                                     const QString &icon);

    md::MdTopAppBar *m_appBar = nullptr;
    QHash<QString, md::MdIconButton *> m_iconButtons;
    md::MdBadgedBox *m_avatarBox = nullptr;
    md::MdSearchBar *m_search = nullptr;
    QHash<QString, md::MdChip *> m_chips;
    md::MdList *m_list = nullptr;
    QHash<QString, md::MdListItem *> m_items;
    md::MdFab *m_fab = nullptr;
    md::MdNavigationBar *m_navBar = nullptr;
    QHash<QString, md::MdNavigationBarItem *> m_navItems;
    QVector<Slot> m_slots;
};

ShowcaseMailPage::ShowcaseMailPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ShowcaseMailPage::~ShowcaseMailPage() = default;

QString ShowcaseMailPage::title() const
{
    return L("示例·邮件", "Showcase·Mail");
}

QString ShowcaseMailPage::slug() const
{
    return QStringLiteral("showcase-mail");
}

QString ShowcaseMailPage::subtitle() const
{
    return L("一个完整的邮件收件箱屏幕：顶部应用栏（搜索 + 头像角标）、搜索栏、"
             "筛选芯片、列表条目、撰写 FAB 与底部导航栏。全部由已移植的组件拼装，"
             "颜色、字号、圆角均来自主題令牌。",
             "A complete inbox screen: a top app bar (search + avatar), a search bar, "
             "filter chips, a message list, a compose FAB and a bottom navigation bar. "
             "Assembled entirely from already-ported components; every colour, type "
             "size and corner comes from theme tokens.");
}

md::MdTopAppBar *ShowcaseMailPage::appBar()
{
    if (m_appBar == nullptr) {
        m_appBar = new md::MdTopAppBar(this);
        m_appBar->hide();
    }
    return m_appBar;
}

md::MdIconButton *ShowcaseMailPage::iconButton(const QString &id, const QString &icon)
{
    const auto found = m_iconButtons.constFind(id);
    if (found != m_iconButtons.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdIconButton(icon, this);
    created->hide();
    m_iconButtons.insert(id, created);
    return created;
}

md::MdBadgedBox *ShowcaseMailPage::avatarBox()
{
    if (m_avatarBox == nullptr) {
        m_avatarBox = new md::MdBadgedBox(this);
        auto *avatarButton = iconButton(QStringLiteral("avatar"), QStringLiteral("person"));
        // Tonal avatar: secondary-container family, so the app bar is not a
        // single primary wash. The badge itself is the error role (MD3 default).
        avatarButton->setVariant(md::IconButtonVariant::Tonal);
        avatarButton->setParent(m_avatarBox);
        m_avatarBox->setContentWidget(avatarButton);
        m_avatarBox->badge()->setText(QStringLiteral("3"));
        m_avatarBox->hide();
    }
    return m_avatarBox;
}

md::MdSearchBar *ShowcaseMailPage::searchBar()
{
    if (m_search == nullptr) {
        m_search = new md::MdSearchBar(this);
        m_search->hide();
    }
    return m_search;
}

md::MdChip *ShowcaseMailPage::chip(const QString &id, const QString &label)
{
    const auto found = m_chips.constFind(id);
    if (found != m_chips.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdChip(label, this);
    created->setVariant(md::MdChipVariant::Filter);
    created->hide();
    m_chips.insert(id, created);
    return created;
}

md::MdList *ShowcaseMailPage::mailList()
{
    if (m_list == nullptr) {
        m_list = new md::MdList(this);
        m_list->setSelectionMode(md::MdList::SelectionMode::Single);
    }
    return m_list;
}

md::MdListItem *ShowcaseMailPage::mailItem(const QString &id)
{
    const auto found = m_items.constFind(id);
    if (found != m_items.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdListItem(this);
    created->setInteractive(true);
    created->setLeadingKind(md::MdListItem::LeadingKind::Avatar);
    m_items.insert(id, created);
    return created;
}

md::MdFab *ShowcaseMailPage::composeFab()
{
    if (m_fab == nullptr) {
        m_fab = new md::MdFab(QStringLiteral("edit"), this);
        m_fab->hide();
    }
    return m_fab;
}

md::MdNavigationBar *ShowcaseMailPage::navBar()
{
    if (m_navBar == nullptr) {
        m_navBar = new md::MdNavigationBar(this);
        m_navBar->hide();
    }
    return m_navBar;
}

md::MdNavigationBarItem *ShowcaseMailPage::navItem(const QString &id, const QString &label,
                                                   const QString &icon)
{
    const auto found = m_navItems.constFind(id);
    if (found != m_navItems.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationBarItem(label, icon, this);
    created->hide();
    m_navItems.insert(id, created);
    return created;
}

void ShowcaseMailPage::build(GalleryContext &context)
{
    m_slots.clear();

    QPainter *painter = context.painter();

    context.section(L("收件箱屏幕", "The inbox screen"));
    context.paragraph(L("下面是一台在页面里整幅呈现的应用：该有的框架元素都在，"
                        "但每个元素都是真的 Md* 组件，不是描绘出来的画。"
                        "点击条目会选中，点击 FAB 与导航栏项是真按钮。",
                        "The application below fills the page frame: every chrome "
                        "element is present, and every one of them is a real Md* "
                        "component rather than a painted picture. Clicking a row "
                        "selects it; the FAB and the navigation destinations are "
                        "live buttons."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();
    qreal y = screen.top();

    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        // Screen surface: tonal containment, not shadow.
        fillRounded(*painter, screen, role(md::ColorRole::SurfaceContainerLow),
                    md::ShapeCorner::ExtraLarge);
        painter->restore();
    }

    // --- top app bar ------------------------------------------------------
    md::MdTopAppBar *bar = appBar();
    bar->setTitle(L("收件箱", "Inbox"));
    bar->setVariant(md::MdAppBarVariant::Small);
    bar->setNavigationWidget(iconButton(QStringLiteral("menu"), QStringLiteral("menu")));
    if (bar->actionWidgets().isEmpty()) {
        bar->addActionWidget(iconButton(QStringLiteral("search"), QStringLiteral("search")));
        bar->addActionWidget(avatarBox());
    }
    const qreal barHeight = bar->sizeHint().height();
    m_slots.append(Slot{bar, QRectF(left, y, width, barHeight)});
    y += barHeight;

    // --- search bar -------------------------------------------------------
    md::MdSearchBar *search = searchBar();
    search->setPlaceholderText(L("搜索邮件", "Search mail"));
    const qreal searchHeight = search->sizeHint().height();
    const qreal sidePad = 16.0;
    m_slots.append(Slot{search, QRectF(left + sidePad, y + 8.0, width - 2.0 * sidePad,
                                       searchHeight)});
    y += searchHeight + 24.0;

    // --- filter chips -----------------------------------------------------
    const qreal chipHeight = 32.0;
    const QRectF chipBand(left + sidePad, y, width - 2.0 * sidePad, chipHeight);
    {
        struct ChipEntry
        {
            const char *id;
            const char *zh;
            const char *en;
            bool selected;
        };
        const ChipEntry entries[] = {
            {"all", "全部", "All", true},
            {"unread", "未读", "Unread", false},
            {"starred", "已加星", "Starred", false},
            {"work", "工作", "Work", false},
        };
        qreal x = chipBand.left();
        for (const ChipEntry &entry : entries) {
            md::MdChip *c = chip(QString::fromUtf8(entry.id), L(entry.zh, entry.en));
            c->setChecked(entry.selected);
            const qreal w = qMax<qreal>(c->sizeHint().width(), 64.0);
            m_slots.append(Slot{c, QRectF(x, chipBand.top(), w, chipHeight)});
            x += w + 8.0;
        }
    }
    y += chipHeight + 16.0;

    // --- inbox list -------------------------------------------------------
    md::MdList *list = mailList();
    // Populate once: build() runs from paintEvent, and re-adding mid-click
    // drops the press.
    if (list->itemCount() == 0) {
        for (const MailEntry &entry : kMails) {
            list->addItem(mailItem(QString::fromUtf8(entry.id)));
        }
    }
    for (const MailEntry &entry : kMails) {
        md::MdListItem *item = m_items.value(QString::fromUtf8(entry.id));
        if (item == nullptr) {
            continue;
        }
        item->setAvatarLabel(QString::fromUtf8(entry.avatar));
        item->setHeadline(L(entry.senderZh, entry.senderEn));
        item->setSupporting(L(entry.subjectZh, entry.subjectEn) + QStringLiteral(" · ")
                            + L(entry.previewZh, entry.previewEn));
        item->setTrailingSupporting(L(entry.timeZh, entry.timeEn));
        item->setSelected(entry.unread && QString::fromUtf8(entry.id) == QStringLiteral("m0"));
        item->setVariant(md::MdListVariant::Expressive);
    }
    const qreal listHeight = qMax<qreal>(list->sizeHint().height(), 6.0 * kMailRowHeight);
    m_slots.append(Slot{list, QRectF(left + sidePad, y, width - 2.0 * sidePad, listHeight)});
    y += listHeight;

    // --- navigation bar + FAB ---------------------------------------------
    md::MdNavigationBar *nav = navBar();
    if (nav->count() == 0) {
        struct Destination
        {
            const char *id;
            const char *zh;
            const char *en;
            const char *icon;
        };
        const Destination destinations[] = {
            {"mail", "邮件", "Mail", "mail"},
            {"chat", "聊天", "Chat", "chat"},
            {"spaces", "空间", "Spaces", "folder"},
            {"meet", "会议", "Meet", "videocam"},
        };
        for (const Destination &d : destinations) {
            nav->addItem(navItem(QStringLiteral("mail-nav-") + QString::fromUtf8(d.id),
                                 L(d.zh, d.en), d.icon));
        }
        nav->setCurrentIndex(0);
    }
    const qreal navHeight = nav->sizeHint().height();
    m_slots.append(Slot{nav, QRectF(left, screen.bottom() - navHeight, width, navHeight)});

    md::MdFab *fab = composeFab();
    const QSize fabSize = fab->sizeHint();
    m_slots.append(Slot{fab,
                        QRectF(screen.right() - qreal(fabSize.width()) - 20.0,
                               screen.bottom() - navHeight - qreal(fabSize.height()) - 16.0,
                               qreal(fabSize.width()), qreal(fabSize.height()))});

    Q_UNUSED(y);
    context.space(12.0);
    context.detail(L("提示：点击邮件条目会切换选中态，底部导航栏与撰写 FAB 均可点击。",
                     "Tip: click a message row to toggle selection; the bottom "
                     "navigation and the compose FAB are clickable too."));
}

void ShowcaseMailPage::placeChildren()
{
    const QRectF content = contentRectForCurrentSize();
    for (const Slot &slot : m_slots) {
        if (slot.widget == nullptr) {
            continue;
        }
        const QRect target(content.topLeft().toPoint() + slot.rect.topLeft().toPoint(),
                           slot.rect.size().toSize());
        if (slot.widget->geometry() != target) {
            slot.widget->setGeometry(target);
        }
        if (!slot.widget->isVisible()) {
            slot.widget->show();
        }
        slot.widget->raise();
    }
}

void ShowcaseMailPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ShowcaseMailPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ShowcaseMailPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ShowcaseMailPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createShowcaseMailPage()
{
    return new ShowcaseMailPage;
}

} // namespace gallery

#include "GalleryPages44.moc"
