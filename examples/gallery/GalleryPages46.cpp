// Gallery page 46: showcase - Settings.
//
// A preference screen: navigation rail on the left, preference rows on the
// right (switch, radio group, slider, menu button), dividers between groups.
// Same rules as the other pages: widgets are banked, build() is re-entrant,
// and the page never styles anything.

#include "GalleryPages.h"

#include "widgets/MdButton.h"
#include "widgets/MdDivider.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdNavigationBarItem.h"
#include "widgets/MdNavigationRail.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTopAppBar.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

constexpr qreal kScreenHeight = 720.0;
constexpr qreal kRailWidth = 88.0;

} // namespace

class ShowcaseSettingsPage : public GalleryPage
{
    Q_OBJECT

public:
    explicit ShowcaseSettingsPage(QWidget *parent = nullptr);
    ~ShowcaseSettingsPage() override;

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
    md::MdNavigationRail *rail();
    md::MdNavigationBarItem *railItem(const QString &id, const QString &label,
                                      const QString &icon);
    md::MdSwitch *toggle(const QString &id);
    md::MdRadioButton *radio(const QString &id, const QString &label);
    md::MdSlider *slider();
    md::MdButton *menuButton();
    md::MdMenu *menu();
    md::MdDivider *divider(const QString &id);

    md::MdTopAppBar *m_appBar = nullptr;
    QHash<QString, md::MdIconButton *> m_iconButtons;
    md::MdNavigationRail *m_rail = nullptr;
    QHash<QString, md::MdNavigationBarItem *> m_railItems;
    QHash<QString, md::MdSwitch *> m_switches;
    QHash<QString, md::MdRadioButton *> m_radios;
    md::MdSlider *m_slider = nullptr;
    md::MdButton *m_menuButton = nullptr;
    md::MdMenu *m_menu = nullptr;
    QHash<QString, md::MdDivider *> m_dividers;
    QVector<Slot> m_slots;
};

ShowcaseSettingsPage::ShowcaseSettingsPage(QWidget *parent)
    : GalleryPage(parent)
{
}

ShowcaseSettingsPage::~ShowcaseSettingsPage() = default;

QString ShowcaseSettingsPage::title() const
{
    return L("示例·设置", "Showcase·Settings");
}

QString ShowcaseSettingsPage::slug() const
{
    return QStringLiteral("showcase-settings");
}

QString ShowcaseSettingsPage::subtitle() const
{
    return L("偏好设置屏幕：左侧导航栏，右侧偏好行——开关、单选组、滑块与"
             "菜单按钮，组间用分割线隔开。全部由已移植的组件拼装。",
             "A preference screen: a navigation rail on the left, preference rows "
             "on the right (switch, radio group, slider, menu button), groups "
             "separated by dividers. Assembled from already-ported components.");
}

md::MdTopAppBar *ShowcaseSettingsPage::appBar()
{
    if (m_appBar == nullptr) {
        m_appBar = new md::MdTopAppBar(this);
        m_appBar->hide();
    }
    return m_appBar;
}

md::MdIconButton *ShowcaseSettingsPage::iconButton(const QString &id, const QString &icon)
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

md::MdNavigationRail *ShowcaseSettingsPage::rail()
{
    if (m_rail == nullptr) {
        m_rail = new md::MdNavigationRail(this);
        m_rail->setVariant(md::MdNavigationRailVariant::Baseline);
        m_rail->hide();
    }
    return m_rail;
}

