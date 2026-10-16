// TestMd3Window — Stage 2 Qt extension (Appendix D, P0).
//
// Pins the frameless main window semantics:
//
//   * title / icon / central widget Q_PROPERTYs notify;
//   * chrome flags (minimizable / maximizable / closable / resizable) gate
//     the three MdIconButton title-bar buttons;
//   * titleBarHeight / titleBarVisible move the chrome and the body;
//   * maximized toggles the painted flag and the maximize glyph;
//   * meta-property surface + non-blank render smoke.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdWindow.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>

using namespace md;

namespace {

QImage renderWidget(QWidget &widget)
{
    QImage image(int(widget.width()), int(widget.height()), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    widget.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32);
}

bool anyPixelPainted(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).alpha() > 8) {
                return true;
            }
        }
    }
    return false;
}

} // namespace

class TestMd3Window : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdWindow window;
        QVERIFY(window.title().isEmpty());
        QVERIFY(window.iconName().isEmpty());
        QVERIFY(window.isMinimizable());
        QVERIFY(window.isMaximizable());
        QVERIFY(window.isClosable());
        QVERIFY(window.isResizable());
        QVERIFY(window.isTitleBarVisible());
        QVERIFY(!window.isMaximized());
        QCOMPARE(window.titleBarHeight(), 64);
        QVERIFY(window.centralWidget() == nullptr);
    }

    void titleAndIconProperties()
    {
        MdWindow window;
        QSignalSpy titleSpy(&window, &MdWindow::titleChanged);
        QSignalSpy iconSpy(&window, &MdWindow::iconNameChanged);

        window.setTitle(QStringLiteral("Gallery"));
        QCOMPARE(window.title(), QStringLiteral("Gallery"));
        QCOMPARE(titleSpy.count(), 1);

        window.setIconName(QStringLiteral("dashboard"));
        QCOMPARE(window.iconName(), QStringLiteral("dashboard"));
        QCOMPARE(iconSpy.count(), 1);
    }

    void centralWidgetLaysOutUnderTitleBar()
    {
        MdWindow window;
        auto *body = new QLabel(QStringLiteral("body"), &window);
        window.setCentralWidget(body);
        QCOMPARE(window.centralWidget(), body);

        window.resize(400, 300);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        QCOMPARE(body->x(), 0);
        QCOMPARE(body->y(), window.titleBarHeight());
        QCOMPARE(body->width(), 400);
        QCOMPARE(body->height(), 300 - window.titleBarHeight());
    }

    void chromeButtonsRespectFlags()
    {
        MdWindow window;
        window.resize(400, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        const QList<MdIconButton *> buttons = window.titleBarButtons();
        QCOMPARE(buttons.size(), 3);
        // minimize / maximize / close
        QCOMPARE(buttons.at(0)->accessibleName(), QStringLiteral("Minimize"));
        QCOMPARE(buttons.at(1)->accessibleName(), QStringLiteral("Maximize"));
        QCOMPARE(buttons.at(2)->accessibleName(), QStringLiteral("Close"));
        QVERIFY(buttons.at(0)->isVisible());
        QVERIFY(buttons.at(1)->isVisible());
        QVERIFY(buttons.at(2)->isVisible());

        window.setMinimizable(false);
        QVERIFY(!buttons.at(0)->isVisible());
        window.setMaximizable(false);
        QVERIFY(!buttons.at(1)->isVisible());
        window.setClosable(false);
        QVERIFY(!buttons.at(2)->isVisible());

        window.setMinimizable(true);
        window.setMaximizable(true);
        window.setClosable(true);
        QVERIFY(buttons.at(0)->isVisible());
        QVERIFY(buttons.at(1)->isVisible());
        QVERIFY(buttons.at(2)->isVisible());
    }

    void maximizedTogglesGlyph()
    {
        MdWindow window;
        window.resize(400, 200);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        QSignalSpy maxSpy(&window, &MdWindow::maximizedChanged);
        QVERIFY(!window.isMaximized());
        window.setMaximized(true);
        QVERIFY(window.isMaximized());
        QCOMPARE(maxSpy.count(), 1);
        // Restore glyph `filter_none` while maximised.
        QCOMPARE(window.titleBarButtons().at(1)->iconName(), QStringLiteral("filter_none"));

        window.toggleMaximized();
        QVERIFY(!window.isMaximized());
        QCOMPARE(maxSpy.count(), 2);
        QCOMPARE(window.titleBarButtons().at(1)->iconName(), QStringLiteral("crop_square"));
    }

    void titleBarHeightAndVisibility()
    {
        MdWindow window;
        auto *body = new QLabel(QStringLiteral("body"), &window);
        window.setCentralWidget(body);
        window.resize(400, 300);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));

        window.setTitleBarHeight(48);
        QCOMPARE(window.titleBarHeight(), 48);
        QCOMPARE(body->y(), 48);

        window.setTitleBarVisible(false);
        QVERIFY(!window.isTitleBarVisible());
        QCOMPARE(body->y(), 0);
        QCOMPARE(window.titleBarRect().height(), 0);
    }

    void metaProperties()
    {
        MdWindow window;
        const QMetaObject *meta = window.metaObject();
        QVERIFY(meta->indexOfProperty("title") >= 0);
        QVERIFY(meta->indexOfProperty("iconName") >= 0);
        QVERIFY(meta->indexOfProperty("minimizable") >= 0);
        QVERIFY(meta->indexOfProperty("maximizable") >= 0);
        QVERIFY(meta->indexOfProperty("closable") >= 0);
        QVERIFY(meta->indexOfProperty("resizable") >= 0);
        QVERIFY(meta->indexOfProperty("titleBarHeight") >= 0);
        QVERIFY(meta->indexOfProperty("titleBarVisible") >= 0);
        QVERIFY(meta->indexOfProperty("maximized") >= 0);

        QSignalSpy titleSpy(&window, &MdWindow::titleChanged);
        window.setProperty("title", QStringLiteral("Meta"));
        QCOMPARE(window.title(), QStringLiteral("Meta"));
        QCOMPARE(titleSpy.count(), 1);
    }

    void themeLifecycle()
    {
        MdWindow window;
        window.resize(320, 200);
        window.show();
        QSignalSpy titleSpy(&window, &MdWindow::titleChanged);
        MdTheme::instance().setSeedColor(QColor(0x006A6A));
        QCOMPARE(titleSpy.count(), 0);
    }

    void renderSmoke()
    {
        MdWindow window;
        window.setTitle(QStringLiteral("qt-md3"));
        window.setIconName(QStringLiteral("dashboard"));
        auto *body = new QLabel(QStringLiteral("content"), &window);
        window.setCentralWidget(body);
        window.resize(480, 320);
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
        const QImage image = renderWidget(window);
        QVERIFY(anyPixelPainted(image));
    }
};

QTEST_MAIN(TestMd3Window)
#include "TestMd3Window.moc"
