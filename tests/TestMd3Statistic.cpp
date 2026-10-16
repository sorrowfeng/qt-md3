// TestMd3Statistic — Stage 2 Qt extension (Appendix D, P2).
//
// The Display/Headline value widget: property defaults, NOTIFY, formatting,
// prefix/suffix/trend, theme colours, and a non-blank render.

#include "TestMd3Common.h"

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "widgets/MdStatistic.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

class TestMd3Statistic : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        qRegisterMetaType<MdStatisticTrend>("md::MdStatisticTrend");
        qRegisterMetaType<TypeStyle>("md::TypeStyle");
    }

    void defaultState()
    {
        MdStatistic stat;
        QCOMPARE(stat.value(), 0.0);
        QCOMPARE(stat.precision(), 0);
        QCOMPARE(stat.title(), QString());
        QCOMPARE(stat.prefix(), QString());
        QCOMPARE(stat.suffix(), QString());
        QCOMPARE(stat.valueStyle(), TypeStyle::DisplaySmall);
        QCOMPARE(stat.trend(), MdStatisticTrend::None);
        QCOMPARE(stat.trendText(), QString());
    }

    void formattedValue()
    {
        MdStatistic stat;
        stat.setValue(1234.5678);
        QCOMPARE(stat.formattedValue(), QStringLiteral("1235"));
        stat.setPrecision(2);
        QCOMPARE(stat.formattedValue(), QStringLiteral("1234.57"));
        stat.setPrecision(0);
        stat.setValue(-12.3);
        QCOMPARE(stat.formattedValue(), QStringLiteral("-12"));
    }

    void propertiesNotify()
    {
        MdStatistic stat;
        QSignalSpy valueSpy(&stat, &MdStatistic::valueChanged);
        QSignalSpy precisionSpy(&stat, &MdStatistic::precisionChanged);
        QSignalSpy titleSpy(&stat, &MdStatistic::titleChanged);
        QSignalSpy prefixSpy(&stat, &MdStatistic::prefixChanged);
        QSignalSpy suffixSpy(&stat, &MdStatistic::suffixChanged);
        QSignalSpy styleSpy(&stat, &MdStatistic::valueStyleChanged);
        QSignalSpy trendSpy(&stat, &MdStatistic::trendChanged);
        QSignalSpy trendTextSpy(&stat, &MdStatistic::trendTextChanged);

        stat.setValue(1.0);
        QCOMPARE(valueSpy.count(), 1);
        stat.setPrecision(3);
        QCOMPARE(precisionSpy.count(), 1);
        stat.setTitle(QStringLiteral("Total"));
        QCOMPARE(titleSpy.count(), 1);
        stat.setPrefix(QStringLiteral("$"));
        QCOMPARE(prefixSpy.count(), 1);
        stat.setSuffix(QStringLiteral("k"));
        QCOMPARE(suffixSpy.count(), 1);
        stat.setValueStyle(TypeStyle::HeadlineMedium);
        QCOMPARE(styleSpy.count(), 1);
        stat.setTrend(MdStatisticTrend::Up);
        QCOMPARE(trendSpy.count(), 1);
        stat.setTrendText(QStringLiteral("+12%"));
        QCOMPARE(trendTextSpy.count(), 1);

        // Re-setting the same value must not re-notify.
        stat.setValue(1.0);
        QCOMPARE(valueSpy.count(), 1);
    }

    void trendColours()
    {
        MdStatistic stat;
        QCOMPARE(stat.resolvedTrendColor(),
                 MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        stat.setTrend(MdStatisticTrend::Up);
        QCOMPARE(stat.resolvedTrendColor(),
                 MdTheme::instance().color(ColorRole::Primary));
        stat.setTrend(MdStatisticTrend::Down);
        QCOMPARE(stat.resolvedTrendColor(),
                 MdTheme::instance().color(ColorRole::Error));
    }

    void nonBlankRender()
    {
        MdStatistic stat;
        stat.setTitle(QStringLiteral("Revenue"));
        stat.setPrefix(QStringLiteral("$"));
        stat.setSuffix(QStringLiteral("M"));
        stat.setValue(42.5);
        stat.setPrecision(1);
        stat.setTrend(MdStatisticTrend::Up);
        stat.setTrendText(QStringLiteral("+8%"));
        stat.resize(240, 96);
        QImage image(stat.size(), QImage::Format_ARGB32_Premultiplied);
        const QRgb transparent = qRgba(0, 0, 0, 0);
        image.fill(transparent);
        {
            QPainter painter(&image);
            stat.render(&painter);
        }
        QVERIFY2(mdtest::paintedPixelCount(image, transparent) > 0,
                 "MdStatistic painted nothing");
    }
};

QTEST_MAIN(TestMd3Statistic)
#include "TestMd3Statistic.moc"
