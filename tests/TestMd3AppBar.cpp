// TestMd3AppBar — the App bars family: MdTopAppBar + MdBottomAppBar.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-app-bar.scss and its five size sets plus
// _md-comp-bottom-app-bar.scss (every row), cross-checked against androidx
// Compose Material3 AppBarTokens.kt / AppBarSmallTokens.kt / … (which agree row
// for row). The *layout* and the scroll model come from AppBar.kt alone —
// material-web has no production top app bar, so nothing else describes how a
// title, a leading button and an action row divide a bar.
//
// Two of the assertions below are the ones worth keeping: `titleInset` is
// 16 - 4, i.e. the spec's "16dp from the edge" is the 4 px container padding
// plus the 12 px an icon button brings itself; and `collapsedRowHeight` is the
// *small* variant's height whatever the variant, which is what makes a medium
// bar lose 48 px and a large bar lose 88.

#include "core/MdAppBarScrollBehavior.h"
#include "core/MdAppBarTokens.h"
#include "core/MdColorMath.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "styles/MdBottomAppBarStyle.h"
#include "styles/MdChildBox.h"
#include "styles/MdTopAppBarStyle.h"
#include "widgets/MdBottomAppBar.h"
#include "widgets/MdFab.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdTopAppBar.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QScrollBar>

#include <cmath>

using namespace md;

namespace {

/// A slot stand-in with a known intrinsic size, so the layout arithmetic is
/// tested without dragging an icon button's own geometry into it.
class ProbeWidget : public QWidget
{
public:
    explicit ProbeWidget(const QSize &hint, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_hint(hint)
    {
        setFixedSize(hint);
    }

    QSize sizeHint() const override { return m_hint; }

private:
    QSize m_hint;
};

QColor pixelColorAt(QWidget &widget, const QPointF &position)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    // DrawChildren only: a top-level widget would otherwise be erased with its
    // opaque window background first.
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

} // namespace

class TestMd3AppBar : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void topAppBarTokenTable();
    void variantHeightsTable();
    void collapsedRowIsTheSmallHeight();
    void titleInsetIsTheEdgeDistance();
    void tokenOverridesReachTheLayout();

    void twoRowGeometry();
    void navigationPushesTheTitlePastItself();
    void containersArePlacedNotWidgets();
    void centredTitleStaysCentredOnTheBar();
    void alignmentMovesTheTitle();

    void colourTransitionTable();
    void oklabLerpEndpoints();
    void scrollModesConsumeTheRightWay();
    void settleSnapsToTheNearerEnd();
    void followScrollBarMirrorsTheOffset();

    void topAppBarPaintsItsContainer();
    void topAppBarFollowsTheThemeToScrolled();

    void bottomAppBarTokenTable();
    void bottomAppBarLayoutAndFab();

private:
    MdTopAppBar *m_bar = nullptr;
    MdBottomAppBar *m_bottom = nullptr;
    MdAppBarScrollBehavior *m_behavior = nullptr;
    ProbeWidget *m_navigation = nullptr;
    ProbeWidget *m_action = nullptr;
    ProbeWidget *m_action2 = nullptr;
    ProbeWidget *m_fab = nullptr;
};

void TestMd3AppBar::init()
{
    m_bar = new MdTopAppBar(QStringLiteral("Title"));
    m_bottom = new MdBottomAppBar;
    m_behavior = new MdAppBarScrollBehavior;
    m_navigation = new ProbeWidget(QSize(48, 48));
    m_action = new ProbeWidget(QSize(48, 48));
    m_action2 = new ProbeWidget(QSize(48, 48));
    m_fab = new ProbeWidget(QSize(56, 56));
    m_bar->resize(360, 64);
}

void TestMd3AppBar::cleanup()
{
    // A failing QCOMPARE / QVERIFY returns out of its slot immediately, so
    // every piece of state a slot can touch is reset here — including the
    // variant, the alignment, the slot widgets and their parenting. Getting
    // this wrong turns one real failure into a cascade of misleading ones,
    // which the list family paid for four times.
    MdComponentTokens::global().remove(
        QStringLiteral("md.comp.app-bar.small.container.height"));
    MdComponentTokens::global().remove(
        QStringLiteral("md.comp.app-bar.leading-space"));
    MdComponentTokens::global().remove(
        QStringLiteral("md.comp.bottom-app-bar.container.height"));

    for (ProbeWidget *probe : {m_navigation, m_action, m_action2, m_fab}) {
        probe->hide();
        probe->setParent(nullptr);
        probe->deleteLater();
    }
    m_navigation = nullptr;
    m_action = nullptr;
    m_action2 = nullptr;
    m_fab = nullptr;

    delete m_bottom;
    m_bottom = nullptr;
    delete m_bar;
    m_bar = nullptr;
    delete m_behavior;
    m_behavior = nullptr;
}

