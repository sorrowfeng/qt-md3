#ifndef MD_GRID_H
#define MD_GRID_H

// MdGrid — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "MD3 responsive grid + Window size classes".
//
// MD3 does not publish a Grid component the way it publishes Buttons; it
// publishes a *layout foundation* — the responsive layout grid and the
// window size classes every adaptive sample switches on. MdGrid is that
// foundation as a Qt container:
//
//   sizeClass   Compact / Medium / Expanded (plus Large / ExtraLarge, which
//               the current M3 foundations also name)
//   columns     how many grid tracks the current class uses
//   gutter      the gap between tracks
//   margin      the inset from the container edges
//
// Breakpoints and the default track counts are transcribed from the M3
// layout foundations (window size classes and the responsive layout grid):
//
//   Compact     width < 600 dp        4 columns
//   Medium      600 dp ≤ w < 840 dp   8 columns
//   Expanded    840 dp ≤ w < 1200 dp  12 columns
//   Large       1200 dp ≤ w < 1600 dp 12 columns
//   ExtraLarge  width ≥ 1600 dp       12 columns
//
// Gutter / margin defaults follow the same page (compact 16/16, medium
// 24/24, expanded 24/24). `md.sys.layout.*` is not a published token family
// in the material-web export, so these numbers are recorded here and in
// docs/porting-todo.md rather than invented silently — the same treatment
// MdFlex gives the spacing scale.
//
// Children are placed left-to-right (right-to-left in RTL) onto the tracks;
// a child's `gridSpan` property (set through setItemSpan) stretches it across
// several tracks. MdGrid paints nothing of its own.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QHash>
#include <QtWidgets/QWidget>

namespace md {

/// M3 window size classes. Compact / Medium / Expanded are the three the
/// adaptive layout guidance names; Large and ExtraLarge complete the current
/// foundations table.
enum class MdWindowSizeClass {
    Compact,
    Medium,
    Expanded,
    Large,
    ExtraLarge,
    Count,
};

/// The per-class defaults the responsive layout grid publishes.
struct QT_MD3_EXPORT MdGridClassSpec
{
    int columns = 4;
    qreal gutter = 16.0;
    qreal margin = 16.0;
};

class QT_MD3_EXPORT MdGrid : public QWidget
{
    Q_OBJECT

    /// Forced size class. `Auto` (the default) derives it from the widget
    /// width; any other value pins the class and its columns/gutter/margin.
    Q_PROPERTY(md::MdWindowSizeClass sizeClass READ sizeClass WRITE setSizeClass
                   NOTIFY sizeClassChanged)
    /// Track count for the current class. Overridable per class.
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    /// Gap between tracks, in logical px.
    Q_PROPERTY(qreal gutter READ gutter WRITE setGutter NOTIFY gutterChanged)
    /// Inset from each container edge, in logical px.
    Q_PROPERTY(qreal margin READ margin WRITE setMargin NOTIFY marginChanged)
    /// The class the widget's width currently maps to (read-only).
    Q_PROPERTY(md::MdWindowSizeClass effectiveSizeClass READ effectiveSizeClass
                   NOTIFY effectiveSizeClassChanged)

public:
    explicit MdGrid(QWidget *parent = nullptr);
    ~MdGrid() override;

    // --- size class --------------------------------------------------------
    MdWindowSizeClass sizeClass() const { return m_sizeClass; }
    void setSizeClass(MdWindowSizeClass sizeClass);

    /// The class `width` maps to under the published breakpoints.
    static MdWindowSizeClass sizeClassForWidth(qreal width);
    /// The published breakpoint table for one class.
    static MdGridClassSpec specFor(MdWindowSizeClass sizeClass);

    /// The class in force right now: the pinned one, or the width-derived one.
    MdWindowSizeClass effectiveSizeClass() const;

    // --- grid metrics ------------------------------------------------------
    int columns() const;
    void setColumns(int columns);

    qreal gutter() const;
    void setGutter(qreal gutter);

    qreal margin() const;
    void setMargin(qreal margin);

    /// Column track width for the current metrics (content box).
    qreal columnWidth() const;

    // --- spans -------------------------------------------------------------
    /// Stretch `child` across `span` tracks (1 = one track). Clamped to
    /// [1, columns]. Stored per child; cleared when the child is removed.
    void setItemSpan(QWidget *child, int span);
    int itemSpan(const QWidget *child) const;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void sizeClassChanged(md::MdWindowSizeClass sizeClass);
    void effectiveSizeClassChanged(md::MdWindowSizeClass sizeClass);
    void columnsChanged(int columns);
    void gutterChanged(qreal gutter);
    void marginChanged(qreal margin);

protected:
    void childEvent(QChildEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void relayout() const;
    void refreshEffectiveClass();

    /// Pinned class, or Count for auto.
    MdWindowSizeClass m_sizeClass = MdWindowSizeClass::Count;
    MdWindowSizeClass m_effective = MdWindowSizeClass::Compact;
    int m_columnsOverride = 0; ///< 0 = use the class default.
    qreal m_gutterOverride = -1.0;
    qreal m_marginOverride = -1.0;

    QHash<const QWidget *, int> m_spans;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdWindowSizeClass)

#endif // MD_GRID_H
