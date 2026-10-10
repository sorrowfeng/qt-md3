#ifndef MD_TEXT_AREA_H
#define MD_TEXT_AREA_H

// MdTextArea — the MD3 multiline text field, `type="textarea"`
// (material-web's contract over the shared `md.comp.{filled,outlined}-text-field.*`
// export, 34.0.21).
//
// The same field as `MdTextField` with a multiline body: the label floats
// (body-large at rest → body-small populated / focused), the active indicator
// carries the state's spine and the error overlay applies. The body is
// `rows` lines tall (material-web's default 2) and the container grows with
// the text; the indicator sits under the *last* line, like the HTML textarea.
//
// The token set is the shared text-field one — the export publishes no
// `text-area` namespace; material-web renders `type="textarea"` through the
// same rows. `MdTextFieldTokens::resolve` serves both.

#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QPlainTextEdit>

namespace md {

class MdTextAreaStyle;

class QT_MD3_EXPORT MdTextArea : public QPlainTextEdit
{
    Q_OBJECT

    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(bool populated READ isPopulated NOTIFY populatedChanged)

public:
    explicit MdTextArea(QWidget *parent = nullptr);
    ~MdTextArea() override;

    QString labelText() const { return m_label; }
    void setLabelText(const QString &label);

    QString supportingText() const { return m_supporting; }
    void setSupportingText(const QString &text);

    bool hasError() const { return m_error; }
    void setError(bool error);

    MdTextFieldVariant variant() const { return m_variant; }
    void setVariant(MdTextFieldVariant variant);

    /// material-web's `rows` — the body's height in text lines (default 2).
    int rows() const { return m_rows; }
    void setRows(int rows);

    /// True when the label is floated: the field is focused or has text.
    bool isPopulated() const;

    // --- geometry -----------------------------------------------------------
    /// The container rect — the whole field including the body.
    QRectF containerRect() const;
    /// The floating label's rect (populated) or the first line's (at rest).
    QRectF labelRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- interaction state -------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    /// The label's 0..1 float factor (0 = in the first line, 1 = at the top).
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
    void errorChanged(bool error);
    void variantChanged(md::MdTextFieldVariant variant);
    void rowsChanged(int rows);
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
    bool m_error = false;
    MdTextFieldVariant m_variant = MdTextFieldVariant::Filled;
    int m_rows = 2;

    bool m_hovered = false;
    bool m_focusIsKeyboard = false;

    qreal m_labelFloat = 0.0;
    qreal m_labelFloatTarget = 0.0;

    mutable MdTextFieldTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_TEXT_AREA_H
