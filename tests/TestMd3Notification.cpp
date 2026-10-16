// TestMd3Notification — Stage 2 Qt extension (Appendix D, P2).
//
// Pins the persistent snackbar variant:
//   * message / actionLabel / hasDismissAction / actionOnNewLine properties;
//   * dismiss is on by default (a persistent surface must be dismissible);
//   * there is no auto-dismiss timer — the notification stays until dismiss();
//   * the visual is a real MdSnackbar on the snackbar token set.

#include "TestMd3Common.h"
#include "core/MdTheme.h"
#include "widgets/MdNotification.h"
#include "widgets/MdSnackbar.h"

#include <QtGui/QImage>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Notification : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdNotification notification;
        QVERIFY(notification.message().isEmpty());
        QVERIFY(notification.actionLabel().isEmpty());
        // Persistent by contract: the close icon is on by default.
        QVERIFY(notification.hasDismissAction());
        QVERIFY(!notification.isActionOnNewLine());
        QVERIFY(notification.snackbar() != nullptr);
    }

    void propertiesNotify()
    {
        MdNotification notification;
        QSignalSpy messageSpy(&notification, &MdNotification::messageChanged);
        QSignalSpy actionSpy(&notification, &MdNotification::actionLabelChanged);
        QSignalSpy dismissSpy(&notification, &MdNotification::hasDismissActionChanged);
        QSignalSpy lineSpy(&notification, &MdNotification::actionOnNewLineChanged);

        notification.setMessage(QStringLiteral("Saved"));
        QCOMPARE(messageSpy.count(), 1);

        notification.setActionLabel(QStringLiteral("Undo"));
        QCOMPARE(actionSpy.count(), 1);

        notification.setHasDismissAction(false);
        QCOMPARE(dismissSpy.count(), 1);
        QVERIFY(!notification.hasDismissAction());

        notification.setActionOnNewLine(true);
        QCOMPARE(lineSpy.count(), 1);
        QVERIFY(notification.isActionOnNewLine());
    }

    void dismissIsManualOnly()
    {
        MdNotification notification(QStringLiteral("Build finished"));
        QSignalSpy dismissedSpy(&notification, &MdNotification::dismissed);
        notification.resize(360, 64);
        notification.show();
        QVERIFY(QTest::qWaitForWindowExposed(&notification));

        // Waiting must never auto-dismiss a notification.
        QTest::qWait(200);
        QVERIFY(notification.isVisible());
        QCOMPARE(dismissedSpy.count(), 0);

        notification.dismiss();
        QCOMPARE(dismissedSpy.count(), 1);
        QVERIFY(!notification.isVisible());

        // Idempotent.
        notification.dismiss();
        QCOMPARE(dismissedSpy.count(), 1);
    }

    void actionSignalForwarded()
    {
        MdNotification notification(QStringLiteral("File deleted"));
        notification.setActionLabel(QStringLiteral("Undo"));
        QSignalSpy actionSpy(&notification, &MdNotification::actionClicked);
        notification.resize(360, 64);
        notification.show();
        QVERIFY(QTest::qWaitForWindowExposed(&notification));

        emit notification.snackbar()->actionClicked();
        QCOMPARE(actionSpy.count(), 1);
    }

    void usesSnackbarVisual()
    {
        MdNotification notification(QStringLiteral("Synced"));
        QVERIFY(notification.snackbar() != nullptr);
        QCOMPARE(notification.snackbar()->message(), QStringLiteral("Synced"));
        QCOMPARE(notification.snackbar()->hasDismissAction(), true);

        // The host reports the snackbar's heightForWidth.
        QCOMPARE(notification.heightForWidth(360),
                 notification.snackbar()->heightForWidth(360));
    }

    void renderSmoke()
    {
        MdNotification notification(QStringLiteral("Upload complete"));
        notification.setActionLabel(QStringLiteral("View"));
        notification.resize(360, notification.heightForWidth(360));
        QImage image(notification.width(), notification.height(),
                     QImage::Format_ARGB32);
        image.fill(Qt::white);
        notification.render(&image);
        QVERIFY(mdtest::paintedPixelCount(image, qRgb(255, 255, 255)) > 0);
    }
};

QTEST_MAIN(TestMd3Notification)
#include "TestMd3Notification.moc"
