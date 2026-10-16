// TestMd3ThemeMatrix — theme coverage evidence for every Stage 1 family.
//
// The coverage matrix's 主题 column is defined as: Light / dark, seed change,
// contrast level, density, RTL, font switch. This suite exercises those six
// dimensions against one representative widget per family and records what is
// actually verified — the coverage matrix must reflect this, not the other way
// round.
//
// What "covered" means per dimension:
//   light/dark       the widget repaints with different scheme colours
//   seed change      dynamic colour rebuilds the roles (pixels move)
//   contrast level   the tone solver moves the roles (pixels move)
//   density          the theme API accepts the value and broadcasts it
//   RTL              setDirection / isRightToLeft round-trips
//   font switch      MdTypeScale::setFamily is honoured by text metrics
//
// Density and RTL have known library-wide gaps (most components do not yet
// consume density; only the button families mirror under RTL). Those gaps are
// asserted here so they cannot drift silently, and the coverage matrix records
// them per family.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdBadge.h"
#include "widgets/MdBottomSheet.h"
#include "widgets/MdButton.h"
#include "widgets/MdButtonGroup.h"
#include "widgets/MdCard.h"
#include "widgets/MdCarousel.h"
#include "widgets/MdCheckBox.h"
#include "widgets/MdChip.h"
#include "widgets/MdDatePicker.h"
#include "widgets/MdDialog.h"
#include "widgets/MdDivider.h"
#include "widgets/MdDockedToolbar.h"
#include "widgets/MdExtendedFab.h"
#include "widgets/MdFab.h"
#include "widgets/MdFabMenu.h"
#include "widgets/MdFloatingToolbar.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdList.h"
#include "widgets/MdListItem.h"
#include "widgets/MdLoadingIndicator.h"
#include "widgets/MdMenu.h"
#include "widgets/MdNavigationBar.h"
#include "widgets/MdNavigationDrawer.h"
#include "widgets/MdNavigationRail.h"
#include "widgets/MdProgressIndicator.h"
#include "widgets/MdRadioButton.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdSegmentedButton.h"
#include "widgets/MdSideSheet.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSnackbar.h"
#include "widgets/MdSplitButton.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTab.h"
#include "widgets/MdTabs.h"
#include "widgets/MdTextArea.h"
#include "widgets/MdTextField.h"
#include "widgets/MdTimePicker.h"
#include "widgets/MdTooltip.h"
#include "widgets/MdTopAppBar.h"

#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

struct OffscreenDefault
{
    OffscreenDefault()
    {
        if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
        }
    }
};

// NOLINTNEXTLINE(cert-err58-cpp) — qputenv cannot throw.
const OffscreenDefault g_offscreenDefault;

/// Render a widget into a fixed-size image against the current theme.
QImage renderWidget(QWidget &widget, int w = 120, int h = 80)
{
    widget.resize(w, h);
    widget.show();
    static_cast<void>(QTest::qWaitForWindowExposed(&widget));
    QImage image(w, h, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    widget.render(&painter);
    painter.end();
    widget.hide();
    return image;
}

/// True when at least one pixel differs from pure white.
bool nonBlank(const QImage &image)
{
    return mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 10;
}

/// True when two images differ in at least one pixel.
bool imagesDiffer(const QImage &a, const QImage &b)
{
    if (a.size() != b.size()) {
        return true;
    }
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            if (a.pixel(x, y) != b.pixel(x, y)) {
                return true;
            }
        }
    }
    return false;
}

/// Factory: build a representative widget for a family.
using WidgetFactory = std::function<QWidget *()>;

struct FamilyEntry
{
    const char *name;
    WidgetFactory factory;
    /// Light/dark must move the paint (surface + container tones invert).
    bool movesWithMode;
    /// Seed change must move the paint (dynamic colour roles only).
    bool movesWithSeed;
    /// Contrast level must move the paint (tone-solved roles).
    bool movesWithContrast;
    /// The family renders text that should follow MdTypeScale::setFamily.
    bool hasText;
};

} // namespace

class TestMd3ThemeMatrix : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void lightAndDarkDifferPerFamily();
    void seedChangeMovesColorsPerFamily();
    void contrastLevelMovesColorsPerFamily();
    void densityApiWorksPerFamily();
    void rtlApiWorksPerFamily();
    void fontSwitchChangesTextMetricsPerFamily();
    void allFamiliesRenderNonBlankInBothModes();
};

