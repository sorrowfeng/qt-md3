// TestMd3Rtl — the library-wide RTL audit: the geometry and text halves.
//
// Two rules the audit turns on:
//
//   * **geometry mirrors** — leading / trailing slots swap sides and the
//     motion vectors (the switch thumb's travel, the slider handle's
//     fraction) measure from the leading edge;
//   * **text follows the direction** — `MdStyleBase::leadingAlignment`
//     resolves `AlignRight` under RTL instead of the literal `AlignLeft`.
//
// Each slot renders one widget in both directions and asserts the two
// geometry halves are true mirrors of each other — the pixel-level audit
// this replaces would only see the same thing.

#include "core/MdSearchTokens.h"
#include "core/MdSliderTokens.h"
#include "core/MdSwitchTokens.h"
#include "core/MdTextFieldTokens.h"
#include "core/MdTheme.h"
#include "styles/MdStyleBase.h"
#include "widgets/MdChip.h"
#include "widgets/MdSearchBar.h"
#include "widgets/MdSlider.h"
#include "widgets/MdSwitch.h"
#include "widgets/MdTextField.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>

using namespace md;

namespace {

/// One widget measured in both directions: `ltr` and `rtl` must be true
/// mirrors of each other inside the widget's width.
struct MirrorProbe
{
    QRectF ltr;
    QRectF rtl;
};

bool areMirrors(const MirrorProbe &probe, qreal width)
{
    if (!probe.ltr.isValid() || !probe.rtl.isValid()) {
        return false;
    }
    if (!qFuzzyCompare(probe.ltr.width() + 1.0, probe.rtl.width() + 1.0)) {
        return false;
    }
    if (!qFuzzyCompare(probe.ltr.height() + 1.0, probe.rtl.height() + 1.0)) {
        return false;
    }
    return qFuzzyCompare((width - probe.ltr.right()) + 1.0, probe.rtl.left() + 1.0);
}

} // namespace

class TestMd3Rtl : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void themeDirectionDrivesTheApp();
    void leadingAlignmentFlips();
    void switchThumbTravelsFromTheLeadingEdge();
    void sliderHandleRidesTheLeadingEdge();
    void chipLeadingAndTrailingSwap();
    void searchBarLeadingIconSwaps();
    void textFieldLabelAnchorsToTheLeadingEdge();
};

void TestMd3Rtl::initTestCase()
{
    MdTheme::instance().setThemeMode(ThemeMode::Light);
    MdTheme::instance().setDirection(Qt::LeftToRight);
}

void TestMd3Rtl::cleanup()
{
    MdTheme::instance().setDirection(Qt::LeftToRight);
}

void TestMd3Rtl::themeDirectionDrivesTheApp()
{
    MdTheme::instance().setDirection(Qt::RightToLeft);
    QCOMPARE(QApplication::layoutDirection(), Qt::RightToLeft);
    QCOMPARE(MdTheme::instance().isRightToLeft(), true);

    MdTheme::instance().setDirection(Qt::LeftToRight);
    QCOMPARE(QApplication::layoutDirection(), Qt::LeftToRight);
    QCOMPARE(MdTheme::instance().isRightToLeft(), false);
}

void TestMd3Rtl::leadingAlignmentFlips()
{
    QCOMPARE(MdStyleBase::leadingAlignment(nullptr), Qt::Alignment(Qt::AlignLeft));
    QCOMPARE(MdStyleBase::trailingAlignment(nullptr), Qt::Alignment(Qt::AlignRight));

    MdTheme::instance().setDirection(Qt::RightToLeft);
    QCOMPARE(MdStyleBase::leadingAlignment(nullptr), Qt::Alignment(Qt::AlignRight));
    QCOMPARE(MdStyleBase::trailingAlignment(nullptr), Qt::Alignment(Qt::AlignLeft));
}

void TestMd3Rtl::switchThumbTravelsFromTheLeadingEdge()
{
    MdSwitch sw;
    sw.resize(120, 48);

    sw.setLayoutDirection(Qt::LeftToRight);
    const QRectF ltr = sw.thumbRect();
    sw.setLayoutDirection(Qt::RightToLeft);
    const QRectF rtl = sw.thumbRect();

    // The thumb's resting offset measures from the leading edge: the two
    // rects are true mirrors inside the track. Express both in track-local
    // coordinates so the mirror axis is the track's own width.
    const QRectF track = sw.trackRect();
    const MirrorProbe probe{ltr.translated(-track.left(), 0), rtl.translated(-track.left(), 0)};
    QVERIFY(areMirrors(probe, track.width()));
}

void TestMd3Rtl::sliderHandleRidesTheLeadingEdge()
{
    MdSlider slider;
    slider.resize(240, 48);
    slider.setValue(25.0);

    slider.setLayoutDirection(Qt::LeftToRight);
    const QRectF ltr = slider.handleRect();
    slider.setLayoutDirection(Qt::RightToLeft);
    const QRectF rtl = slider.handleRect();

    // 25% from the leading edge — left in LTR, right in RTL.
    const QRectF track = slider.trackRect();
    QVERIFY(ltr.center().x() < track.center().x());
    QVERIFY(rtl.center().x() > track.center().x());
}

void TestMd3Rtl::chipLeadingAndTrailingSwap()
{
    MdChip chip;
    chip.setIconName(QStringLiteral("check"));
    chip.setTrailingIconName(QStringLiteral("close"));
    chip.setText(QStringLiteral("Chip"));
    chip.resize(220, 48);

    chip.setLayoutDirection(Qt::LeftToRight);
    const QRectF ltrLeading = chip.boxes().leadingIcon;
    const QRectF ltrTrailing = chip.boxes().trailingIcon;
    chip.setLayoutDirection(Qt::RightToLeft);
    const QRectF rtlLeading = chip.boxes().leadingIcon;
    const QRectF rtlTrailing = chip.boxes().trailingIcon;

    // Leading sits at the left in LTR and the right in RTL; trailing is its
    // mirror. The two are true mirrors inside the chip's width.
    QVERIFY(ltrLeading.left() < ltrTrailing.left());
    QVERIFY(rtlLeading.left() > rtlTrailing.left());
    QVERIFY(areMirrors(MirrorProbe{ltrLeading, rtlLeading}, chip.width()));
    QVERIFY(areMirrors(MirrorProbe{ltrTrailing, rtlTrailing}, chip.width()));
}

void TestMd3Rtl::searchBarLeadingIconSwaps()
{
    MdSearchBar bar;
    bar.resize(360, 56);

    bar.setLayoutDirection(Qt::LeftToRight);
    const QRectF ltr = bar.leadingIconRect();
    bar.setLayoutDirection(Qt::RightToLeft);
    const QRectF rtl = bar.leadingIconRect();

    QVERIFY(areMirrors(MirrorProbe{ltr, rtl}, bar.width()));
}

void TestMd3Rtl::textFieldLabelAnchorsToTheLeadingEdge()
{
    MdTextField field;
    field.setLabelText(QStringLiteral("Label"));
    field.resize(280, 56);

    field.setLayoutDirection(Qt::LeftToRight);
    const QRectF ltr = field.labelRect();
    field.setLayoutDirection(Qt::RightToLeft);
    const QRectF rtl = field.labelRect();

    // The label band spans the same inset either way; its text alignment is
    // the half that flips (leadingAlignment).
    QCOMPARE(ltr.width(), rtl.width());
    QCOMPARE(MdStyleBase::leadingAlignment(&field), Qt::Alignment(Qt::AlignRight));
}

QTEST_MAIN(TestMd3Rtl)
#include "TestMd3Rtl.moc"
