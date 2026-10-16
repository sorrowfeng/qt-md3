#ifndef MD_SPLITTER_H
#define MD_SPLITTER_H

// MdSplitter — Stage 2 Qt extension (Appendix D, P1).
//
// Semantic anchor (appendix D): "outline-variant 分割线；hover 时显示 handle".
// MD3 has no splitter family. The control is the outline-variant 1 px divider
// the Divider family already publishes (`md.comp.divider.thickness` 1px,
// `md.comp.divider.color` outline-variant), promoted into a draggable pane
// boundary with an Expressive hover affordance.
//
// Resting: a single hairline in `md.sys.color.outline-variant`, device-pixel
// aligned. No grab bar, no arrows — the divider itself is the hit target.
// Hover / drag: a short rounded handle bar appears on the line (the usual
// 32 x 3 px pill in `outline`), so the user can see where the boundary is
// grabbable without permanently chrome-ing the layout.
//
// Animation is minimal by design: the handle fades in with a short opacity
// ramp (MdMotion duration short-2 + the expressive-spatial easing used by the
// rest of the library's hover chrome). No spring — the handle is not a
// spatial morph, it is a reveal.
//
// Qt::Orientation is re-declared as a Q_PROPERTY with NOTIFY so the gallery
// and tests can bind to it; QSplitter already stores the value.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QSplitter>

class QPaintEvent;

namespace md {

class MdSplitterHandle;

class QT_MD3_EXPORT MdSplitter : public QSplitter
{
    Q_OBJECT

    /// Horizontal (left|right panes) or vertical (top/bottom panes).
    /// Re-declared so callers get a NOTIFY signal on change.
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY
                   orientationChanged)
    /// Resting handle hit-thickness in logical px. The painted hairline is
    /// always the divider token (1 px); this is the grab area.
    Q_PROPERTY(int handleWidth READ handleWidth WRITE setHandleWidth NOTIFY handleWidthChanged)
    /// Extra pixels around the line the hover bar occupies (the bar length).
    Q_PROPERTY(int handleBarLength READ handleBarLength WRITE setHandleBarLength NOTIFY
                   handleBarLengthChanged)
    /// Explicit divider colour. An invalid colour means outline-variant.
    Q_PROPERTY(QColor dividerColor READ dividerColor WRITE setDividerColor NOTIFY
                   dividerColorChanged)

public:
    explicit MdSplitter(QWidget *parent = nullptr);
    explicit MdSplitter(Qt::Orientation orientation, QWidget *parent = nullptr);
    ~MdSplitter() override;

    void setOrientation(Qt::Orientation orientation);

    int handleWidth() const { return m_handleWidth; }
    void setHandleWidth(int width);

    int handleBarLength() const { return m_handleBarLength; }
    void setHandleBarLength(int length);

    QColor dividerColor() const { return m_dividerColor; }
    void setDividerColor(const QColor &color);

    /// Resolved hairline colour: the override when valid, else outline-variant.
    QColor resolvedDividerColor() const;
    /// Hover-bar colour: outline.
    QColor resolvedHandleColor() const;

    /// Opacity 0..1 of the hover handle bar (for tests and for the
    /// handle's own paint).
    qreal handleReveal() const { return m_handleReveal; }

signals:
    void orientationChanged(Qt::Orientation orientation);
    void handleWidthChanged(int width);
    void handleBarLengthChanged(int length);
    void dividerColorChanged(const QColor &color);
    /// Emitted while a handle is hovered or being dragged. `reveal` is the
    /// current 0..1 opacity of the hover bar.
    void handleRevealChanged(qreal reveal);

protected:
    QSplitterHandle *createHandle() override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    friend class MdSplitterHandle;
    void init();
    void setHandleReveal(qreal reveal);
    void tickHandleReveal();

    int m_handleWidth = 8;
    int m_handleBarLength = 32;
    QColor m_dividerColor;
    qreal m_handleReveal = 0.0;
    bool m_handleActive = false;
};

} // namespace md

#endif // MD_SPLITTER_H
