// TestMd3Descriptions — Stage 2 Qt extension (Appendix D, P2).
//
// Key/value list: property defaults, NOTIFY, item model, theme colours,
// column layout, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdDescriptions.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Descriptions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdDescriptionsLayout>("md::MdDescriptionsLayout");
        qRegisterMetaType<MdDescriptionItem>("md::MdDescriptionItem");
        qRegisterMetaType<QList<md::MdDescriptionItem>>("QList<md::MdDescriptionItem>");
    }

    void defaultState()
    {
        MdDescriptions d;
        QVERIFY(d.items().isEmpty());
        QCOMPARE(d.layout(), MdDescriptionsLayout::Vertical);
        QCOMPARE(d.columns(), 1);
        QCOMPARE(d.isBordered(), false);
        QCOMPARE(d.rowSpacing(), 0.0);
        QCOMPARE(d.rowCount(), 0);
    }

    void itemsAndNotify()
    {
        MdDescriptions d;
        QSignalSpy itemsSpy(&d, &MdDescriptions::itemsChanged);
        QSignalSpy layoutSpy(&d, &MdDescriptions::layoutChanged);
        QSignalSpy columnsSpy(&d, &MdDescriptions::columnsChanged);
        QSignalSpy borderedSpy(&d, &MdDescriptions::borderedChanged);
        QSignalSpy spacingSpy(&d, &MdDescriptions::rowSpacingChanged);

        d.addItem(QStringLiteral("Name"), QStringLiteral("Ada"));
        QCOMPARE(itemsSpy.count(), 1);
        QCOMPARE(d.items().size(), 1);
        QCOMPARE(d.rowCount(), 1);

        d.addItem(QStringLiteral("Role"), QStringLiteral("Engineer"));
        QCOMPARE(d.items().size(), 2);

        d.setLayout(MdDescriptionsLayout::Horizontal);
        QCOMPARE(layoutSpy.count(), 1);
        d.setColumns(2);
        QCOMPARE(columnsSpy.count(), 1);
        QCOMPARE(d.rowCount(), 1);
        d.setBordered(true);
        QCOMPARE(borderedSpy.count(), 1);
        d.setRowSpacing(8.0);
        QCOMPARE(spacingSpy.count(), 1);

        d.clearItems();
        QVERIFY(d.items().isEmpty());
    }

    void themeColours()
    {
        MdDescriptions d;
        QCOMPARE(d.resolvedLabelColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        QCOMPARE(d.resolvedValueColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
        QCOMPARE(d.resolvedBorderColor(),
                 MdTheme::instance().color(ColorRole::OutlineVariant));
    }

    void heightForWidth()
    {
        MdDescriptions d;
        d.addItem(QStringLiteral("A"), QStringLiteral("1"));
        d.addItem(QStringLiteral("B"), QStringLiteral("2"));
        const int narrow = d.heightForWidth(200);
        const int wide = d.heightForWidth(600);
        QVERIFY(narrow > 0);
        QVERIFY(wide > 0);
    }

    void nonBlankRender()
    {
        MdDescriptions d;
        d.addItem(QStringLiteral("Language"), QStringLiteral("C++"));
        d.addItem(QStringLiteral("Toolkit"), QStringLiteral("Qt 6"));
        d.setBordered(true);
        d.resize(320, 160);
        QImage image(d.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            d.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdDescriptions painted nothing");
    }
};

QTEST_MAIN(TestMd3Descriptions)
#include "TestMd3Descriptions.moc"