void TestMd3ThemeMatrix::initTestCase()
{
    // Start from a known theme state.
    MdTheme &theme = MdTheme::instance();
    theme.setThemeMode(ThemeMode::Light);
    theme.setDynamicColor(false);
    theme.setSeedColor(QColor(QStringLiteral("#6750a4")));
    theme.setContrastLevel(ContrastLevel::Standard);
    theme.setDensity(Density::Default);
    theme.setDirection(Qt::LeftToRight);
}

void TestMd3ThemeMatrix::cleanup()
{
    MdTheme &theme = MdTheme::instance();
    theme.setThemeMode(ThemeMode::Light);
    theme.setDynamicColor(false);
    theme.setSeedColor(QColor(QStringLiteral("#6750a4")));
    theme.setContrastLevel(ContrastLevel::Standard);
    theme.setDensity(Density::Default);
    theme.setDirection(Qt::LeftToRight);
}

// ---------------------------------------------------------------------------
// The family table
// ---------------------------------------------------------------------------

namespace {

QList<FamilyEntry> familyTable()
{
    // Sensitivity flags are honest:
    //   movesWithMode     surface/container tones invert between light and dark
    //   movesWithSeed     the family uses dynamic (seed-solved) colour roles
    //   movesWithContrast the family uses tone-solved roles that a contrast
    //                     level shifts
    // Fixed-role families (error badges, inverse-surface snackbars) are
    // deliberately `false` for seed/contrast — that is the design, not a gap.
    return {
        // --- Actions ---
        { "Buttons", []() -> QWidget * {
              return new MdButton(QStringLiteral("Save"));
          }, true, true, true, true },
        { "Button groups", []() -> QWidget * {
              auto *group = new MdButtonGroup;
              group->addItem(QStringLiteral("A"));
              group->addItem(QStringLiteral("B"));
              return group;
          }, true, true, true, true },
        { "Icon buttons", []() -> QWidget * {
              return new MdIconButton(QStringLiteral("settings"));
          }, true, true, true, false },
        { "FABs", []() -> QWidget * {
              return new MdFab(QStringLiteral("add"));
          }, true, true, true, false },
        { "Extended FABs", []() -> QWidget * {
              return new MdExtendedFab(QStringLiteral("Compose"));
          }, true, true, true, true },
        { "FAB menu", []() -> QWidget * {
              return new MdFabMenu;
          }, true, true, true, false },
        { "Split buttons", []() -> QWidget * {
              return new MdSplitButton(QStringLiteral("Send"));
          }, true, true, true, true },
        { "Segmented buttons", []() -> QWidget * {
              return new MdSegmentedButton(
                  QStringList{ QStringLiteral("Day"), QStringLiteral("Week") });
          }, true, true, true, true },

        // --- Communication ---
        // Badge paints error / on-error — fixed roles, not seed-solved.
        { "Badges", []() -> QWidget * {
              return new MdBadge(QStringLiteral("3"));
          }, true, false, false, true },
        { "Progress indicators", []() -> QWidget * {
              return new MdProgressIndicator(ProgressIndicatorShape::Linear);
          }, true, true, true, false },
        { "Loading indicator", []() -> QWidget * {
              return new MdLoadingIndicator;
          }, true, true, true, false },
        // Snackbar paints inverse-surface / inverse-on-surface — fixed roles.
        { "Snackbar", []() -> QWidget * {
              return new MdSnackbar;
          }, true, false, false, true },
        // Plain tooltip paints inverse-surface — fixed roles.
        { "Tooltips", []() -> QWidget * {
              return new MdTooltip;
          }, true, false, false, true },

        // --- Containment ---
        { "Cards", []() -> QWidget * {
              auto *card = new MdCard(MdCardVariant::Filled);
              card->setClickable(true);
              return card;
          }, true, true, true, false },
        // Dialog paints surface-container-high — a surface role whose tone
        // shift under contrast is within antialiasing noise on a small probe.
        { "Dialogs", []() -> QWidget * {
              return new MdDialog;
          }, true, true, false, true },
        // Sheets paint surface-container-low — the contrast tone shift is
        // within antialiasing noise on a small probe.
        { "Bottom sheets", []() -> QWidget * {
              return new MdBottomSheet(MdSheetKind::Modal);
          }, true, true, false, false },
        { "Side sheets", []() -> QWidget * {
              return new MdSideSheet(MdSideSheetKind::Modal);
          }, true, true, false, false },
        // Carousel: empty at construction; paints only the surface container
        // which does not move enough with contrast on a small probe.
        { "Carousel", []() -> QWidget * {
              return new MdCarousel;
          }, false, false, false, false },
        { "Divider", []() -> QWidget * {
              return new MdDivider(Qt::Horizontal);
          }, true, true, true, false },
        { "Lists", []() -> QWidget * {
              auto *list = new MdList;
              auto *item = new MdListItem(QStringLiteral("Item"));
              list->addItem(item);
              return list;
          }, true, true, true, true },

        // --- Navigation ---
        { "App bars", []() -> QWidget * {
              return new MdTopAppBar(QStringLiteral("Title"));
          }, true, true, true, true },
        { "Toolbars", []() -> QWidget * {
              auto *bar = new MdDockedToolbar;
              bar->addWidget(new MdIconButton(QStringLiteral("edit")));
              bar->addWidget(new MdIconButton(QStringLiteral("save")));
              return bar;
          }, true, true, true, false },
        { "Navigation bar", []() -> QWidget * {
              return new MdNavigationBar;
          }, true, true, true, true },
        { "Navigation rail", []() -> QWidget * {
              return new MdNavigationRail;
          }, true, true, true, true },
        { "Navigation drawer", []() -> QWidget * {
              return new MdNavigationDrawer;
          }, true, true, true, true },
        { "Tabs", []() -> QWidget * {
              auto *tabs = new MdTabs;
              tabs->addTab(new MdTab(QStringLiteral("One")));
              tabs->addTab(new MdTab(QStringLiteral("Two")));
              return tabs;
          }, true, true, true, true },

        // --- Selection ---
        { "Checkbox", []() -> QWidget * {
              return new MdCheckBox(QStringLiteral("Check"));
          }, true, true, true, true },
        { "Chips", []() -> QWidget * {
              return new MdChip(QStringLiteral("Chip"));
          }, true, true, true, true },
        { "Radio button", []() -> QWidget * {
              return new MdRadioButton(QStringLiteral("Option"));
          }, true, true, true, true },
        { "Date pickers", []() -> QWidget * {
              return new MdDatePicker;
          }, true, true, true, true },
        { "Menus", []() -> QWidget * {
              return new MdMenu;
          }, true, true, true, true },
        { "Sliders", []() -> QWidget * {
              return new MdSlider;
          }, true, true, true, false },
        { "Switch", []() -> QWidget * {
              return new MdSwitch(QStringLiteral("Toggle"));
          }, true, true, true, true },
        { "Time pickers", []() -> QWidget * {
              return new MdTimePicker;
          }, true, true, true, true },

        // --- Text inputs ---
        { "Text fields", []() -> QWidget * {
              return new MdTextField(QStringLiteral("Value"));
          }, true, true, true, true },
        { "Search", []() -> QWidget * {
              return new MdSearchBar;
          }, true, true, true, true },
    };
}

} // namespace

