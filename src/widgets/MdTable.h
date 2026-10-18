#ifndef MD_TABLE_H
#define MD_TABLE_H

// MdTable — Stage 2 Qt extension (Appendix D, P0), the QTableView-like data
// table.
//
// MD3 has no data-table spec (nothing under m3.material.io/components). The
// appendix D row anchors the design on "List row semantics + Surface
// container header", and this control is exactly that composition:
//
//   header   surface-container, label-large (on-surface), the label-style
//            the list's trailing meta uses; a single row, not a list item
//   rows     list one-line height (56 px default), body-medium text in
//            on-surface, hover state layer 0.08 / pressed 0.12 / selected
//            secondary-container + on-secondary-container (the list's
//            selected family)
//   dividers outline-variant 1 px between rows and under the header —
//            `md.comp.divider.thickness` / `.color`
//   optional alternating rows surface-container-lowest / surface-container-low
//            (the tonal surface ladder, not a tinted stripe)
//
// The model surface is deliberately simple: a string grid with optional
// cell widgets, the "setRowCount / setCellWidget" side of the appendix D
// note. A QAbstractTableModel can be layered on top by a caller; this
// control does not own one.
//
// Interaction: click selects a row (single selection), hover paints the
// row state layer, keyboard Up/Down / Home/End move the selection. Row
// activation emits `cellActivated(row, column)`.
//
// There is no QSS. Painting is Pattern C (self-paint) with the same
// theme-update subscription every other component uses.

#include "core/MdDialogTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace md {

class QT_MD3_EXPORT MdTable : public QWidget
{
    Q_OBJECT

    /// Number of body rows.
    Q_PROPERTY(int rowCount READ rowCount NOTIFY rowCountChanged)
    /// Number of columns.
    Q_PROPERTY(int columnCount READ columnCount NOTIFY columnCountChanged)
    /// Zero-based selected row, or -1.
    Q_PROPERTY(int currentRow READ currentRow WRITE setCurrentRow NOTIFY currentRowChanged)
    /// Draw the horizontal header.
    Q_PROPERTY(bool showHeader READ showHeader WRITE setShowHeader NOTIFY showHeaderChanged)
    /// Draw the outline-variant row dividers.
    Q_PROPERTY(bool showDividers READ showDividers WRITE setShowDividers NOTIFY
                   showDividersChanged)
    /// Alternate surface-container-lowest / low behind every other row.
    Q_PROPERTY(bool alternatingRowColors READ alternatingRowColors WRITE setAlternatingRowColors
                   NOTIFY alternatingRowColorsChanged)
    /// Body row height in logical px. 0 means the list one-line height.
    Q_PROPERTY(qreal rowHeight READ rowHeight WRITE setRowHeight NOTIFY rowHeightChanged)
    /// Header height in logical px. 0 means the list one-line height.
    Q_PROPERTY(qreal headerHeight READ headerHeight WRITE setHeaderHeight NOTIFY
                   headerHeightChanged)

public:
    explicit MdTable(QWidget *parent = nullptr);
    ~MdTable() override;

    // --- shape ---------------------------------------------------------------
    int rowCount() const { return m_rows; }
    void setRowCount(int rows);

    int columnCount() const { return m_columns; }
    void setColumnCount(int columns);

    /// Header labels, left to right. Size may differ from columnCount; the
    /// extra labels are ignored / missing ones paint blank.
    QStringList horizontalHeaderLabels() const { return m_headerLabels; }
    void setHorizontalHeaderLabels(const QStringList &labels);

    // --- cells ----------------------------------------------------------------
    void setCellText(int row, int column, const QString &text);
    QString cellText(int row, int column) const;

    /// Widget shown in a cell. The table takes ownership through the Qt
    /// parent relationship and positions it inside the cell rect. Only rows
    /// inside the realised window are shown; the rest are hidden until they
    /// scroll into view.
    void setCellWidget(int row, int column, QWidget *widget);
    QWidget *cellWidget(int row, int column) const;

    /// The window of rows whose cell widgets are currently realised (shown
    /// and positioned). Rows outside are hidden so a tall table does not
    /// keep a live child widget per row. Empty range is (-1, -1).
    void realisedRange(int *first, int *last) const;

    // --- selection ------------------------------------------------------------
    int currentRow() const { return m_currentRow; }
    void setCurrentRow(int row);

    // --- appearance ------------------------------------------------------------
    bool showHeader() const { return m_showHeader; }
    void setShowHeader(bool show);

    bool showDividers() const { return m_showDividers; }
    void setShowDividers(bool show);

    bool alternatingRowColors() const { return m_alternating; }
    void setAlternatingRowColors(bool alternating);

    qreal rowHeight() const;
    void setRowHeight(qreal height);

    qreal headerHeight() const;
    void setHeaderHeight(qreal height);

    // --- geometry --------------------------------------------------------------
    /// The body rect (below the header), in widget coordinates.
    QRectF bodyRect() const;
    /// The painted header rect. Empty when showHeader is false.
    QRectF headerRect() const;
    /// The rect of one body row.
    QRectF rowRect(int row) const;
    /// The rect of one header cell.
    QRectF headerCellRect(int column) const;
    /// The rect of one body cell.
    QRectF cellRect(int row, int column) const;

    QSize sizeHint() const override;
    int heightForWidth(int width) const override;

signals:
    void rowCountChanged(int rows);
    void columnCountChanged(int columns);
    void currentRowChanged(int row);
    void showHeaderChanged(bool show);
    void showDividersChanged(bool show);
    void alternatingRowColorsChanged(bool alternating);
    void rowHeightChanged(qreal height);
    void headerHeightChanged(qreal height);
    void cellActivated(int row, int column);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void recomputeColumnWidths() const;
    void layoutCellWidgets();
    /// Compute the realised cell-widget window and show / hide accordingly.
    void syncCellWidgetWindow();
    /// The visible band in table coordinates — the ancestor scroll area's
    /// viewport when there is one, else the widget's own rect.
    QRect virtualViewportRect() const;
    /// Connect the ancestor scroll area's scrollbars to `onScrollChanged`.
    void installScrollHook();
    void onScrollChanged();
    int rowAtPosition(const QPointF &position) const;
    int columnAtPosition(const QPointF &position) const;

    int m_rows = 0;
    int m_columns = 0;
    QStringList m_headerLabels;
    QList<QStringList> m_cells;
    QList<QList<QWidget *>> m_cellWidgets;
    int m_currentRow = -1;
    int m_hoveredRow = -1;
    bool m_showHeader = true;
    bool m_showDividers = true;
    bool m_alternating = false;
    qreal m_rowHeight = 0.0;
    qreal m_headerHeight = 0.0;
    mutable QList<qreal> m_columnWidths;
    int m_realisedFirst = -1;
    int m_realisedLast = -1;
    bool m_scrollFilterInstalled = false;
};

} // namespace md

#endif // MD_TABLE_H
