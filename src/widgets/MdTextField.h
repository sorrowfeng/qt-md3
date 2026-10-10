#ifndef MD_TEXT_FIELD_H
#define MD_TEXT_FIELD_H

// MdTextField — the MD3 text field, filled and outlined
// (`md.comp.{filled,outlined}-text-field.*`, export 34.0.21).
//
// One widget over QLineEdit's editing contract: the label floats from the
// input line (body-large) to the container's top edge (body-small) once the
// field is populated or focused, the active indicator carries the state's
// spine (1 px on-surface-variant resting / on-surface hover / 2 px primary
// focus / error), and the supporting text and icons resolve by state.
//
// The filled variant is surface-container-highest with corner-extra-small-top
// (4 px top corners only); the outlined variant is transparent with a 1 px
// outline and corner-extra-small (4 px all round).
//
// Keyboard is QLineEdit's: Tab moves focus, the text edits normally. The
// `error` property swaps the indicator / label / supporting text / trailing
// icon to the error colour.

#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QLineEdit>

namespace md {

class MdTextFieldStyle;

class QT_MD3_EXPORT MdTextField : public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(bool populated READ isPopulated NOTIFY populatedChanged)

public:
    explicit MdTextField(QWidget *parent = nullptr);
    explicit MdTextField(const QString &contents, QWidget *parent = nullptr);
    ~MdTextField() override;

    QString labelText() const { return m_label; }
    void setLabelText(const QString &label);

    QString supportingText() const { return m_supporting; }
    void setSupportingText(const QString &text);

    QString placeholderText() const { return m_placeholder; }
    void setPlaceholderText(const QString &text);

    bool hasError() const { return m_error; }
    void setError(bool error);

    MdTextFieldVariant variant() const { return m_variant; }
    void setVariant(MdTextFieldVariant variant);

    /// True when the label is floated: the field is focused or has text.
    bool isPopulated() const;

    // --- geometry -----------------------------------------------------------
    /// The container rect — the 56 px band the indicator sits under.
    QRectF containerRect() const;
    /// The floating label's rect (populated) or the input line's (at rest).
    QRectF labelRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- interaction state -------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    /// The label's 0..1 float factor (0 = in the input line, 1 = at the top).
    qreal labelFloat() const { return m_labelFloat; }

    const MdTextFieldTokens &textFieldTokens() const;
    void setTextFieldTokens(const MdTextFieldTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void labelTextChanged(const QString &label);
    void supportingTextChanged(const QString &text);
    void placeholderTextChanged(const QString &text);
    void errorChanged(bool error);
    void variantChanged(md::MdTextFieldVariant variant);
    void populatedChanged(bool populated);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void retargetAnimations();
    void tickAnimations();

    QString m_label;
    QString m_supporting;
    QString m_placeholder;
    bool m_error = false;
    MdTextFieldVariant m_variant = MdTextFieldVariant::Filled;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    qreal m_labelFloat = 0.0;
    qreal m_labelFloatTarget = 0.0;

    mutable MdTextFieldTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_TEXT_FIELD_H
