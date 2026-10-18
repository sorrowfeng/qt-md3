// Gallery page 46: showcase - Settings.
//
// A preference screen composed as product UI: a profile hero at the top,
// then grouped tonal cards (not bare rows) with Title-scale section headers,
// preference rows carrying a leading icon, a description and a trailing
// control. Every control is a real Md* widget; the panels and the profile
// art are QPainter from theme tokens.

#include "GalleryPages.h"

#include "core/MdTheme.h"

#include "widgets/MdButton.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"

#include "I18n.h"

#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <cmath>

namespace gallery {

namespace {

struct Slot
{
    QWidget *widget = nullptr;
    QRectF rect;
};

constexpr qreal kScreenHeight = 860.0;
constexpr qreal kHeroHeight = 120.0;

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

    md::MdIconButton *iconButton(const QString &id, const QString &icon);
    md::MdSwitch *toggle(const QString &id);
    md::MdRadioButton *radio(const QString &id, const QString &label);
    md::MdSlider *slider();
    md::MdButton *menuButton();
    md::MdMenu *menu();

    QHash<QString, md::MdIconButton *> m_iconButtons;
    QHash<QString, md::MdSwitch *> m_switches;
    QHash<QString, md::MdRadioButton *> m_radios;
    md::MdSlider *m_slider = nullptr;
    md::MdButton *m_menuButton = nullptr;
    md::MdMenu *m_menu = nullptr;
    QVector<Slot> m_slots;
    /// Ids whose interaction state has already been seeded from the scene.
    /// build() re-runs on every measure/paint and must not clobber a click.
    QSet<QString> m_stateSeeded;
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
    return L("偏好设置：资料头图 + 分组卡片，每行有图标、说明与尾随控件。",
             "Preferences: a profile hero plus grouped cards; every row has an "
             "icon, a description and a trailing control.");
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

md::MdSwitch *ShowcaseSettingsPage::toggle(const QString &id)
{
    const auto found = m_switches.constFind(id);
    if (found != m_switches.constEnd()) {
        return found.value();
    }
    auto *created = new md::MdSwitch(this);
    created->setObjectName(id);
    created->hide();
    if (id == QLatin1String("dark")) {
        // The row claims "switch between light and dark surfaces", so the
        // control has to drive MdTheme rather than only flip its own thumb.
        connect(created, &QAbstractButton::clicked, this, [] {
            md::MdTheme::instance().toggleThemeMode();
        });
    }
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
    created->setObjectName(id);
    created->hide();
    if (id == QLatin1String("theme-light")) {
        connect(created, &QAbstractButton::clicked, this, [] {
            md::MdTheme::instance().setThemeMode(md::ThemeMode::Light);
        });
    } else if (id == QLatin1String("theme-dark")) {
        connect(created, &QAbstractButton::clicked, this, [] {
            md::MdTheme::instance().setThemeMode(md::ThemeMode::Dark);
        });
    }
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
        m_menuButton->setObjectName(QStringLiteral("language-button"));
        m_menuButton->setVariant(md::ButtonVariant::Outlined);
        m_menuButton->setTrailingIcon(QStringLiteral("expand_more"));
        m_menuButton->hide();
        connect(m_menuButton, &QAbstractButton::clicked, this, [this] {
            md::MdMenu *popup = menu();
            const QPoint topLeft = m_menuButton->mapToGlobal(QPoint(0, 0));
            popup->popup(QRect(topLeft, m_menuButton->size()));
        });
    }
    return m_menuButton;
}

md::MdMenu *ShowcaseSettingsPage::menu()
{
    if (m_menu == nullptr) {
        m_menu = new md::MdMenu(this);
        m_menu->setObjectName(QStringLiteral("language-menu"));
        const QStringList labels = {
            L("简体中文", "Simplified Chinese"),
            L("英语", "English"),
            QString(), // divider
            L("跟随系统", "Follow system"),
        };
        for (const QString &label : labels) {
            if (label.isEmpty()) {
                m_menu->addDivider();
                continue;
            }
            md::MdMenuItem *item = m_menu->addItem(label);
            connect(item, &QAbstractButton::clicked, this, [this, item] {
                if (m_menuButton != nullptr) {
                    m_menuButton->setText(item->text());
                }
            });
        }
    }
    return m_menu;
}

void ShowcaseSettingsPage::build(GalleryContext &context)
{
    m_slots.clear();
    QPainter *painter = context.painter();

    context.paragraph(L(
        "开关、单选组、滑块与语言按钮均可交互；单击语言按钮弹出菜单。",
        "The switches, radio group, slider and language button all respond; "
        "clicking the language button pops a menu."));

    const QRectF screen = context.band(kScreenHeight);
    const qreal left = screen.left();
    const qreal width = screen.width();

    if (painter != nullptr) {
        paintTonalPanel(*painter, screen, md::ColorRole::SurfaceContainerLow,
                        md::ShapeCorner::ExtraLarge);
    }

    const qreal sidePad = 20.0;
    const qreal contentLeft = left + sidePad;
    const qreal contentWidth = width - 2.0 * sidePad;

    // --- profile hero ----------------------------------------------------
    const QRectF hero(contentLeft, screen.top() + 16.0, contentWidth, kHeroHeight);
    if (painter != nullptr) {
        paintGradientArt(*painter, hero, md::ColorRole::SecondaryContainer,
                         md::ColorRole::TertiaryContainer, md::ShapeCorner::ExtraLarge);

        // Avatar disc with initials over the hero.
        const QRectF avatar(hero.left() + 24.0, hero.top() + 28.0, 64.0, 64.0);
        paintAvatarDisc(*painter, avatar, QStringLiteral("林"),
                        md::ColorRole::PrimaryContainer, md::ColorRole::OnPrimaryContainer);

        painter->save();
        const QFont nameFont = fontFor(md::TypeStyle::HeadlineSmall);
        painter->setFont(nameFont);
        painter->setPen(role(md::ColorRole::OnSecondaryContainer));
        painter->drawText(QRectF(avatar.right() + 16.0, hero.top() + 32.0,
                                 hero.width() - 64.0 - 80.0, 32.0),
                          Qt::AlignLeft | Qt::AlignVCenter, L("林小满", "Avery Chen"));

        const QFont mailFont = fontFor(md::TypeStyle::BodyMedium);
        painter->setFont(mailFont);
        painter->setPen(role(md::ColorRole::OnSecondaryContainer));
        painter->drawText(QRectF(avatar.right() + 16.0, hero.top() + 66.0,
                                 hero.width() - 64.0 - 80.0, 22.0),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          QStringLiteral("avery.chen@example.com"));
        painter->restore();
    }

    // Trailing chevron affordance on the hero.
    {
        md::MdIconButton *chev = iconButton(QStringLiteral("chev"), QStringLiteral("chevron_right"));
        chev->setVariant(md::IconButtonVariant::Standard);
        m_slots.append(Slot{chev, QRectF(hero.right() - 52.0, hero.top() + 34.0, 48.0, 48.0)});
    }

    qreal y = hero.bottom() + 24.0;

    // --- grouped preference cards ---------------------------------------
    // Each group is its own tonal panel with a Title header and icon rows.

    auto paintGroupHeader = [&](const QString &text, const QRectF &panel) {
        if (painter == nullptr) {
            return;
        }
        painter->save();
        const QFont font = fontFor(md::TypeStyle::TitleMedium);
        painter->setFont(font);
        painter->setPen(role(md::ColorRole::Primary));
        painter->drawText(QRectF(panel.left() + 20.0, panel.top() + 16.0,
                                 panel.width() - 40.0, 28.0),
                          Qt::AlignLeft | Qt::AlignVCenter, text);
        painter->restore();
    };

    auto paintRow = [&](const QRectF &row, const QString &icon, const QString &label,
                        const QString &supporting) {
        if (painter == nullptr) {
            return;
        }
        painter->save();
        // Leading icon in a small tonal chip — icons with purpose.
        const QRectF iconBox(row.left() + 16.0, row.top() + (row.height() - 36.0) / 2.0,
                             36.0, 36.0);
        fillRounded(*painter, iconBox, role(md::ColorRole::PrimaryContainer),
                    md::ShapeCorner::Full);
        md::MdIcon::paint(painter, iconBox.adjusted(8.0, 8.0, -8.0, -8.0), icon,
                          role(md::ColorRole::OnPrimaryContainer), md::MdIconSet::Auto,
                          md::MdIconFamily::Outlined, md::MdIconStyle{0.0, 400.0, 0.0, 20.0});

        const QFont labelFont = fontFor(md::TypeStyle::BodyLarge);
        painter->setFont(labelFont);
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(QRectF(iconBox.right() + 14.0, row.top() + 10.0,
                                 row.width() - 36.0 - 14.0 - 140.0, 24.0),
                          Qt::AlignLeft | Qt::AlignVCenter, label);

        if (!supporting.isEmpty()) {
            const QFont subFont = fontFor(md::TypeStyle::BodySmall);
            painter->setFont(subFont);
            painter->setPen(role(md::ColorRole::OnSurfaceVariant));
            painter->drawText(QRectF(iconBox.right() + 14.0, row.top() + 34.0,
                                     row.width() - 36.0 - 14.0 - 140.0, 20.0),
                              Qt::AlignLeft | Qt::AlignVCenter, supporting);
        }
        painter->restore();
    };

    // Group 1 — Appearance
    {
        const qreal rowH = 64.0;
        const qreal panelH = 52.0 + 3.0 * rowH + 12.0;
        const QRectF panel(contentLeft, y, contentWidth, panelH);
        if (painter != nullptr) {
            paintTonalPanel(*painter, panel, md::ColorRole::SurfaceContainerHighest,
                            md::ShapeCorner::Large);
        }
        paintGroupHeader(L("外观", "Appearance"), panel);

        qreal rowY = panel.top() + 52.0;
        // Dark theme
        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("dark_mode"), L("深色模式", "Dark theme"),
                 L("切换浅色与深色表面", "Switch between light and dark surfaces"));
        {
            md::MdSwitch *sw = toggle(QStringLiteral("dark"));
            const QSize hint = sw->sizeHint();
            m_slots.append(Slot{sw, QRectF(panel.right() - qreal(hint.width()) - 20.0,
                                           rowY + (rowH - qreal(hint.height())) / 2.0,
                                           qreal(hint.width()), qreal(hint.height()))});
        }
        rowY += rowH;

        // Theme colour
        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("palette"), L("主题色", "Theme colour"),
                 L("从种子色生成整套调色板", "Generate the whole palette from a seed"));
        {
            md::MdRadioButton *light = radio(QStringLiteral("theme-light"), L("浅色", "Light"));
            md::MdRadioButton *dark = radio(QStringLiteral("theme-dark"), L("深色", "Dark"));
            md::MdRadioButton *system = radio(QStringLiteral("theme-system"),
                                              L("跟随系统", "System"));
            if (!m_stateSeeded.contains(QStringLiteral("theme-system"))) {
                system->setChecked(true);
                m_stateSeeded.insert(QStringLiteral("theme-system"));
            }
            const qreal radioW = 96.0;
            const qreal baseX = panel.right() - 3.0 * radioW - 16.0;
            m_slots.append(Slot{light, QRectF(baseX, rowY + 12.0, radioW, 40.0)});
            m_slots.append(Slot{dark, QRectF(baseX + radioW, rowY + 12.0, radioW, 40.0)});
            m_slots.append(Slot{system, QRectF(baseX + 2.0 * radioW, rowY + 12.0, radioW, 40.0)});
        }
        rowY += rowH;