// ---------------------------------------------------------------------------
// Tokens
// ---------------------------------------------------------------------------

void TestMd3AppBar::topAppBarTokenTable()
{
    const MdAppBarTokens tokens = MdAppBarTokens::resolve(MdAppBarVariant::Small);

    // The common set.
    QCOMPARE(tokens.avatarSize, 32.0);
    QCOMPARE(tokens.iconButtonSpace, 0.0);
    QCOMPARE(tokens.iconSize, 24.0);
    QCOMPARE(tokens.leadingSpace, 4.0);
    QCOMPARE(tokens.trailingSpace, 4.0);
    QCOMPARE(tokens.searchLeadingSpace, 8.0);
    QCOMPARE(tokens.searchTrailingSpace, 8.0);

    QCOMPARE(int(tokens.containerColor), int(ColorRole::Surface));
    QCOMPARE(int(tokens.onScrollContainerColor), int(ColorRole::SurfaceContainer));
    QCOMPARE(int(tokens.leadingIconColor), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.titleColor), int(ColorRole::OnSurface));
    QCOMPARE(int(tokens.trailingIconColor), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.subtitleColor), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.searchContainerColor), int(ColorRole::SurfaceContainer));
    QCOMPARE(int(tokens.searchLabelColor), int(ColorRole::OnSurfaceVariant));
    QCOMPARE(int(tokens.searchOnScrollContainerColor), int(ColorRole::SurfaceContainerHighest));

    QCOMPARE(int(tokens.containerElevation), int(ElevationLevel::Level0));
    QCOMPARE(int(tokens.onScrollContainerElevation), int(ElevationLevel::Level2));
    QCOMPARE(int(tokens.containerShape), int(ShapeCorner::None));

    // The small size set.
    QCOMPARE(tokens.containerHeight, 64.0);
    QCOMPARE(tokens.withSubtitleContainerHeight, 0.0);
    QCOMPARE(int(tokens.titleTypeStyle), int(TypeStyle::TitleLarge));
    QCOMPARE(int(tokens.subtitleTypeStyle), int(TypeStyle::LabelMedium));
    QCOMPARE(tokens.searchContainerHeight, 56.0);
    QCOMPARE(int(tokens.searchContainerShape), int(ShapeCorner::Full));
    QCOMPARE(int(tokens.searchLabelTypeStyle), int(TypeStyle::BodyLarge));

    // The collapsed row's styles, from `AppBarSmallTokens`.
    QCOMPARE(int(tokens.collapsedTitleTypeStyle), int(TypeStyle::TitleLarge));
    QCOMPARE(int(tokens.collapsedSubtitleTypeStyle), int(TypeStyle::LabelMedium));

    // The derived constants.
    QCOMPARE(tokens.edgeSpace, 16.0);
    QCOMPARE(tokens.mediumTitleBottomPadding, 24.0);
    QCOMPARE(tokens.largeTitleBottomPadding, 28.0);
    QCOMPARE(tokens.scrolledColourThreshold, 0.01);

    QVERIFY(!tokens.isTwoRows());
    QVERIFY(tokens.supportsSubtitle());
    QCOMPARE(tokens.titleBottomPadding(), 0.0);
}

void TestMd3AppBar::variantHeightsTable()
{
    struct Row
    {
        MdAppBarVariant variant;
        qreal height;
        qreal withSubtitle;
        TypeStyle title;
        TypeStyle subtitle;
        qreal bottomPadding;
        bool twoRows;
    };

    const Row rows[] = {
        {MdAppBarVariant::Small, 64.0, 0.0, TypeStyle::TitleLarge, TypeStyle::LabelMedium, 0.0,
         false},
        {MdAppBarVariant::Medium, 112.0, 0.0, TypeStyle::HeadlineSmall, TypeStyle::LabelLarge, 24.0,
         true},
        {MdAppBarVariant::Large, 152.0, 0.0, TypeStyle::HeadlineMedium, TypeStyle::TitleMedium, 28.0,
         true},
        {MdAppBarVariant::MediumFlexible, 112.0, 136.0, TypeStyle::HeadlineMedium,
         TypeStyle::LabelLarge, 24.0, true},
        {MdAppBarVariant::LargeFlexible, 120.0, 152.0, TypeStyle::DisplaySmall,
         TypeStyle::TitleMedium, 28.0, true},
    };

    for (const Row &row : rows) {
        const MdAppBarTokens tokens = MdAppBarTokens::resolve(row.variant);
        QCOMPARE(tokens.containerHeight, row.height);
        QCOMPARE(tokens.withSubtitleContainerHeight, row.withSubtitle);
        QCOMPARE(int(tokens.titleTypeStyle), int(row.title));
        QCOMPARE(int(tokens.subtitleTypeStyle), int(row.subtitle));
        QCOMPARE(tokens.titleBottomPadding(), row.bottomPadding);
        QCOMPARE(tokens.isTwoRows(), row.twoRows);

        // Only the small and the two flexible variants grow for a subtitle;
        // the baseline medium and large sets publish a `subtitle.font` row
        // under a deprecation that says they have no subtitle support.
        const bool grows = row.withSubtitle > 0.0;
        QCOMPARE(tokens.expandedHeight(true), grows ? row.withSubtitle : row.height);
        QCOMPARE(tokens.expandedHeight(false), row.height);
    }
}

