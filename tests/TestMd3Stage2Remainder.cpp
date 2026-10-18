// TestMd3Stage2Remainder — Stage 2 Qt extensions (appendix D remainder).
//
// Pins the remaining Stage 2 widgets: MdInputDialog, MdStackedWidget,
// MdResult, MdBreadcrumb, MdLog, MdPlainTextEdit, MdDrawer, MdNav,
// MdQRCode, MdMasonry, MdAnchor, MdTour, MdRibbon, MdCascader.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdBreadcrumb.h"
#include "widgets/MdButton.h"
#include "widgets/MdCascader.h"
#include "widgets/MdDrawer.h"
#include "widgets/MdInputDialog.h"
#include "widgets/MdLog.h"
#include "widgets/MdMasonry.h"
#include "widgets/MdNav.h"
#include "widgets/MdPlainTextEdit.h"
#include "widgets/MdQRCode.h"
#include "widgets/MdResult.h"
#include "widgets/MdRibbon.h"
#include "widgets/MdStackedWidget.h"
#include "widgets/MdTour.h"
#include "widgets/MdAnchor.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGraphicsOpacityEffect>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>

using namespace md;

namespace {

QImage renderWidget(QWidget &widget)
{
    widget.ensurePolished();
    QImage image(qMax(1, int(widget.width())), qMax(1, int(widget.height())),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32);
}

} // namespace

