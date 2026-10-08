#ifndef MD_SNACKBAR_H
#define MD_SNACKBAR_H

// MdSnackbar — MD3 snackbars, the fourth Communication family.
//
// One token set (`md.comp.snackbar.*`), no variants: an inverse-surface
// container at elevation 3 with the extra-small corner, a body-medium
// supporting text in inverse-on-surface, an optional label-large action in
// inverse-primary and an optional 24 px close icon in inverse-on-surface.
// The export publishes hover / focus / pressed rows for BOTH interactive
// elements, so this is the first Communication family where every published
// state row is painted.
//
// Two layouts, ported from Compose's measure policies (the export publishes
// no layout rows):
//
//   one-row   the message left, the action and dismiss right, everything
//             vertically centred; max(48, content) tall — or the wrapped
//             rules: first line at 30 px, max(68, …) tall
//   new-line  the message on top, the action row bottom-right (recommended
//             for long action text)
//
// The action and dismiss are internal regions of one widget, same reasoning
// as MdSplitButton: the export styles the label text and icon directly, and
// composing real button children would drag the button family's focus rings
// into a surface that publishes its own state rows instead. Each region
// paints its state layer and ripple from the snackbar's own rows, supports
// keyboard focus (Tab cycles action → dismiss) and emits actionClicked() /
// dismissClicked().
//
// A standalone MdSnackbar is the visuals only — it never appears or
// disappears on its own. Placement, queueing and the 4000/10000/Indefinite
// durations are MdSnackbarHost's contract (Compose: "This component provides
// only the visuals of the Snackbar").

#include "core/MdQtCompat.h"
#include "core/MdRipple.h"
#include "core/MdSnackbarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdSnackbarStyle;

class QT_MD3_EXPORT MdSnackbar : public QWidget
{
    Q_OBJECT

    /// The supporting text (body-medium, inverse-on-surface).
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    /// The optional action label (label-large, inverse-primary); empty = none.
    Q_PROPERTY(QString actionLabel READ actionLabel WRITE setActionLabel
                   NOTIFY actionLabelChanged)
    /// Show the 24 px close icon (inverse-on-surface). Recommended whenever
    /// the snackbar does not self-dismiss.
    Q_PROPERTY(bool hasDismissAction READ hasDismissAction WRITE setHasDismissAction
                   NOTIFY hasDismissActionChanged)
    /// Put the action on its own line below the message (recommended for long
    /// action text).
    Q_PROPERTY(bool actionOnNewLine READ isActionOnNewLine WRITE setActionOnNewLine
                   NOTIFY actionOnNewLineChanged)

public:
    explicit MdSnackbar(QWidget *parent = nullptr);
    MdSnackbar(const QString &message, QWidget *parent = nullptr);
    ~MdSnackbar() override;

    // --- content -----------------------------------------------------------
    QString message() const { return m_message; }
    void setMessage(const QString &message);

    QString actionLabel() const { return m_actionLabel; }
    void setActionLabel(const QString &actionLabel);
    bool hasAction() const { return !m_actionLabel.isEmpty(); }

    bool hasDismissAction() const { return m_hasDismissAction; }
    void setHasDismissAction(bool show);

    bool isActionOnNewLine() const { return m_actionOnNewLine; }
    void setActionOnNewLine(bool onNewLine);

    // --- interactive element states (painted by the style) ------------------
    MdSnackbarState actionState() const;
    MdSnackbarState iconState() const;

    /// `:focus-visible` semantics, shared with the button families: only
    /// keyboard focus counts.
    bool actionHasKeyboardFocus() const
    {
        return m_focusIndex == 0 && hasFocus() && m_focusIsKeyboard;
    }
    bool dismissHasKeyboardFocus() const
    {
        return m_focusIndex == 1 && hasFocus() && m_focusIsKeyboard;
    }

    /// The container rect inside the widget's shadow margin.
    QRectF containerRect() const;

    // --- ripples (owned here, painted by the style) -------------------------
    MdRippleFrame actionRippleFrame() const { return m_actionRipple.currentFrame(); }
    MdRippleFrame iconRippleFrame() const { return m_iconRipple.currentFrame(); }

    /// Extra alpha multiplied into every painted colour — the host's
    /// enter/exit transition drives this (a child widget has no window
    /// opacity; the style applies it with the painter's opacity instead).
    qreal paintOpacity() const { return m_paintOpacity; }
    void setPaintOpacity(qreal opacity);

    // --- geometry -----------------------------------------------------------
    QRectF actionRect() const;
    QRectF dismissRect() const;
    /// The interactive regions (the label/icon grown by their chrome).
    QRectF actionHitRect() const;
    QRectF dismissHitRect() const;

    // --- tokens ------------------------------------------------------------
    const MdSnackbarTokens &tokens() const;
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    int heightForWidth(int width) const override;

signals:
    void messageChanged(const QString &message);
    void actionLabelChanged(const QString &actionLabel);
    void hasDismissActionChanged(bool show);
    void actionOnNewLineChanged(bool onNewLine);
    /// The action label was activated (pointer, keyboard or accessibility).
    void actionClicked();
    /// The dismiss icon was activated.
    void dismissClicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void rebuildControllers();
    enum Region
    {
        None,
        Action,
        Dismiss,
    };
    Region regionAt(const QPointF &position) const;
    void setHovered(Region region);
    void activate(Region region);

    QString m_message;
    QString m_actionLabel;
    bool m_hasDismissAction = false;
    bool m_actionOnNewLine = false;

    Region m_hovered = None;
    Region m_pressed = None;
    /// 0 = action, 1 = dismiss, -1 = none (Tab order).
    int m_focusIndex = -1;
    bool m_focusIsKeyboard = false;

    mutable MdSnackbarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    qreal m_paintOpacity = 1.0;

    MdRippleController m_actionRipple;
    MdRippleController m_iconRipple;
};

} // namespace md

#endif // MD_SNACKBAR_H
