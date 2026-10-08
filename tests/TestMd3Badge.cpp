// TestMd3Badge — the badge family against its token sources.
//
// Numbers transcribed from material-web
// tokens/versions/latest/sass/_md-comp-badge.scss (the only material-web fact
// — the component itself is not implemented there), with the behaviour rows
// (anchoring offsets, 4 px label padding, non-interactivity) from androidx
// Compose material3 Badge.kt — the two sources the token header names.

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "core/MdTypes.h"
#include "styles/MdBadgeStyle.h"
#include "widgets/MdBadge.h"
#include "widgets/MdBadgedBox.h"

#include <QtGui/QFontDatabase>
#include <QtGui/QFontMetrics>
#include <QtGui/QImage>
#include <QtGui/QPainter>
#include <QtTest/QtTest>
#include <QtWidgets/QApplication>

using namespace md;

namespace {

/// A fixed 40x40 stand-in for the anchored content.
class AnchorWidget : public QWidget
{
public:
    using QWidget::QWidget;
    QSize sizeHint() const override { return QSize(40, 40); }
    QSize minimumSizeHint() const override { return sizeHint(); }
};

QColor pixelColorAt(MdBadge &badge, const QPointF &position)
{
    QImage image(int(badge.width()), int(badge.height()),
                 QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    badge.render(&painter);
    painter.end();
    return image.convertToFormat(QImage::Format_ARGB32).pixelColor(position.toPoint());
}

} // namespace

class TestMd3Badge : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void theTokenTableMatchesTheExport();
    void theLabelTypeIsLabelSmall();
    void aBadgeIsNeverInteractive();
    void dotFormGeometry();
    void contentFormGeometry();
    void textDecidesTheForm();
    void overridesReachTheTokens();
    void badgedBoxPositionsTheDot();
    void badgedBoxReservesThePillOverhang();
    void renderSmokeBothForms();

private:
    MdBadge *m_dot = nullptr;
    MdBadge *m_count = nullptr;
};

void TestMd3Badge::initTestCase()
{
    MdTheme::instance();

    m_dot = new MdBadge;
    m_dot->resize(m_dot->sizeHint());
    m_dot->show();

    m_count = new MdBadge(QStringLiteral("1"));
    m_count->resize(m_count->sizeHint());
    m_count->show();
}

void TestMd3Badge::cleanup()
{
    // Token overrides are global state; every test that sets one removes it.
    MdComponentTokens::global().remove(QStringLiteral("md.comp.badge.size"));
    MdComponentTokens::global().remove(QStringLiteral("md.comp.badge.large.size"));
    MdComponentTokens::global().remove(QStringLiteral("md.comp.badge.shape"));
    m_dot->componentTokens().clear();
    m_count->componentTokens().clear();
    m_dot->setText(QString());
}

void TestMd3Badge::theTokenTableMatchesTheExport()
{
    // md.comp.badge.* — the dot form.
    const MdBadgeTokens dot = MdBadgeTokens::resolve(false);
    QCOMPARE(dot.size, 6.0);
    QCOMPARE(int(dot.shape), int(ShapeCorner::Full));
    QCOMPARE(int(dot.color), int(ColorRole::Error));

    // md.comp.badge.large.* — the content form, minimum 16 px.
    const MdBadgeTokens large = MdBadgeTokens::resolve(true);
    QCOMPARE(large.size, 6.0);
    QCOMPARE(large.largeSize, 16.0);
    QCOMPARE(int(large.shape), int(ShapeCorner::Full));
    QCOMPARE(int(large.largeShape), int(ShapeCorner::Full));
    QCOMPARE(int(large.largeColor), int(ColorRole::Error));
    QCOMPARE(int(large.largeLabelTextColor), int(ColorRole::OnError));

    // The whole state half of a component token table is absent here: no
    // hover, no focus, no press, no disabled row is published at all.
    // Non-interactivity is the contract, verified in
    // aBadgeIsNeverInteractive().
}

void TestMd3Badge::theLabelTypeIsLabelSmall()
{
    // md.comp.badge.large.label-text.* = the label-small style.
    const MdBadgeTokens tokens = MdBadgeTokens::resolve(true);
    QCOMPARE(int(tokens.largeLabelTextType), int(TypeStyle::LabelSmall));

    const MdTypeStyleSpec spec = MdTypeScale::spec(TypeStyle::LabelSmall);
    QCOMPARE(spec.size, 11.0);
    QCOMPARE(spec.lineHeight, 16.0);
    QCOMPARE(spec.tracking, 0.5);
    QCOMPARE(spec.weight, 500);
}

