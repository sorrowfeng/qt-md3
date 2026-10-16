#ifndef MD_STATUS_BAR_H
#define MD_STATUS_BAR_H

// MdStatusBar — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "surface container + label typography".
// MD3 has no status-bar family. This is the window's bottom message strip:
// a `md.sys.color.surface-container` band carrying one-line status text in
// `label-large` / `body-medium`, with an optional trailing size grip.
//
// Arrangement matches QStatusBar's mental model so it drops into existing
// window shells: a leading message area, a trailing permanent area (right
// message and the optional grip). Neither side is styled by the bar — any
// child widget keeps its own component painting.
//
// Tokens:
//   background   md.sys.color.surface-container
//   text         md.sys.color.on-surface (leading) /
//                md.sys.color.on-surface-variant (trailing)
//   grip        outline-variant ticks on surface-container
//
// Not a button surface: the messages themselves take no focus and paint no
// state layer. The size grip is the only interactive region and it only
// resizes the top-level window when one is present.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

class QT_MD3_EXPORT MdStatusBar : public QWidget
{
    Q_OBJECT

    /// Leading status text (start edge). Empty collapses the leading area.
    Q_PROPERTY(QString leftMessage READ leftMessage WRITE setLeftMessage NOTIFY leftMessageChanged)
    /// Trailing status text (end edge, before the size grip).
    Q_PROPERTY(QString rightMessage READ rightMessage WRITE setRightMessage NOTIFY
                   rightMessageChanged)
    /// Show the resize grip on the trailing edge of a top-level window.
    Q_PROPERTY(bool sizeGripVisible READ isSizeGripVisible WRITE setSizeGripVisible NOTIFY
                   sizeGripVisibleChanged)
    /// Preferred height in logical px. 0 means the label-large row height.
    Q_PROPERTY(int barHeight READ barHeight WRITE setBarHeight NOTIFY barHeightChanged)
    /// Background override. An invalid colour means the surface-container token.
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY
                   backgroundColorChanged)

public:
    explicit MdStatusBar(QWidget *parent = nullptr);
    ~MdStatusBar() override;

    // --- messages -----------------------------------------------------------
    QString leftMessage() const { return m_leftMessage; }
    void setLeftMessage(const QString &message);

    QString rightMessage() const { return m_rightMessage; }
    void setRightMessage(const QString &message);

    // --- chrome -------------------------------------------------------------
    bool isSizeGripVisible() const { return m_sizeGripVisible; }
    void setSizeGripVisible(bool visible);

    int barHeight() const { return m_barHeight; }
    void setBarHeight(int height);

    QColor backgroundColor() const { return m_backgroundOverride; }
    void setBackgroundColor(const QColor &color);

    /// Resolved background: the override when set, else surface-container.
    QColor resolvedBackgroundColor() const;
    /// Resolved leading-text colour: on-surface.
    QColor resolvedLeftColor() const;
    /// Resolved trailing-text colour: on-surface-variant.
    QColor resolvedRightColor() const;

    /// Geometry of the size-grip region in widget coordinates. Empty when the
    /// grip is hidden or the bar is too narrow.
    QRect sizeGripRect() const;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void leftMessageChanged(const QString &message);
    void rightMessageChanged(const QString &message);
    void sizeGripVisibleChanged(bool visible);
    void barHeightChanged(int height);
    void backgroundColorChanged(const QColor &color);
    /// Emitted when the grip is dragged; `delta` is the mouse movement in
    /// logical pixels. The bar itself only resizes a top-level window.
    void sizeGripDragged(const QPoint &delta);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void paintMessages(QPainter *painter) const;
    void paintSizeGrip(QPainter *painter) const;

    QString m_leftMessage;
    QString m_rightMessage;
    bool m_sizeGripVisible = false;
    int m_barHeight = 0; // 0 = derived from label-large
    QColor m_backgroundOverride;
    bool m_gripDragging = false;
    QPoint m_gripLastPos;
};

} // namespace md

#endif // MD_STATUS_BAR_H
