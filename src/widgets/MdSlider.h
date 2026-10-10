#ifndef MD_SLIDER_H
#define MD_SLIDER_H

// MdSlider — the MD3 slider, `md.comp.slider.*` (export 34.0.21).
//
// One widget, two forms (material-web's contract): the single slider carries
// `value`, and `range` turns it into a two-handle slider over `valueStart` /
// `valueEnd`. Five Expressive sizes (`MdSliderSize`), continuous or discrete
// (`step` > 0), optional tick marks (`ticks`) and an optional value label
// (`labeled`) that scales in from the handle while it is hovered, focused or
// pressed — material-web's `.label` triple.
//
// The handle is the export's vertical pill (4 px wide, 44 px tall at the base
// size), not a circle: the width narrows to 2 px under focus and press and
// returns to 4 px on release; hover keeps 4 px. Colours resolve by state with
// no animation (the same standing as chips and switch). The handle carries the
// family's only shadow (level 1, level 0 disabled).
//
// Keyboard (the material-web native-input contract):
//   Left/Down −step, Right/Up +step, PageDown/PageUp ±(max−min)/10,
//   Home = min, End = max. In range form the arrows move the *active* handle
//   (the one that was last pressed, or the end handle at rest); Tab cycles
//   the active handle.

#include "core/MdSliderTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

namespace md {

class MdSliderStyle;

class QT_MD3_EXPORT MdSlider : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(qreal valueStart READ valueStart WRITE setValueStart NOTIFY valueStartChanged)
    Q_PROPERTY(qreal valueEnd READ valueEnd WRITE setValueEnd NOTIFY valueEndChanged)
    Q_PROPERTY(qreal min READ min WRITE setMin NOTIFY minChanged)
    Q_PROPERTY(qreal max READ max WRITE setMax NOTIFY maxChanged)
    Q_PROPERTY(qreal step READ step WRITE setStep NOTIFY stepChanged)
    Q_PROPERTY(bool ticks READ hasTicks WRITE setTicks NOTIFY ticksChanged)
    Q_PROPERTY(bool labeled READ isLabeled WRITE setLabeled NOTIFY labeledChanged)
    Q_PROPERTY(bool range READ isRange WRITE setRange NOTIFY rangeChanged)
    Q_PROPERTY(md::MdSliderSize sliderSize READ sliderSize WRITE setSliderSize NOTIFY
                   sliderSizeChanged)
    Q_PROPERTY(QString valueLabel READ valueLabel WRITE setValueLabel NOTIFY valueLabelChanged)

public:
    explicit MdSlider(QWidget *parent = nullptr);
    ~MdSlider() override;

    // --- value model ------------------------------------------------------
    qreal value() const { return m_value; }
    void setValue(qreal value);

    qreal valueStart() const { return m_valueStart; }
    void setValueStart(qreal value);

    qreal valueEnd() const { return m_valueEnd; }
    void setValueEnd(qreal value);

    qreal min() const { return m_min; }
    void setMin(qreal min);

    qreal max() const { return m_max; }
    void setMax(qreal max);

    /// The discrete step. `0` (default) means continuous.
    qreal step() const { return m_step; }
    void setStep(qreal step);

    bool hasTicks() const { return m_ticks; }
    void setTicks(bool ticks);

    bool isLabeled() const { return m_labeled; }
    void setLabeled(bool labeled);

    bool isRange() const { return m_range; }
    void setRange(bool range);

    MdSliderSize sliderSize() const { return m_size; }
    void setSliderSize(MdSliderSize size);

    /// The label shown in the value indicator; empty shows the number.
    QString valueLabel() const { return m_valueLabel; }
    void setValueLabel(const QString &label);

    // --- geometry -----------------------------------------------------------
    /// The track rect — the horizontal band the handles travel along.
    QRectF trackRect() const;
    /// The handle's pill rect for the active (or given) handle.
    QRectF handleRect(bool startHandle = false) const;
    /// The 40 px state-layer circle at the handle's centre.
    QRectF stateLayerRect(bool startHandle = false) const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- interaction state ---------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }
    /// True while the active handle is pressed (the value indicator shows).
    bool isPressed() const { return m_pressed; }

    /// The handle width the paint should use for the current state.
    qreal animatedHandleWidth() const { return m_handleWidth; }
    /// The value indicator's 0..1 reveal factor.
    qreal labelReveal() const { return m_labelReveal; }

    // --- tokens ---------------------------------------------------------------
    const MdSliderTokens &sliderTokens() const;
    void setSliderTokens(const MdSliderTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    /// The colour table row for the paint: Disabled first, matching Compose's
    /// `!enabled -> …` ordering.
    MdSliderInteraction interactionState() const;

signals:
    void valueChanged(qreal value);
    void valueStartChanged(qreal value);
    void valueEndChanged(qreal value);
    void minChanged(qreal min);
    void maxChanged(qreal max);
    void stepChanged(qreal step);
    void ticksChanged(bool ticks);
    void labeledChanged(bool labeled);
    void rangeChanged(bool range);
    void sliderSizeChanged(md::MdSliderSize size);
    void valueLabelChanged(const QString &label);
    void sliderPressed();
    void sliderReleased();
    void sliderMoved(qreal value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    qreal clampValue(qreal value) const;
    qreal snap(qreal value) const;
    qreal fractionOf(qreal value) const;
    qreal valueAt(qreal x) const;
    void setActiveFromPoint(const QPointF &pos);
    void moveActiveBy(qreal delta);
    /// Which handle the pointer at `pos` is nearer to (range form).
    bool startHandleNear(const QPointF &pos) const;
    void retargetAnimations();
    void tickAnimations();
    QString labelFor(qreal value) const;

    qreal m_value = 0.0;
    qreal m_valueStart = 0.0;
    qreal m_valueEnd = 100.0;
    qreal m_min = 0.0;
    qreal m_max = 100.0;
    qreal m_step = 0.0;
    bool m_ticks = false;
    bool m_labeled = false;
    bool m_range = false;
    MdSliderSize m_size = MdSliderSize::Medium;
    QString m_valueLabel;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;
    bool m_pressed = false;
    bool m_activeIsStart = false;

    qreal m_handleWidth = 4.0;
    qreal m_handleWidthTarget = 4.0;
    qreal m_labelReveal = 0.0;
    qreal m_labelRevealTarget = 0.0;

    mutable MdSliderTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_SLIDER_H
