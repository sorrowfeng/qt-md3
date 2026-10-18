#include "GalleryWindow.h"

#include "GalleryPage.h"
#include "GalleryPages.h"
#include "core/MdMotion.h"
#include "styles/MdScrollBarStyle.h"
#include "styles/MdStyleBase.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtGui/QFontMetricsF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QShortcut>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QVBoxLayout>

#include <algorithm>

namespace gallery {

namespace {

constexpr qreal kNavWidth = 208.0;
constexpr qreal kNavItemHeight = 52.0;
constexpr qreal kHeaderHeight = 68.0;

void drawLabel(QPainter &painter, const QRectF &rect, const QFont &font, const QColor &color,
               const QString &text, int flags = Qt::AlignLeft | Qt::AlignVCenter)
{
    painter.save();
    painter.setFont(font);
    painter.setPen(color);
    painter.drawText(rect, flags, text);
    painter.restore();
}

} // namespace

// ---------------------------------------------------------------------------
// GalleryNav
// ---------------------------------------------------------------------------

GalleryNav::GalleryNav(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    // Width is owned by the wrapping QScrollArea (fixed at kNavWidth); the
    // nav itself must be free to narrow by the scrollbar strip without
    // triggering a horizontal bar, so only the width hint is published.
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

void GalleryNav::setItems(const QStringList &titles)
{
    m_titles = titles;
    qDeleteAll(m_ripples);
    m_ripples.clear();
    for (int i = 0; i < titles.size(); ++i) {
        auto *ripple = new md::MdRippleController(this);
        connect(ripple, &md::MdRippleController::repaintRequested, this,
                qOverload<>(&QWidget::update));
        m_ripples.append(ripple);
    }
    m_current = qBound(0, m_current, qMax(titles.size() - 1, 0));
    updateGeometry();
    update();
}

void GalleryNav::setCurrentIndex(int index)
{
    const int clamped = qBound(0, index, qMax(m_titles.size() - 1, 0));
    if (clamped == m_current) {
        return;
    }
    m_current = clamped;
    update();
    ensureCurrentItemVisible();
    emit currentChanged(clamped);
}

QSize GalleryNav::sizeHint() const
{
    return QSize(int(kNavWidth), contentHeight());
}

QSize GalleryNav::minimumSizeHint() const
{
    // Height is the full content list so the surrounding scroll area always
    // offers a vertical bar once the page list outgrows the window. Width
    // stays flexible: the scroll area may shrink the rail by the scrollbar
    // strip, and forcing kNavWidth here would trade that for a horizontal bar.
    return QSize(int(kNavWidth * 0.75), contentHeight());
}

int GalleryNav::contentHeight() const
{
    // 16 top pad + one row per item + 16 bottom pad (matches itemRect).
    return int(16.0 + m_titles.size() * kNavItemHeight + 16.0);
}

void GalleryNav::ensureCurrentItemVisible()
{
    if (m_titles.isEmpty()) {
        return;
    }
    // The nav is hosted in a QScrollArea (viewport is the direct parent).
    auto *viewport = parentWidget();
    auto *area = viewport ? qobject_cast<QScrollArea *>(viewport->parentWidget()) : nullptr;
    if (!area) {
        return;
    }
    const QRect item = itemRect(m_current).toAlignedRect();
    const int margin = int(kNavItemHeight / 2.0);
    area->ensureVisible(item.center().x(), item.center().y(), margin, margin);
}

QRectF GalleryNav::itemRect(int index) const
{
    return QRectF(12.0, 16.0 + index * kNavItemHeight, width() - 24.0, kNavItemHeight - 4.0);
}

int GalleryNav::itemAt(const QPointF &position) const
{
    for (int i = 0; i < m_titles.size(); ++i) {
        if (itemRect(i).contains(position)) {
            return i;
        }
    }
    return -1;
}

void GalleryNav::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const md::MdTheme &theme = md::MdTheme::instance();
    painter.fillRect(rect(), theme.color(md::ColorRole::SurfaceContainerLow));

    for (int i = 0; i < m_titles.size(); ++i) {
        const QRectF item = itemRect(i);
        const bool selected = i == m_current;

        const QColor base = theme.color(md::ColorRole::SurfaceContainerLow);
        const QColor tinted =
            md::MdStateLayer::over(base, theme.color(md::ColorRole::OnSurface), i == m_hovered,
                                   false, i == m_pressed, false);
        if (selected) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme.color(md::ColorRole::SecondaryContainer));
            painter.drawPath(md::MdShape::roundedRect(
                item, md::MdShape::resolvedRadii(md::ShapeCorner::Full, item.size())));
        } else if (tinted != base) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(tinted);
            painter.drawPath(md::MdShape::roundedRect(
                item, md::MdShape::resolvedRadii(md::ShapeCorner::Full, item.size())));
        }

        const QPainterPath clip = md::MdShape::roundedRect(
            item, md::MdShape::resolvedRadii(md::ShapeCorner::Full, item.size()));
        m_ripples[i]->setClipPath(clip);
        m_ripples[i]->setContentColor(theme.color(md::ColorRole::OnSurface));
        md::MdRipple::paint(&painter, m_ripples[i]->currentFrame(), clip,
                            theme.color(md::ColorRole::OnSurface));

        drawLabel(painter, item.adjusted(20.0, 0.0, -12.0, 0.0),
                  md::MdTypeScale::font(md::TypeStyle::LabelLarge),
                  selected ? theme.color(md::ColorRole::OnSecondaryContainer)
                           : theme.color(md::ColorRole::OnSurface),
                  m_titles.at(i));
    }
}

