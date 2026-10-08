#ifndef MD_TOOLTIP_H
#define MD_TOOLTIP_H

// MdTooltip — MD3 tooltips, the Communication family's closing pair: the
// `md.comp.plain-tooltip.*` visuals (inverse-surface, extra-small corner, no
// elevation, body-small inverse-on-surface text) and the
// `md.comp.rich-tooltip.*` visuals (surface-container at level 2, medium
// corner, title-small subhead + body-medium text in on-surface-variant, a
// label-large primary action) plus the Expressive caret.
//
// material-web ships only the token exports for this family, so the layout is
// a faithful port of Compose M3's Tooltip.kt and the behaviour of the host
// from BasicTooltip.kt — see MdTooltipHost.
//
// This widget is the visuals only ("This component provides only the visuals
// of the Tooltip", the same split as MdSnackbar): content, geometry, the rich
// action's pointer + keyboard contract and the caret. When it hosts an action
// the action is an internal region — the export styles the label text
// directly and publishes its own hover / focus / pressed rows, so composing a
// real button child would drag the button family's focus rings in.
//
// The widget rectangle carries margins the way the snackbar does: the rich
// container's level-2 shadow spreads past the container (kShadowMargin), and
// an enabled caret protrudes kCaretMargin past the anchor-facing edge. A
// plain tooltip without a caret is exactly its container.

#include "core/MdQtCompat.h"
#include "core/MdRipple.h"
#include "core/MdTooltipTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdTooltipStyle;

class QT_MD3_EXPORT MdTooltip : public QWidget
{
    Q_OBJECT

    /// Which family this is — the export's two token sets.
    Q_PROPERTY(md::MdTooltipVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    /// The supporting text (body-small inverse-on-surface plain, body-medium
    /// on-surface-variant rich).
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// The rich subhead (title-small on-surface-variant); empty = none.
    /// Ignored while the variant is Plain.
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// The rich action's label (label-large primary); empty = none.
    Q_PROPERTY(QString actionLabel READ actionLabel WRITE setActionLabel
                   NOTIFY actionLabelChanged)
    /// Which side the caret protrudes on. None by default — Compose's
    /// caretShape parameter is also opt-in.
    Q_PROPERTY(md::MdTooltipCaretSide caretSide READ caretSide WRITE setCaretSide NOTIFY
                   caretSideChanged)

public:
    explicit MdTooltip(QWidget *parent = nullptr);
    MdTooltip(const QString &text, QWidget *parent = nullptr);
    ~MdTooltip() override;

    // --- content -------------------------------------------------------------
    MdTooltipVariant variant() const { return m_variant; }
    void setVariant(MdTooltipVariant variant);

    QString text() const { return m_text; }
    void setText(const QString &text);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString actionLabel() const { return m_actionLabel; }
    void setActionLabel(const QString &actionLabel);
    bool hasAction() const { return !m_actionLabel.isEmpty(); }

    MdTooltipCaretSide caretSide() const { return m_caretSide; }
    void setCaretSide(MdTooltipCaretSide side);

    // --- interactive element state (rich action, painted by the style) -------
    MdTooltipActionState actionState() const;

    /// `:focus-visible` semantics, shared with the button families: only
    /// keyboard focus counts.
    bool actionHasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    // --- geometry -------------------------------------------------------------
    /// The surface (container) rect inside the shadow / caret margins.
    QRectF containerRect() const;
    /// The caret triangle; invalid with MdTooltipCaretSide::None.
    QRectF caretRect() const;
    QRectF actionRect() const;
    /// The action's interactive region: the label grown by the text-button
    /// chrome the export does not publish (12 px each side).
    QRectF actionHitRect() const;

    // --- ripples (owned here, painted by the style) ---------------------------
    MdRippleFrame actionRippleFrame() const { return m_actionRipple.currentFrame(); }

    /// Extra alpha multiplied into every painted colour — the host's
    /// enter/exit transition drives this.
    qreal paintOpacity() const { return m_paintOpacity; }
    void setPaintOpacity(qreal opacity);

    // --- tokens ---------------------------------------------------------------
    const MdTooltipTokens &tokens() const;
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
    void variantChanged(md::MdTooltipVariant variant);
    void textChanged(const QString &text);
    void titleChanged(const QString &title);
    void actionLabelChanged(const QString &actionLabel);
    void caretSideChanged(md::MdTooltipCaretSide side);
    /// The rich action was activated (pointer, keyboard or accessibility).
    void actionClicked();

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
    void setHovered(bool hovered);
    bool actionRegionAt(const QPointF &position) const;

    MdTooltipVariant m_variant = MdTooltipVariant::Plain;
    QString m_text;
    QString m_title;
    QString m_actionLabel;
    MdTooltipCaretSide m_caretSide = MdTooltipCaretSide::None;

    bool m_hovered = false;
    bool m_pressed = false;
    bool m_focusIsKeyboard = false;

    mutable MdTooltipTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    qreal m_paintOpacity = 1.0;

    MdRippleController m_actionRipple;
};

} // namespace md

#endif // MD_TOOLTIP_H