void TestMd3AppBar::collapsedRowIsTheSmallHeight()
{
    // Compose reads `AppBarSmallTokens.ContainerHeight` for
    // `MediumAppBarCollapsedHeight` and `LargeAppBarCollapsedHeight` alike, so
    // the *small* row is what stops every two-row collapse.
    for (MdAppBarVariant variant : {MdAppBarVariant::Medium, MdAppBarVariant::Large,
                                    MdAppBarVariant::MediumFlexible,
                                    MdAppBarVariant::LargeFlexible}) {
        const MdAppBarTokens tokens = MdAppBarTokens::resolve(variant);
        QCOMPARE(tokens.collapsedRowHeight, 64.0);
        // Fully collapsed the bar is exactly the icon row.
        QCOMPARE(tokens.heightFor(1.0, false), 64.0);
        // Fully expanded it is the published container height.
        QCOMPARE(tokens.heightFor(0.0, false), tokens.containerHeight);
    }

    // A single-row bar has no row to keep: the whole height is collapsible.
    const MdAppBarTokens small = MdAppBarTokens::resolve(MdAppBarVariant::Small);
    QCOMPARE(small.heightFor(0.0, false), 64.0);
    QCOMPARE(small.heightFor(0.5, false), 32.0);
    QCOMPARE(small.heightFor(1.0, false), 0.0);

    // Medium loses 48, large loses 88; a flexible large loses 32 without a
    // subtitle and 88 with one.
    QCOMPARE(MdAppBarTokens::resolve(MdAppBarVariant::Medium).heightFor(1.0, false), 64.0);
    QCOMPARE(MdAppBarTokens::resolve(MdAppBarVariant::Large).heightFor(1.0, false), 64.0);
    QCOMPARE(MdAppBarTokens::resolve(MdAppBarVariant::LargeFlexible).heightFor(0.5, false), 92.0);
    QCOMPARE(MdAppBarTokens::resolve(MdAppBarVariant::LargeFlexible).heightFor(0.5, true), 108.0);
}

void TestMd3AppBar::titleInsetIsTheEdgeDistance()
{
    // Compose: `TopAppBarTitleInset = 16.dp - TopAppBarHorizontalPadding`, and
    // the horizontal padding *is* the published `leading-space`. The 16 px is
    // the spec's "16dp from the edge": 4 px of container padding plus the 12 px
    // an icon button brings itself.
    const MdAppBarTokens tokens = MdAppBarTokens::resolve(MdAppBarVariant::Small);
    QCOMPARE(tokens.titleInset(), 12.0);
    QCOMPARE(tokens.leadingSpace + tokens.titleInset(), tokens.edgeSpace);
}

void TestMd3AppBar::tokenOverridesReachTheLayout()
{
    MdComponentTokens &bag = MdComponentTokens::global();
    bag.setValue(QStringLiteral("md.comp.app-bar.small.container.height"), QStringLiteral("72px"));
    // The published row, with no intermediate group segment: `resolve()` reads
    // `md.comp.app-bar.leading-space`, not `...app-bar.container.leading-space`.
    bag.setValue(QStringLiteral("md.comp.app-bar.leading-space"), QStringLiteral("8"));

    const MdAppBarTokens tokens = MdAppBarTokens::resolve(MdAppBarVariant::Small);
    QCOMPARE(tokens.containerHeight, 72.0);
    QCOMPARE(tokens.leadingSpace, 8.0);
    // Retuning the starting space must not silently move the edge distance the
    // spec pins; the inset absorbs the difference.
    QCOMPARE(tokens.titleInset(), 8.0);
    QCOMPARE(tokens.leadingSpace + tokens.titleInset(), tokens.edgeSpace);

    // The collapsed row follows the *small* row, so overriding it moves where
    // a medium bar stops collapsing.
    const MdAppBarTokens medium = MdAppBarTokens::resolve(MdAppBarVariant::Medium);
    QCOMPARE(medium.collapsedRowHeight, 72.0);
    QCOMPARE(medium.heightFor(1.0, false), 72.0);

    // A typo falls back to the published value instead of being coerced.
    MdComponentTokens::global().setValue(
        QStringLiteral("md.comp.app-bar.container.shape"), QStringLiteral("corner-banana"));
    QCOMPARE(int(MdAppBarTokens::resolve(MdAppBarVariant::Small).containerShape),
             int(ShapeCorner::None));
    MdComponentTokens::global().remove(QStringLiteral("md.comp.app-bar.container.shape"));
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------

void TestMd3AppBar::twoRowGeometry()
{
    m_bar->setVariant(MdAppBarVariant::Medium);
    m_bar->setSubtitle(QString());
    m_bar->resize(360, 112);

    const MdTopAppBarStyle::Layout layout =
        MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0);

    QVERIFY(layout.twoRows);
    QCOMPARE(layout.container, QRectF(0, 0, 360, 112));
    QCOMPARE(layout.leadingRow, QRectF(0, 0, 360, 64));
    QCOMPARE(layout.textRow, QRectF(0, 64, 360, 48));
    QCOMPARE(layout.titleAlpha, 1.0);
    QCOMPARE(layout.leadingTitleAlpha, 0.0);

    // Fully collapsed: the icon row survives, the text row is gone, and the
    // two alphas have swapped ends.
    const MdTopAppBarStyle::Layout collapsed =
        MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 1.0);
    QCOMPARE(collapsed.leadingRow, QRectF(0, 0, 360, 64));
    QCOMPARE(collapsed.titleAlpha, 0.0);
    QCOMPARE(collapsed.leadingTitleAlpha, 1.0);

    // Half way the expanded title is half transparent and the small one is
    // part way in on `TopTitleAlphaEasing` = cubic-bezier(.8, 0, .8, .15),
    // whose value at 0.5 is far below the linear 0.5.
    const MdTopAppBarStyle::Layout half =
        MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.5);
    QCOMPARE(half.titleAlpha, 0.5);
    QVERIFY(half.leadingTitleAlpha > 0.0);
    QVERIFY(half.leadingTitleAlpha < 0.2);
}