void GalleryNav::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    const int index = itemAt(event->pos());
    if (index < 0 || event->button() != Qt::LeftButton) {
        return;
    }
    m_pressed = index;
    if (index < m_ripples.size()) {
        m_ripples[index]->press(event->pos() - itemRect(index).topLeft());
    }
    update();
}

void GalleryNav::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_pressed >= 0 && m_pressed < m_ripples.size()) {
        m_ripples[m_pressed]->release();
    }
    const int index = itemAt(event->pos());
    m_pressed = -1;
    if (index >= 0) {
        setCurrentIndex(index);
    }
    update();
}

void GalleryNav::mouseMoveEvent(QMouseEvent *event)
{
    const int index = itemAt(event->pos());
    if (index != m_hovered) {
        m_hovered = index;
        update();
    }
}

void GalleryNav::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = -1;
    update();
}

void GalleryNav::keyPressEvent(QKeyEvent *event)
{
    // Full list-navigation contract: Up/Down step, PageUp/PageDown jump a
    // screenful, Home/End land on the ends. Selection follows focus so the
    // page stack stays in lockstep with the rail.
    constexpr int kPageStep = 8;
    switch (event->key()) {
    case Qt::Key_Up:
        setCurrentIndex(m_current - 1);
        return;
    case Qt::Key_Down:
        setCurrentIndex(m_current + 1);
        return;
    case Qt::Key_PageUp:
        setCurrentIndex(m_current - kPageStep);
        return;
    case Qt::Key_PageDown:
        setCurrentIndex(m_current + kPageStep);
        return;
    case Qt::Key_Home:
        setCurrentIndex(0);
        return;
    case Qt::Key_End:
        setCurrentIndex(m_titles.size() - 1);
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void GalleryNav::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    update();
}

void GalleryNav::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    update();
}

// ---------------------------------------------------------------------------
// GalleryHeader
// ---------------------------------------------------------------------------

GalleryHeader::GalleryHeader(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumHeight(kHeaderHeight);
    setMaximumHeight(kHeaderHeight);
}

void GalleryHeader::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    update();
}

