#ifndef MD_COLLAPSE_H
#define MD_COLLAPSE_H

// MdCollapse — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "List 的展开折叠语义" — the List family's
// expand / collapse semantics.
//
// A collapse is a list section: a header row the user can toggle, and a body
// that grows and shrinks underneath it. MD3 has no standalone Collapse
// family; the semantics come from the list (header row measurement, the
// one-line 56 px floor, the hover/focus state layers and the inward focus
// ring of an interactive list item) and the expand/collapse motion comes
// from the Expressive springs:
//
//   header   a List one-line row (headline + optional supporting text and a
//            trailing expand_more / expand_less glyph)
//   body     a caller widget, height-animated on the FastSpatial spring
//   toggle   click / Space / Enter on the header; the chevron rotates
//
// The body is real widget content, not painted: the caller parents it to
// the collapse and MdCollapse clips its geometry while the spring is in
// flight. Height animation is spring physics (MdSpring::valueAt), never a
// fixed-duration QEasingCurve — the two coexist in this project, and expand
// / collapse is the spatial case.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdCollapse : public QWidget
{
    Q_OBJECT

    /// The header headline (body-large, on-surface).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Optional supporting line under the headline (body-medium).
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText
                   NOTIFY supportingTextChanged)
    /// Expanded state. Animates the body height on the FastSpatial spring.
    Q_PROPERTY(bool expanded READ isExpanded WRITE setExpanded NOTIFY expandedChanged)
    /// Header height floor. Defaults to the List one-line height (56).
    Q_PROPERTY(int headerHeight READ headerHeight WRITE setHeaderHeight
                   NOTIFY headerHeightChanged)

public:
    explicit MdCollapse(QWidget *parent = nullptr);
    explicit MdCollapse(const QString &title, QWidget *parent = nullptr);
    ~MdCollapse() override;

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString supportingText() const { return m_supportingText; }
    void setSupportingText(const QString &text);

    bool isExpanded() const { return m_expanded; }
    void setExpanded(bool expanded);
    void toggle() { setExpanded(!m_expanded); }

    int headerHeight() const { return m_headerHeight; }
    void setHeaderHeight(int height);

    /// The body widget. Takes ownership through the Qt parent relationship.
    QWidget *bodyWidget() const { return m_body; }
    void setBodyWidget(QWidget *body);

    /// Header hit-test / focus state (painted from these).
    bool isHeaderHovered() const { return m_headerHovered; }
    bool headerHasKeyboardFocus() const
    {
        return hasFocus() && m_focusIsKeyboard;
    }

    /// The header row inside the widget.
    QRectF headerRect() const;
    /// The chevron's target rect (48 px trailing slot, list measurement).
    QRectF chevronRect() const;

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;
    bool hasHeightForWidth() const override { return true; }

signals:
    void titleChanged(const QString &title);
    void supportingTextChanged(const QString &text);
    void expandedChanged(bool expanded);
    void headerHeightChanged(int height);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void childEvent(QChildEvent *event) override;

private slots:
    void onThemeChanged();
    void onAnimationFrame();

private:
    void init();
    void relayout();
    void updateAnimationState();
    /// 0 = collapsed body height, 1 = full body height. Spring-driven.
    qreal bodyProgress() const;
    int bodyContentHeight() const;

    QString m_title;
    QString m_supportingText;
    bool m_expanded = false;
    int m_headerHeight = 56;

    bool m_headerHovered = false;
    bool m_headerPressed = false;
    bool m_focusIsKeyboard = false;

    QWidget *m_body = nullptr;

    QTimer *m_timer = nullptr;
    QElapsedTimer m_clock;
    /// Target progress for the spring (0 collapsed, 1 expanded).
    qreal m_target = 0.0;
    /// Progress when the current animation started.
    qreal m_from = 0.0;

    MdFocusRingController *m_focusRing = nullptr;
    MdRippleController *m_ripple = nullptr;
};

} // namespace md

#endif // MD_COLLAPSE_H
