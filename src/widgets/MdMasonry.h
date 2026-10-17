#ifndef MD_MASONRY_H
#define MD_MASONRY_H

// MdMasonry — Stage 2 Qt extension (Appendix D, P3).
//
// Semantic anchor (appendix D): "同上" — the masonry (staggered multi-column)
// variant of the responsive grid.
//
// MdGrid is the track-aligned layout grid. MdMasonry is its free-height
// sibling: child widgets keep their own heightHint and the columns fill
// shortest-first, the way a Pinterest-style board works. The spacing comes
// from the same MdFlex spacing ladder (the MD3 4 dp base unit), and the
// column count follows the MdGrid window size classes unless pinned.
//
// MdMasonry paints nothing of its own. It is a pure container, exactly like
// MdFlex / MdGrid / MdSpace.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"
#include "widgets/MdGrid.h"

#include <QtWidgets/QWidget>

namespace md {

class QT_MD3_EXPORT MdMasonry : public QWidget
{
    Q_OBJECT

    /// Pinned column count. 0 means "derive from the window size class".
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    /// Gap between columns and rows, in logical px (the MdFlex spacing
    /// ladder: 0 / 4 / 8 / 12 / 16 / 24 / 32).
    Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY gapChanged)
    /// The size class currently in effect (derived when `columns` is 0).
    Q_PROPERTY(md::MdWindowSizeClass effectiveSizeClass READ effectiveSizeClass NOTIFY
                   effectiveSizeClassChanged)

public:
    explicit MdMasonry(QWidget *parent = nullptr);
    ~MdMasonry() override;

    int columns() const { return m_columns; }
    void setColumns(int columns);

    qreal gap() const { return m_gap; }
    void setGap(qreal gap);

    /// Derived column count for the current width (honours a pinned value).
    int effectiveColumns() const;

    MdWindowSizeClass effectiveSizeClass() const { return m_effective; }

    /// Default columns for a size class: 1 / 2 / 3 / 4 / 4.
    static int columnsFor(MdWindowSizeClass sizeClass);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void columnsChanged(int columns);
    void gapChanged(qreal gap);
    void effectiveSizeClassChanged(md::MdWindowSizeClass sizeClass);

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

    int m_columns = 0; // 0 = derive
    qreal m_gap = 12.0;
    MdWindowSizeClass m_effective = MdWindowSizeClass::Compact;
};

} // namespace md

#endif // MD_MASONRY_H
