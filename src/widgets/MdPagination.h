#ifndef MD_PAGINATION_H
#define MD_PAGINATION_H

// MdPagination — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Text button + Icon button 组合".
// MD3 has no pagination family. The control composes two Stage 1 families
// the way every Material list's pager already does:
//
//   prev / next   MdIconButton (standard, xsmall) — chevron_left / chevron_right
//   page numbers  MdButton (Text variant, xsmall) per visible page
//   current page  MdButton (Filled variant, xsmall) — primary / on-primary,
//                 the "selected" reading of a page chip
//
// Ellipsis hops use a non-interactive label, not a button, so keyboard order
// never lands on a dead control. Left/Right (and Home/End) walk the pages
// from the pager itself; each page button is tab-reachable and activates on
// Space/Enter through QAbstractButton.
//
// Layout is a horizontal row with 4 px gaps, mirrored in RTL. The widget
// paints nothing of its own — every pixel is a Stage 1 component.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

class MdButton;
class MdIconButton;

class QT_MD3_EXPORT MdPagination : public QWidget
{
    Q_OBJECT

    /// Total number of pages. Values below 1 are clamped to 1.
    Q_PROPERTY(int pageCount READ pageCount WRITE setPageCount NOTIFY pageCountChanged)
    /// Selected page, 1-based. Clamped into [1, pageCount].
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    /// How many page-number buttons are shown around the current page,
    /// including the current page itself. Minimum 1.
    Q_PROPERTY(int visiblePages READ visiblePages WRITE setVisiblePages NOTIFY visiblePagesChanged)
    /// Show first/last page buttons when they fall outside the window.
    Q_PROPERTY(bool showFirstLast READ isShowFirstLast WRITE setShowFirstLast NOTIFY
                   showFirstLastChanged)
    /// Enable the previous button on page 1 and the next button on the last page.
    Q_PROPERTY(bool wrap READ isWrap WRITE setWrap NOTIFY wrapChanged)

public:
    explicit MdPagination(QWidget *parent = nullptr);
    ~MdPagination() override;

    int pageCount() const { return m_pageCount; }
    void setPageCount(int count);

    int currentPage() const { return m_currentPage; }
    void setCurrentPage(int page);

    int visiblePages() const { return m_visiblePages; }
    void setVisiblePages(int count);

    bool isShowFirstLast() const { return m_showFirstLast; }
    void setShowFirstLast(bool show);

    bool isWrap() const { return m_wrap; }
    void setWrap(bool wrap);

    // --- navigation slots ---------------------------------------------------
public slots:
    void previousPage();
    void nextPage();
    void firstPage();
    void lastPage();

signals:
    void pageCountChanged(int pageCount);
    void currentPageChanged(int currentPage);
    void visiblePagesChanged(int visiblePages);
    void showFirstLastChanged(bool show);
    void wrapChanged(bool wrap);
    void pageSelected(int page);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onButtonClicked();

private:
    void init();
    void rebuild();
    void clearButtons();
    /// 1-based page numbers that should appear as buttons, with 0 meaning
    /// "ellipsis gap" before the following entry.
    QList<int> visiblePagePlan() const;
    void applyEnabledStates();

    int m_pageCount = 1;
    int m_currentPage = 1;
    int m_visiblePages = 5;
    bool m_showFirstLast = false;
    bool m_wrap = false;

    MdIconButton *m_prevButton = nullptr;
    MdIconButton *m_nextButton = nullptr;
    MdIconButton *m_firstButton = nullptr;
    MdIconButton *m_lastButton = nullptr;
    QList<MdButton *> m_pageButtons;
    QVector<QWidget *> m_ellipsisLabels;
};

} // namespace md

#endif // MD_PAGINATION_H
