#ifndef MD_DOCK_MANAGER_H
#define MD_DOCK_MANAGER_H

// MdDockManager — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Side sheet + Surface container；停靠引导用
// shape 高亮" — the container that owns a central area plus left / right /
// bottom dock areas, and paints a `secondary-container` shape highlight
// over the drop zone while a dock widget is dragged.
//
//   center     the main content, fills whatever the dock areas leave
//   left/right bottom  side-sheet-shaped strips, each holding MdDockWidgets
//   guide      while dragging, the target area lights up with a
//              `secondary-container` rounded rect (ShapeCorner::Medium)
//   split      neighbouring dock widgets share an area through an
//              `MdSplitter` (outline-variant divider, hover handle)
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include "widgets/MdDockWidget.h"

#include <QtWidgets/QWidget>

class QPaintEvent;
class QResizeEvent;

namespace md {

class MdDockWidget;
class MdSplitter;

class QT_MD3_EXPORT MdDockManager : public QWidget
{
    Q_OBJECT

    /// The central content widget. Ownership stays with the caller.
    Q_PROPERTY(QWidget *centerWidget READ centerWidget WRITE setCenterWidget NOTIFY centerWidgetChanged)
    /// Width of the left dock area. 0 collapses it.
    Q_PROPERTY(int leftWidth READ leftWidth WRITE setLeftWidth NOTIFY leftWidthChanged)
    /// Width of the right dock area. 0 collapses it.
    Q_PROPERTY(int rightWidth READ rightWidth WRITE setRightWidth NOTIFY rightWidthChanged)
    /// Height of the bottom dock area. 0 collapses it.
    Q_PROPERTY(int bottomHeight READ bottomHeight WRITE setBottomHeight NOTIFY bottomHeightChanged)
    /// True while a dock guide highlight is showing.
    Q_PROPERTY(bool guideVisible READ isGuideVisible NOTIFY guideVisibleChanged)

public:
    explicit MdDockManager(QWidget *parent = nullptr);
    ~MdDockManager() override;

    QWidget *centerWidget() const { return m_center; }
    void setCenterWidget(QWidget *widget);

    int leftWidth() const { return m_leftWidth; }
    void setLeftWidth(int width);

    int rightWidth() const { return m_rightWidth; }
    void setRightWidth(int width);

    int bottomHeight() const { return m_bottomHeight; }
    void setBottomHeight(int height);

    bool isGuideVisible() const { return m_guideVisible; }

    /// The area currently highlighted by the drag guide.
    MdDockArea guideArea() const { return m_guideArea; }

    /// Dock `widget` into `area`. Reparents the widget to the manager.
    void addDockWidget(MdDockWidget *widget, MdDockArea area);
    /// Remove `widget` from its area without destroying it.
    void removeDockWidget(MdDockWidget *widget);
    /// Dock widgets currently in `area`, in stacking order.
    QList<MdDockWidget *> dockWidgets(MdDockArea area) const;

    /// Resolved guide fill: `secondary-container`.
    QColor resolvedGuideColor() const;

    QSize sizeHint() const override;

public slots:
    /// Show / hide the drop-zone guide for `area`.
    void showGuide(MdDockArea area);
    void hideGuide();

signals:
    void centerWidgetChanged(QWidget *widget);
    void leftWidthChanged(int width);
    void rightWidthChanged(int width);
    void bottomHeightChanged(int height);
    void guideVisibleChanged(bool visible);
    void dockWidgetAdded(md::MdDockWidget *widget, md::MdDockArea area);
    void dockWidgetRemoved(md::MdDockWidget *widget);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onSplitterMoved();

private:
    void init();
    void layoutAreas();
    QRect areaRect(MdDockArea area) const;
    void syncAreaSizes();

    QWidget *m_center = nullptr;
    MdSplitter *m_leftSplitter = nullptr;
    MdSplitter *m_rightSplitter = nullptr;
    MdSplitter *m_bottomSplitter = nullptr;

    QList<MdDockWidget *> m_leftWidgets;
    QList<MdDockWidget *> m_rightWidgets;
    QList<MdDockWidget *> m_bottomWidgets;

    int m_leftWidth = 0;
    int m_rightWidth = 0;
    int m_bottomHeight = 0;

    bool m_guideVisible = false;
    MdDockArea m_guideArea = MdDockArea::None;
};

} // namespace md

// Incomplete QObject subclass in dockWidgetAdded / dockWidgetRemoved.
// Opaque + registered: either macro alone leaves the pointer unusable on Qt 5.
Q_DECLARE_OPAQUE_POINTER(md::MdDockWidget *)
Q_DECLARE_METATYPE(md::MdDockWidget *)

#endif // MD_DOCK_MANAGER_H