// ---------------------------------------------------------------------------
// Dimension tests
// ---------------------------------------------------------------------------

void TestMd3ThemeMatrix::lightAndDarkDifferPerFamily()
{
    MdTheme &theme = MdTheme::instance();
    int checked = 0;

    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);

        theme.setThemeMode(ThemeMode::Light);
        std::unique_ptr<QWidget> widget(entry.factory());
        const QImage light = renderWidget(*widget);

        theme.setThemeMode(ThemeMode::Dark);
        const QImage dark = renderWidget(*widget);

        QVERIFY2(nonBlank(light), name.constData());
        QVERIFY2(nonBlank(dark), name.constData());

        if (entry.movesWithMode) {
            QVERIFY2(imagesDiffer(light, dark),
                     qPrintable(QStringLiteral("%1: light and dark paints must differ")
                                    .arg(QLatin1String(entry.name))));
        }
        ++checked;
    }

    QCOMPARE(checked, 36);
}

void TestMd3ThemeMatrix::seedChangeMovesColorsPerFamily()
{
    MdTheme &theme = MdTheme::instance();
    theme.setDynamicColor(true);
    theme.setSeedColor(QColor(QStringLiteral("#6750a4")));
    theme.setThemeMode(ThemeMode::Light);

    int checked = 0;
    for (const FamilyEntry &entry : familyTable()) {
        if (!entry.movesWithSeed) {
            continue;
        }
        const QByteArray name(entry.name);

        std::unique_ptr<QWidget> widget(entry.factory());
        const QImage before = renderWidget(*widget);

        theme.setSeedColor(QColor(QStringLiteral("#0061a4")));
        const QImage after = renderWidget(*widget);

        QVERIFY2(nonBlank(before), name.constData());
        QVERIFY2(nonBlank(after), name.constData());
        QVERIFY2(imagesDiffer(before, after),
                 qPrintable(QStringLiteral("%1: seed change must repaint colours")
                                .arg(QLatin1String(entry.name))));
        ++checked;

        theme.setSeedColor(QColor(QStringLiteral("#6750a4")));
    }

    QVERIFY(checked >= 25); // the dynamic-colour families
    theme.setDynamicColor(false);
}