class TestMd3Stage2Remainder : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
        // Offscreen has no font database on some Windows kits; the bundled
        // Roboto is enough for the text-only render smokes (MdBreadcrumb).
        MdFont::registerBundledFonts();
    }

    // --- MdInputDialog ------------------------------------------------------
    void inputDialogDefaults()
    {
        MdInputDialog d;
        QCOMPARE(d.title(), QString());
        QCOMPARE(d.mode(), MdInputDialogMode::TextInput);
        QVERIFY(d.field() != nullptr);
        QVERIFY(d.acceptButton() != nullptr);
        QVERIFY(d.rejectButton() != nullptr);
    }

    void inputDialogPropertiesNotify()
    {
        MdInputDialog d;
        QSignalSpy titleSpy(&d, &MdInputDialog::titleChanged);
        QSignalSpy textSpy(&d, &MdInputDialog::textChanged);
        QSignalSpy modeSpy(&d, &MdInputDialog::modeChanged);
        d.setTitle(QStringLiteral("Name"));
        d.setText(QStringLiteral("Ada"));
        d.setMode(MdInputDialogMode::PasswordInput);
        QCOMPARE(titleSpy.count(), 1);
        QCOMPARE(textSpy.count(), 1);
        QCOMPARE(modeSpy.count(), 1);
        QCOMPARE(d.text(), QStringLiteral("Ada"));
    }

    void inputDialogAcceptReject()
    {
        MdInputDialog d(QStringLiteral("Title"), QStringLiteral("Label"));
        d.setText(QStringLiteral("x"));
        QSignalSpy acceptedSpy(&d, &MdInputDialog::accepted);
        QSignalSpy rejectedSpy(&d, &MdInputDialog::rejected);
        QSignalSpy finishedSpy(&d, &MdInputDialog::finished);
        d.acceptButton()->click();
        QCOMPARE(acceptedSpy.count(), 1);
        QCOMPARE(finishedSpy.count(), 1);
        QCOMPARE(rejectedSpy.count(), 0);
    }

    // --- MdStackedWidget ----------------------------------------------------
    void stackedWidgetBasics()
    {
        MdStackedWidget stack;
        auto *a = new QLabel(QStringLiteral("A"));
        auto *b = new QLabel(QStringLiteral("B"));
        QCOMPARE(stack.addWidget(a), 0);
        QCOMPARE(stack.addWidget(b), 1);
        QCOMPARE(stack.count(), 2);
        QCOMPARE(stack.currentIndex(), 0);
        QCOMPARE(stack.currentWidget(), a);
        QSignalSpy indexSpy(&stack, &MdStackedWidget::currentIndexChanged);
        stack.setCurrentIndex(1);
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(stack.currentIndex(), 1);
        QCOMPARE(stack.indexOf(b), 1);
    }

    void stackedWidgetFadeApplied()
    {
        // The effects-fast spring must land on the pages, not just m_progress.
        // Both pages stay visible for the flight and carry a
        // QGraphicsOpacityEffect driven by transitionProgress().
        MdStackedWidget stack;
        stack.resize(200, 160);
        auto *a = new QLabel(QStringLiteral("A"));
        auto *b = new QLabel(QStringLiteral("B"));
        QCOMPARE(stack.addWidget(a), 0);
        QCOMPARE(stack.addWidget(b), 1);
        stack.show();
        QVERIFY(QTest::qWaitForWindowExposed(&stack));

        stack.setCurrentIndex(1);
        QCOMPARE(stack.transitionProgress(), 0.0);
        auto *outFx = qobject_cast<QGraphicsOpacityEffect *>(a->graphicsEffect());
        auto *inFx = qobject_cast<QGraphicsOpacityEffect *>(b->graphicsEffect());
        QVERIFY(outFx != nullptr);
        QVERIFY(inFx != nullptr);
        QCOMPARE(outFx->opacity(), 1.0);
        QCOMPARE(inFx->opacity(), 0.0);
        QVERIFY(a->isVisible());
        QVERIFY(b->isVisible());

        // Mid-flight: both opacities move with the spring, never a hard cut.
        QTest::qWait(40);
        const qreal p = stack.transitionProgress();
        QVERIFY(p > 0.0);
        QVERIFY(p < 1.0);
        outFx = qobject_cast<QGraphicsOpacityEffect *>(a->graphicsEffect());
        inFx = qobject_cast<QGraphicsOpacityEffect *>(b->graphicsEffect());
        QVERIFY(outFx != nullptr);
        QVERIFY(inFx != nullptr);
        QVERIFY(outFx->opacity() < 1.0);
        QVERIFY(inFx->opacity() > 0.0);
        QVERIFY(inFx->opacity() <= p + 0.05);
        QVERIFY(outFx->opacity() <= 1.0 - p + 0.05);

        QTRY_COMPARE(stack.transitionProgress(), 1.0);
        QVERIFY(!a->isVisible());
        QVERIFY(b->isVisible());
        // Effects are dropped once the spring settles — no permanent cache.
        QVERIFY(a->graphicsEffect() == nullptr);
        QVERIFY(b->graphicsEffect() == nullptr);
    }

    // --- MdResult -----------------------------------------------------------
    void resultDefaultsAndStatus()
    {
        MdResult r;
        QCOMPARE(r.status(), MdResultStatus::Success);
        QCOMPARE(r.titleStyle(), TypeStyle::HeadlineMedium);
        QCOMPARE(r.artSize(), 96);
        QVERIFY(r.actionButton() != nullptr);

        QSignalSpy statusSpy(&r, &MdResult::statusChanged);
        r.setStatus(MdResultStatus::Error);
        QCOMPARE(statusSpy.count(), 1);
        QCOMPARE(r.status(), MdResultStatus::Error);
        QCOMPARE(r.resolvedStatusColor(), MdTheme::instance().color(ColorRole::Error));
    }

    void resultRenders()
    {
        MdResult r(MdResultStatus::Success, QStringLiteral("Done"));
        r.setSubtitle(QStringLiteral("All good"));
        r.setActionText(QStringLiteral("Continue"));
        r.resize(360, 320);
        r.show();
        QVERIFY(QTest::qWaitForWindowExposed(&r));
        const QImage image = renderWidget(r);
        QVERIFY2(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0,
                 "MdResult painted nothing");
    }

    // --- MdBreadcrumb -------------------------------------------------------
    void breadcrumbBasics()
    {
        MdBreadcrumb b;
        b.setItems(QStringList() << QStringLiteral("Home") << QStringLiteral("Docs")
                                 << QStringLiteral("API"));
        QCOMPARE(b.count(), 3);
        QSignalSpy indexSpy(&b, &MdBreadcrumb::currentIndexChanged);
        b.setCurrentIndex(1);
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(b.currentIndex(), 1);
        QCOMPARE(b.separatorIconName(), QStringLiteral("chevron_right"));
    }

    void breadcrumbRenders()
    {
        MdBreadcrumb b;
        b.setItems(QStringList() << QStringLiteral("Home") << QStringLiteral("Page"));
        b.resize(320, 40);
        b.show();
        QVERIFY(QTest::qWaitForWindowExposed(&b));
        const QImage image = renderWidget(b);
        QVERIFY2(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0,
                 "MdBreadcrumb painted nothing");
    }

    // --- MdLog --------------------------------------------------------------
    void logBasics()
    {
        MdLog log;
        QVERIFY(log.autoScroll());
        QVERIFY(log.isMonospace());
        QCOMPARE(log.maxLines(), 2000);
        log.appendLine(QStringLiteral("hello"), MdLogLevel::Info);
        log.appendLine(QStringLiteral("warn"), MdLogLevel::Warning);
        QCOMPARE(log.lineCount(), 2);
        QCOMPARE(log.lineAt(1).level, MdLogLevel::Warning);
        QSignalSpy clearSpy(&log, &MdLog::linesChanged);
        log.clearLines();
        QCOMPARE(log.lineCount(), 0);
        QVERIFY(clearSpy.count() >= 1);
    }

    void logRenders()
    {
        MdLog log;
        log.appendLine(QStringLiteral("line one"));
        log.appendLine(QStringLiteral("line two"), MdLogLevel::Error);
        log.resize(280, 120);
        log.show();
        QVERIFY(QTest::qWaitForWindowExposed(&log));
        const QImage image = renderWidget(log);
        QVERIFY2(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0,
                 "MdLog painted nothing");
    }

    // --- MdPlainTextEdit ----------------------------------------------------
    void plainTextEditBasics()
    {
        MdPlainTextEdit edit;
        QVERIFY(edit.lineNumbersVisible());
        QVERIFY(edit.currentLineHighlight());
        QCOMPARE(edit.zoomFactor(), 1.0);
        QSignalSpy zoomSpy(&edit, &MdPlainTextEdit::zoomFactorChanged);
        edit.setZoomFactor(1.5);
        QCOMPARE(zoomSpy.count(), 1);
        QCOMPARE(edit.zoomFactor(), 1.5);
        edit.setPlainText(QStringLiteral("alpha\nbeta"));
        QCOMPARE(edit.replaceAll(QStringLiteral("a"), QStringLiteral("A"), true), 3);
    }

    // --- MdDrawer -----------------------------------------------------------
    void drawerBasics()
    {
        MdDrawer drawer;
        QCOMPARE(drawer.edge(), MdDrawerEdge::Start);
        QCOMPARE(drawer.kind(), MdDrawerKind::Modal);
        QVERIFY(!drawer.isOpen());
        QSignalSpy openSpy(&drawer, &MdDrawer::openChanged);
        drawer.setOpen(true);
        QCOMPARE(openSpy.count(), 1);
        QVERIFY(drawer.isOpen());
        drawer.setPanelSize(320);
        QCOMPARE(drawer.panelSize(), 320);
    }

    // --- MdNav / MdNavItem --------------------------------------------------
    void navBasics()
    {
        MdNav nav;
        auto *home = new MdNavItem(QStringLiteral("Home"), QStringLiteral("home"));
        auto *info = new MdNavItem(QStringLiteral("Info"), QStringLiteral("info"));
        nav.addItem(home);
        nav.addItem(info);
        QCOMPARE(nav.count(), 2);
        QCOMPARE(nav.currentIndex(), 0);
        QVERIFY(home->isSelected());
        QSignalSpy indexSpy(&nav, &MdNav::currentIndexChanged);
        nav.setCurrentIndex(1);
        QCOMPARE(indexSpy.count(), 1);
        QVERIFY(info->isSelected());
        QVERIFY(!home->isSelected());
    }

    void navRenders()
    {
        MdNav nav;
        nav.addItem(new MdNavItem(QStringLiteral("Home"), QStringLiteral("home")));
        nav.addItem(new MdNavItem(QStringLiteral("Search"), QStringLiteral("search")));
        nav.resize(200, 112);
        nav.show();
        QVERIFY(QTest::qWaitForWindowExposed(&nav));
        const QImage image = renderWidget(nav);
        QVERIFY2(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0,
                 "MdNav painted nothing");
    }

    // --- MdQRCode -----------------------------------------------------------
    void qrCodeBasics()
    {
        MdQRCode qr;
        QCOMPARE(qr.moduleSize(), 4);
        QVERIFY(qr.hasQuietZone());
        qr.setText(QStringLiteral("https://m3.material.io"));
        QVERIFY(qr.matrixDimension() > 0);
        QSignalSpy textSpy(&qr, &MdQRCode::textChanged);
        qr.setText(QStringLiteral("other"));
        QCOMPARE(textSpy.count(), 1);
    }

    void qrCodeRenders()
    {
        MdQRCode qr(QStringLiteral("hello"));
        qr.resize(qr.sizeHint());
        qr.show();
        QVERIFY(QTest::qWaitForWindowExposed(&qr));
        const QImage image = renderWidget(qr);
        QVERIFY2(mdtest::paintedPixelCount(image, qRgba(0, 0, 0, 0)) > 0,
                 "MdQRCode painted nothing");
    }

    // --- MdMasonry ----------------------------------------------------------
    void masonryBasics()
    {
        MdMasonry masonry;
        QCOMPARE(masonry.columns(), 0);
        QCOMPARE(masonry.gap(), 12.0);
        QSignalSpy colSpy(&masonry, &MdMasonry::columnsChanged);
        masonry.setColumns(3);
        QCOMPARE(colSpy.count(), 1);
        QCOMPARE(masonry.columns(), 3);
        QCOMPARE(masonry.effectiveColumns(), 3);
    }

    // --- MdAnchor / MdAffix -------------------------------------------------
    void anchorBasics()
    {
        MdAnchor anchor;
        anchor.addItem(QStringLiteral("Intro"), 0);
        anchor.addItem(QStringLiteral("Details"), 200);
        QCOMPARE(anchor.count(), 2);
        QCOMPARE(anchor.itemAt(1).contentY, 200);
        QSignalSpy indexSpy(&anchor, &MdAnchor::currentIndexChanged);
        anchor.setCurrentIndex(1);
        QCOMPARE(indexSpy.count(), 1);
    }

    void affixBasics()
    {
        MdAffix affix;
        QVERIFY(!affix.isPinned());
        QCOMPARE(affix.offset(), 0);
        QSignalSpy offsetSpy(&affix, &MdAffix::offsetChanged);
        affix.setOffset(120);
        QCOMPARE(offsetSpy.count(), 1);
        QCOMPARE(affix.offset(), 120);
    }

    // --- MdTour / MdCoachMark -----------------------------------------------
    void tourBasics()
    {
        MdTour tour;
        QCOMPARE(tour.stepCount(), 0);
        QVERIFY(!tour.isActive());
        QSignalSpy countSpy(&tour, &MdTour::stepCountChanged);
        MdTourStep step;
        step.title = QStringLiteral("Welcome");
        step.body = QStringLiteral("This is the first step.");
        tour.addStep(step);
        QCOMPARE(tour.stepCount(), 1);
        QVERIFY(countSpy.count() >= 1);
    }

    // --- MdRibbon -----------------------------------------------------------
    void ribbonBasics()
    {
        MdRibbon ribbon;
        QCOMPARE(ribbon.bandHeight(), 88);
        QCOMPARE(ribbon.count(), 0);
        MdRibbonTab tab(QStringLiteral("Home"), QStringList() << QStringLiteral("Clipboard"));
        ribbon.addTab(tab);
        QCOMPARE(ribbon.count(), 1);
        QCOMPARE(ribbon.currentIndex(), 0);
        QSignalSpy heightSpy(&ribbon, &MdRibbon::bandHeightChanged);
        ribbon.setBandHeight(96);
        QCOMPARE(heightSpy.count(), 1);
        QCOMPARE(ribbon.bandHeight(), 96);
    }

    // --- MdCascader ---------------------------------------------------------
    void cascaderBasics()
    {
        MdCascader cascader;
        QVERIFY(!cascader.isPopupVisible());
        QCOMPARE(cascader.currentPath(), QStringList());
        auto *root = new MdCascaderNode(QStringLiteral("Region"));
        root->appendChild(new MdCascaderNode(QStringLiteral("East")));
        cascader.addTopLevelNode(root);
        QCOMPARE(cascader.topLevelNodes().size(), 1);
    }

    // --- shared theme reaction ----------------------------------------------
    void themeRefreshUpdates()
    {
        MdResult result;
        MdBreadcrumb crumb;
        MdLog log;
        MdQRCode qr(QStringLiteral("x"));
        const QColor before = result.resolvedStatusColor();
        MdTheme::instance().setSeedColor(QColor(0x00696D));
        QVERIFY(result.resolvedStatusColor() != before
                || result.resolvedStatusColor()
                       == MdTheme::instance().color(ColorRole::Primary));
        Q_UNUSED(crumb);
        Q_UNUSED(log);
        Q_UNUSED(qr);
    }
};

QTEST_MAIN(TestMd3Stage2Remainder)
#include "TestMd3Stage2Remainder.moc"