void TestMd3AppBar::navigationPushesTheTitlePastItself()
{
    m_bar->resize(360, 64);
    m_bar->setTitle(QStringLiteral("M"));
    m_bar->addActionWidget(m_action);

    // Without a leading element the title sits at `titleInset + 4` px — the
    // spec's 16 dp, reached the Compose way — and there is no nav container at
    // all rather than an empty one parked at the row's centre.
    MdTopAppBarStyle::Layout layout = MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0);
    QVERIFY(layout.navigation.isEmpty());
    QCOMPARE(layout.title.left(), 16.0);

    // The trailing band is inset by the same 4 px on the far edge, and holds
    // the 48 px probe centred in the 64 px row.
    QCOMPARE(layout.actions, QRectF(308, 0, 48, 64));
    QCOMPARE(layout.actionBoxes.size(), 1);
    QCOMPARE(layout.actionBoxes.at(0), QRectF(308, 8, 48, 48));

    // A 48 px leading button pushes the title's start to 4 + 48 + 4 = 56.
    m_bar->setNavigationWidget(m_navigation);
    layout = MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0);
    QCOMPARE(layout.navigation, QRectF(4, 8, 48, 48));
    QCOMPARE(layout.title.left(), 56.0);
}

void TestMd3AppBar::containersArePlacedNotWidgets()
{
    // The distinction the whole family turns on. `MdIconButton` is a 55 x 55
    // widget wrapped around a 40 x 40 container — 7.5 px of focus-ring margin
    // on every side — so a layout that drops the *widget* on the 4 px token
    // edge draws the container at 11.5, and one that measures the widget when
    // deciding where the title starts pushes the title out to 4 + 55 + 4.
    // `MdButtonGroup` established the rule (`docs/porting-todo.md`): place the
    // container, not the widget.
    m_bar->resize(360, 64);
    m_bar->setTitle(QStringLiteral("M"));

    auto *nav = new MdIconButton(QStringLiteral("menu"), m_bar);
    m_bar->setNavigationWidget(nav);
    auto *first = new MdIconButton(QStringLiteral("favorite"), m_bar);
    auto *second = new MdIconButton(QStringLiteral("more_vert"), m_bar);
    m_bar->addActionWidget(first);
    m_bar->addActionWidget(second);

    // The margin is real: the container is strictly inside the widget.
    QCOMPARE(nav->sizeHint(), QSize(55, 55));
    QCOMPARE(nav->containerRect().size(), QSizeF(40, 40));

    const MdTopAppBarStyle::Layout layout =
        MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0);

    // The nav container sits on the token edge (4 px in, centred in the 64 px
    // row), so the widget starts 7.5 px *outside* the bar — clipped by it, and
    // transparent there anyway. The half pixel is the floor: a widget position
    // is integral and a 7.5 px margin is not.
    QCOMPARE(layout.navigation, QRectF(4, 12, 40, 40));
    const QRectF navContainer = nav->containerRect().translated(nav->pos());
    QVERIFY(std::abs(navContainer.left() - 4.0) <= 0.5);
    QVERIFY(std::abs(navContainer.top() - 12.0) <= 0.5);

    // The title clears the *container*, not the margin hiding behind it.
    QCOMPARE(layout.title.left(), 48.0);

    // Two action containers touch — `iconButtonSpace` is 0 — which is the
    // widget-rect overlap the style header records as deliberate rather than
    // overlooked.
    QCOMPARE(layout.actionBoxes.size(), 2);
    QCOMPARE(layout.actionBoxes.at(0), QRectF(276, 12, 40, 40));
    QCOMPARE(layout.actionBoxes.at(1), QRectF(316, 12, 40, 40));
    QVERIFY(std::abs(first->containerRect().translated(first->pos()).left() - 276.0) <= 0.5);
    QVERIFY(std::abs(second->containerRect().translated(second->pos()).left() - 316.0) <= 0.5);
    // ...and the widgets themselves overlap by `2 * margin` because of it.
    QVERIFY(first->geometry().right() > second->geometry().left());
}

