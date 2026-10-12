// Gallery page 44: showcase - Mail / Inbox.
//
// A product screen, not a component grid: a painted hero header (title +
// avatar), a search field, filter chips, a rich inbox list with unread
// markers, a compose FAB above the bottom navigation. Every control is a
// real Md* widget; the hero, the unread dots and the layered surfaces are
// QPainter from theme tokens.

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

constexpr qreal kScreenHeight = 820.0;
constexpr qreal kHeroHeight = 132.0;

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
     "后端接口的边界情况是……", "Edge cases on the API surface are…",
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

/// Colour rotation for the avatar discs, all tonal families.
const md::ColorRole kAvatarBg[] = {
    md::ColorRole::PrimaryContainer,
    md::ColorRole::SecondaryContainer,
    md::ColorRole::TertiaryContainer,
};
const md::ColorRole kAvatarFg[] = {
    md::ColorRole::OnPrimaryContainer,
    md::ColorRole::OnSecondaryContainer,
    md::ColorRole::OnTertiaryContainer,
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
    /// Unread markers painted beside list rows (content-local).
    QVector<QPointF> m_unreadDots;
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
    return L("一台收件箱：英雄标题、搜索、筛选芯片、带未读点的信件列表，"
             "撰写 FAB 浮在底部导航之上。",
             "An inbox screen: hero title, search, filter chips, a message list "
             "with unread markers, and a compose FAB above the bottom navigation.");
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
        m_fab->setVariant(md::FabVariant::Tertiary);
        m_fab->setFabSize(md::FabSize::Large);
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
    m_unreadDots.clear();

    QPainter *painter = context.painter();

    // One short product caption; the screen itself carries the story.
    context.paragraph(L(
        "点信件可选中；FAB 与底部导航是真按钮。颜色、圆角、字号全部来自主令牌。",
        "Click a message to select it; the FAB and the destinations are live "
        "buttons. Colours, radii and type all come from theme tokens."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();
    const qreal right = screen.right();

    // Screen surface: layered tonal containment.
    if (painter != nullptr) {
        paintTonalPanel(*painter, screen, md::ColorRole::SurfaceContainerLow,
                        md::ShapeCorner::ExtraLarge);
    }

    // --- hero header ------------------------------------------------------
    // Full-bleed tonal gradient with the inbox title and the account avatar.
    // This is the colour ceremony of an official showcase: one hero surface
    // in a hue family, then neutral panels below it.
    const QRectF hero(left, screen.top(), width, kHeroHeight);
    if (painter != nullptr) {
        paintGradientArt(*painter, hero, md::ColorRole::PrimaryContainer,
                         md::ColorRole::SecondaryContainer, md::ShapeCorner::ExtraLarge);

        painter->save();
        const QFont helloFont = fontFor(md::TypeStyle::LabelLarge);
        painter->setFont(helloFont);
        painter->setPen(role(md::ColorRole::OnPrimaryContainer));
        painter->drawText(QRectF(left + 24.0, screen.top() + 22.0, width - 160.0, 22.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("你好，林小满", "Good morning, Avery"));

        const QFont titleFont = fontFor(md::TypeStyle::HeadlineLarge);
        painter->setFont(titleFont);
        painter->setPen(role(md::ColorRole::OnPrimaryContainer));
        painter->drawText(QRectF(left + 24.0, screen.top() + 48.0, width - 160.0, 52.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("收件箱", "Inbox"));

        const QFont countFont = fontFor(md::TypeStyle::BodyMedium);
        painter->setFont(countFont);
        painter->setPen(role(md::ColorRole::OnPrimaryContainer));
        painter->drawText(QRectF(left + 24.0, screen.top() + 100.0, width - 160.0, 20.0),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          L("2 封未读 · 今天", "2 unread · today"));
        painter->restore();
    }

    // Account avatar (badged) sits on the hero, trailing side.
    {
        md::MdBadgedBox *box = avatarBox();
        const qreal boxSize = 48.0;
        m_slots.append(Slot{box, QRectF(right - 24.0 - boxSize, screen.top() + 42.0,
                                        boxSize, boxSize)});
    }

    // --- search ----------------------------------------------------------
    md::MdSearchBar *search = searchBar();
    search->setPlaceholderText(L("搜索邮件", "Search mail"));
    const qreal searchHeight = search->sizeHint().height();
    const qreal sidePad = 20.0;
    qreal y = screen.top() + kHeroHeight + 16.0;
    m_slots.append(Slot{search, QRectF(left + sidePad, y, width - 2.0 * sidePad, searchHeight)});
    y += searchHeight + 16.0;

    // --- filter chips ----------------------------------------------------
    const qreal chipHeight = 32.0;
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
        qreal x = left + sidePad;
        for (const ChipEntry &entry : entries) {
            md::MdChip *c = chip(QString::fromUtf8(entry.id), L(entry.zh, entry.en));
            c->setChecked(entry.selected);
            const qreal w = qMax<qreal>(c->sizeHint().width(), 64.0);
            m_slots.append(Slot{c, QRectF(x, y, w, chipHeight)});
            x += w + 8.0;
        }
    }
    y += chipHeight + 20.0;

    // --- inbox list on a nested tonal panel -------------------------------
    md::MdList *list = mailList();
    if (list->itemCount() == 0) {
        for (const MailEntry &entry : kMails) {
            list->addItem(mailItem(QString::fromUtf8(entry.id)));
        }
    }

    const qreal listHeight = 6.0 * 76.0 + 16.0;
    const QRectF listPanel(left + sidePad, y, width - 2.0 * sidePad, listHeight);
    if (painter != nullptr) {
        paintTonalPanel(*painter, listPanel, md::ColorRole::SurfaceContainerLowest,
                        md::ShapeCorner::Large);
    }

    const qreal rowHeight = 76.0;
    qreal rowY = y + 8.0;
    int colourIndex = 0;
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
        m_slots.append(Slot{item, QRectF(left + sidePad + 8.0, rowY, width - 2.0 * sidePad - 16.0,
                                         rowHeight)});
        if (entry.unread) {
            // Indicator column in the 8dp gap between the panel edge and the
            // row — never on the avatar itself.
            m_unreadDots.append(QPointF(left + sidePad + 4.5, rowY + rowHeight / 2.0));
        }
        rowY += rowHeight;
        ++colourIndex;
    }
    y = listPanel.bottom() + 8.0;

    // --- navigation bar + FAB --------------------------------------------
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
                               screen.bottom() - navHeight - qreal(fabSize.height()) - 12.0,
                               qreal(fabSize.width()), qreal(fabSize.height()))});

    Q_UNUSED(colourIndex);
    context.space(8.0);
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

    // Unread markers ride on top of the list rows.
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    beginOverlay(painter);
    for (const QPointF &dot : m_unreadDots) {
        paintDot(painter, dot, 4.5, role(md::ColorRole::Primary));
    }
    painter.restore();

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
