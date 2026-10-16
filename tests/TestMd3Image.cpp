// TestMd3Image — Stage 2 Qt extension (Appendix D, P2).
//
// Image tile + full-screen preview: property defaults, NOTIFY, fit modes,
// shape corners, preview open/close, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdImage.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

QPixmap samplePixmap()
{
    QPixmap px(64, 48);
    px.fill(Qt::red);
    QPainter p(&px);
    p.fillRect(8, 8, 20, 20, Qt::white);
    return px;
}

} // namespace

class TestMd3Image : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdImageFit>("md::MdImageFit");
        qRegisterMetaType<ShapeCorner>("md::ShapeCorner");
    }

    void defaultState()
    {
        MdImage image;
        QVERIFY(image.pixmap().isNull());
        QCOMPARE(image.source(), QString());
        QCOMPARE(image.fit(), MdImageFit::Cover);
        QCOMPARE(image.shape(), ShapeCorner::Medium);
        QCOMPARE(image.isPreviewEnabled(), true);
        QCOMPARE(image.altText(), QString());
        QCOMPARE(image.isPreviewOpen(), false);
    }

    void propertiesNotify()
    {
        MdImage image;
        QSignalSpy pixmapSpy(&image, &MdImage::pixmapChanged);
        QSignalSpy sourceSpy(&image, &MdImage::sourceChanged);
        QSignalSpy fitSpy(&image, &MdImage::fitChanged);
        QSignalSpy shapeSpy(&image, &MdImage::shapeChanged);
        QSignalSpy enabledSpy(&image, &MdImage::previewEnabledChanged);
        QSignalSpy altSpy(&image, &MdImage::altTextChanged);

        image.setPixmap(samplePixmap());
        QCOMPARE(pixmapSpy.count(), 1);
        image.setSource(QStringLiteral("a.png"));
        QCOMPARE(sourceSpy.count(), 1);
        image.setFit(MdImageFit::Contain);
        QCOMPARE(fitSpy.count(), 1);
        image.setShape(ShapeCorner::Large);
        QCOMPARE(shapeSpy.count(), 1);
        image.setPreviewEnabled(false);
        QCOMPARE(enabledSpy.count(), 1);
        image.setAltText(QStringLiteral("A photo"));
        QCOMPARE(altSpy.count(), 1);
    }

    void themeColours()
    {
        MdImage image;
        QCOMPARE(image.resolvedBackgroundColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainerLowest));
        QCOMPARE(image.resolvedScrimColor(),
                 MdTheme::instance().color(ColorRole::Scrim));
    }

    void previewOpenClose()
    {
        MdImage image;
        image.setPixmap(samplePixmap());
        image.resize(120, 90);
        QSignalSpy openedSpy(&image, &MdImage::previewOpened);
        QSignalSpy closedSpy(&image, &MdImage::previewClosed);

        image.openPreview();
        QCOMPARE(openedSpy.count(), 1);
        QVERIFY(image.isPreviewOpen());

        image.closePreview();
        QVERIFY(!image.isPreviewOpen());
    }

    void previewDisabledIsNoop()
    {
        MdImage image;
        image.setPixmap(samplePixmap());
        image.setPreviewEnabled(false);
        QSignalSpy openedSpy(&image, &MdImage::previewOpened);
        image.openPreview();
        QCOMPARE(openedSpy.count(), 0);
    }

    void nonBlankRender()
    {
        MdImage image;
        image.setPixmap(samplePixmap());
        image.resize(120, 90);
        QImage buffer(image.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        buffer.fill(transparent);
        {
            QPainter painter(&buffer);
            image.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(buffer, transparent) > 0,
                 "MdImage painted nothing");
    }
};

QTEST_MAIN(TestMd3Image)
#include "TestMd3Image.moc"