void TestMd3AppBar::centredTitleStaysCentredOnTheBar()
{
    m_bar->resize(360, 64);
    m_bar->setTitle(QStringLiteral("M"));
    m_bar->setAlignment(MdAppBarAlignment::Center);
    m_bar->setNavigationWidget(m_navigation);

    const MdTopAppBarStyle::Layout layout =
        MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0);
    const qreal centre = layout.title.left() + layout.title.width() / 2.0;
    // Centred on the *bar*, not in the gap between its neighbours: Compose
    // aligns against `constraints.maxWidth` first and only then pushes the
    // title past the leading element.
    QCOMPARE(centre, 180.0);
    // ...and the push respects the 56 px floor the leading button sets.
    QVERIFY(layout.title.left() >= 56.0);
}

void TestMd3AppBar::alignmentMovesTheTitle()
{
    m_bar->resize(360, 64);
    m_bar->setTitle(QStringLiteral("M"));

    const qreal leading = MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0).title.left();
    m_bar->setAlignment(MdAppBarAlignment::Center);
    const qreal centred = MdTopAppBarStyle::layoutFor(*m_bar, m_bar->tokens(), 0.0).title.left();

    QCOMPARE(leading, 16.0);
    QVERIFY(centred > leading);
}

// ---------------------------------------------------------------------------
// Colour
// ---------------------------------------------------------------------------

void TestMd3AppBar::colourTransitionTable()
{
    const MdAppBarTokens small = MdAppBarTokens::resolve(MdAppBarVariant::Small);
    const MdAppBarTokens medium = MdAppBarTokens::resolve(MdAppBarVariant::Medium);

    // A single-row bar has no height to collapse, so only the *overlapped*
    // fraction moves its colour, and it steps at Compose's `> 0.01` threshold.
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(small, 0.0, 0.0), 0.0);
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(small, 0.0, 0.005), 0.0);
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(small, 0.0, 0.02), 1.0);

    // A two-row bar reads its collapsed fraction directly — "the bottom part
    // of this TwoRowsTopAppBar changes color at the same rate the app bar
    // expands or collapse".
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(medium, 0.0, 1.0), 0.0);
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(medium, 0.25, 0.0), 0.25);
    QCOMPARE(MdTopAppBarStyle::colorTransitionFraction(medium, 1.0, 0.0), 1.0);

    // The endpoints are the two published roles exactly.
    const QColor base = MdTheme::instance().color(ColorRole::Surface);
    const QColor scrolled = MdTheme::instance().color(ColorRole::SurfaceContainer);
    QCOMPARE(MdTopAppBarStyle::containerColorFor(small, 0.0), base);
    QCOMPARE(MdTopAppBarStyle::containerColorFor(small, 1.0), scrolled);
    QCOMPARE(MdTopAppBarStyle::containerColorFor(medium, 0.0), base);
    QCOMPARE(MdTopAppBarStyle::containerColorFor(medium, 1.0), scrolled);

    // The midpoint is not the naive average: Compose eases the fraction on
    // `FastOutLinearInEasing` and interpolates the colour in Oklab.
    // `cubic-bezier(0.4, 0, 1, 1)` is *below* linear across the whole first
    // half — it accelerates *out* of the start, so at x = 0.5 it yields
    // 0.32481, not something above the diagonal. Asserting `> 0.5` here would
    // have encoded the opposite curve.
    const qreal eased = MdTopAppBarStyle::fastOutLinearIn(0.5);
    QVERIFY(eased < 0.5);
    QCOMPARE(MdTopAppBarStyle::fastOutLinearIn(0.0), 0.0);
    QCOMPARE(MdTopAppBarStyle::fastOutLinearIn(1.0), 1.0);
    QCOMPARE(MdTopAppBarStyle::containerColorFor(medium, 0.5),
             QColor::fromRgba(MdColorMath::lerpOklab(base.rgba(), scrolled.rgba(), eased)));
}