void TestMd3Badge::aBadgeIsNeverInteractive()
{
    // The export publishes no state rows, so the widget contracts for none of
    // them: no focus, and — the fact MdBadgedBox relies on, straight out of
    // Compose's BadgedBox — clicks pass through the badge.
    QCOMPARE(int(m_dot->focusPolicy()), int(Qt::NoFocus));
    QVERIFY(m_dot->testAttribute(Qt::WA_TransparentForMouseEvents));
    QVERIFY(m_count->testAttribute(Qt::WA_TransparentForMouseEvents));

    // And the anchor container never steals focus from its content either.
    MdBadgedBox box;
    QCOMPARE(int(box.focusPolicy()), int(Qt::NoFocus));
}

void TestMd3Badge::dotFormGeometry()
{
    // The dot is exactly the published 6 px square, centred in the widget.
    QCOMPARE(m_dot->sizeHint(), QSize(6, 6));
    QCOMPARE(m_dot->minimumSizeHint(), QSize(6, 6));
    m_dot->resize(12, 12);
    const MdBadgeStyle::Layout layout =
        MdBadgeStyle::layoutFor(*m_dot, m_dot->tokens());
    QCOMPARE(layout.container, QRectF(3.0, 3.0, 6.0, 6.0));
    QCOMPARE(layout.radius, 3.0);
    // No label in the dot form.
    QVERIFY(!layout.label.isValid());
    QVERIFY(!m_dot->hasContent());
    m_dot->resize(m_dot->sizeHint());
}

void TestMd3Badge::contentFormGeometry()
{
    // The pill is the label width plus the Compose 4 px side padding, at
    // least the published 16 px minimum on both axes.
    const QFont font = MdTypeScale::font(TypeStyle::LabelSmall, TypeEmphasis::Baseline,
                                         MdTheme::instance().scriptCategory());
    const QFontMetrics metrics(font);
    const qreal advance = metrics.horizontalAdvance(QStringLiteral("1"));

    const MdBadgeStyle::Layout layout =
        MdBadgeStyle::layoutFor(*m_count, m_count->tokens());
    QCOMPARE(layout.container.width(), qMax<qreal>(16.0, advance + 8.0));
    QCOMPARE(layout.container.height(), qMax<qreal>(16.0, metrics.height()));
    // corner-full: half the shorter side.
    QCOMPARE(layout.radius,
             qMin(layout.container.width(), layout.container.height()) / 2.0);
    // The label box sits 4 px inside the pill on each side, label-small's
    // line height tall, vertically centred.
    QCOMPARE(layout.label.left(), layout.container.left() + 4.0);
    QCOMPARE(layout.label.width(), layout.container.width() - 8.0);
    QCOMPARE(layout.label.height(), 16.0);
    QCOMPARE(layout.label.center().y(), layout.container.center().y());

    // A wider label widens the pill; "999+" cannot fit a circle.
    m_count->setText(QStringLiteral("999+"));
    const MdBadgeStyle::Layout wide =
        MdBadgeStyle::layoutFor(*m_count, m_count->tokens());
    QVERIFY(wide.container.width() > 16.0);
    QVERIFY(m_count->sizeHint().width() > m_count->sizeHint().height());
    m_count->resize(m_count->sizeHint());

    // ...but the height minimum still holds.
    QCOMPARE(wide.container.height(), qMax<qreal>(16.0, metrics.height()));
}

void TestMd3Badge::textDecidesTheForm()
{
    // One set, two forms, decided by content — there is no variant token.
    QVERIFY(!m_dot->hasContent());
    QCOMPARE(m_dot->sizeHint(), QSize(6, 6));

    m_dot->setText(QStringLiteral("3"));
    QVERIFY(m_dot->hasContent());
    QVERIFY(m_dot->sizeHint().width() >= 16);
    QVERIFY(m_dot->sizeHint().height() >= 16);

    m_dot->setText(QString());
    QVERIFY(!m_dot->hasContent());
    QCOMPARE(m_dot->sizeHint(), QSize(6, 6));
}

void TestMd3Badge::overridesReachTheTokens()
{
    // --- instance-level ---------------------------------------------------
    m_dot->componentTokens().setValue(QStringLiteral("md.comp.badge.size"),
                                      QStringLiteral("8px"));
    QCOMPARE(m_dot->sizeHint(), QSize(8, 8));

    m_count->componentTokens().setValue(QStringLiteral("md.comp.badge.large.size"),
                                        QStringLiteral("20px"));
    // The more qualified large.* key wins over the base one.
    QVERIFY(m_count->sizeHint().height() >= 20);

    // --- application-wide -------------------------------------------------
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.badge.size"),
                                         QStringLiteral("10px"));
    MdBadge fresh;
    QCOMPARE(fresh.sizeHint(), QSize(10, 10));

    // A typo'd shape name is ignored, not coerced. The size override from
    // above is removed first so it cannot answer instead.
    MdComponentTokens::global().remove(QStringLiteral("md.comp.badge.size"));
    MdComponentTokens::global().setValue(QStringLiteral("md.comp.badge.shape"),
                                         QStringLiteral("corner-nonsense"));
    MdBadge typo;
    QCOMPARE(typo.sizeHint(), QSize(6, 6));
}

