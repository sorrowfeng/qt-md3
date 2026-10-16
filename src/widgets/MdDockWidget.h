#ifndef MD_DOCK_WIDGET_H
#define MD_DOCK_WIDGET_H

// MdDockWidget — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "Side sheet + Surface container；停靠引导用
// shape 高亮" — Side sheet + surface container, dock guides with a shape
// highlight. `QSplitter`-like docking.
//
// A dock widget is one pannable panel: a title bar (the Side sheet header
// language) plus a content widget. It can live inside an `MdDockManager`
// area (left / right / bottom / center) or float.
//
//   surface    `surface-container` @ level 2 (Side sheet container)
//   header     48 px row, `title-medium` label, trailing close icon button
//   divider    1 px `outline-variant` on the edge facing the manager center
//   guide      while dragging, the manager paints a `secondary-container`
//              shape highlight over the drop zone (ShapeCorner::Medium)
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QMouseEvent;
class QPaintEvent;

namespace md {

class MdIconButton;
class MdDockManager;

/// Which manager area hosts a dock widget.
enum class MdDockArea {
    None,
    Left,
    Right,
    Bottom,
    Center,
    Floating,
    Count,
};

class QT_MD3_EXPORT MdDockWidget : public QWidget
{
    Q_OBJECT

    /// Panel title shown in the header row.
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Current hosting area.
    Q_PROPERTY(md::MdDockArea area READ area WRITE setArea NOTIFY areaChanged)
    /// Close (X) button visibility in the header.
    Q_PROPERTY(bool closable READ isClosable WRITE setClosable NOTIFY closableChanged)
    /// Whether the panel currently shows its content (false = collapsed).
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)

public:
    explicit MdDockWidget(const QString &title = QString(), QWidget *parent = nullptr);
    ~MdDockWidget() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    MdDockArea area() const { return m_area; }
    void setArea(MdDockArea area);

    bool isClosable() const { return m_closable; }
    void setClosable(bool closable);

    bool isOpen() const { return m_open; }

    /// Content hosted under the header. Ownership stays with the caller.
    void setContentWidget(QWidget *content);
    QWidget *contentWidget() const { return m_content; }

    /// Resolved surface: `surface-container`.
    QColor resolvedSurfaceColor() const;
    /// Resolved header label: `on-surface`.
    QColor resolvedTitleColor() const;
    /// Resolved divider: `outline-variant`.
    QColor resolvedDividerColor() const;

    /// Header height (`md.comp.side-sheet` header, 48 px).
    static int headerHeight();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

public slots:
    void setOpen(bool open);

signals:
    void titleChanged(const QString &title);
    void areaChanged(md::MdDockArea area);
    void closableChanged(bool closable);
    void openChanged(bool open);
    /// The close button was clicked.
    void closeRequested();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onCloseClicked();

private:
    void init();
    void layoutChildren();
    QRect headerRect() const;
    QRect contentRect() const;

    QString m_title;
    MdDockArea m_area = MdDockArea::None;
    bool m_closable = true;
    bool m_open = true;
    QWidget *m_content = nullptr;
    MdIconButton *m_closeButton = nullptr;
    bool m_dragging = false;
    QPoint m_dragOrigin;

    friend class MdDockManager;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdDockArea)

#endif // MD_DOCK_WIDGET_H