md::MdNavigationBarItem *ShowcaseSettingsPage::railItem(const QString &id, const QString &label,
                                                        const QString &icon)
{
    const auto found = m_railItems.constFind(id);
    if (found != m_railItems.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdNavigationBarItem(label, icon, this);
    created->hide();
    m_railItems.insert(id, created);
    return created;
}

md::MdSwitch *ShowcaseSettingsPage::toggle(const QString &id)
{
    const auto found = m_switches.constFind(id);
    if (found != m_switches.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSwitch(this);
    created->hide();
    m_switches.insert(id, created);
    return created;
}

md::MdRadioButton *ShowcaseSettingsPage::radio(const QString &id, const QString &label)
{
    const auto found = m_radios.constFind(id);
    if (found != m_radios.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdRadioButton(label, this);
    created->hide();
    m_radios.insert(id, created);
    return created;
}

md::MdSlider *ShowcaseSettingsPage::slider()
{
    if (m_slider == nullptr) {
        m_slider = new md::MdSlider(this);
        m_slider->setMin(0.0);
        m_slider->setMax(100.0);
        m_slider->setValue(60.0);
        m_slider->hide();
    }
    return m_slider;
}

md::MdButton *ShowcaseSettingsPage::menuButton()
{
    if (m_menuButton == nullptr) {
        m_menuButton = new md::MdButton(this);
        m_menuButton->setVariant(md::ButtonVariant::Outlined);
        m_menuButton->setTrailingIcon(QStringLiteral("expand_more"));
        m_menuButton->hide();
    }
    return m_menuButton;
}

md::MdMenu *ShowcaseSettingsPage::menu()
{
    if (m_menu == nullptr) {
        m_menu = new md::MdMenu(this);
        m_menu->addItem(L("简体中文", "Simplified Chinese"));
        m_menu->addItem(L("英语", "English"));
        m_menu->addDivider();
        m_menu->addItem(L("跟随系统", "Follow system"));
    }
    return m_menu;
}

md::MdDivider *ShowcaseSettingsPage::divider(const QString &id)
{
    const auto found = m_dividers.constFind(id);
    if (found != m_dividers.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdDivider(this);
    created->setInsetMode(md::MdDivider::InsetMode::Both);
    created->hide();
    m_dividers.insert(id, created);
    return created;
}

void ShowcaseSettingsPage::build(GalleryContext &context)
{
    m_slots.clear();
    QPainter *painter = context.painter();

    context.section(L("偏好设置", "Preferences"));
    context.paragraph(L("一个典型的桌面设置任务：左边是主要目的地的地图，"
                        "右边是可调的参数。每一行都是真的组件——"
                        "开关、单选组、滑块和菜单按钮都能点。",
                        "A typical desktop settings task: a map of the primary "
                        "destinations on the left, adjustable parameters on the "
                        "right. Every row is a real component - the switches, "
                        "radio group, slider and menu button all respond."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();
    qreal y = screen.top();

    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        fillRounded(*painter, screen, role(md::ColorRole::SurfaceContainerLow),
                    md::ShapeCorner::ExtraLarge);
        painter->restore();
    }

    // --- top app bar ------------------------------------------------------
    md::MdTopAppBar *bar = appBar();
    bar->setTitle(L("设置", "Settings"));
    bar->setVariant(md::MdAppBarVariant::Small);
    bar->setNavigationWidget(iconButton(QStringLiteral("menu"), QStringLiteral("menu")));
    if (bar->actionWidgets().isEmpty()) {
        bar->addActionWidget(iconButton(QStringLiteral("search"), QStringLiteral("search")));
    }
    const qreal barHeight = bar->sizeHint().height();
    m_slots.append(Slot{bar, QRectF(left, y, width, barHeight)});
    y += barHeight;

    // --- navigation rail --------------------------------------------------
    md::MdNavigationRail *navRail = rail();
    if (navRail->count() == 0) {
        struct Destination
        {
            const char *id;
            const char *zh;
            const char *en;
            const char *icon;
        };
        const Destination destinations[] = {
            {"account", "账户", "Account", "person"},
            {"appearance", "外观", "Appearance", "palette"},
            {"privacy", "隐私", "Privacy", "lock"},
            {"about", "关于", "About", "info"},
        };
        for (const Destination &d : destinations) {
            navRail->addItem(railItem(QStringLiteral("set-rail-") + QString::fromUtf8(d.id),
                                      L(d.zh, d.en), d.icon));
        }
        navRail->setCurrentIndex(1);
    }
    const qreal railHeight = screen.bottom() - y;
    m_slots.append(Slot{navRail, QRectF(left, y, kRailWidth, railHeight)});

    // --- preference column ------------------------------------------------
    const qreal contentLeft = left + kRailWidth + 16.0;
    const qreal contentWidth = width - kRailWidth - 32.0;
    qreal rowY = y + 8.0;

    auto placeRow = [&](QWidget *widget, qreal rowHeight) {
        const qreal hintW = qMax<qreal>(widget->sizeHint().width(), 48.0);
        const qreal hintH = qMax<qreal>(widget->sizeHint().height(), rowHeight);
        m_slots.append(Slot{
            widget, QRectF(contentLeft + contentWidth - hintW, rowY, hintW, hintH)});
        rowY += rowHeight;
    };

    auto paintLabel = [&](const QString &text, const QString &supporting) {
        if (painter == nullptr) {
            return;
        }
        painter->save();
        const QFont labelFont = fontFor(md::TypeStyle::BodyLarge);
        painter->setFont(labelFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(contentLeft, rowY, contentWidth * 0.6, 24.0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        if (!supporting.isEmpty()) {
            const QFont subFont = fontFor(md::TypeStyle::BodySmall);
            painter->setFont(subFont);
            painter->setPen(role(md::ColorRole::OnSurfaceVariant));
            painter->drawText(QRectF(contentLeft, rowY + 24.0, contentWidth * 0.7, 20.0),
                              Qt::AlignLeft | Qt::AlignVCenter, supporting);
        }
        painter->restore();
    };

    auto sectionHeading = [&](const QString &text) {
        if (painter != nullptr) {
            painter->save();
            const QFont font = fontFor(md::TypeStyle::TitleSmall);
            painter->setFont(font);
            painter->setPen(role(md::ColorRole::Primary));
            painter->drawText(QRectF(contentLeft, rowY, contentWidth, 28.0),
                              Qt::AlignLeft | Qt::AlignVCenter, text);
            painter->restore();
        }
        rowY += 32.0;
    };

    // Appearance
    sectionHeading(L("外观", "Appearance"));
    paintLabel(L("深色模式", "Dark theme"),
               L("切换浅色与深色表面", "Switch between light and dark surfaces"));
    placeRow(toggle(QStringLiteral("dark")), 56.0);

    paintLabel(L("主题色", "Theme colour"),
               L("从种子色生成整套调色板", "Generate the whole palette from a seed"));
    {
        const qreal radioRow = 40.0;
        md::MdRadioButton *light = radio(QStringLiteral("theme-light"), L("浅色", "Light"));
        md::MdRadioButton *dark = radio(QStringLiteral("theme-dark"), L("深色", "Dark"));
        md::MdRadioButton *system = radio(QStringLiteral("theme-system"), L("跟随系统", "System"));
        system->setChecked(true);
        const qreal radioW = 120.0;
        m_slots.append(Slot{light, QRectF(contentLeft, rowY, radioW, radioRow)});
        m_slots.append(Slot{dark, QRectF(contentLeft + radioW, rowY, radioW, radioRow)});
        m_slots.append(Slot{system, QRectF(contentLeft + 2.0 * radioW, rowY, radioW, radioRow)});
        rowY += radioRow + 4.0;
    }

    paintLabel(L("文字大小", "Text size"), QString());
    {
        md::MdSlider *sizeSlider = slider();
        const qreal sliderH = sizeSlider->sizeHint().height();
        m_slots.append(Slot{sizeSlider,
                            QRectF(contentLeft + contentWidth * 0.35, rowY,
                                   contentWidth * 0.6, sliderH)});
        rowY += sliderH + 8.0;
    }

    md::MdDivider *first = divider(QStringLiteral("appearance-notifications"));
    m_slots.append(Slot{first, QRectF(contentLeft, rowY, contentWidth, 1.0)});
    rowY += 17.0;

    // Notifications
    sectionHeading(L("通知", "Notifications"));
    paintLabel(L("推送通知", "Push notifications"), L("新邮件与提醒", "New mail and reminders"));
    placeRow(toggle(QStringLiteral("push")), 56.0);

    paintLabel(L("邮件摘要", "Email digest"), L("每日汇总", "A daily summary"));
    placeRow(toggle(QStringLiteral("digest")), 56.0);

    md::MdDivider *second = divider(QStringLiteral("notifications-language"));
    m_slots.append(Slot{second, QRectF(contentLeft, rowY, contentWidth, 1.0)});
    rowY += 17.0;

    // Language
    sectionHeading(L("语言", "Language"));
    paintLabel(L("界面语言", "Interface language"),
               L("重新生效后打开导航重新进入生效", "Re-enter the app after switching"));
    {
        md::MdButton *button = menuButton();
        button->setText(L("简体中文", "Simplified Chinese"));
        const QSize hint = button->sizeHint();
        m_slots.append(Slot{button,
                            QRectF(contentLeft + contentWidth - qreal(hint.width()) - 8.0, rowY,
                                   qreal(hint.width()), qreal(hint.height()))});
        rowY += qreal(hint.height()) + 8.0;
    }

    // Seed the menu items once; opening the popup is the interaction.
    md::MdMenu *popup = menu();
    Q_UNUSED(popup);

    Q_UNUSED(y);
    context.space(12.0);
    context.detail(L("提示：开关、单选组、滑块与语言按钮均可交互；单击语言按钮会弹出菜单。",
                     "Tip: the switches, radio group, slider and language button "
                     "are all interactive; clicking the language button pops a menu."));
}

void ShowcaseSettingsPage::placeChildren()
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

void ShowcaseSettingsPage::layoutChildren()
{
    remeasure();
    placeChildren();
}

void ShowcaseSettingsPage::resizeEvent(QResizeEvent *event)
{
    GalleryPage::resizeEvent(event);
    layoutChildren();
}

void ShowcaseSettingsPage::showEvent(QShowEvent *event)
{
    GalleryPage::showEvent(event);
    layoutChildren();
}

void ShowcaseSettingsPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
    placeChildren();
}

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

GalleryPage *createShowcaseSettingsPage()
{
    return new ShowcaseSettingsPage;
}

} // namespace gallery

#include "GalleryPages46.moc"