void GalleryHeader::rebuildButtons()
{
    m_buttons.clear();
    const md::MdTheme &theme = md::MdTheme::instance();

    struct Spec
    {
        int action;
        QString label;
        QString icon;
    };
    const QVector<Spec> specs = {
        {1, theme.isDynamicColor() ? QStringLiteral("dynamic on") : QStringLiteral("dynamic off"),
         QStringLiteral("palette")},
        {2, md::schemeVariantName(theme.schemeVariant()), QStringLiteral("palette")},
        {3, md::contrastLevelName(theme.contrastLevel()), QStringLiteral("visibility")},
        {4, md::densityName(theme.density()), QStringLiteral("straighten")},
        {5, theme.isRightToLeft() ? QStringLiteral("rtl") : QStringLiteral("ltr"),
         QStringLiteral("arrow_forward")},
        // Two languages only: Simplified Chinese (default) and English.
        {6, theme.languageTag() == QStringLiteral("zh-Hans") ? QStringLiteral("中文")
                                                            : QStringLiteral("English"),
         QStringLiteral("text_fields")},
        {7, QStringLiteral("seed"), QStringLiteral("color_lens")},
        {0, theme.themeMode() == md::ThemeMode::Dark ? QStringLiteral("dark")
                                                     : QStringLiteral("light"),
         theme.themeMode() == md::ThemeMode::Dark ? QStringLiteral("dark_mode")
                                                  : QStringLiteral("light_mode")},
    };

    const QFont font = md::MdTypeScale::font(md::TypeStyle::LabelLarge);
    const QFontMetricsF metrics(font);
    qreal x = width() - 20.0;
    // Lay the strip out right to left so it hugs the trailing edge.
    QVector<Button> reversed;
    for (const Spec &spec : specs) {
        const qreal textWidth = metrics.horizontalAdvance(spec.label);
        const qreal buttonWidth = textWidth + 52.0;
        x -= buttonWidth;
        Button button;
        button.rect = QRectF(x, 16.0, buttonWidth, 36.0);
        button.label = spec.label;
        button.action = spec.action;
        button.icon = spec.icon;
        reversed.append(button);
        x -= 8.0;
    }
    std::reverse(reversed.begin(), reversed.end());
    m_buttons = reversed;
}

void GalleryHeader::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    rebuildButtons();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const md::MdTheme &theme = md::MdTheme::instance();
    painter.fillRect(rect(), theme.color(md::ColorRole::Surface));

    drawLabel(painter, QRectF(24.0, 12.0, width() * 0.5, 26.0),
              md::MdTypeScale::font(md::TypeStyle::TitleMedium),
              theme.color(md::ColorRole::OnSurface), m_title);
    drawLabel(painter, QRectF(24.0, 36.0, width() * 0.5, 22.0),
              md::MdTypeScale::font(md::TypeStyle::LabelMedium),
              theme.color(md::ColorRole::OnSurfaceVariant),
              QStringLiteral("qt-md3 组件画廊 / component gallery"));

    for (int i = 0; i < m_buttons.size(); ++i) {
        const Button &button = m_buttons.at(i);
        const QColor base = theme.color(md::ColorRole::Surface);
        const QColor tinted = md::MdStateLayer::over(
            base, theme.color(md::ColorRole::OnSurface), i == m_hovered, false, i == m_pressed,
            false);
        if (tinted != base) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(tinted);
            painter.drawPath(md::MdShape::roundedRect(
                button.rect, md::MdShape::resolvedRadii(md::ShapeCorner::Full, button.rect.size())));
        }
        // Outline so an inactive "off" state is still legible against surface.
        QPen border(theme.color(md::ColorRole::OutlineVariant));
        border.setCosmetic(true);
        painter.setPen(border);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(md::MdShape::roundedRect(
            button.rect.adjusted(0.5, 0.5, -0.5, -0.5),
            md::MdShape::resolvedRadii(md::ShapeCorner::Full, button.rect.size())));

        if (button.action == 7) {
            // Seed control: a colour lens instead of a mono glyph, so the
            // current seed is visible at a glance.
            const QRectF swatch(button.rect.left() + 12.0, button.rect.center().y() - 9.0, 18.0,
                                18.0);
            painter.setPen(Qt::NoPen);
            painter.setBrush(theme.seedColor());
            painter.drawEllipse(swatch);
            QPen ring(theme.color(md::ColorRole::Outline));
            ring.setCosmetic(true);
            painter.setPen(ring);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(swatch.adjusted(0.5, 0.5, -0.5, -0.5));
        } else {
            md::MdIcon::paint(&painter,
                              QRectF(button.rect.left() + 12.0, button.rect.center().y() - 9.0, 18.0,
                                     18.0),
                              button.icon, theme.color(md::ColorRole::OnSurfaceVariant));
        }
        drawLabel(painter, button.rect.adjusted(36.0, 0.0, -12.0, 0.0),
                  md::MdTypeScale::font(md::TypeStyle::LabelLarge),
                  theme.color(md::ColorRole::OnSurfaceVariant), button.label);
    }
}

