#ifndef MD_PLAIN_TEXT_EDIT_H
#define MD_PLAIN_TEXT_EDIT_H

// MdPlainTextEdit — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "在阶段一 Text field 多行 variant 之上补编辑/
// 缩放能力，不重造外观" — editing capabilities over the Stage 1 text area,
// not a new look.
//
// MdPlainTextEdit is the *capability* reading of the multiline text field:
// a QPlainTextEdit with the MdTextArea chrome contract (filled / outlined
// variant, floating label, supporting text, error state) plus the editor
// features a plain text control usually needs and MdTextArea deliberately
// does not carry:
//
//   * line numbers in a leading gutter (label-small / on-surface-variant)
//   * current-line highlight (the state-layer hover reading, 0.08)
//   * find / replace API with match highlighting
//   * zoom (Ctrl+Wheel) that scales the type size within published bounds
//
// The look stays the Stage 1 text-field tokens; nothing here invents a
// colour or radius.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTextFieldTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QPlainTextEdit>

namespace md {

class QT_MD3_EXPORT MdPlainTextEdit : public QPlainTextEdit
{
    Q_OBJECT

    Q_PROPERTY(QString labelText READ labelText WRITE setLabelText NOTIFY labelTextChanged)
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    Q_PROPERTY(bool error READ hasError WRITE setError NOTIFY errorChanged)
    Q_PROPERTY(md::MdTextFieldVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(bool lineNumbersVisible READ lineNumbersVisible WRITE setLineNumbersVisible NOTIFY
                   lineNumbersVisibleChanged)
    Q_PROPERTY(bool currentLineHighlight READ currentLineHighlight WRITE setCurrentLineHighlight
                   NOTIFY currentLineHighlightChanged)
    Q_PROPERTY(qreal zoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)

public:
    explicit MdPlainTextEdit(QWidget *parent = nullptr);
    ~MdPlainTextEdit() override;

    QString labelText() const { return m_label; }
    void setLabelText(const QString &label);

    QString supportingText() const { return m_supporting; }
    void setSupportingText(const QString &text);

    bool hasError() const { return m_error; }
    void setError(bool error);

    MdTextFieldVariant variant() const { return m_variant; }
    void setVariant(MdTextFieldVariant variant);

    bool lineNumbersVisible() const { return m_lineNumbers; }
    void setLineNumbersVisible(bool visible);

    bool currentLineHighlight() const { return m_currentLineHighlight; }
    void setCurrentLineHighlight(bool on);

    /// Font scale applied on top of the type scale. Clamped to 0.5 .. 3.0.
    qreal zoomFactor() const { return m_zoom; }
    void setZoomFactor(qreal factor);

    /// Find `needle` from the cursor. Returns true and selects the match.
    bool findNext(const QString &needle, bool caseSensitive = false);
    /// Replace the current selection (if it equals `from`) and find the next.
    bool replace(const QString &from, const QString &to, bool caseSensitive = false);
    /// Replace every occurrence of `from` with `to`. Returns the count.
    int replaceAll(const QString &from, const QString &to, bool caseSensitive = false);

    /// Width the line-number gutter occupies right now (0 when hidden).
    int gutterWidth() const;

    /// Resolved border / indicator colour for the current state.
    QColor resolvedIndicatorColor() const;
    /// Resolved supporting-text colour.
    QColor resolvedSupportingColor() const;
    /// Resolved gutter text colour.
    QColor resolvedGutterColor() const;

signals:
    void labelTextChanged(const QString &label);
    void supportingTextChanged(const QString &text);
    void errorChanged(bool error);
    void variantChanged(md::MdTextFieldVariant variant);
    void lineNumbersVisibleChanged(bool visible);
    void currentLineHighlightChanged(bool on);
    void zoomFactorChanged(qreal factor);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void updateGutterWidth();

private:
    void init();
    void applyZoomFont();
    void paintGutter(QPaintEvent *event);

    QString m_label;
    QString m_supporting;
    bool m_error = false;
    MdTextFieldVariant m_variant = MdTextFieldVariant::Filled;
    bool m_lineNumbers = true;
    bool m_currentLineHighlight = true;
    qreal m_zoom = 1.0;
};

} // namespace md

#endif // MD_PLAIN_TEXT_EDIT_H
