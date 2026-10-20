// TestMd3Popover — Stage 2 Qt extension (Appendix D, P2).
//
// The Menu-semantics anchored surface, plus the MdPopconfirm action variant:
// property defaults, NOTIFY, open/close, confirm/cancel, non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdButton.h"
#include "widgets/MdPopconfirm.h"
#include "widgets/MdPopover.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

using namespace md;

class TestMd3Popover : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<ShapeCorner>("md::ShapeCorner");
    }

    void defaultState()
    {
        MdPopover pop;
        QCOMPARE(pop.title(), QString());
        QVERIFY(!pop.isOpen());
        QCOMPARE(pop.shape(), ShapeCorner::ExtraSmall);
        QCOMPARE(pop.openProgress(), 1.0);
    }

    void propertiesNotify()
    {
        MdPopover pop;
        QSignalSpy titleSpy(&pop, &MdPopover::titleChanged);
        QSignalSpy shapeSpy(&pop, &MdPopover::shapeChanged);
        QSignalSpy openSpy(&pop, &MdPopover::openChanged);

        pop.setTitle(QStringLiteral("Details"));
        QCOMPARE(titleSpy.count(), 1);
        pop.setShape(ShapeCorner::Medium);
        QCOMPARE(shapeSpy.count(), 1);

        pop.setOpen(true);
        QCOMPARE(openSpy.count(), 1);
        QVERIFY(pop.isOpen());
        pop.setOpen(true);
        QCOMPARE(openSpy.count(), 1);
    }

    void surfaceColours()
    {
        MdPopover pop;
        QCOMPARE(pop.resolvedSurfaceColor(),
                 MdTheme::instance().color(ColorRole::SurfaceContainer));
        QCOMPARE(pop.resolvedTitleColor(),
                 MdTheme::instance().color(ColorRole::OnSurface));
    }

    void contentWidgetIsHosted()
    {
        MdPopover pop;
        auto *inner = new QWidget;
        pop.setContentWidget(inner);
        QCOMPARE(pop.contentWidget(), inner);
        QCOMPARE(inner->parent(), &pop);
    }

    void popconfirmDefaults()
    {
        MdPopconfirm confirm;
        QCOMPARE(confirm.text(), QString());
        QVERIFY(confirm.confirmButton() != nullptr);
        QVERIFY(confirm.cancelButton() != nullptr);
    }

    void popconfirmConfirmCancel()
    {
        MdPopconfirm confirm;
        QSignalSpy confirmedSpy(&confirm, &MdPopconfirm::confirmed);
        QSignalSpy cancelledSpy(&confirm, &MdPopconfirm::cancelled);
        QSignalSpy textSpy(&confirm, &MdPopconfirm::textChanged);

        confirm.setText(QStringLiteral("Delete this?"));
        QCOMPARE(textSpy.count(), 1);
        QCOMPARE(confirm.text(), QStringLiteral("Delete this?"));

        confirm.confirmButton()->click();
        QCOMPARE(confirmedSpy.count(), 1);
        QCOMPARE(cancelledSpy.count(), 0);
    }

    void nonBlankRender()
    {
        MdPopover pop;
        pop.setTitle(QStringLiteral("Preview"));
        auto *inner = new QWidget;
        inner->setMinimumSize(160, 48);
        pop.setContentWidget(inner);
        pop.resize(220, 120);
        QImage image(pop.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            pop.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdPopover painted nothing");
    }

    /// Black-edge / residue gate: the popover surface must never leave an
    /// opaque near-black ring on a transparent backing store — not on the
    /// first open frame (scale 0.8), not settled, not mid-close.
    void noOpaqueBlackEdge()
    {
        auto analyse = [](const QImage &src, const char *label) {
            const QImage image = src.convertToFormat(QImage::Format_ARGB32);
            const int w = image.width();
            const int h = image.height();
            // Only the outer 6 px ring counts as "edge". Title glyphs sit
            // inside that band and are legitimately near-black on a light
            // surface — they must not trip the gate.
            int edgeBlack = 0;
            int transparent = 0;
            int opaque = 0;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const QRgb p = image.pixel(x, y);
                    const int a = qAlpha(p);
                    if (a < 8) {
                        ++transparent;
                        continue;
                    }
                    ++opaque;
                    const bool edge = (x < 6 || y < 6 || x >= w - 6 || y >= h - 6);
                    if (edge && a > 200 && qRed(p) < 40 && qGreen(p) < 40 && qBlue(p) < 40) {
                        ++edgeBlack;
                    }
                }
            }
            qInfo("%s: %dx%d transparent=%d opaque=%d edgeBlack=%d",
                  label, w, h, transparent, opaque, edgeBlack);
            return edgeBlack;
        };

        MdTheme::instance().setThemeMode(ThemeMode::Light);
        auto *host = new QWidget;
        host->resize(360, 240);
        auto *trigger = new QPushButton(QStringLiteral("Open"), host);
        trigger->move(20, 20);
        auto *body = new QWidget;
        body->setMinimumSize(180, 64);
        auto *pop = new MdPopover(host);
        pop->setTitle(QStringLiteral("Details"));
        pop->setContentWidget(body);
        pop->setShape(ShapeCorner::Small);
        host->show();
        QVERIFY(QTest::qWaitForWindowExposed(host));

        const QRect anchor(trigger->mapToGlobal(QPoint(0, 0)), trigger->size());
        pop->popup(anchor);

        // First painted frame is mid-flight (scale < 1). The area outside the
        // scaled surface must be transparent, not opaque black.
        QImage flight = pop->grab().toImage();
        const int flightBlack = analyse(flight, "open-t0");
        const QRgb corner = flight.pixel(1, 1);
        qInfo("open-t0 corner(1,1)=A%dR%dG%dB%d", qAlpha(corner), qRed(corner),
              qGreen(corner), qBlue(corner));
        QVERIFY2(qAlpha(corner) < 8 || (qRed(corner) + qGreen(corner) + qBlue(corner)) > 80,
                 qPrintable(QStringLiteral(
                     "first open frame has an opaque black corner (A%1 R%2 G%3 B%4)")
                                .arg(qAlpha(corner))
                                .arg(qRed(corner))
                                .arg(qGreen(corner))
                                .arg(qBlue(corner))));

        QTRY_VERIFY_WITH_TIMEOUT(!pop->grab().isNull(), 200);
        QTest::qWait(200);
        QImage settled = pop->grab().toImage();
        const int settledBlack = analyse(settled, "open-settled");
        QVERIFY2(settledBlack == 0,
                 qPrintable(QStringLiteral("settled popover has %1 opaque black pixels")
                                .arg(settledBlack)));

        pop->hide();
        QTest::qWait(250);
        QVERIFY2(!pop->isVisible(), "popover still mapped after close");
        QImage closed = pop->grab().toImage();
        analyse(closed, "close-settled");

        // Re-open once: residue often only appears on the second flight.
        pop->popup(anchor);
        QTest::qWait(200);
        QImage reopen = pop->grab().toImage();
        const int reopenBlack = analyse(reopen, "reopen-settled");
        QVERIFY2(reopenBlack == 0,
                 qPrintable(QStringLiteral("reopen popover has %1 opaque black pixels")
                                .arg(reopenBlack)));
        pop->hide();
        QTest::qWait(250);
        QVERIFY(!pop->isVisible());

        delete host;
        Q_UNUSED(flightBlack);
    }
};

QTEST_MAIN(TestMd3Popover)
#include "TestMd3Popover.moc"