void TestMd3AppBar::oklabLerpEndpoints()
{
    // Both ends are returned untouched, and the midpoint is a real blend
    // rather than a round-trip artefact.
    const quint32 black = 0xff000000u;
    const quint32 white = 0xffffffffu;
    QCOMPARE(MdColorMath::lerpOklab(black, white, 0.0), black);
    QCOMPARE(MdColorMath::lerpOklab(black, white, 1.0), white);

    const quint32 mid = MdColorMath::lerpOklab(black, white, 0.5);
    // Oklab's lightness midpoint for black/white sits at L = 0.5, i.e. a linear
    // luminance of 0.125, which encodes to sRGB ~0.3886 -> 99. The whole point
    // of interpolating there rather than in gamma space is that this lands far
    // below the naive 127/128; the band below is tight on purpose.
    const quint32 midValue = mid & 0xffu;
    QVERIFY2(midValue >= 99u && midValue <= 100u, qPrintable(QString::number(midValue)));
}

// ---------------------------------------------------------------------------
// Scrolling
// ---------------------------------------------------------------------------

void TestMd3AppBar::scrollModesConsumeTheRightWay()
{
    // --- Pinned: the bar never moves, the content takes everything ----------
    m_behavior->setMode(MdAppBarScrollMode::Pinned);
    m_behavior->setHeightOffsetLimit(-64.0);
    QCOMPARE(m_behavior->consumeScroll(-10.0, true), 0.0);
    QCOMPARE(m_behavior->heightOffset(), 0.0);
    QCOMPARE(m_behavior->contentOffset(), -10.0);
    QCOMPARE(m_behavior->collapsedFraction(), 0.0);
    QVERIFY(m_behavior->overlappedFraction() > 0.0);
    QVERIFY(m_behavior->overlappedFraction() < 1.0);

    // Once the content has travelled a full limit the overlap saturates.
    m_behavior->consumeScroll(-100.0, false);
    QCOMPARE(m_behavior->overlappedFraction(), 1.0);

    // --- EnterAlways: the bar takes the whole delta while it can move -------
    m_behavior->setMode(MdAppBarScrollMode::EnterAlways);
    m_behavior->setHeightOffset(0.0);
    m_behavior->setContentOffset(0.0);
    QCOMPARE(m_behavior->consumeScroll(-10.0, true), -10.0);
    QCOMPARE(m_behavior->heightOffset(), -10.0);
    QCOMPARE(m_behavior->contentOffset(), 0.0);

    // Compose answers its pre-scroll hook with `available.copy(x = 0f)` — the
    // whole delta — even though only 54 px of it fitted, so the surplus is not
    // handed back to the content.
    QCOMPARE(m_behavior->consumeScroll(-100.0, true), -100.0);
    QCOMPARE(m_behavior->heightOffset(), -64.0);
    QCOMPARE(m_behavior->contentOffset(), 0.0);

    // Now that the bar is at its limit the content takes the delta.
    QCOMPARE(m_behavior->consumeScroll(-30.0, true), 0.0);
    QCOMPARE(m_behavior->contentOffset(), -30.0);
    QCOMPARE(m_behavior->heightOffset(), -64.0);

    // --- ExitUntilCollapsed: down is immediate, up waits for the start -----
    m_behavior->setMode(MdAppBarScrollMode::ExitUntilCollapsed);
    m_behavior->setHeightOffset(0.0);
    m_behavior->setContentOffset(0.0);
    QCOMPARE(m_behavior->consumeScroll(-20.0, false), -20.0);
    QCOMPARE(m_behavior->heightOffset(), -20.0);

    // Expanding while the content is *not* at its start never reaches the bar.
    QCOMPARE(m_behavior->consumeScroll(20.0, false), 0.0);
    QCOMPARE(m_behavior->heightOffset(), -20.0);
    QCOMPARE(m_behavior->contentOffset(), 20.0);

    // At the start it does, and here the hook returns the real travel.
    QCOMPARE(m_behavior->consumeScroll(10.0, true), 10.0);
    QCOMPARE(m_behavior->heightOffset(), -10.0);
    QCOMPARE(m_behavior->contentOffset(), 20.0);

    // A vetoed behaviour ignores everything.
    m_behavior->setCanScroll(false);
    QCOMPARE(m_behavior->consumeScroll(-50.0, true), 0.0);
    QCOMPARE(m_behavior->heightOffset(), -10.0);
    m_behavior->setCanScroll(true);
}