        // Text size
        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("format_size"), L("文字大小", "Text size"),
                 L("影响界面字号", "Scales interface text"));
        {
            md::MdSlider *sizeSlider = slider();
            const qreal sliderH = sizeSlider->sizeHint().height();
            m_slots.append(Slot{sizeSlider,
                                QRectF(panel.right() - panel.width() * 0.38 - 16.0,
                                       rowY + (rowH - sliderH) / 2.0,
                                       panel.width() * 0.38, sliderH)});
        }

        y = panel.bottom() + 20.0;
    }

    // Group 2 — Notifications
    {
        const qreal rowH = 64.0;
        const qreal panelH = 52.0 + 2.0 * rowH + 12.0;
        const QRectF panel(contentLeft, y, contentWidth, panelH);
        if (painter != nullptr) {
            paintTonalPanel(*painter, panel, md::ColorRole::SurfaceContainerHigh,
                            md::ShapeCorner::Large);
        }
        paintGroupHeader(L("通知", "Notifications"), panel);

        qreal rowY = panel.top() + 52.0;
        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("notifications"), L("推送通知", "Push notifications"),
                 L("新邮件与提醒", "New mail and reminders"));
        {
            md::MdSwitch *sw = toggle(QStringLiteral("push"));
            const QSize hint = sw->sizeHint();
            m_slots.append(Slot{sw, QRectF(panel.right() - qreal(hint.width()) - 20.0,
                                           rowY + (rowH - qreal(hint.height())) / 2.0,
                                           qreal(hint.width()), qreal(hint.height()))});
        }
        rowY += rowH;

        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("mail"), L("邮件摘要", "Email digest"),
                 L("每日汇总", "A daily summary"));
        {
            md::MdSwitch *sw = toggle(QStringLiteral("digest"));
            const QSize hint = sw->sizeHint();
            m_slots.append(Slot{sw, QRectF(panel.right() - qreal(hint.width()) - 20.0,
                                           rowY + (rowH - qreal(hint.height())) / 2.0,
                                           qreal(hint.width()), qreal(hint.height()))});
        }

        y = panel.bottom() + 20.0;
    }

    // Group 3 — Language
    {
        const qreal rowH = 72.0;
        const qreal panelH = 52.0 + rowH + 12.0;
        const QRectF panel(contentLeft, y, contentWidth, panelH);
        if (painter != nullptr) {
            paintTonalPanel(*painter, panel, md::ColorRole::SurfaceContainer,
                            md::ShapeCorner::Large);
        }
        paintGroupHeader(L("语言", "Language"), panel);

        const qreal rowY = panel.top() + 52.0;
        paintRow(QRectF(panel.left(), rowY, panel.width(), rowH),
                 QStringLiteral("translate"), L("界面语言", "Interface language"),
                 L("重新生效后打开导航重新进入生效", "Re-enter the app after switching"));
        {
            md::MdButton *button = menuButton();
            button->setText(L("简体中文", "Simplified Chinese"));
            const QSize hint = button->sizeHint();
            m_slots.append(Slot{button,
                                QRectF(panel.right() - qreal(hint.width()) - 16.0,
                                       rowY + (rowH - qreal(hint.height())) / 2.0,
                                       qreal(hint.width()), qreal(hint.height()))});
        }

        y = panel.bottom() + 12.0;
    }

    // Seed the menu items once; opening the popup is the interaction.
    md::MdMenu *popup = menu();
    Q_UNUSED(popup);

    Q_UNUSED(y);
    context.space(8.0);
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