int GalleryHeader::buttonAt(const QPointF &position) const
{
    for (int i = 0; i < m_buttons.size(); ++i) {
        if (m_buttons.at(i).rect.contains(position)) {
            return i;
        }
    }
    return -1;
}

void GalleryHeader::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    m_pressed = buttonAt(event->pos());
    update();
}

void GalleryHeader::mouseReleaseEvent(QMouseEvent *event)
{
    const int pressed = m_pressed;
    m_pressed = -1;
    const int index = buttonAt(event->pos());
    update();
    if (pressed < 0 || pressed != index) {
        return;
    }
    switch (m_buttons.at(index).action) {
    case 0: emit themeModeToggled(); break;
    case 1: emit dynamicColorToggled(); break;
    case 2: emit variantCycled(); break;
    case 3: emit contrastCycled(); break;
    case 4: emit densityCycled(); break;
    case 5: emit directionToggled(); break;
    case 6: emit languageCycled(); break;
    case 7: emit seedCycled(); break;
    default: break;
    }
}

void GalleryHeader::mouseMoveEvent(QMouseEvent *event)
{
    const int index = buttonAt(event->pos());
    if (index != m_hovered) {
        m_hovered = index;
        update();
    }
}

void GalleryHeader::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = -1;
    update();
}

// ---------------------------------------------------------------------------
// GalleryWindow
// ---------------------------------------------------------------------------

