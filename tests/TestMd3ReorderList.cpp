// TestMd3ReorderList — move up/down, itemMoved signal, drag flag.

#include "TestMd3Common.h"

#include "core/MdFont.h"
#include "core/MdTheme.h"
#include "widgets/MdListItem.h"
#include "widgets/MdReorderList.h"

#include <QtTest/QtTest>

using namespace md;

namespace {

MdListItem *makeItem(const QString &headline)
{
    MdListItem *item = new MdListItem(headline);
    item->setInteractive(true);
    return item;
}

} // namespace

class TestMd3ReorderList : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void defaults();
    void moveItemSwapsOrder();
    void moveItemClampsAtEdges();
    void moveActiveUsesRovingItem();
    void dragEnabledNotifies();
    void moveButtonsVisibleNotifies();
    void metaProperties();
};

void TestMd3ReorderList::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdFont::registerBundledFonts();
}

void TestMd3ReorderList::defaults()
{
    MdReorderList list;
    QVERIFY(list.isDragEnabled());
    QVERIFY(!list.isMoveButtonsVisible());
}

void TestMd3ReorderList::moveItemSwapsOrder()
{
    MdReorderList list;
    list.addItem(makeItem(QStringLiteral("A")));
    list.addItem(makeItem(QStringLiteral("B")));
    list.addItem(makeItem(QStringLiteral("C")));

    QSignalSpy spy(&list, &MdReorderList::itemMoved);
    const int newIndex = list.moveItem(0, 1);
    QCOMPARE(newIndex, 1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);
    QCOMPARE(spy.at(0).at(1).toInt(), 1);

    const QList<MdListItem *> items = list.items();
    QCOMPARE(items.at(0)->headline(), QStringLiteral("B"));
    QCOMPARE(items.at(1)->headline(), QStringLiteral("A"));
    QCOMPARE(items.at(2)->headline(), QStringLiteral("C"));
}

void TestMd3ReorderList::moveItemClampsAtEdges()
{
    MdReorderList list;
    list.addItem(makeItem(QStringLiteral("A")));
    list.addItem(makeItem(QStringLiteral("B")));

    QCOMPARE(list.moveItem(0, -1), 0);
    QCOMPARE(list.moveItem(1, 1), 1);
    QCOMPARE(list.itemCount(), 2);
}

void TestMd3ReorderList::moveActiveUsesRovingItem()
{
    MdReorderList list;
    list.addItem(makeItem(QStringLiteral("A")));
    list.addItem(makeItem(QStringLiteral("B")));
    list.addItem(makeItem(QStringLiteral("C")));
    list.setActiveItem(list.items().at(1));

    QVERIFY(list.moveActive(-1));
    QCOMPARE(list.items().at(0)->headline(), QStringLiteral("B"));
    list.moveDown();
    QCOMPARE(list.items().at(1)->headline(), QStringLiteral("B"));
}

void TestMd3ReorderList::dragEnabledNotifies()
{
    MdReorderList list;
    QSignalSpy spy(&list, &MdReorderList::dragEnabledChanged);
    list.setDragEnabled(false);
    QVERIFY(!list.isDragEnabled());
    QCOMPARE(spy.count(), 1);
}

void TestMd3ReorderList::moveButtonsVisibleNotifies()
{
    MdReorderList list;
    QSignalSpy spy(&list, &MdReorderList::moveButtonsVisibleChanged);
    list.setMoveButtonsVisible(true);
    QVERIFY(list.isMoveButtonsVisible());
    QCOMPARE(spy.count(), 1);
}

void TestMd3ReorderList::metaProperties()
{
    MdReorderList list;
    const QMetaObject *meta = list.metaObject();
    QVERIFY(meta->indexOfProperty("dragEnabled") >= 0);
    QVERIFY(meta->indexOfProperty("moveButtonsVisible") >= 0);
}

QTEST_MAIN(TestMd3ReorderList)
#include "TestMd3ReorderList.moc"
