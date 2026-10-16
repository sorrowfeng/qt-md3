// TestMd3Tree — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the tree rows and the select popup:
//   * MdTreeItem model (children, paths, expansion);
//   * visible-row flattening with indent and depth;
//   * keyboard walk (Up/Down/Home/End, Left/Right expand-collapse);
//   * selection with NOTIFY;
//   * MdTreeSelect field + popup composition.

#include "TestMd3Common.h"
#include "core/MdMotion.h"
#include "core/MdTheme.h"
#include "widgets/MdTree.h"
#include "widgets/MdTreeSelect.h"

#include <QtGui/QImage>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

MdTreeItem *makeNode(const QString &text, const QStringList &children = QStringList())
{
    auto *node = new MdTreeItem(text);
    for (const QString &child : children) {
        node->appendChild(new MdTreeItem(child));
    }
    return node;
}

} // namespace

class TestMd3Tree : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void itemModel()
    {
        auto *parent = makeNode(QStringLiteral("root"),
                                QStringList() << QStringLiteral("a")
                                              << QStringLiteral("b"));
        QCOMPARE(parent->text(), QStringLiteral("root"));
        QCOMPARE(parent->childCount(), 2);
        QCOMPARE(parent->childAt(0)->text(), QStringLiteral("a"));
        QCOMPARE(parent->childAt(1)->text(), QStringLiteral("b"));
        QCOMPARE(parent->childAt(0)->parentItem(), parent);
        QVERIFY(!parent->isExpanded());
        delete parent;
    }

    void treeDefaultState()
    {
        MdTree tree;
        QCOMPARE(tree.indent(), 16);
        QCOMPARE(tree.rowHeight(), 56);
        QVERIFY(tree.wrapsNavigation());
        QCOMPARE(tree.topLevelItemCount(), 0);
        QVERIFY(tree.selectedPath().isEmpty());
    }

    void pathsAndRows()
    {
        MdTree tree;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("a")
                                            << QStringLiteral("b"));
        auto *nested = makeNode(QStringLiteral("nested"));
        root->childAt(0)->appendChild(nested);
        tree.addTopLevelItem(root);

        QCOMPARE(tree.topLevelItemCount(), 1);
        QCOMPARE(tree.itemAt(MdTreePath{0}), root);
        QCOMPARE(tree.itemAt(MdTreePath{0, 0}), root->childAt(0));
        QCOMPARE(tree.itemAt(MdTreePath{0, 0, 0}), nested);
        QCOMPARE(tree.itemAt(MdTreePath{0, 1}), root->childAt(1));
        QCOMPARE(tree.itemAt(MdTreePath{1}), nullptr);

        QCOMPARE(tree.pathOf(nested), (MdTreePath{0, 0, 0}));

        // Collapsed: only the root is visible.
        QCOMPARE(tree.visibleRowCount(), 1);

        tree.setExpanded(MdTreePath{0}, true);
        QCOMPARE(tree.visibleRowCount(), 3); // root, a, b

        tree.setExpanded(MdTreePath{0, 0}, true);
        QCOMPARE(tree.visibleRowCount(), 4); // root, a, nested, b

        const QList<MdTree::Row> rows = tree.visibleRows();
        QCOMPARE(rows.at(0).depth, 0);
        QVERIFY(rows.at(0).hasChildren);
        QCOMPARE(rows.at(1).depth, 1);
        QCOMPARE(rows.at(2).depth, 2);
        QCOMPARE(rows.at(2).item, nested);
    }

    void propertiesNotify()
    {
        MdTree tree;
        QSignalSpy indentSpy(&tree, &MdTree::indentChanged);
        QSignalSpy rowSpy(&tree, &MdTree::rowHeightChanged);
        QSignalSpy wrapSpy(&tree, &MdTree::wrapNavigationChanged);
        QSignalSpy selectedSpy(&tree, &MdTree::selectedPathChanged);
        QSignalSpy expandedSpy(&tree, &MdTree::itemExpanded);

        tree.setIndent(24);
        QCOMPARE(indentSpy.count(), 1);

        tree.setRowHeight(48);
        QCOMPARE(rowSpy.count(), 1);

        tree.setWrapNavigation(false);
        QCOMPARE(wrapSpy.count(), 1);

        tree.addTopLevelItem(makeNode(QStringLiteral("n")));
        tree.setSelectedPath(MdTreePath{0});
        QCOMPARE(selectedSpy.count(), 1);
        QCOMPARE(tree.selectedItem()->text(), QStringLiteral("n"));

        tree.setExpanded(MdTreePath{0}, true);
        QCOMPARE(expandedSpy.count(), 1);
    }

    void keyboardWalk()
    {
        MdTree tree;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("a")
                                            << QStringLiteral("b"));
        root->setExpanded(true);
        tree.addTopLevelItem(root);
        tree.addTopLevelItem(makeNode(QStringLiteral("second")));
        tree.resize(240, 200);
        tree.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tree));
        tree.setFocus();

        QCOMPARE(tree.visibleRowCount(), 4); // root, a, b, second

        // First Down from "nothing selected" lands on the focus row (root).
        QTest::keyClick(&tree, Qt::Key_Down);
        QCOMPARE(tree.selectedPath(), (MdTreePath{0}));

        QTest::keyClick(&tree, Qt::Key_Down);
        QCOMPARE(tree.selectedPath(), (MdTreePath{0, 0}));

        QTest::keyClick(&tree, Qt::Key_End);
        QCOMPARE(tree.selectedPath(), (MdTreePath{1}));

        QTest::keyClick(&tree, Qt::Key_Home);
        QCOMPARE(tree.selectedPath(), (MdTreePath{0}));

        // Right expands a collapsed branch; Left collapses it.
        tree.setSelectedPath(MdTreePath{1});
        auto *second = tree.itemAt(MdTreePath{1});
        second->appendChild(new MdTreeItem(QStringLiteral("leaf")));
        QTest::keyClick(&tree, Qt::Key_Right);
        QVERIFY(tree.isExpanded(MdTreePath{1}));
        QTest::keyClick(&tree, Qt::Key_Left);
        QVERIFY(!tree.isExpanded(MdTreePath{1}));
    }

    void expandCollapseAll()
    {
        MdTree tree;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("a")
                                            << QStringLiteral("b"));
        auto *nested = makeNode(QStringLiteral("nested"));
        root->childAt(0)->appendChild(nested);
        tree.addTopLevelItem(root);

        tree.expandAll();
        QVERIFY(tree.isExpanded(MdTreePath{0}));
        QVERIFY(tree.isExpanded(MdTreePath{0, 0}));
        QCOMPARE(tree.visibleRowCount(), 4);

        tree.collapseAll();
        QVERIFY(!tree.isExpanded(MdTreePath{0}));
        QCOMPARE(tree.visibleRowCount(), 1);
    }

    void clickSelectsAndActivates()
    {
        MdTree tree;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("a"));
        root->setExpanded(true);
        tree.addTopLevelItem(root);
        tree.resize(240, 160);
        tree.show();
        QVERIFY(QTest::qWaitForWindowExposed(&tree));

        QSignalSpy activatedSpy(&tree, &MdTree::itemActivated);
        // Row 1 is the child at y = 56..112.
        QTest::mouseClick(&tree, Qt::LeftButton, Qt::NoModifier, QPoint(120, 80));
        QCOMPARE(tree.selectedPath(), (MdTreePath{0, 0}));
        QCOMPARE(activatedSpy.count(), 1);
    }

    void renderSmoke()
    {
        MdTree tree;
        auto *root = makeNode(QStringLiteral("Documents"),
                              QStringList() << QStringLiteral("a")
                                            << QStringLiteral("b"));
        root->setExpanded(true);
        tree.addTopLevelItem(root);
        tree.resize(280, 56 * 3);
        QImage image(280, 56 * 3, QImage::Format_ARGB32);
        image.fill(Qt::white);
        tree.render(&image);
        QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 0);
    }

    // --- MdTreeSelect -------------------------------------------------------

    void selectDefaultState()
    {
        MdTreeSelect select;
        QVERIFY(select.tree() != nullptr);
        QVERIFY(select.currentText().isEmpty());
        QVERIFY(select.currentPath().isEmpty());
        QVERIFY(!select.isPopupVisible());
    }

    void selectShowsChosenLeaf()
    {
        MdTreeSelect select;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("leaf"));
        root->setExpanded(true);
        select.addTopLevelItem(root);
        select.resize(240, 56);

        QSignalSpy textSpy(&select, &MdTreeSelect::currentTextChanged);
        select.setSelectedPath(MdTreePath{0, 0});
        QCOMPARE(select.currentText(), QStringLiteral("leaf"));
        QCOMPARE(select.currentPath(), (MdTreePath{0, 0}));
        QCOMPARE(textSpy.count(), 1);

        select.clearSelection();
        QVERIFY(select.currentText().isEmpty());
    }

    void selectPopupOpensAndCloses()
    {
        MdTreeSelect select;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("leaf"));
        select.addTopLevelItem(root);
        select.resize(240, 56);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));

        select.showPopup();
        QVERIFY(select.isPopupVisible());
        // Re-entrant show toggles closed, matching MdSelect.
        select.showPopup();
        QVERIFY(!select.isPopupVisible());

        select.showPopup();
        QVERIFY(select.isPopupVisible());
        select.hidePopup();
        QVERIFY(!select.isPopupVisible());
    }

    void selectActivatingLeafCommits()
    {
        MdTreeSelect select;
        auto *root = makeNode(QStringLiteral("root"),
                              QStringList() << QStringLiteral("leaf"));
        root->setExpanded(true);
        select.addTopLevelItem(root);
        select.resize(240, 56);
        select.show();
        QVERIFY(QTest::qWaitForWindowExposed(&select));

        QSignalSpy textSpy(&select, &MdTreeSelect::currentTextChanged);
        select.showPopup();
        // Activate the leaf row through the tree.
        emit select.tree()->itemActivated(MdTreePath{0, 0});
        QCOMPARE(select.currentText(), QStringLiteral("leaf"));
        QVERIFY(!select.isPopupVisible());
        QCOMPARE(textSpy.count(), 1);
    }
};

QTEST_MAIN(TestMd3Tree)
#include "TestMd3Tree.moc"