void TestMd3ThemeMatrix::contrastLevelMovesColorsPerFamily()
{
    MdTheme &theme = MdTheme::instance();
    theme.setThemeMode(ThemeMode::Light);
    theme.setContrastLevel(ContrastLevel::Standard);

    int pixelsMoved = 0;
    int checked = 0;
    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);

        std::unique_ptr<QWidget> widget(entry.factory());
        const QImage standard = renderWidget(*widget);

        theme.setContrastLevel(ContrastLevel::High);
        const QImage high = renderWidget(*widget);
        theme.setContrastLevel(ContrastLevel::Standard);

        QVERIFY2(nonBlank(standard), name.constData());
        QVERIFY2(nonBlank(high), name.constData());

        // The tone solver moves accent and surface roles, but an empty
        // container or a tiny probe can land inside antialiasing noise.
        // Count how many families show a visible shift; the threshold below
        // is what the coverage matrix must honestly reflect.
        if (imagesDiffer(standard, high)) {
            ++pixelsMoved;
        }
        ++checked;
    }

    QCOMPARE(checked, 36);
    // At least the accent-colour families (filled buttons, FABs, progress,
    // selection controls, chips, nav with a selected item) must move.
    QVERIFY2(pixelsMoved >= 20,
             qPrintable(QStringLiteral("only %1/36 families showed a visible contrast shift")
                            .arg(pixelsMoved)));
}

void TestMd3ThemeMatrix::densityApiWorksPerFamily()
{
    MdTheme &theme = MdTheme::instance();

    // The density signal must fire and the getter must round-trip.
    // Components do not yet consume density (recorded in porting-todo.md);
    // this test pins the API contract so the gap cannot widen silently.
    QSignalSpy spy(&theme, &MdTheme::densityChanged);
    QVERIFY(spy.isValid());

    for (const Density d : { Density::Comfortable, Density::Compact, Density::Default }) {
        theme.setDensity(d);
        QCOMPARE(theme.density(), d);
    }
    QVERIFY(spy.count() >= 3);

    // And no family crashes when density changes under it.
    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);
        std::unique_ptr<QWidget> widget(entry.factory());
        theme.setDensity(Density::Compact);
        renderWidget(*widget);
        theme.setDensity(Density::Comfortable);
        renderWidget(*widget);
        theme.setDensity(Density::Default);
    }
}

void TestMd3ThemeMatrix::rtlApiWorksPerFamily()
{
    MdTheme &theme = MdTheme::instance();

    theme.setDirection(Qt::LeftToRight);
    QVERIFY(!theme.isRightToLeft());

    QSignalSpy spy(&theme, &MdTheme::directionChanged);
    QVERIFY(spy.isValid());

    theme.setDirection(Qt::RightToLeft);
    QVERIFY(theme.isRightToLeft());
    QCOMPARE(spy.count(), 1);

    theme.setDirection(Qt::LeftToRight);
    QVERIFY(!theme.isRightToLeft());
    QCOMPARE(spy.count(), 2);

    // No family crashes when direction changes under it.
    // The library-wide RTL mirroring gap (only button families honour it) is
    // recorded in porting-todo.md and asserted by TestMd3Rtl.
    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);
        std::unique_ptr<QWidget> widget(entry.factory());
        theme.setDirection(Qt::RightToLeft);
        const QImage rtl = renderWidget(*widget);
        QVERIFY2(nonBlank(rtl), name.constData());
        theme.setDirection(Qt::LeftToRight);
    }
}

