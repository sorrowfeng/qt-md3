#ifndef MD_INPUT_NUMBER_H
#define MD_INPUT_NUMBER_H

// MdInputNumber — Stage 2 Qt extension (Appendix D, P1), the spin-box
// replacement.
//
// Semantic anchor (appendix D): "Text field + Icon button 步进" — the
// stepper is not a native QSpinBox arrow, it is a pair of real
// `MdIconButton`s sitting in the field's trailing slot:
//
//   field      md.comp.filled-text-field | outlined-text-field (export
//              34.0.21), the same MdTextField MdSelect anchors on
//   steppers   md.comp.icon-button.standard, xsmall (32 px container),
//              `arrow_drop_up` / `arrow_drop_down` — the same pair
//              MdSelect uses for its dropdown indicator, mirrored
//   value      label-large input text, on-surface
//   prefix / suffix  body-medium, on-surface-variant, painted as part of
//              the editable string so the caret stays honest
//
// Only the *composition* is new. Every colour, radius, height, type style
// and state layer comes from the text-field and icon-button token sets.
//
// Behaviour follows QSpinBox / QDoubleSpinBox where the semantics match:
// `value` is clamped to [minimum, maximum], `singleStep` moves it, Up/Down
// step when the field has focus, and editing the text commits on Enter or
// on focus-out. A non-numeric edit restores the last good value and raises
// the text field's error overlay.
//
// There is no QSS. The field is an `MdTextField`, the steppers
// `MdIconButton`s.

#include "core/MdQtCompat.h"
#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QEvent;
class QFocusEvent;
class QKeyEvent;

namespace md {

class MdIconButton;
class MdTextField;

class QT_MD3_EXPORT MdInputNumber : public QWidget
{
    Q_OBJECT

    /// Current numeric value, clamped to [minimum, maximum].
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    /// Inclusive lower bound.
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum NOTIFY minimumChanged)
    /// Inclusive upper bound.
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum NOTIFY maximumChanged)
    /// Increment applied by one stepper press or one Up/Down key.
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep NOTIFY singleStepChanged)
    /// Digits after the decimal point used when formatting. 0 shows an int.
    Q_PROPERTY(int decimals READ decimals WRITE setDecimals NOTIFY decimalsChanged)
    /// Optional literal drawn before the number inside the field.
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    /// Optional literal drawn after the number inside the field.
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY suffixChanged)
    /// When true the field refuses edits and the steppers go disabled.
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    /// The field chrome: filled or outlined text field.
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    /// Floating label above the input line.
    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    /// Body-small supporting line under the container.
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    /// The error overlay (indicator / label / supporting text).
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)
    /// Whether the stepper pair is shown at all.
    Q_PROPERTY(bool steppersVisible READ areSteppersVisible WRITE setSteppersVisible NOTIFY
                   steppersVisibleChanged)

public:
    explicit MdInputNumber(QWidget *parent = nullptr);
    ~MdInputNumber() override;

    // --- value ---------------------------------------------------------------
    double value() const { return m_value; }
    void setValue(double value);

    double minimum() const { return m_minimum; }
    void setMinimum(double minimum);

    double maximum() const { return m_maximum; }
    void setMaximum(double maximum);

    double singleStep() const { return m_singleStep; }
    void setSingleStep(double step);

    int decimals() const { return m_decimals; }
    void setDecimals(int decimals);

    /// Set the bounds together so a new range cannot leave `value` out of
    /// range for a frame.
    void setRange(double minimum, double maximum);

    // --- formatting ----------------------------------------------------------
    QString prefix() const { return m_prefix; }
    void setPrefix(const QString &prefix);

    QString suffix() const { return m_suffix; }
    void setSuffix(const QString &suffix);

    /// The number formatted with `decimals` and locale-aware separators,
    /// without prefix/suffix.
    QString valueText() const;

    // --- field chrome --------------------------------------------------------
    bool isReadOnly() const;
    void setReadOnly(bool readOnly);

    MdTextFieldVariant variant() const;
    void setVariant(MdTextFieldVariant variant);

    QString labelText() const;
    void setLabelText(const QString &label);

    QString supportingText() const;
    void setSupportingText(const QString &text);

    bool hasError() const;
    void setError(bool error);

    bool areSteppersVisible() const { return m_steppersVisible; }
    void setSteppersVisible(bool visible);

    /// The underlying field, for callers that need QLineEdit access.
    MdTextField *textField() const { return m_field; }

    // --- interaction ---------------------------------------------------------
    /// Step the value by `+singleStep` / `-singleStep`. Clamped. No-ops when
    /// the control is read-only or disabled. Returns the new value.
    double stepUp();
    double stepDown();

    // QWidget
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(double value);
    void minimumChanged(double minimum);
    void maximumChanged(double maximum);
    void singleStepChanged(double step);
    void decimalsChanged(int decimals);
    void prefixChanged(const QString &prefix);
    void suffixChanged(const QString &suffix);
    void readOnlyChanged(bool readOnly);
    void variantChanged(md::MdTextFieldVariant variant);
    void labelTextChanged(const QString &label);
    void supportingTextChanged(const QString &text);
    void errorChanged(bool error);
    void steppersVisibleChanged(bool visible);
    /// Emitted after a stepper press or an Up/Down key, once `value` settled.
    void stepped(double value);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onFieldEditingFinished();
    void onFieldTextChanged(const QString &text);
    void onUpClicked();
    void onDownClicked();
    void onThemeChanged();

private:
    void init();
    void layoutChildren();
    void syncFieldText();
    void applyClamped(double value, bool emitStepped);
    bool commitFromText();
    bool isStepperEnabled(bool up) const;

    MdTextField *m_field = nullptr;
    MdIconButton *m_up = nullptr;
    MdIconButton *m_down = nullptr;

    double m_value = 0.0;
    double m_minimum = 0.0;
    double m_maximum = 99.0;
    double m_singleStep = 1.0;
    int m_decimals = 0;
    QString m_prefix;
    QString m_suffix;
    bool m_steppersVisible = true;
    bool m_syncingText = false;
    QString m_lastGoodText;
};

} // namespace md

#endif // MD_INPUT_NUMBER_H
