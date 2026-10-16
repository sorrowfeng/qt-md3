#ifndef MD_SCAFFOLD_H
#define MD_SCAFFOLD_H

// MdScaffold — Stage 2 Qt extension (Appendix D, P1), the page skeleton.
//
// Semantic anchor (appendix D): "组合 App bar + Navigation + FAB 的页面骨架".
// MD3 has no Scaffold family of its own; the skeleton is the layout every
// Material app page already implies, made into one widget so the gallery,
// the showcase pages and downstream apps stop re-deriving it:
//
//   top        an optional Top app bar (md.comp.app-bar.*) spanning the
//              leading edge
//   start      an optional Navigation rail / drawer column
//   end        an optional trailing side-sheet / inspector column
//   bottom     an optional Navigation bar
//   body       the page content, filling whatever is left
//   fab        an optional FAB overlaid on the body's trailing-bottom corner
//              (md.comp.fab.*), or wherever `fabPlacement` puts it
//
// Nothing is painted by the scaffold itself except the body background
// (`md.sys.color.surface`) and the FAB's placement shadow reservation. The
// slots are caller-owned widgets — the scaffold lays them out and never
// restyles them. That is deliberate: Stage 1 already owns every pixel of
// the app bar / nav / FAB families, and a second painter here would drift.
//
// Placement follows the official layout guidance:
//   * the top app bar is full-width along the top
//   * a start rail/drawer sits under the app bar, on the leading edge
//   * a bottom navigation bar sits along the bottom, above the body
//   * the FAB floats over the body, 16 px inset from the trailing and
//     (bottom-nav) bottom edges
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

/// Where the FAB sits inside the body area.
enum class MdFabPlacement {
    /// Trailing edge, 16 px above the body's bottom (or above a bottom nav).
    EndFloat,
    /// Leading edge, 16 px above the body's bottom.
    StartFloat,
    /// Centred along the body's bottom edge (the bottom-app-bar style).
    BottomCenter,
    /// Centred on the body's trailing edge, half-overlapping a start rail.
    EndDocked,
    Count,
};

/// Which edge the navigation widget occupies.
enum class MdNavPlacement {
    None,
    /// Under the app bar, along the leading edge (rail / drawer).
    Start,
    /// Along the trailing edge (end rail / inspector).
    End,
    /// Along the bottom edge (navigation bar).
    Bottom,
    Count,
};

class QT_MD3_EXPORT MdScaffold : public QWidget
{
    Q_OBJECT

    /// The Top app bar slot. Null when the page has none.
    Q_PROPERTY(QWidget *appBar READ appBar WRITE setAppBar NOTIFY appBarChanged)
    /// The navigation slot (bar / rail / drawer). Null when the page has none.
    Q_PROPERTY(QWidget *navigation READ navigation WRITE setNavigation NOTIFY navigationChanged)
    /// Which edge `navigation` occupies.
    Q_PROPERTY(md::MdNavPlacement navPlacement READ navPlacement WRITE setNavPlacement NOTIFY
                   navPlacementChanged)
    /// The optional trailing column (side sheet). Null when the page has none.
    Q_PROPERTY(QWidget *endPane READ endPane WRITE setEndPane NOTIFY endPaneChanged)
    /// The page body. Null until the caller installs one.
    Q_PROPERTY(QWidget *body READ body WRITE setBody NOTIFY bodyChanged)
    /// The floating action button. Null when the page has none.
    Q_PROPERTY(QWidget *fab READ fab WRITE setFab NOTIFY fabChanged)
    /// Where the FAB sits inside the body area.
    Q_PROPERTY(md::MdFabPlacement fabPlacement READ fabPlacement WRITE setFabPlacement NOTIFY
                   fabPlacementChanged)
    /// Inset from the body edge used when placing the FAB. Logical px.
    Q_PROPERTY(int fabMargin READ fabMargin WRITE setFabMargin NOTIFY fabMarginChanged)

public:
    explicit MdScaffold(QWidget *parent = nullptr);
    ~MdScaffold() override;

    // --- slots ---------------------------------------------------------------
    QWidget *appBar() const { return m_appBar; }
    void setAppBar(QWidget *widget);

    QWidget *navigation() const { return m_navigation; }
    void setNavigation(QWidget *widget, MdNavPlacement placement = MdNavPlacement::Start);

    MdNavPlacement navPlacement() const { return m_navPlacement; }
    void setNavPlacement(MdNavPlacement placement);

    QWidget *endPane() const { return m_endPane; }
    void setEndPane(QWidget *widget);

    QWidget *body() const { return m_body; }
    void setBody(QWidget *widget);

    QWidget *fab() const { return m_fab; }
    void setFab(QWidget *widget);

    MdFabPlacement fabPlacement() const { return m_fabPlacement; }
    void setFabPlacement(MdFabPlacement placement);

    int fabMargin() const { return m_fabMargin; }
    void setFabMargin(int margin);

    /// The body area currently available after the app bar / nav / end pane
    /// are subtracted, in widget coordinates. Useful for overlay children.
    QRect bodyRect() const;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void appBarChanged(QWidget *appBar);
    void navigationChanged(QWidget *navigation);
    void navPlacementChanged(md::MdNavPlacement placement);
    void endPaneChanged(QWidget *endPane);
    void bodyChanged(QWidget *body);
    void fabChanged(QWidget *fab);
    void fabPlacementChanged(md::MdFabPlacement placement);
    void fabMarginChanged(int margin);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void layoutSlots();
    void adopt(QWidget *widget);
    void release(QWidget *widget);

    QWidget *m_appBar = nullptr;
    QWidget *m_navigation = nullptr;
    QWidget *m_endPane = nullptr;
    QWidget *m_body = nullptr;
    QWidget *m_fab = nullptr;
    MdNavPlacement m_navPlacement = MdNavPlacement::Start;
    MdFabPlacement m_fabPlacement = MdFabPlacement::EndFloat;
    int m_fabMargin = 16;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdFabPlacement)
Q_DECLARE_METATYPE(md::MdNavPlacement)

#endif // MD_SCAFFOLD_H
