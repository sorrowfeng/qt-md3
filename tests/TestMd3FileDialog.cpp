// TestMd3FileDialog — Stage 2 Qt extension (Appendix D, P0).
//
// Pins the "Dialog + Navigation drawer + List + Text field" composition:
//
//   * directory / selectedName / mode / filters Q_PROPERTYs notify;
//   * refresh() lists a real directory through QDir, dirs first, with the
//     name filter applied to files only;
//   * index selection drives selectedName and selectedFilePath();
//   * activateCurrent() enters directories and accepts files;
//   * meta-property surface + non-blank render smoke.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdFileDialog.h"

#include <QtCore/QTemporaryDir>
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

void writeEmpty(const QString &path)
{
    QFile file(path);
    file.open(QIODevice::WriteOnly);
    file.write("x");
    file.close();
}

} // namespace

class TestMd3FileDialog : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdFileDialog dialog;
        QCOMPARE(dialog.mode(), MdFileDialogMode::OpenFile);
        QVERIFY(!dialog.showHidden());
        QCOMPARE(dialog.currentFilterIndex(), -1);
        QCOMPARE(dialog.currentIndex(), -1);
        QVERIFY(dialog.nameFilters().isEmpty());
        QVERIFY(!dialog.directory().isEmpty());
    }

    void titleAndModeProperties()
    {
        MdFileDialog dialog;
        QSignalSpy titleSpy(&dialog, &MdFileDialog::titleChanged);
        QSignalSpy modeSpy(&dialog, &MdFileDialog::modeChanged);

        dialog.setTitle(QStringLiteral("Open project"));
        QCOMPARE(dialog.title(), QStringLiteral("Open project"));
        QCOMPARE(titleSpy.count(), 1);

        dialog.setMode(MdFileDialogMode::SaveFile);
        QCOMPARE(dialog.mode(), MdFileDialogMode::SaveFile);
        QCOMPARE(modeSpy.count(), 1);
    }

    void listsDirectory()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeEmpty(dir.filePath(QStringLiteral("b.txt")));
        writeEmpty(dir.filePath(QStringLiteral("a.png")));
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("sub")));

        MdFileDialog dialog;
        QSignalSpy dirSpy(&dialog, &MdFileDialog::directoryChanged);
        QSignalSpy entriesSpy(&dialog, &MdFileDialog::entriesChanged);
        dialog.setDirectory(dir.path());
        QCOMPARE(dialog.directory(), QFileInfo(dir.path()).absoluteFilePath());
        QVERIFY(dirSpy.count() >= 1);
        QVERIFY(entriesSpy.count() >= 1);

        const QList<MdFileEntry> entries = dialog.entries();
        QCOMPARE(entries.size(), 3);
        // Dirs first, then names.
        QVERIFY(entries.at(0).isDir);
        QCOMPARE(entries.at(0).name, QStringLiteral("sub"));
        QCOMPARE(entries.at(1).name, QStringLiteral("a.png"));
        QCOMPARE(entries.at(2).name, QStringLiteral("b.txt"));
    }

    void nameFilterAppliesToFiles()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeEmpty(dir.filePath(QStringLiteral("a.png")));
        writeEmpty(dir.filePath(QStringLiteral("b.txt")));

        MdFileDialog dialog;
        dialog.setDirectory(dir.path());
        dialog.setNameFilters(QStringList() << QStringLiteral("Images (*.png)"));
        dialog.setCurrentFilterIndex(0);

        const QList<MdFileEntry> entries = dialog.entries();
        // Dirs (none) + a.png only.
        QCOMPARE(entries.size(), 1);
        QCOMPARE(entries.at(0).name, QStringLiteral("a.png"));
    }

    void selectionAndPath()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeEmpty(dir.filePath(QStringLiteral("doc.md")));

        MdFileDialog dialog;
        dialog.setDirectory(dir.path());
        QCOMPARE(dialog.entries().size(), 1);

        QSignalSpy indexSpy(&dialog, &MdFileDialog::currentIndexChanged);
        QSignalSpy nameSpy(&dialog, &MdFileDialog::selectedNameChanged);
        dialog.setCurrentIndex(0);
        QCOMPARE(dialog.currentIndex(), 0);
        QCOMPARE(indexSpy.count(), 1);
        QCOMPARE(dialog.selectedName(), QStringLiteral("doc.md"));
        QCOMPARE(nameSpy.count(), 1);
        QCOMPARE(dialog.selectedFilePath(), QFileInfo(dir.filePath(QStringLiteral("doc.md"))).absoluteFilePath());

        dialog.setSelectedName(QStringLiteral("other.txt"));
        QCOMPARE(dialog.selectedFilePath(),
                 QFileInfo(dir.filePath(QStringLiteral("other.txt"))).absoluteFilePath());
    }

    void activateCurrentEntersAndAccepts()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeEmpty(dir.filePath(QStringLiteral("file.txt")));
        QVERIFY(QDir(dir.path()).mkpath(QStringLiteral("nested")));

        MdFileDialog dialog;
        dialog.setDirectory(dir.path());
        // index 0 is "nested"
        dialog.setCurrentIndex(0);
        QSignalSpy acceptSpy(&dialog, &MdFileDialog::accepted);
        dialog.activateCurrent();
        QCOMPARE(acceptSpy.count(), 0);
        QVERIFY(dialog.directory().endsWith(QStringLiteral("nested")));

        dialog.setDirectory(dir.path());
        dialog.setCurrentIndex(1); // file.txt
        dialog.activateCurrent();
        QCOMPARE(acceptSpy.count(), 1);
        QCOMPARE(acceptSpy.at(0).at(0).toString(),
                 QFileInfo(dir.filePath(QStringLiteral("file.txt"))).absoluteFilePath());
    }

    void metaProperties()
    {
        MdFileDialog dialog;
        const QMetaObject *meta = dialog.metaObject();
        QVERIFY(meta->indexOfProperty("title") >= 0);
        QVERIFY(meta->indexOfProperty("directory") >= 0);
        QVERIFY(meta->indexOfProperty("selectedName") >= 0);
        QVERIFY(meta->indexOfProperty("showHidden") >= 0);
        QVERIFY(meta->indexOfProperty("mode") >= 0);
        QVERIFY(meta->indexOfProperty("nameFilters") >= 0);
        QVERIFY(meta->indexOfProperty("currentFilterIndex") >= 0);
        QVERIFY(meta->indexOfProperty("currentIndex") >= 0);

        QSignalSpy showSpy(&dialog, &MdFileDialog::showHiddenChanged);
        dialog.setProperty("showHidden", true);
        QVERIFY(dialog.showHidden());
        QCOMPARE(showSpy.count(), 1);
    }

    void themeLifecycle()
    {
        MdFileDialog dialog;
        dialog.resize(640, 480);
        QSignalSpy titleSpy(&dialog, &MdFileDialog::titleChanged);
        MdTheme::instance().setSeedColor(QColor(0x4A148C));
        QCOMPARE(titleSpy.count(), 0);
    }

    void renderSmoke()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        writeEmpty(dir.filePath(QStringLiteral("hello.txt")));

        MdFileDialog dialog;
        dialog.setTitle(QStringLiteral("Open file"));
        dialog.setDirectory(dir.path());
        dialog.resize(720, 480);
        dialog.show();
        QVERIFY(QTest::qWaitForWindowExposed(&dialog));
        const QImage image = renderWidget(dialog);
        QVERIFY(anyPixelPainted(image));
    }
};

QTEST_MAIN(TestMd3FileDialog)
#include "TestMd3FileDialog.moc"
