#include "GalleryWindow.h"

#include "GalleryPage.h"
#include "GalleryPages.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetricsF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QScrollArea>
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
    setMinimumWidth(kNavWidth);
    setMaximumWidth(kNavWidth);
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
    emit currentChanged(clamped);
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
    switch (event->key()) {
    case Qt::Key_Up:
        setCurrentIndex(m_current - 1);
        return;
    case Qt::Key_Down:
        setCurrentIndex(m_current + 1);
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

        md::MdIcon::paint(&painter,
                          QRectF(button.rect.left() + 12.0, button.rect.center().y() - 9.0, 18.0,
                                 18.0),
                          button.icon, theme.color(md::ColorRole::OnSurfaceVariant));
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
    row->addWidget(m_nav);

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
    };

    QStringList titles;
    for (GalleryPage *page : pages) {
        // Each page scrolls independently; the page reports its own height.
        auto *scroll = new QScrollArea(m_stack);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setWidget(page);
        m_stack->addWidget(scroll);
        m_pages.append(page);
        titles.append(page->title());
    }
    m_nav->setItems(titles);

    connect(m_nav, &GalleryNav::currentChanged, this, [this](int index) {
        m_stack->setCurrentIndex(index);
        if (index >= 0 && index < m_pages.size()) {
            m_header->setTitle(m_pages.at(index)->title());
        }
    });
    m_stack->setCurrentIndex(0);
    m_header->setTitle(m_pages.first()->title());

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

void GalleryWindow::onThemeChanged()
{
    m_header->update();
    m_nav->update();
    for (GalleryPage *page : m_pages) {
        page->updateGeometry();
        page->update();
    }
}

} // namespace gallery
