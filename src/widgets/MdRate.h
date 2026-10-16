#ifndef MD_RATE_H
#define MD_RATE_H

// MdRate — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Icon button + 选中态" — icon button plus the
// selected state. MD3 has no rating family. The control is the standard
// star / heart scorer built from the icon-button language:
//
//   glyph       Material Symbols `star` (or `favorite` for the heart variant)
//   selected    filled glyph in `primary` — the icon-button selected reading
//   unselected  outlined glyph in `on-surface-variant`
//   hover       a preview fill up to the hovered index, plus the icon-button
//               hover state layer 0.08 on the hovered cell
//   half        optional half-fill, drawn as a clipped filled glyph
//
// Cells are 48×48 targets (`md.comp.icon-button` medium container height is
// 40 with the default track; the extra 8 px is the focus-indicator margin the
// other icon-button hosts reserve). The widget paints the glyphs itself so a
// half cell can be expressed — a real MdIconButton cannot split its icon —
// but the colours, state layers, and the focus ring are the Stage 1 ones.
//
// Keyboard: Left/Right (or Up/Down) move the value by 1 (0.5 when
// `halfSteps` is on), Home/End jump to the ends, digits 0..9 set absolute
// values. Tab-reachable; the focus ring is `:focus-visible` only.
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QKeyEvent;
class QMouseEvent;
class QPaintEvent;

namespace md {

/// The glyph set a rate control draws.
enum class MdRateGlyph {
    Star,
    Heart,
    Count,
};

class QT_MD3_EXPORT MdRate : public QWidget
{
    Q_OBJECT

    /// Current value in [0, max]. May be fractional when `halfSteps` is on.
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    /// Number of cells. Minimum 1.
    Q_PROPERTY(int max READ max WRITE setMax NOTIFY maxChanged)
    /// When true the control ignores input and paints the value only.
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly NOTIFY readOnlyChanged)
    /// Allow half values (0.5 steps) on click and keyboard.
    Q_PROPERTY(bool halfSteps READ halfSteps WRITE setHalfSteps NOTIFY halfStepsChanged)
    /// Star (default) or heart glyph.
    Q_PROPERTY(md::MdRateGlyph glyph READ glyph WRITE setGlyph NOTIFY glyphChanged)

public:
    explicit MdRate(QWidget *parent = nullptr);
    ~MdRate() override;

    qreal value() const { return m_value; }
    void setValue(qreal value);

    int max() const { return m_max; }
    void setMax(int max);

    bool isReadOnly() const { return m_readOnly; }
    void setReadOnly(bool readOnly);

    bool halfSteps() const { return m_halfSteps; }
    void setHalfSteps(bool halfSteps);

    MdRateGlyph glyph() const { return m_glyph; }
    void setGlyph(MdRateGlyph glyph);

    /// Hover preview value (the cell under the pointer), or -1 when not
    /// hovering. Paint uses `qMax(0, preview)` when it is active.
    qreal hoverValue() const { return m_hoverValue; }

    /// Resolved selected fill: primary.
    QColor resolvedSelectedColor() const;
    /// Resolved unselected outline: on-surface-variant.
    QColor resolvedUnselectedColor() const;
    /// Resolved hover state-layer content colour: on-surface.
    QColor resolvedStateLayerContent() const;

    /// Edge length of one cell, in logical px (48).
    static qreal cellSize();

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(qreal value);
    void maxChanged(int max);
    void readOnlyChanged(bool readOnly);
    void halfStepsChanged(bool halfSteps);
    void glyphChanged(md::MdRateGlyph glyph);
    /// Emitted after a click or keyboard commit, with the new value.
    void rated(qreal value);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    qreal cellValueAt(const QPointF &position) const;
    void commit(qreal value);

    qreal m_value = 0.0;
    int m_max = 5;
    bool m_readOnly = false;
    bool m_halfSteps = false;
    MdRateGlyph m_glyph = MdRateGlyph::Star;

    qreal m_hoverValue = -1.0;
    bool m_hovered = false;

    /// Whether the focus currently held arrived by keyboard (`:focus-visible`).
    bool m_focusIsKeyboard = false;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdRateGlyph)

#endif // MD_RATE_H