void TestMd3AppBar::settleSnapsToTheNearerEnd()
{
    m_behavior->setMode(MdAppBarScrollMode::EnterAlways);
    m_behavior->setHeightOffsetLimit(-64.0);

    // Less than half collapsed: the bar goes back to fully expanded.
    m_behavior->setHeightOffset(-20.0);
    QVERIFY(!m_behavior->isSettled());
    m_behavior->snapNow();
    QCOMPARE(m_behavior->heightOffset(), 0.0);
    QVERIFY(m_behavior->isSettled());

    // More than half: it finishes the collapse.
    m_behavior->setHeightOffset(-50.0);
    m_behavior->snapNow();
    QCOMPARE(m_behavior->heightOffset(), -64.0);
    QCOMPARE(m_behavior->collapsedFraction(), 1.0);

    // `settle()` runs the same decision on the DefaultEffects spring, so it
    // has to reach the same place.
    m_behavior->setHeightOffset(-40.0);
    m_behavior->settle();
    QVERIFY(m_behavior->isSettling());
    QTRY_VERIFY_WITH_TIMEOUT(!m_behavior->isSettling(), 5000);
    QCOMPARE(m_behavior->heightOffset(), -64.0);
}

void TestMd3AppBar::followScrollBarMirrorsTheOffset()
{
    m_behavior->setMode(MdAppBarScrollMode::Pinned);
    m_behavior->setHeightOffsetLimit(-64.0);

    QScrollBar bar;
    bar.setRange(0, 400);
    bar.setPageStep(200);
    m_behavior->followScrollBar(&bar);

    QVERIFY(m_behavior->isScrollingContentAtStart());
    QCOMPARE(m_behavior->contentOffset(), 0.0);

    bar.setValue(30);
    // The scroll position *is* the offset, with Compose's sign: a scroll bar's
    // value grows as the content moves up.
    QCOMPARE(m_behavior->contentOffset(), -30.0);
    QVERIFY(!m_behavior->isScrollingContentAtStart());
    QVERIFY(m_behavior->overlappedFraction() > 0.0);

    // Back at the top the special case in `overlappedFraction` applies.
    bar.setValue(0);
    QVERIFY(m_behavior->isScrollingContentAtStart());
    QCOMPARE(m_behavior->overlappedFraction(), 0.0);
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void TestMd3AppBar::topAppBarPaintsItsContainer()
{
    m_bar->resize(360, 64);
    const QColor container = MdTheme::instance().color(ColorRole::Surface);
    QCOMPARE(pixelColorAt(*m_bar, QPointF(180, 32)), container);
    QCOMPARE(pixelColorAt(*m_bar, QPointF(2, 60)), container);

    // Nothing is painted outside the container: `container.shape` is
    // corner-none, so the top-left pixel is the container's own.
    QCOMPARE(pixelColorAt(*m_bar, QPointF(0, 0)), container);
}

void TestMd3AppBar::topAppBarFollowsTheThemeToScrolled()
{
    m_bar->resize(360, 64);
    m_behavior->setMode(MdAppBarScrollMode::Pinned);
    // The behaviour writes `heightOffsetLimit` from the bar, but a pinned bar
    // has no collapsible row of its own here, so give it one to keep
    // `overlappedFraction` meaningful.
    m_bar->setScrollBehavior(m_behavior);

    QCOMPARE(m_bar->containerColor(), MdTheme::instance().color(ColorRole::Surface));

    // Push the content well past the bar's limit: the target steps to 1, and
    // the widget rides it on the DefaultEffects spring.
    m_behavior->setContentOffset(-500.0);
    QCOMPARE(m_behavior->overlappedFraction(), 1.0);
    QTRY_COMPARE_WITH_TIMEOUT(m_bar->containerColor(),
                              MdTheme::instance().color(ColorRole::SurfaceContainer), 5000);

    // ...and back.
    m_behavior->setContentOffset(0.0);
    m_behavior->setScrollingContentAtStart(true);
    QTRY_COMPARE_WITH_TIMEOUT(m_bar->containerColor(),
                              MdTheme::instance().color(ColorRole::Surface), 5000);
}

// ---------------------------------------------------------------------------
// Bottom app bar
// ---------------------------------------------------------------------------

void TestMd3AppBar::bottomAppBarTokenTable()
{
    const MdBottomAppBarTokens tokens = MdBottomAppBarTokens::resolve();

    QCOMPARE(tokens.containerHeight, 80.0);
    QCOMPARE(tokens.withFabContainerHeight, 72.0);
    QCOMPARE(int(tokens.containerColor), int(ColorRole::SurfaceContainer));
    QCOMPARE(int(tokens.containerElevation), int(ElevationLevel::Level2));
    QCOMPARE(int(tokens.containerShape), int(ShapeCorner::None));
    QCOMPARE(int(tokens.containerSurfaceTintLayerColor), int(ColorRole::SurfaceTint));

    // `PaddingValues(start = 4, top = 4, end = 4)` — and no bottom, which is
    // why the content band ends at the container's own bottom edge.
    QCOMPARE(tokens.contentLeadingSpace, 4.0);
    QCOMPARE(tokens.contentTopSpace, 4.0);
    QCOMPARE(tokens.contentTrailingSpace, 4.0);
    QCOMPARE(tokens.fabLeadingSpace, 12.0);
    QCOMPARE(tokens.fabTopSpace, 8.0);
    QCOMPARE(tokens.edgeSpace, 16.0);

    // The FAB's own padding plus the container's is what puts it 16 px from the
    // trailing edge and 12 px from the top.
    QCOMPARE(tokens.contentTrailingSpace + tokens.fabLeadingSpace, tokens.edgeSpace);
    QCOMPARE(tokens.contentTopSpace + tokens.fabTopSpace, 12.0);
}

void TestMd3AppBar::bottomAppBarLayoutAndFab()
{
    m_bottom->resize(360, 80);

    MdBottomAppBarStyle::Layout layout = MdBottomAppBarStyle::layoutFor(*m_bottom,
                                                                       m_bottom->tokens());
    QCOMPARE(layout.container, QRectF(0, 0, 360, 80));
    // No FAB: the children own the whole band.
    QCOMPARE(layout.actions, layout.content);
    QCOMPARE(layout.content, QRectF(4, 4, 352, 76));
    QVERIFY(layout.fabBox.isEmpty());

    // With a FAB the actions band stops where the FAB's strip begins, and the
    // FAB's *container* lands 12 px from the top and 16 px from the trailing
    // edge. `fabBox` is the container's target rect, not a full-height strip.
    m_bottom->setFloatingActionButton(m_fab);
    layout = MdBottomAppBarStyle::layoutFor(*m_bottom, m_bottom->tokens());
    QCOMPARE(layout.fabBox, QRectF(288, 12, 56, 56));
    QCOMPARE(layout.fabBox.right(), 344.0);
    QCOMPARE(360.0 - layout.fabBox.right(), 16.0);
    QCOMPARE(layout.fabBox.top(), 12.0);
    QVERIFY(layout.actions.right() <= layout.fabBox.left());

    const QColor container = MdTheme::instance().color(ColorRole::SurfaceContainer);
    QCOMPARE(pixelColorAt(*m_bottom, QPointF(180, 40)), container);
    QCOMPARE(pixelColorAt(*m_bottom, QPointF(0, 0)), container);

    // And the arrangement moves the row's own children only. The box it
    // reports is the child's *container* box — here a 48 px probe, whose
    // container is its whole rect — centred in the band the FAB leaves it.
    m_bottom->addWidget(m_action);
    m_bottom->setArrangement(MdBottomAppBar::Arrangement::End);
    const MdBottomAppBarStyle::Layout endLayout =
        MdBottomAppBarStyle::layoutFor(*m_bottom, m_bottom->tokens());
    QVERIFY(endLayout.actions.right() <= endLayout.fabBox.left());
    QCOMPARE(endLayout.childBoxes.size(), 1);
    QCOMPARE(endLayout.childBoxes.at(0), QRectF(240, 18, 48, 48));

    // A *real* MdFab, which is the case the fixed-size probe above cannot
    // reach. MdFab reserves the focus ring's room on every side and so reports
    // a sizeHint of `container + 2 * margin`: the default medium FAB is a
    // 71 x 71 widget wrapped around a 56 x 56 container at (7.5, 7.5). The
    // layout therefore has to place the container rather than the widget;
    // placing the widget on the box parks the painted disc 7.5 px inside the
    // token position, which is what the first revision did and what the
    // gallery page's pixel audit caught. Parented to the bar so it dies with it
    // in cleanup(); `setFloatingActionButton` runs the placement itself, so the
    // result is observable without showing anything.
    auto *realFab = new MdFab(QStringLiteral("add"), m_bottom);
    m_bottom->setFloatingActionButton(realFab);
    layout = MdBottomAppBarStyle::layoutFor(*m_bottom, m_bottom->tokens());

    const QRectF containerInWidget = MdChildBox::measure(realFab).container();
    QCOMPARE(containerInWidget.size(), QSizeF(56, 56));
    // The margin is real: the container does not fill the widget.
    QVERIFY(containerInWidget.left() > 0.0);
    QCOMPARE(realFab->size(), realFab->sizeHint());

    // The widget is backed off by exactly that margin...
    QCOMPARE(realFab->pos().x(), int(std::lround(layout.fabBox.left() - containerInWidget.left())));
    QCOMPARE(realFab->pos().y(), int(std::lround(layout.fabBox.top() - containerInWidget.top())));
    // ...so the *container* is what lands on the token position: 16 px from the
    // trailing edge and 12 px from the top. Half a pixel of slack is the floor
    // here — the widget's position is integral and the offset is not.
    const QRectF placed = containerInWidget.translated(realFab->pos());
    QCOMPARE(placed.size(), QSizeF(56, 56));
    QVERIFY(std::abs((360.0 - placed.right()) - 16.0) <= 0.5);
    QVERIFY(std::abs(placed.top() - 12.0) <= 0.5);
}

QTEST_MAIN(TestMd3AppBar)
#include "TestMd3AppBar.moc"
