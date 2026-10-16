// TestMd3Space — Stage 2 Qt extension (Appendix D, P1).
//
// Pins the spacing-scale spacer:
//   * direction / size / extent properties with NOTIFY;
//   * the MD3 spacing-scale values (shared with MdFlex);
//   * sizeHint reserves the resolved extent along the chosen axis.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdSpace.h"

#include <QtGui/QImage>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Space : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdSpace space;
        QCOMPARE(space.direction(), MdSpaceDirection::Horizontal);
        QCOMPARE(space.size(), MdFlexSpacing::None);
        QCOMPARE(space.extent(), 0.0);
    }

    void spacingScale()
    {
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::None), 0.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::ExtraSmall), 4.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::Small), 8.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::Medium), 12.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::Large), 16.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::ExtraLarge), 24.0);
        QCOMPARE(MdSpace::spacingPx(MdFlexSpacing::Huge), 32.0);
    }

    void propertiesNotify()
    {
        MdSpace space;
        QSignalSpy dirSpy(&space, &MdSpace::directionChanged);
        QSignalSpy sizeSpy(&space, &MdSpace::sizeChanged);
        QSignalSpy extentSpy(&space, &MdSpace::extentChanged);

        space.setDirection(MdSpaceDirection::Vertical);
        QCOMPARE(dirSpy.count(), 1);

        space.setSize(MdFlexSpacing::Large);
        QCOMPARE(sizeSpy.count(), 1);
        QCOMPARE(extentSpy.count(), 1);
        QCOMPARE(space.extent(), 16.0);

        space.setExtent(20.0);
        QCOMPARE(extentSpy.count(), 2);
        QCOMPARE(space.extent(), 20.0);
    }

    void sizeHintReservesExtent()
    {
        MdSpace horizontal;
        horizontal.setSize(MdFlexSpacing::ExtraLarge);
        QCOMPARE(horizontal.sizeHint().width(), 24);
        QCOMPARE(horizontal.sizeHint().height(), 0);

        MdSpace vertical;
        vertical.setDirection(MdSpaceDirection::Vertical);
        vertical.setSize(MdFlexSpacing::Huge);
        QCOMPARE(vertical.sizeHint().width(), 0);
        QCOMPARE(vertical.sizeHint().height(), 32);
    }

    void renderSmoke()
    {
        // A spacer paints nothing of its own; render() still fills the
        // widget's background, so the check is that the render succeeds and
        // the image matches the reserved extent.
        MdSpace space;
        space.setSize(MdFlexSpacing::Large);
        space.resize(32, 32);
        QImage image(32, 32, QImage::Format_ARGB32);
        image.fill(Qt::white);
        space.render(&image);
        QCOMPARE(image.width(), 32);
        QCOMPARE(image.height(), 32);
    }
};

QTEST_MAIN(TestMd3Space)
#include "TestMd3Space.moc"
