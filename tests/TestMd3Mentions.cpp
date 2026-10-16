// TestMd3Mentions — Stage 2 Qt extension (Appendix D, P2).
//
// Text field + Menu popup: property defaults, NOTIFY, trigger query,
// commit insertion, popup visibility, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTypes.h"
#include "widgets/MdMentions.h"
#include "widgets/MdTextField.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Mentions : public QObject
{
    Q_OBJECT

private slots:
    void defaultState()
    {
        MdMentions m;
        QCOMPARE(m.text(), QString());
        QCOMPARE(m.trigger(), QChar(QLatin1Char('@')));
        QVERIFY(m.items().isEmpty());
        QCOMPARE(m.isPopupVisible(), false);
        QCOMPARE(m.maxVisible(), 6);
    }

    void propertiesNotify()
    {
        MdMentions m;
        QSignalSpy textSpy(&m, &MdMentions::textChanged);
        QSignalSpy labelSpy(&m, &MdMentions::labelTextChanged);
        QSignalSpy placeholderSpy(&m, &MdMentions::placeholderTextChanged);
        QSignalSpy triggerSpy(&m, &MdMentions::triggerChanged);
        QSignalSpy itemsSpy(&m, &MdMentions::itemsChanged);
        QSignalSpy maxSpy(&m, &MdMentions::maxVisibleChanged);

        m.setText(QStringLiteral("hello"));
        QVERIFY(textSpy.count() >= 1);
        m.setLabelText(QStringLiteral("To"));
        QCOMPARE(labelSpy.count(), 1);
        m.setPlaceholderText(QStringLiteral("Write…"));
        QCOMPARE(placeholderSpy.count(), 1);
        m.setTrigger(QLatin1Char('#'));
        QCOMPARE(triggerSpy.count(), 1);
        m.setItems(QStringList() << QStringLiteral("Ada"));
        QCOMPARE(itemsSpy.count(), 1);
        m.setMaxVisible(3);
        QCOMPARE(maxSpy.count(), 1);
    }

    void queryAfterTrigger()
    {
        MdMentions m;
        m.setItems(QStringList() << QStringLiteral("Ada") << QStringLiteral("Alan")
                                 << QStringLiteral("Grace"));
        m.setText(QStringLiteral("ping @Ad"));
        QCOMPARE(m.currentQuery(), QStringLiteral("Ad"));
        QCOMPARE(m.filteredItems(), QStringList() << QStringLiteral("Ada"));

        m.setText(QStringLiteral("no trigger here"));
        QCOMPARE(m.currentQuery(), QString());
    }

    void commitInsertsMention()
    {
        MdMentions m;
        m.setItems(QStringList() << QStringLiteral("Ada"));
        m.setText(QStringLiteral("hi @Ad"));
        QSignalSpy mentionedSpy(&m, &MdMentions::mentioned);
        m.commit(QStringLiteral("Ada"));
        QCOMPARE(mentionedSpy.count(), 1);
        QCOMPARE(mentionedSpy.at(0).at(0).toString(), QStringLiteral("Ada"));
        QVERIFY(m.text().contains(QStringLiteral("@Ada")));
    }

    void fieldIsRealTextField()
    {
        MdMentions m;
        QVERIFY(m.field() != nullptr);
        QVERIFY(qobject_cast<MdTextField *>(m.field()) != nullptr);
    }

    void nonBlankRender()
    {
        MdMentions m;
        m.setLabelText(QStringLiteral("Message"));
        m.setText(QStringLiteral("Hello @Ada"));
        m.resize(320, 56);
        QImage image(m.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            m.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdMentions painted nothing");
    }
};

QTEST_MAIN(TestMd3Mentions)
#include "TestMd3Mentions.moc"
