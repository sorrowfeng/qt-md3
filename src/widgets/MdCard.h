#ifndef MD_CARD_H
#define MD_CARD_H

// MdCard — MD3 cards: the containment family's raised content surfaces.
//
// Three variants, one class:
//
//   variant  md.comp.filled-card.* | elevated-card.* | outlined-card.*
//
// material-web publishes no card *web component* (token export only), so the
// behaviour source is Compose M3's Card.kt, which publishes exactly two
// shapes per variant: a plain surface (no input handling, no interactive
// state) and a clickable one (state layers, ripple, focusability). This class
// is one widget with a `clickable` switch carrying the same distinction:
//
//   * non-clickable (default): the enabled row is the whole story — no state
//     layer, no ripple, no focus, no reserved ring margin. The container is
//     the widget rect.
//   * clickable: hover / keyboard-focus state layers, a press ripple (the
//     pressed row's colour — press itself paints no flat layer), the
//     :focus-visible focus ring with its reserved margin, Space/Enter
//     activation, and the `clicked()` signal.
//
// Cards are containers: children are placed by any layout installed on the
// widget, and the widget's contents margins always match the container rect
// (the focus margin for a clickable card, zero otherwise), so a layout fills
// the painted container exactly. Compose publishes no default content
// padding — neither does this class.
//
// Two behaviours Compose publishes that the flat token export does not show:
//
//   * the elevation *animates* between the state rows' dp values (Compose
//     runs it through the theme's motion scheme; this port uses the
//     published standard easing over the short4 duration — recorded in
//     docs/porting-todo.md). Disabled snaps, as Compose does.
//   * a `dragged` state, driven here by an explicit setter for external drag
//     frameworks (Compose's DragInteraction), painting the dragged row: the
//     highest elevation and its own state layer.
//
// Why plain QWidget (not QAbstractButton): a card's primary job is to hold
// arbitrary children; the click surface is optional and small enough to
// handle by hand (press/release/keys), and keeping the widget a plain
// container avoids QAbstractButton's checkable/autorepeat machinery leaking
// into the card API.

#include "core/MdCardTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>
#include <QtCore/QVariantAnimation>
#include <QtWidgets/QWidget>

class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

namespace md {

class MdCardStyle;
class MdFocusRingController;

class QT_MD3_EXPORT MdCard : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(md::MdCardVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(bool clickable READ isClickable WRITE setClickable NOTIFY clickableChanged)
    Q_PROPERTY(bool dragged READ isDragged WRITE setDragged NOTIFY draggedChanged)

public:
    explicit MdCard(QWidget *parent = nullptr);
    explicit MdCard(MdCardVariant variant, QWidget *parent = nullptr);
    ~MdCard() override;

    // --- variant ------------------------------------------------------------
    MdCardVariant variant() const { return m_variant; }
    void setVariant(MdCardVariant variant);

    // --- clickability -------------------------------------------------------
    /// True when the card handles input (state layers, ripple, focus ring,
    /// `clicked()`). Toggling this re-reserves the focus margin, so the
    /// contents margins change with it.
    bool isClickable() const { return m_clickable; }
    void setClickable(bool clickable);

    // --- drag ---------------------------------------------------------------
    /// External drag frameworks set this while a drag that moves the card is
    /// in progress; it paints the dragged row (highest elevation, its own
    /// state layer).
    bool isDragged() const { return m_dragged; }
    void setDragged(bool dragged);

    // --- interaction state --------------------------------------------------
    /// Hover is tracked explicitly — QWidget::underMouse() is not dependable
    /// before the first enter event and is never set under the offscreen
    /// platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }
    /// True between a mouse press and its release, clickable only.
    bool isPressed() const { return m_pressed; }

    /// True when the card holds focus *and* that focus is keyboard focus —
    /// the `:focus-visible` rule. See MdButton for the reason-classification.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    // --- tokens -------------------------------------------------------------
    /// The resolved `md.comp.<variant>-card.*` set for this card, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdCardTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this card alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers --------------------------------------------------------
    /// Owned by the card; the style paints from them. Null while the card is
    /// non-clickable (no ripple is ever painted).
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    // --- geometry -----------------------------------------------------------
    /// The painted container, in widget coordinates — the widget rect minus
    /// the focus margin (zero for a non-clickable card). Contents margins
    /// match, so a layout installed on the card fills this exactly.
    QRectF containerRect() const;

    /// The animated elevation the paint uses, in dp. Follows the current
    /// state's row; non-clickable cards report the enabled row's value
    /// directly (no ladder to animate).
    qreal currentShadowDp() const { return m_animatedShadowDp; }

    /// True while the elevation is animating towards the current state's
    /// target. Exists for the test suite.
    bool isElevationAnimating() const;

    // QWidget
    QSize sizeHint() const override;

signals:
    void variantChanged(md::MdCardVariant variant);
    void clickableChanged(bool clickable);
    void draggedChanged(bool dragged);
    /// Emitted for a click (mouse release inside, or Space / Enter) —
    /// clickable cards only.
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    /// Drop the cached token set; the next tokens() call rebuilds it.
    void invalidateTokens();
    /// Keep the ripple controllers' bounds and clip in step with the
    /// container.
    void syncRippleGeometry();
    /// Start (or snap) the elevation animation towards the current state's
    /// row value.
    void updateElevationTarget();
    /// The elevation row value (dp) for the state painting would use now.
    qreal targetShadowDp() const;
    /// Apply the contents margins + focus policy a clickability change implies.
    void applyClickability();

    MdCardVariant m_variant = MdCardVariant::Filled;
    bool m_clickable = false;
    bool m_dragged = false;

    bool m_hovered = false;
    bool m_pressed = false;
    bool m_focusIsKeyboard = false;

    mutable MdCardTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;

    QVariantAnimation m_elevationAnimation;
    qreal m_animatedShadowDp = 0.0;
};

} // namespace md

#endif // MD_CARD_H
