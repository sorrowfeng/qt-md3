#ifndef MD_RADIO_BUTTON_H
#define MD_RADIO_BUTTON_H

// MdRadioButton — the MD3 radio button, over Qt's QRadioButton.
//
// Compose's `RadioButton` is a 20 px stroke circle centred in a 48 px touch
// target, and its behaviours are what this widget ports:
//
//   * **the dot scales in and out** — Compose's `dotRadius` is
//     `RadioButtonDotSize / 2` when selected and 0 otherwise, run through
//     `animateDpAsState` on the *fast spatial* spring. The drawn dot radius is
//     `animatedDotRadius - strokeWidth / 2`, so a selected button's dot fills
//     to 5 px inside the 2 px stroke.
//   * **the colours cross-fade between the unselected and selected tables** on
//     the effects springs (in on `DefaultEffects`, out on `FastEffects`), and a
//     transition into disabled snaps — "there should be no animations between
//     enabled / disabled".
//   * **the press ripples in the pressed row's colour** — an unbounded circle
//     of the state layer's 40 px diameter: `primary` unselected (the colour
//     the button is about to earn), `on-surface` selected.
//
// One colour paints both the stroke circle and the dot (Compose draws both in
// one `animatedColor`). The group exclusivity is `QRadioButton`'s native
// `autoExclusive`; a click on an already-checked button keeps it checked.
// The paint is `MdRadioButtonStyle`'s, reached through the shared paint filter.

#include "core/MdQtCompat.h"
#include "core/MdRadioButtonTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QRadioButton>

class QTimer;

namespace md {

class MdFocusRingController;
class MdRippleController;

class QT_MD3_EXPORT MdRadioButton : public QRadioButton
{
    Q_OBJECT

public:
    explicit MdRadioButton(QWidget *parent = nullptr);
    explicit MdRadioButton(const QString &text, QWidget *parent = nullptr);
    ~MdRadioButton() override;

    // --- geometry --------------------------------------------------------------
    /// The 20 px icon canvas, centred in the widget.
    QRectF iconRect() const;
    /// The 40 px circular state layer / ripple bounds, centred in the widget.
    QRectF stateLayerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- animation readouts -------------------------------------------------------
    /// The dot's animated diameter (Compose's `animatedDotRadius`), before the
    /// stroke inset. 0 unselected, 12 selected.
    qreal animatedDotDiameter() const { return m_dotDiameter; }
    /// 0 fully in the unselected colour tables, 1 fully in the selected ones.
    qreal colourProgress() const { return m_colourProgress; }
    bool dotAnimationRunning() const { return m_dotTimer->isActive(); }
    bool colourAnimationRunning() const { return m_colourTimer->isActive(); }

    // --- tokens --------------------------------------------------------------------
    const MdRadioButtonTokens &radioButtonTokens() const;
    void setRadioButtonTokens(const MdRadioButtonTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- interaction state -----------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers --------------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

protected:
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
    void onDotTick();
    void onColourTick();

private:
    void init();
    void invalidateTokens();
    void restartAnimations();

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;
    bool m_lastChecked = false;

    // The dot's diameter transition (Compose's `animateDpAsState` on the fast
    // spatial spring) and the colour fade's record.
    qreal m_dotDiameter = 0.0;
    qreal m_dotFrom = 0.0;
    qreal m_dotTo = 0.0;
    QElapsedTimer m_dotClock;

    qreal m_colourProgress = 0.0;
    qreal m_colourFrom = 0.0;
    qreal m_colourTo = 0.0;
    QElapsedTimer m_colourClock;

    mutable MdRadioButtonTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    QTimer *m_dotTimer = nullptr;
    QTimer *m_colourTimer = nullptr;
    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

} // namespace md

#endif // MD_RADIO_BUTTON_H
