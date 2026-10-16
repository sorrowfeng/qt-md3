// TestMd3Avatar — Stage 2 Qt extension (Appendix D, P1).
//
// MD3 has no standalone Avatar family; this suite pins the promoted list
// leading-avatar semantics the header documents:
//
//   * surface-variant background / on-surface-variant content, with colour
//     overrides for callers that need a different pair;
//   * ShapeCorner::Full resolved against the widget box (circle for a
//     square);
//   * initials-or-icon content, and initialsFrom() as the name -> initials
//     helper;
//   * non-blank render smoke.

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdAvatar.h"

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

} // namespace

class TestMd3Avatar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        MdTheme::instance().setSeedColor(QColor(0x6750A4));
    }

    void defaultState()
    {
        MdAvatar avatar;
        QVERIFY(avatar.text().isEmpty());
        QVERIFY(avatar.iconName().isEmpty());
        QCOMPARE(avatar.corner(), ShapeCorner::Full);
        QCOMPARE(avatar.avatarSize(), 40);
        QCOMPARE(avatar.sizeHint(), QSize(40, 40));
    }

    void tokenColors()
    {
        MdAvatar avatar;
        QCOMPARE(avatar.resolvedBackgroundColor(),
                 MdTheme::instance().color(ColorRole::SurfaceVariant));
        QCOMPARE(avatar.resolvedContentColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));

        avatar.setBackgroundColor(QColor(1, 2, 3));
        avatar.setContentColor(QColor(4, 5, 6));
        QCOMPARE(avatar.resolvedBackgroundColor(), QColor(1, 2, 3));
        QCOMPARE(avatar.resolvedContentColor(), QColor(4, 5, 6));
    }

    void initialsFromNames()
    {
        QCOMPARE(MdAvatar::initialsFrom(QStringLiteral("Alice Lee")), QStringLiteral("AL"));
        QCOMPARE(MdAvatar::initialsFrom(QStringLiteral("alice")), QStringLiteral("A"));
        QCOMPARE(MdAvatar::initialsFrom(QStringLiteral("Ada Lovelace Byron")), QStringLiteral("AB"));
        QVERIFY(MdAvatar::initialsFrom(QString()).isEmpty());
    }

    void textAndIconProperties()
    {
        MdAvatar avatar;
        QSignalSpy textSpy(&avatar, &MdAvatar::textChanged);
        QSignalSpy iconSpy(&avatar, &MdAvatar::iconNameChanged);

        avatar.setText(QStringLiteral("AL"));
        QCOMPARE(avatar.text(), QStringLiteral("AL"));
        QCOMPARE(textSpy.count(), 1);

        avatar.setIconName(QStringLiteral("person"));
        QCOMPARE(avatar.iconName(), QStringLiteral("person"));
        QCOMPARE(iconSpy.count(), 1);
    }

    void sizeAndCornerAreLive()
    {
        MdAvatar avatar;
        QSignalSpy sizeSpy(&avatar, &MdAvatar::avatarSizeChanged);
        QSignalSpy cornerSpy(&avatar, &MdAvatar::cornerChanged);

        avatar.setAvatarSize(64);
        QCOMPARE(avatar.avatarSize(), 64);
        QCOMPARE(sizeSpy.count(), 1);

        // Clamped into [16, 256].
        avatar.setAvatarSize(1);
        QCOMPARE(avatar.avatarSize(), 16);
        avatar.setAvatarSize(9999);
        QCOMPARE(avatar.avatarSize(), 256);

        avatar.setCorner(ShapeCorner::Medium);
        QCOMPARE(avatar.corner(), ShapeCorner::Medium);
        QCOMPARE(cornerSpy.count(), 1);
    }

    void nonBlankRender()
    {
        MdAvatar avatar(QStringLiteral("AL"));
        avatar.resize(40, 40);
        avatar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&avatar));

        const QImage image = renderWidget(avatar);
        QVERIFY2(anyPixelPainted(image), "MdAvatar must paint its disc");
    }

    void iconRender()
    {
        MdAvatar avatar;
        avatar.setIconName(QStringLiteral("person"));
        avatar.resize(40, 40);
        avatar.show();
        QVERIFY(QTest::qWaitForWindowExposed(&avatar));

        const QImage image = renderWidget(avatar);
        QVERIFY2(anyPixelPainted(image), "MdAvatar must paint its icon disc");
    }
};

QTEST_MAIN(TestMd3Avatar)
#include "TestMd3Avatar.moc"
