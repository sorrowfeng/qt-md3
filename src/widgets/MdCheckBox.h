#ifndef MD_CHECKBOX_H
#define MD_CHECKBOX_H

// MdCheckBox — the MD3 checkbox, over Qt's QCheckBox.
//
// Compose's `Checkbox` is an 18 px canvas centred in a 48 px touch target, and
// its behaviours are what this widget ports:
//
//   * **the check draws itself in** — Compose's `checkDrawFraction` reveals the
//     checkmark path along its length on the *default spatial* spring; an undo
//     (anything → `Off`) holds the old visual for the 100 ms `SnapAnimationDelay`
//     and then snaps it away. The **indeterminate dash is the same path**:
//     `crossCenterGravitation` lerps the path's three points towards the centre
//     line, so a checkmark morphs into the dash and back. `Off → Indeterminate`
//     snaps the gravitation (the dash draws in from nothing, it does not grow
//     out of a checkmark).
//   * **the colours cross-fade between the unselected and selected tables** —
//     the box, border and check each animate on the effects springs (in on
//     `DefaultEffects`, out on `FastEffects`), and a transition into disabled
//     snaps rather than animates.
//   * **the press ripples in the pressed row's colour** — an unbounded circle
//     of the state-layer's 40 px diameter. Compose's default colours ripple
//     transparent when unchecked (its `indicatorColor` returns the transparent
//     box fill); the export's state-layer rows win here.
//
// The box, its outline, the check and the focus ring all centre in the widget;
// the paint is `MdCheckBoxStyle`'s, reached through the shared paint filter.

#include "core/MdCheckBoxTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QCheckBox>

class QTimer;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdCheckBox : public QCheckBox
{
    Q_OBJECT

    /// The error variant (`md.comp.checkbox.error.*`): a form-validation colour
    /// family, independent of the selection.
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)

public:
    explicit MdCheckBox(QWidget *parent = nullptr);
    explicit MdCheckBox(const QString &text, QWidget *parent = nullptr);
    ~MdCheckBox() override;

    // --- state ---------------------------------------------------------------
    bool hasError() const { return m_error; }
    void setError(bool error);

    // --- geometry --------------------------------------------------------------
    /// The 18 px box, centred in the widget.
    QRectF boxRect() const;
    /// The 40 px circular state layer / ripple bounds, centred in the widget.
    QRectF stateLayerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- animation readouts ------------------------------------------------------
    /// The check path's revealed fraction (Compose's `checkDrawFraction`).
    qreal checkFraction() const { return m_checkFraction; }
    /// The check-to-dash morph (Compose's `crossCenterGravitation`).
    qreal crossGravitation() const { return m_gravitation; }
    /// 0 fully in the unselected colour tables, 1 fully in the selected ones.
    qreal colourProgress() const { return m_colourProgress; }
    bool checkAnimationRunning() const { return m_checkTimer->isActive(); }
    bool colourAnimationRunning() const { return m_colourTimer->isActive(); }

    // --- tokens ------------------------------------------------------------------
    const MdCheckBoxTokens &checkBoxTokens() const;
    void setCheckBoxTokens(const MdCheckBoxTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state ---------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers -------------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

signals:
    void errorChanged(bool error);

protected:
    void nextCheckState() override;
    void checkStateSet() override;
    bool hitButton(const QPoint &pos) const override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onCheckTick();
    void onColourTick();

private:
    void init();
    void invalidateTokens();
    void restartCheckAnimation();
    void restartColourAnimation();

    bool m_error = false;
    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    // The three animated values, each with its own transition record. A snap
    // transition (Compose's `snap(delayMillis)`) holds the current value for
    // the delay and then lands on the target instantly.
    struct Transition
    {
        qreal from = 0.0;
        qreal to = 0.0;
        QElapsedTimer clock;
        bool snap = false;
        int delayMs = 0;
    };
    Transition m_fractionTransition;
    Transition m_gravitationTransition;
    qreal m_checkFraction = 0.0;
    qreal m_gravitation = 0.0;
    Qt::CheckState m_lastState = Qt::Unchecked;

    qreal m_colourProgress = 0.0;
    qreal m_colourFrom = 0.0;
    qreal m_colourTo = 0.0;
    QElapsedTimer m_colourClock;

    mutable MdCheckBoxTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    QTimer *m_checkTimer = nullptr;
    QTimer *m_colourTimer = nullptr;
    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_CHECKBOX_H
