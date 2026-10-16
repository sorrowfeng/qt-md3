// TestMd3EmptyState — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the empty-state semantics the header documents:
//   * surface-variant art disc + on-surface-variant content;
//   * title / description / action-text properties with NOTIFY;
//   * action button is a real Text MdButton and emits actionTriggered;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdButton.h"
#include "widgets/MdEmptyState.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

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

} // namespace

class TestMd3EmptyState : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdEmptyState empty;
        QVERIFY(empty.title().isEmpty());
        QVERIFY(empty.description().isEmpty());
        QVERIFY(empty.iconName().isEmpty());
        QVERIFY(empty.actionText().isEmpty());
        QCOMPARE(empty.titleStyle(), TypeStyle::HeadlineSmall);
        QCOMPARE(empty.artSize(), 96);
        QCOMPARE(empty.artCorner(), ShapeCorner::ExtraLarge);
        QVERIFY(empty.actionButton() != nullptr);
        QVERIFY(!empty.actionButton()->isVisible());
    }

    void contentProperties()
    {
        MdEmptyState empty;
        QSignalSpy titleSpy(&empty, &MdEmptyState::titleChanged);
        QSignalSpy descSpy(&empty, &MdEmptyState::descriptionChanged);
        QSignalSpy iconSpy(&empty, &MdEmptyState::iconNameChanged);
        QSignalSpy actionSpy(&empty, &MdEmptyState::actionTextChanged);

        empty.setTitle(QStringLiteral("No results"));
        empty.setDescription(QStringLiteral("Try a different search."));
        empty.setIconName(QStringLiteral("inbox"));
        empty.setActionText(QStringLiteral("Clear filters"));

        QCOMPARE(empty.title(), QStringLiteral("No results"));
        QCOMPARE(titleSpy.count(), 1);
        QCOMPARE(descSpy.count(), 1);
        QCOMPARE(iconSpy.count(), 1);
        QCOMPARE(actionSpy.count(), 1);
        QCOMPARE(empty.actionButton()->text(), QStringLiteral("Clear filters"));
        QVERIFY(!empty.actionButton()->isHidden());
    }

    void tokenColors()
    {
        MdEmptyState empty;
        QCOMPARE(empty.resolvedArtBackground(),
                 MdTheme::instance().color(ColorRole::SurfaceVariant));
        QCOMPARE(empty.resolvedArtContent(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));
    }

    void actionTriggers()
    {
        MdEmptyState empty;
        empty.setActionText(QStringLiteral("Retry"));
        empty.resize(320, 280);
        empty.show();
        QVERIFY(QTest::qWaitForWindowExposed(&empty));

        QSignalSpy spy(&empty, &MdEmptyState::actionTriggered);
        empty.actionButton()->click();
        QCOMPARE(spy.count(), 1);
    }

    void artAndTypeAreLive()
    {
        MdEmptyState empty;
        QSignalSpy sizeSpy(&empty, &MdEmptyState::artSizeChanged);
        QSignalSpy styleSpy(&empty, &MdEmptyState::titleStyleChanged);
        QSignalSpy cornerSpy(&empty, &MdEmptyState::artCornerChanged);

        empty.setArtSize(128);
        QCOMPARE(empty.artSize(), 128);
        QCOMPARE(sizeSpy.count(), 1);

        empty.setTitleStyle(TypeStyle::DisplaySmall);
        QCOMPARE(empty.titleStyle(), TypeStyle::DisplaySmall);
        QCOMPARE(styleSpy.count(), 1);

        empty.setArtCorner(ShapeCorner::Full);
        QCOMPARE(empty.artCorner(), ShapeCorner::Full);
        QCOMPARE(cornerSpy.count(), 1);
    }

    void nonBlankRender()
    {
        MdEmptyState empty(QStringLiteral("Nothing here"));
        empty.setDescription(QStringLiteral("Create the first item."));
        empty.setIconName(QStringLiteral("inbox"));
        empty.setActionText(QStringLiteral("Create"));
        empty.resize(360, 320);
        empty.show();
        QVERIFY(QTest::qWaitForWindowExposed(&empty));

        const QImage image = renderWidget(empty);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdEmptyState must paint its art disc and text");
    }
};

QTEST_MAIN(TestMd3EmptyState)
#include "TestMd3EmptyState.moc"