GalleryWindow::GalleryWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("qt-md3 gallery"));

    auto *central = new QWidget(this);
    auto *row = new QHBoxLayout(central);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(0);

    m_nav = new GalleryNav(central);
    // The rail is taller than a normal window once every Stage 1 family is
    // listed, so it lives in a scroll area rather than being clipped.
    auto *navScroll = new QScrollArea(central);
    navScroll->setWidget(m_nav);
    navScroll->setWidgetResizable(true);
    navScroll->setFrameShape(QFrame::NoFrame);
    navScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    navScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    navScroll->setMinimumWidth(int(kNavWidth));
    navScroll->setMaximumWidth(int(kNavWidth));
    navScroll->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    // The nav paints its own surface; keep the scroll area chrome out of the way.
    navScroll->viewport()->setAutoFillBackground(false);
    row->addWidget(navScroll);
    m_scrollers.append(navScroll);

    auto *right = new QWidget(central);
    auto *column = new QVBoxLayout(right);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    m_header = new GalleryHeader(right);
    column->addWidget(m_header);

    m_stack = new QStackedWidget(right);
    column->addWidget(m_stack, 1);
    row->addWidget(right, 1);

    setCentralWidget(central);

    // Pages, in navigation order: the foundation modules first, then one page
    // per Stage 1 component family in the order the families land.
    const QVector<GalleryPage *> pages = {
        new OverviewPage,
        new ColourPage,
        new TypePage,
        new SurfacePage,
        new MotionPage,
        new InteractionPage,
        new IconPage,
        createButtonPage(),
        createButtonGroupPage(),
        createIconButtonPage(),
        createFabPage(),
        createExtendedFabPage(),
        createFabMenuPage(),
        createSplitButtonPage(),
        createSegmentedButtonPage(),
        createBadgePage(),
        createProgressIndicatorPage(),
        createLoadingIndicatorPage(),
        createSnackbarPage(),
        createTooltipPage(),
        createCardPage(),
        createDialogPage(),
        createBottomSheetPage(),
        createSideSheetPage(),
        createCarouselPage(),
        createDividerPage(),
        createListPage(),
        createAppBarPage(),
        createToolbarPage(),
        createNavigationBarPage(),
        createNavigationRailPage(),
        createNavigationDrawerPage(),
        createTabsPage(),
        createCheckboxPage(),
        createChipPage(),
        createRadioButtonPage(),
        createSwitchPage(),
        createMenuPage(),
        createSliderPage(),
        createTimePickerPage(),
        createDatePickerPage(),
        createTextFieldPage(),
        createSearchPage(),
        createStage2Page(),
        createStage2ShellPage(),
        createStage2ChromePage(),
        createStage2DataPage(),
        createStage2LayoutPage(),
        createStage2EntryPage(),
        createStage2DetailPage(),
        createStage2RemainderPage(),
        createDynamicColourPage(),
        createShowcaseMailPage(),
        createShowcaseMusicPage(),
        createShowcaseSettingsPage(),
    };

    QStringList titles;
    md::MdScrollBarStyle *scrollStyle = md::MdScrollBarStyle::shared();
    for (GalleryPage *page : pages) {
        // Each page scrolls independently; the page reports its own height.
        auto *scroll = new QScrollArea(m_stack);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidget(page);
        scroll->setStyle(scrollStyle);
        if (QScrollBar *vbar = scroll->verticalScrollBar()) {
            vbar->setStyle(scrollStyle);
        }
        if (QScrollBar *hbar = scroll->horizontalScrollBar()) {
            hbar->setStyle(scrollStyle);
        }
        m_stack->addWidget(scroll);
        m_scrollers.append(scroll);
        m_pages.append(page);
        titles.append(page->title());
    }
    m_nav->setItems(titles);
    // Install the same overlay chrome on the nav rail's bar.
    navScroll->setStyle(scrollStyle);
    if (QScrollBar *navBar = navScroll->verticalScrollBar()) {
        navBar->setStyle(scrollStyle);
    }

    connect(m_nav, &GalleryNav::currentChanged, this, &GalleryWindow::onPageChanged);
    m_stack->setCurrentIndex(0);
    m_header->setTitle(m_pages.first()->title());

    // Global page jump: Ctrl+1..9 lands on the first nine pages. Digits alone
    // stay with the focused control so a text field can still type numbers.
    for (int i = 0; i < 9 && i < m_pages.size(); ++i) {
        auto *shortcut =
            new QShortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, [this, i] {
            setCurrentPage(i);
            m_nav->setFocus(Qt::ShortcutFocusReason);
        });
    }

    connect(m_header, &GalleryHeader::themeModeToggled, this, [] {
        md::MdTheme::instance().toggleThemeMode();
    });
    connect(m_header, &GalleryHeader::dynamicColorToggled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        theme.setDynamicColor(!theme.isDynamicColor());
    });
    connect(m_header, &GalleryHeader::variantCycled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        const int next = (int(theme.schemeVariant()) + 1) % int(md::SchemeVariant::Count);
        theme.setSchemeVariant(md::SchemeVariant(next));
    });
    connect(m_header, &GalleryHeader::contrastCycled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        const int next = (int(theme.contrastLevel()) + 1) % int(md::ContrastLevel::Count);
        theme.setContrastLevel(md::ContrastLevel(next));
    });
    connect(m_header, &GalleryHeader::densityCycled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        const int next = (int(theme.density()) + 1) % int(md::Density::Count);
        theme.setDensity(md::Density(next));
    });
    connect(m_header, &GalleryHeader::directionToggled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        theme.setDirection(theme.isRightToLeft() ? Qt::LeftToRight : Qt::RightToLeft);
    });
    connect(m_header, &GalleryHeader::languageCycled, this, [] {
        md::MdTheme &theme = md::MdTheme::instance();
        // A plain toggle: Simplified Chinese is the default, English is the
        // only alternative. Switching the tag re-runs the script-category
        // lookup, which is what drives the line-height adjustment on the Type
        // page, and re-renders every page's bilingual copy.
        const bool toChinese = theme.languageTag() != QStringLiteral("zh-Hans");
        theme.setLanguageTag(toChinese ? QStringLiteral("zh-Hans") : QStringLiteral("en"));
    });
    connect(m_header, &GalleryHeader::seedCycled, this, [] {
        // Wallpaper-style seed set, shared with the Dynamic colour page.
        static const QColor seeds[] = {
            QColor(QStringLiteral("#6750A4")), QColor(QStringLiteral("#00A3A3")),
            QColor(QStringLiteral("#E8710A")), QColor(QStringLiteral("#2E7D32")),
            QColor(QStringLiteral("#D81B60")), QColor(QStringLiteral("#1B6EF3")),
            QColor(QStringLiteral("#F9A825")),
        };
        md::MdTheme &theme = md::MdTheme::instance();
        theme.setDynamicColor(true);
        int next = 0;
        for (int i = 0; i < int(sizeof(seeds) / sizeof(seeds[0])); ++i) {
            if (seeds[i] == theme.seedColor()) {
                next = (i + 1) % int(sizeof(seeds) / sizeof(seeds[0]));
                break;
            }
        }
        theme.setSeedColor(seeds[next]);
    });

    // One subscription for the whole shell.
    md::MdStyleBase::connectThemeUpdate(this, &GalleryWindow::onThemeChanged);

    resize(1280, 860);
}

