#ifndef GALLERY_PAGES_H
#define GALLERY_PAGES_H

// Every gallery page. Grouped in one header so the example stays easy to read
// as a whole; the implementations are split across two translation units.

#include "GalleryPage.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QVariant>
#include <QtCore/QVector>

namespace gallery {

/// What the library is, what is done, and what the current environment is.
class OverviewPage : public GalleryPage
{
    Q_OBJECT
public:
    using GalleryPage::GalleryPage;
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;
};

/// All colour roles, in both modes, plus the dynamic-colour controls.
class ColourPage : public GalleryPage
{
    Q_OBJECT
public:
    using GalleryPage::GalleryPage;
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;
};

/// The 15 baseline + 15 emphasized type styles, with the script-category
/// line-height behaviour made visible.
class TypePage : public GalleryPage
{
    Q_OBJECT
public:
    using GalleryPage::GalleryPage;
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;
};

/// Shape scale, shape morph and the tonal elevation levels.
class SurfacePage : public GalleryPage
{
    Q_OBJECT
public:
    explicit SurfacePage(QWidget *parent = nullptr);
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<QRectF> m_sliderTracks;
    bool m_dragging = false;
    /// 0..1 morph position for the shape-morph demo.
    qreal m_morph = 0.35;
};

/// Easing curves, duration tokens and the six Expressive springs.
class MotionPage : public GalleryPage
{
    Q_OBJECT
public:
    explicit MotionPage(QWidget *parent = nullptr);
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QElapsedTimer m_clock;
};

/// State layers, the press ripple and the focus indicator, all interactive.
class InteractionPage : public GalleryPage
{
    Q_OBJECT
public:
    explicit InteractionPage(QWidget *parent = nullptr);
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QRectF targetAt(int index) const;
    int targetAtPosition(const QPointF &position) const;

    /// One interactive surface: state layer, ripple and focus ring together.
    struct Target
    {
        QRectF rect;
        QString label;
        bool hovered = false;
        bool pressed = false;
        QString icon;
    };
    QVector<Target> m_targets;
    QVector<md::MdRippleController *> m_ripples;
    md::MdFocusRingController *m_focusRing = nullptr;
    int m_pressedIndex = -1;
    int m_hoveredIndex = -1;
    int m_focusedIndex = 0;
    bool m_showFocus = false;
};

/// The bundled icon back ends, with the four Material Symbols axes.
class IconPage : public GalleryPage
{
    Q_OBJECT
public:
    using GalleryPage::GalleryPage;
    QString title() const override;
    QString slug() const override;
    QString subtitle() const override;
    void build(GalleryContext &context) override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct Chip
    {
        QRectF rect;
        QVariant value;
    };
    /// One row of preset chips per axis, hit-tested on click.
    QVector<QVector<Chip>> m_axisRows;
    md::MdIconStyle m_style;
    md::MdIconSet m_set = md::MdIconSet::Auto;
};

// ---------------------------------------------------------------------------
// Component pages
// ---------------------------------------------------------------------------
//
// One per Stage 1 component family, added in the order the families land.
// Declared as factories rather than classes so the page header does not have to
// grow a class per component: the page's own type stays private to its
// translation unit, which is where its bespoke state belongs.

/// Actions — Buttons (all five styles x five sizes x two shapes).
GalleryPage *createButtonPage();

/// Actions — Button groups (two variants x five sizes x four selection modes).
GalleryPage *createButtonGroupPage();

/// Actions — Icon buttons (four styles x five sizes x three tracks x toggle).
GalleryPage *createIconButtonPage();

/// Actions — FABs (four colour sets x three sizes x lowered/raised).
GalleryPage *createFabPage();

/// Actions — Extended FABs (six colour sets x three sizes x lowered/raised).
GalleryPage *createExtendedFabPage();

/// Actions — FAB menus (three colour groups, up to six staggered items).
GalleryPage *createFabMenuPage();

/// Actions — Split buttons (button-family colour rows, five sizes, inner-corner morph).
GalleryPage *createSplitButtonPage();

/// Actions — Segmented buttons (one outlined set, single/multi choice, check scale-in).
GalleryPage *createSegmentedButtonPage();

/// Communication — Badges (dot and content forms, anchored over icon buttons).
GalleryPage *createBadgePage();

/// Communication — Progress indicators (linear + circular, determinate/indeterminate/four-color).
GalleryPage *createProgressIndicatorPage();
GalleryPage *createLoadingIndicatorPage();
GalleryPage *createSnackbarPage();
GalleryPage *createTooltipPage();
GalleryPage *createCardPage();
GalleryPage *createDialogPage();
GalleryPage *createBottomSheetPage();
GalleryPage *createSideSheetPage();
GalleryPage *createCarouselPage();
GalleryPage *createDividerPage();
GalleryPage *createListPage();

/// Navigation — App bars (`md.comp.app-bar.*` and `md.comp.bottom-app-bar.*`).
GalleryPage *createAppBarPage();

} // namespace gallery

#endif // GALLERY_PAGES_H
