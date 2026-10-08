#ifndef MD_FAB_H
#define MD_FAB_H

// MdFab — MD3 floating action buttons.
//
// Four colour sets, three sizes, and the lowered form:
//
//   variant  md.comp.fab.surface | primary | secondary | tertiary
//   size     md.comp.fab.small | medium | large
//   lowered  the `lowered-*` elevation rows (and the surface variant's
//            `lowered.container.color`)
//
// A FAB is an icon-only action on a raised, shadowed container: the token
// arithmetic gives it exactly one icon and no label (the label form is the
// *extended* FAB, a separate component — see docs/porting-todo.md for what is
// deliberately not in this class).
//
// Interaction shape, shared with MdButton / MdIconButton and recorded in
// docs/interaction-reference.md:
//
//   * hover and keyboard focus paint the flat state layer; press does not —
//     the press response is the ripple alone ("Pressed (ripple)" rows);
//   * the ripple colour is the pressed row's state-layer role, not a global
//     on-surface;
//   * the focus indicator and the focused colours follow `:focus-visible`
//     semantics — pointer focus shows neither;
//   * the container shape never morphs on press: the export publishes no
//     pressed shape for this family, unlike the button/icon-button ones.
//
// Why QPushButton: exactly the argument MdButton makes — QAbstractButton
// already owns clicked/pressed/released, the keyboard activation and the
// accessibility role; overriding the painting is the small surface.

#include "core/MdFabTokens.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtWidgets/QPushButton>

class QFocusEvent;
class QMouseEvent;

namespace md {

class MdFabStyle;
class MdFocusRingController;

class QT_MD3_EXPORT MdFab : public QPushButton
{
    Q_OBJECT

    Q_PROPERTY(md::FabVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::FabSize fabSize READ fabSize WRITE setFabSize NOTIFY fabSizeChanged)
    Q_PROPERTY(bool lowered READ isLowered WRITE setLowered NOTIFY loweredChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily
                   NOTIFY iconFamilyChanged)

public:
    explicit MdFab(QWidget *parent = nullptr);
    /// `iconName` follows the Material Symbols / classic-SVG name convention
    /// the rest of the library uses (`"add"`, `"edit"`, …).
    explicit MdFab(const QString &iconName, QWidget *parent = nullptr);
    ~MdFab() override;

    // --- variant / size / lowered ------------------------------------------
    FabVariant variant() const { return m_variant; }
    void setVariant(FabVariant variant);

    FabSize fabSize() const { return m_size; }
    void setFabSize(FabSize size);

    /// The `lowered-*` token rows: a lower resting elevation (level1, hovered
    /// level2) and — for the surface variant — the darker lowered container.
    bool isLowered() const { return m_lowered; }
    void setLowered(bool lowered);

    // --- icon --------------------------------------------------------------
    QString iconName() const { return m_iconName; }
    void setIconName(const QString &iconName);

    MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(MdIconSet set);

    MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(MdIconFamily family);

    // --- disabled ----------------------------------------------------------
    /// True when the FAB neither accepts input nor paints an interactive
    /// state: `!isEnabled()`.
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- interaction state -------------------------------------------------
    /// Hover is tracked explicitly — QWidget::underMouse() is not dependable
    /// before the first enter event and is never set under the offscreen
    /// platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }

    // --- tokens ------------------------------------------------------------
    /// The resolved `md.comp.fab.*` set for this FAB, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdFabTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this FAB alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers -------------------------------------------------------
    /// Owned by the FAB; the style paints from them.
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    /// True when the FAB holds focus *and* that focus is keyboard focus — the
    /// `:focus-visible` rule. See MdButton for the reason-classification.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    /// The painted container, in widget coordinates.
    QRectF containerRect() const;

    // --- geometry -----------------------------------------------------------
    /// Container size plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::FabVariant variant);
    void fabSizeChanged(md::FabSize size);
    void loweredChanged(bool lowered);
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onPressed();
    void onReleased();

private:
    void init();
    /// Drop the cached token set; the next tokens() call rebuilds it.
    void invalidateTokens();

    FabVariant m_variant = FabVariant::Primary;
    FabSize m_size = FabSize::Medium;
    bool m_lowered = false;
    QString m_iconName;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    bool m_hovered = false;

    mutable MdFabTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
    /// Whether the focus currently held arrived by keyboard.
    bool m_focusIsKeyboard = false;

    bool m_hasPressPosition = false;
    QPointF m_pressPosition;
};

} // namespace md

#endif // MD_FAB_H