void TestMd3Badge::badgedBoxPositionsTheDot()
{
    // The dot sits inside the anchor's top-end corner: end edge on the
    // anchor's end edge, top edge on the anchor's top edge (offset 6/6
    // [compose]). Nothing overhangs, so the box is exactly the anchor.
    AnchorWidget *anchor = new AnchorWidget;
    MdBadgedBox box;
    box.setContentWidget(anchor);
    box.resize(box.sizeHint());
    box.show();

    QCOMPARE(box.sizeHint(), QSize(40, 40));
    QCOMPARE(anchor->geometry(), QRect(0, 0, 40, 40));
    QCOMPARE(box.badge()->geometry(), QRect(34, 0, 6, 6));
}

void TestMd3Badge::badgedBoxReservesThePillOverhang()
{
    // The pill's start edge is 12 px inside the anchor's end edge and its
    // bottom edge 14 px below the anchor's top edge (offset 12 / overlap 14
    // [compose]) — so it overhangs by pill-size minus 12 / 14. Compose lets
    // that overlap the surroundings; a Qt child would be clipped, so the box
    // reserves the overhang instead and the anchor keeps its own size.
    AnchorWidget *anchor = new AnchorWidget;
    MdBadgedBox box;
    box.setContentWidget(anchor);
    box.badge()->setText(QStringLiteral("1"));
    box.resize(box.sizeHint());
    box.show();

    const QSize badgeSize = box.badge()->sizeHint();
    const int overhangRight = badgeSize.width() - 12;
    const int overhangTop = badgeSize.height() - 14;
    QVERIFY(overhangRight >= 4);
    QVERIFY(overhangTop >= 2);

    QCOMPARE(box.sizeHint(), QSize(40 + overhangRight, 40 + overhangTop));
    // The anchor keeps its published 40x40, pushed down by the top overhang.
    QCOMPARE(anchor->geometry(), QRect(0, overhangTop, 40, 40));
    // The badge's own top-left lands at the box's top-left, and its end edge
    // is 12 px inside the anchor's end edge.
    QCOMPARE(box.badge()->geometry(),
             QRect(40 - 12, 0, badgeSize.width(), badgeSize.height()));
    QCOMPARE(box.badge()->geometry().right(), 40 - 12 + badgeSize.width() - 1);
    // ...which is exactly the box's right edge minus the reserved overhang.
    QCOMPARE(box.badge()->geometry().right(), box.width() - 1);
}

void TestMd3Badge::renderSmokeBothForms()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Bare offscreen environments (local Windows offscreen runs; the CI
    // containers ship fonts) cannot rasterise the label, so the on-error
    // text colour is legitimately absent.
    if (QFontDatabase().families().isEmpty()) {
        QSKIP("no fonts installed on this platform — label pixels cannot render");
    }
#endif

    // The dot renders as one error-coloured disc.
    m_dot->resize(m_dot->sizeHint());
    const QColor error = MdTheme::instance().color(ColorRole::Error);
    const QColor dotCentre = pixelColorAt(*m_dot, QPointF(3, 3));
    QCOMPARE(dotCentre, error);
    // Outside the 6 px disc: nothing painted. The disc is antialiased and the
    // widget is exactly the disc, so the canvas gets a 3 px transparent frame
    // and the sample is taken on the canvas corner, clear of the AA fringe.
    {
        QImage canvas(12, 12, QImage::Format_ARGB32_Premultiplied);
        canvas.fill(Qt::transparent);
        QPainter painter(&canvas);
        painter.translate(3, 3);
        m_dot->render(&painter);
        painter.end();
        QCOMPARE(canvas.convertToFormat(QImage::Format_ARGB32).pixelColor(0, 0).alpha(), 0);
    }

    // The pill renders error with the label on top: the container colour at
    // the inner edge, on-error text somewhere in the middle band.
    m_count->setText(QStringLiteral("1"));
    m_count->resize(m_count->sizeHint());
    const int h = m_count->height();
    const QColor pillEdge = pixelColorAt(*m_count, QPointF(1, h / 2));
    QCOMPARE(pillEdge, error);

    bool sawTextColour = false;
    const QColor onError = MdTheme::instance().color(ColorRole::OnError);
    for (int x = 0; x < m_count->width(); ++x) {
        if (pixelColorAt(*m_count, QPointF(x, h / 2)) == onError) {
            sawTextColour = true;
            break;
        }
    }
    QVERIFY(sawTextColour);
}

QTEST_MAIN(TestMd3Badge)
#include "TestMd3Badge.moc"
