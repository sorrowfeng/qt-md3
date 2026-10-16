#ifndef MD_POPOVER_H
#define MD_POPOVER_H

// MdPopover — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "用 MD3 Menu / Dialog 语义收敛，不另造浮层样式"
// — collapse onto Menu / Dialog semantics, do not invent another overlay skin.
//
// A popover is the *content* reading of the menu popup: the same
// `surface-container` rounded surface at elevation level 2
// (`md.comp.menu.container.*`), the same corner-extra-small shape, the same
// fast-spatial / fast-effects open pair, anchored to a trigger rect. The
// difference is the body: instead of a list of `MdMenuItem`s the host paints
// an arbitrary child widget (a form, a preview, a mini panel).
//
//   surface   `surface-container` @ level 2, corner-extra-small
//   open      scale 0.8→1.0 (fast spatial) + alpha 0→1 (fast effects)
//   close     Esc / outside click / `hide()`
//   keyboard  Tab cycles the child; Esc closes
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QKeyEvent;
class QPaintEvent;
class QTimer;

namespace md {

class QT_MD3_EXPORT MdPopover : public QWidget
{
    Q_OBJECT

    /// Title drawn in the popover header (TitleMedium). Empty hides the header.
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Open/closed state of the overlay.
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
    /// Surface corner. Menu uses corner-extra-small (`md.comp.menu.container.shape`).
    Q_PROPERTY(md::ShapeCorner shape READ shape WRITE setShape NOTIFY shapeChanged)

public:
    explicit MdPopover(QWidget *parent = nullptr);
    ~MdPopover() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    bool isOpen() const { return m_open; }

    ShapeCorner shape() const { return m_shape; }
    void setShape(ShapeCorner shape);

    /// The content widget hosted under the header. Ownership stays with the caller.
    void setContentWidget(QWidget *content);
    QWidget *contentWidget() const { return m_content; }

    /// Open / close animation progress, 0 closed → 1 open (the menu curve).
    qreal openProgress() const { return m_openProgress; }

    /// Resolved surface colour: `surface-container`.
    QColor resolvedSurfaceColor() const;
    /// Resolved title colour: `on-surface`.
    QColor resolvedTitleColor() const;

    QSize sizeHint() const override;

public slots:
    /// Anchor the popover below `anchorRect` (global coordinates) and open it.
    void popup(const QRect &anchorRect);
    void setOpen(bool open);
    void hide();

signals:
    void titleChanged(const QString &title);
    void openChanged(bool open);
    void shapeChanged(md::ShapeCorner shape);
    /// Emitted after the popover closed (outside click, Esc or `hide()`).
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onOpenTick();
    void onThemeChanged();

private:
    void init();
    void relayout();
    void closeAndEmit();
    void applyContentGeometry();

    QString m_title;
    bool m_open = false;
    ShapeCorner m_shape = ShapeCorner::ExtraSmall;
    QWidget *m_content = nullptr;
    qreal m_openProgress = 1.0;
    QTimer *m_openTimer = nullptr;
    QSize m_fitSize;
};

} // namespace md

#endif // MD_POPOVER_H