// The three accessors below exist so the shell can be driven without a mouse:
// `--screenshot` walks the pages, and a future render check can do the same.
// They go through the navigation rail so the selection highlight and the header
// title stay in step with the visible page.

int GalleryWindow::pageCount() const
{
    return m_pages.size();
}

QString GalleryWindow::pageTitle(int index) const
{
    if (index < 0 || index >= m_pages.size()) {
        return QString();
    }
    return m_pages.at(index)->title();
}

QString GalleryWindow::pageSlug(int index) const
{
    if (index < 0 || index >= m_pages.size()) {
        return QString();
    }
    return m_pages.at(index)->slug();
}

QWidget *GalleryWindow::pageWidget(int index) const
{
    if (index < 0 || index >= m_pages.size()) {
        return nullptr;
    }
    return m_pages.at(index);
}

void GalleryWindow::setCurrentPage(int index)
{
    if (index < 0 || index >= m_pages.size()) {
        return;
    }
    m_nav->setCurrentIndex(index);
}

int GalleryWindow::currentPage() const
{
    return m_nav->currentIndex();
}

void GalleryWindow::onPageChanged(int index)
{
    if (index < 0 || index >= m_pages.size()) {
        return;
    }
    const int outgoing = m_stack->currentIndex();
    if (outgoing != index && outgoing >= 0 && outgoing < m_stack->count()) {
        startPageFade(outgoing, index);
    } else {
        m_stack->setCurrentIndex(index);
    }
    m_header->setTitle(m_pages.at(index)->title());
}

