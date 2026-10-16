// TestMd3Upload — Stage 2 Qt extension (Appendix D, P2).
//
// Pins the upload semantics the header documents:
//   * files property with NOTIFY and list-row height;
//   * progress updates drive the child MdProgressIndicator;
//   * remove button deletes a row and emits fileRemoved;
//   * non-blank render smoke.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdIconButton.h"
#include "widgets/MdProgressIndicator.h"
#include "widgets/MdUpload.h"

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

class TestMd3Upload : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdUpload upload;
        QVERIFY(upload.files().isEmpty());
        QCOMPARE(upload.fileCount(), 0);
        QCOMPARE(upload.maxVisibleRows(), 0);
        QCOMPARE(MdUpload::rowHeight(), 56.0);
    }

    void addAndProgress()
    {
        MdUpload upload;
        QSignalSpy filesSpy(&upload, &MdUpload::filesChanged);

        upload.addFile(QStringLiteral("report.pdf"), 2048);
        QCOMPARE(upload.fileCount(), 1);
        QCOMPARE(filesSpy.count(), 1);
        QCOMPARE(upload.files().at(0).name, QStringLiteral("report.pdf"));
        QVERIFY(upload.files().at(0).transferring);

        upload.setFileProgress(0, 0.5, true);
        QVERIFY(qFuzzyCompare(upload.files().at(0).progress, 0.5));
        QVERIFY(filesSpy.count() >= 2);

        upload.setFileProgress(0, 1.0, false);
        QVERIFY(!upload.files().at(0).transferring);
    }

    void removeEmits()
    {
        MdUpload upload;
        upload.addFile(QStringLiteral("a.txt"));
        upload.addFile(QStringLiteral("b.txt"));

        QSignalSpy removedSpy(&upload, &MdUpload::fileRemoved);
        upload.removeFile(0);
        QCOMPARE(upload.fileCount(), 1);
        QCOMPARE(removedSpy.count(), 1);
        QCOMPARE(removedSpy.at(0).at(0).toInt(), 0);
        QCOMPARE(upload.files().at(0).name, QStringLiteral("b.txt"));
    }

    void clearFiles()
    {
        MdUpload upload;
        upload.addFile(QStringLiteral("x"));
        upload.addFile(QStringLiteral("y"));
        upload.clearFiles();
        QCOMPARE(upload.fileCount(), 0);
    }

    void childWidgetsExist()
    {
        MdUpload upload;
        upload.addFile(QStringLiteral("photo.png"), 1024);
        upload.resize(360, 56);
        upload.show();
        QVERIFY(QTest::qWaitForWindowExposed(&upload));

        auto *remove = upload.findChild<MdIconButton *>(
            QStringLiteral("md-upload-remove"));
        QVERIFY2(remove != nullptr, "every row must host a remove IconButton");
        auto *bar = upload.findChild<MdProgressIndicator *>(
            QStringLiteral("md-upload-progress"));
        QVERIFY2(bar != nullptr, "every row must host a ProgressIndicator");
    }

    void setFilesRoundTrip()
    {
        MdUpload upload;
        QList<MdUploadFile> files;
        MdUploadFile a;
        a.name = QStringLiteral("one.bin");
        a.progress = 0.25;
        a.transferring = true;
        files.append(a);

        QSignalSpy spy(&upload, &MdUpload::filesChanged);
        upload.setFiles(files);
        QCOMPARE(upload.fileCount(), 1);
        QCOMPARE(spy.count(), 1);

        upload.setFiles(files);
        QCOMPARE(spy.count(), 1);
    }

    void nonBlankRender()
    {
        MdUpload upload;
        upload.addFile(QStringLiteral("design.fig"), 4096);
        upload.addFile(QStringLiteral("notes.md"), 512);
        upload.setFileProgress(0, 0.4, true);
        upload.resize(360, 112);
        upload.show();
        QVERIFY(QTest::qWaitForWindowExposed(&upload));

        const QImage image = renderWidget(upload);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdUpload must paint its list rows");
    }
};

QTEST_MAIN(TestMd3Upload)
#include "TestMd3Upload.moc"