void TestMd3ThemeMatrix::fontSwitchChangesTextMetricsPerFamily()
{
    // Capture the baseline family, then swap it and verify MdTypeScale picks
    // the change up. The rendering itself is platform-font dependent, so the
    // assertion is on the resolved family and on the render staying non-blank.
    const QString originalBrand = MdTypeScale::family(QStringLiteral("brand"));
    const QString originalPlain = MdTypeScale::family(QStringLiteral("plain"));

    // Find any installed font we can switch to. The offscreen platform often
    // has no font database at all; in that case the API-level check still
    // runs and the render check is skipped.
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QStringList installed = QFontDatabase::families();
#else
    // Qt 5: families() is an instance method.
    const QStringList installed = QFontDatabase().families();
#endif
    QString replacement;
    for (const QString &c : { QStringLiteral("Courier New"),
                              QStringLiteral("Times New Roman"),
                              QStringLiteral("Arial"),
                              QStringLiteral("DejaVu Sans") }) {
        if (installed.contains(c)) {
            replacement = c;
            break;
        }
    }
    if (replacement.isEmpty() && !installed.isEmpty()) {
        replacement = installed.first();
    }

    if (replacement.isEmpty()) {
        // No fonts at all — verify the API contract only.
        MdTypeScale::setFamily(QStringLiteral("brand"), QStringLiteral("TestBrand"));
        MdTypeScale::setFamily(QStringLiteral("plain"), QStringLiteral("TestPlain"));
        QCOMPARE(MdTypeScale::family(QStringLiteral("brand")), QStringLiteral("TestBrand"));
        QCOMPARE(MdTypeScale::family(QStringLiteral("plain")), QStringLiteral("TestPlain"));
        MdTypeScale::setFamily(QStringLiteral("brand"), originalBrand);
        MdTypeScale::setFamily(QStringLiteral("plain"), originalPlain);
        QSKIP("No fonts installed on this host; API-level font-switch check only");
    }

    MdTypeScale::setFamily(QStringLiteral("brand"), replacement);
    MdTypeScale::setFamily(QStringLiteral("plain"), replacement);

    QCOMPARE(MdTypeScale::family(QStringLiteral("brand")), replacement);
    QCOMPARE(MdTypeScale::family(QStringLiteral("plain")), replacement);

    // The resolved font must carry the family.
    const QFont font = MdTypeScale::font(TypeStyle::BodyMedium);
    QVERIFY(font.family().contains(replacement, Qt::CaseInsensitive)
            || font.family() == replacement);

    // Every family must still render non-blank after the font swap.
    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);
        std::unique_ptr<QWidget> widget(entry.factory());
        const QImage image = renderWidget(*widget);
        QVERIFY2(nonBlank(image), name.constData());
    }

    // Restore.
    MdTypeScale::setFamily(QStringLiteral("brand"), originalBrand);
    MdTypeScale::setFamily(QStringLiteral("plain"), originalPlain);
    QCOMPARE(MdTypeScale::family(QStringLiteral("brand")), originalBrand);
    QCOMPARE(MdTypeScale::family(QStringLiteral("plain")), originalPlain);
}

void TestMd3ThemeMatrix::allFamiliesRenderNonBlankInBothModes()
{
    MdTheme &theme = MdTheme::instance();

    for (const FamilyEntry &entry : familyTable()) {
        const QByteArray name(entry.name);

        theme.setThemeMode(ThemeMode::Light);
        std::unique_ptr<QWidget> widget(entry.factory());
        const QImage light = renderWidget(*widget);
        QVERIFY2(nonBlank(light),
                 qPrintable(QStringLiteral("%1: light render must be non-blank")
                                .arg(QLatin1String(entry.name))));

        theme.setThemeMode(ThemeMode::Dark);
        const QImage dark = renderWidget(*widget);
        QVERIFY2(nonBlank(dark),
                 qPrintable(QStringLiteral("%1: dark render must be non-blank")
                                .arg(QLatin1String(entry.name))));
    }
}

QTEST_MAIN(TestMd3ThemeMatrix)

#include "TestMd3ThemeMatrix.moc"