void GalleryWindow::startPageFade(int fromIndex, int toIndex)
{
    // Cross-fade on the effects-fast spring (the same slot MdStackedWidget
    // uses). Both scroll areas stay visible for the flight: the outgoing one
    // carries a QGraphicsOpacityEffect down to 0, the incoming one up to 1.
    // Interrupting a page change drops the page that was fading out and
    // retargets from the on-screen opacity, so a fast nav walk never jumps.
    auto *fromPage = qobject_cast<QWidget *>(m_stack->widget(fromIndex));
    auto *toPage = qobject_cast<QWidget *>(m_stack->widget(toIndex));
    if (fromPage == nullptr || toPage == nullptr || fromPage == toPage) {
        m_stack->setCurrentIndex(toIndex);
        return;
    }

    // The page that was fading in becomes the new outgoing one: keep its
    // effect so the retarget continues from the on-screen opacity. The page
    // that was fading out is done - snap it clear.
    QGraphicsOpacityEffect *carried = m_fadeIn;
    QWidget *carriedPage = m_fadeInPage;
    if (m_fadeOut != nullptr) {
        if (m_fadeOutPage != nullptr) {
            m_fadeOutPage->setGraphicsEffect(nullptr);
            m_fadeOutPage->setVisible(false);
        }
        m_fadeOut = nullptr;
        m_fadeOutPage = nullptr;
    }
    m_fadeIn = nullptr;
    m_fadeInPage = nullptr;
    m_fadeToIndex = toIndex;

    const qreal startOpacity = carried != nullptr ? carried->opacity() : 1.0;
    m_fadeStartOut = startOpacity;

    if (m_fadeTimer == nullptr) {
        m_fadeTimer = new QTimer(this);
        m_fadeTimer->setInterval(16);
        m_fadeTimer->setTimerType(Qt::PreciseTimer);
        m_fadeClock = new QElapsedTimer;
        connect(m_fadeTimer, &QTimer::timeout, this, [this] {
            if (!m_fadeRunning) {
                return;
            }
            const qreal seconds = qreal(m_fadeClock->elapsed()) / 1000.0;
            const md::MdSpring spring = md::MdMotion::spring(md::MotionSpring::EffectsFast);
            const qreal t = qBound<qreal>(0.0, spring.valueAt(seconds), 1.0);
            // Outgoing continues from whatever opacity it had on screen; the
            // incoming always rises from 0 so a retarget never snaps.
            if (m_fadeOut != nullptr) {
                m_fadeOut->setOpacity(m_fadeStartOut * (1.0 - t));
            }
            if (m_fadeIn != nullptr) {
                m_fadeIn->setOpacity(t);
            }
            if (t >= 0.999 || m_fadeClock->elapsed() >= 400) {
                m_fadeRunning = false;
                m_fadeTimer->stop();
                if (m_fadeOut != nullptr) {
                    if (m_fadeOutPage != nullptr) {
                        m_fadeOutPage->setGraphicsEffect(nullptr);
                        m_fadeOutPage->setVisible(false);
                    }
                    m_fadeOut = nullptr;
                    m_fadeOutPage = nullptr;
                }
                if (m_fadeIn != nullptr) {
                    if (m_fadeInPage != nullptr) {
                        m_fadeInPage->setGraphicsEffect(nullptr);
                    }
                    m_fadeIn = nullptr;
                    m_fadeInPage = nullptr;
                }
                for (int i = 0; i < m_stack->count(); ++i) {
                    if (QWidget *page = m_stack->widget(i)) {
                        page->setVisible(i == m_fadeToIndex);
                    }
                }
            }
        });
    }

    if (carried != nullptr) {
        m_fadeOut = carried;
        m_fadeOutPage = carriedPage != nullptr ? carriedPage : fromPage;
    } else {
        m_fadeOut = new QGraphicsOpacityEffect(fromPage);
        m_fadeOut->setOpacity(startOpacity);
        fromPage->setGraphicsEffect(m_fadeOut);
        m_fadeOutPage = fromPage;
    }

    m_fadeIn = new QGraphicsOpacityEffect(toPage);
    m_fadeIn->setOpacity(0.0);
    toPage->setGraphicsEffect(m_fadeIn);
    m_fadeInPage = toPage;

    // QStackedWidget hides the previous page on setCurrentIndex; re-show both
    // so the cross-fade has something to paint.
    m_stack->blockSignals(true);
    m_stack->setCurrentIndex(toIndex);
    m_stack->blockSignals(false);
    fromPage->setVisible(true);
    fromPage->lower();
    toPage->setVisible(true);
    toPage->raise();

    m_fadeClock->restart();
    m_fadeRunning = true;
    if (!m_fadeTimer->isActive()) {
        m_fadeTimer->start();
    }
}

void GalleryWindow::onThemeChanged()
{
    m_header->update();
    m_nav->update();
    for (GalleryPage *page : m_pages) {
        page->updateGeometry();
        page->update();
    }
    for (QScrollArea *scroller : m_scrollers) {
        if (!scroller) {
            continue;
        }
        scroller->update();
        if (QScrollBar *bar = scroller->verticalScrollBar()) {
            bar->update();
        }
        if (QScrollBar *bar = scroller->horizontalScrollBar()) {
            bar->update();
        }
    }
}

} // namespace gallery
