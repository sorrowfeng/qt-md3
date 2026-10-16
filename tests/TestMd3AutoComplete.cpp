// TestMd3AutoComplete — Stage 2 Qt extension (Appendix D, P2).
//
// Text field + exposed dropdown: property defaults, filtering, NOTIFY,
// item management, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdAutoComplete.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3AutoComplete : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdAutoCompleteFilter>("md::MdAutoCompleteFilter");
    }

    void defaultState()
    {
        MdAutoComplete ac;
        QCOMPARE(ac.text(), QString());
        QCOMPARE(ac.filter(), MdAutoCompleteFilter::Contains);
        QCOMPARE(ac.maxVisible(), 8);
        QVERIFY(!ac.isPopupVisible());
        QVERIFY(ac.items().isEmpty());
        QVERIFY(ac.filteredItems().isEmpty());
    }

    void propertiesNotify()
    {
        MdAutoComplete ac;
        QSignalSpy textSpy(&ac, &MdAutoComplete::textChanged);
        QSignalSpy labelSpy(&ac, &MdAutoComplete::labelTextChanged);
        QSignalSpy placeholderSpy(&ac, &MdAutoComplete::placeholderTextChanged);
        QSignalSpy filterSpy(&ac, &MdAutoComplete::filterChanged);
        QSignalSpy maxSpy(&ac, &MdAutoComplete::maxVisibleChanged);

        ac.setLabelText(QStringLiteral("City"));
        QCOMPARE(labelSpy.count(), 1);
        ac.setPlaceholderText(QStringLiteral("Type a city"));
        QCOMPARE(placeholderSpy.count(), 1);
        ac.setFilter(MdAutoCompleteFilter::StartsWith);
        QCOMPARE(filterSpy.count(), 1);
        ac.setMaxVisible(4);
        QCOMPARE(maxSpy.count(), 1);
        ac.setText(QStringLiteral("to"));
        QCOMPARE(textSpy.count(), 1);
    }

    void containsFilter()
    {
        MdAutoComplete ac;
        ac.setItems(QStringList() << QStringLiteral("Tokyo") << QStringLiteral("Toronto")
                                  << QStringLiteral("Osaka"));
        ac.setFilter(MdAutoCompleteFilter::Contains);
        ac.setText(QStringLiteral("to"));
        QCOMPARE(ac.text(), QStringLiteral("to"));
        // Filtered list is computed on text change.
        const QStringList filtered = ac.filteredItems();
        QVERIFY(filtered.contains(QStringLiteral("Tokyo")));
        QVERIFY(filtered.contains(QStringLiteral("Toronto")));
        QVERIFY(!filtered.contains(QStringLiteral("Osaka")));
    }

    void startsWithFilter()
    {
        MdAutoComplete ac;
        ac.setItems(QStringList() << QStringLiteral("Tokyo") << QStringLiteral("Toronto")
                                  << QStringLiteral("Osaka"));
        ac.setFilter(MdAutoCompleteFilter::StartsWith);
        ac.setText(QStringLiteral("Tok"));
        const QStringList filtered = ac.filteredItems();
        QVERIFY(filtered.contains(QStringLiteral("Tokyo")));
        QVERIFY(!filtered.contains(QStringLiteral("Toronto")));
    }

    void itemManagement()
    {
        MdAutoComplete ac;
        ac.addItem(QStringLiteral("a"));
        ac.addItem(QStringLiteral("b"));
        QCOMPARE(ac.items().size(), 2);
        ac.clearItems();
        QVERIFY(ac.items().isEmpty());
        ac.setItems(QStringList() << QStringLiteral("x") << QStringLiteral("y"));
        QCOMPARE(ac.items().size(), 2);
    }

    void setFilteredItems()
    {
        MdAutoComplete ac;
        ac.setFilter(MdAutoCompleteFilter::Custom);
        ac.setFilteredItems(QStringList() << QStringLiteral("one") << QStringLiteral("two"));
        QCOMPARE(ac.filteredItems().size(), 2);
    }

    void nonBlankRender()
    {
        MdAutoComplete ac;
        ac.setLabelText(QStringLiteral("Search"));
        ac.setText(QStringLiteral("hello"));
        ac.resize(280, 56);
        QImage image(ac.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            ac.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdAutoComplete painted nothing");
    }
};

QTEST_MAIN(TestMd3AutoComplete)
#include "TestMd3AutoComplete.moc"
