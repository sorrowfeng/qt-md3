#ifndef MD_EXTENDED_FAB_H
#define MD_EXTENDED_FAB_H

// MdExtendedFab — MD3 extended floating action buttons.
//
// Six colour sets, three sizes, and the lowered form:
//
//   variant  md.comp.extended-fab.primary | secondary | tertiary |
//            primary-container | secondary-container | tertiary-container
//   size     md.comp.extended-fab.small | medium | large
//   lowered  the `lowered-*` elevation rows
//
// An extended FAB is a FAB that grew a label: icon + text on a raised,
// shadowed container whose width is derived from its content (leading space +
// icon + icon-label space + label + trailing space). The colour sets are
// *not* the FAB's — this family has no surface set and three container sets
// of its own; see docs/porting-todo.md for the recorded facts.
//
// Interaction shape, shared with MdFab and the button families and recorded
// in docs/interaction-reference.md:
//
//   * hover and keyboard focus paint the flat state layer; press does not —
//     the press response is the ripple alone ("Pressed (ripple)" rows);
//   * the ripple colour is the pressed row's state-layer role, not a global
//     on-surface;
//   * the focus indicator and the focused colours follow `:focus-visible`
//     semantics — pointer focus shows neither;
//   * the container shape never morphs on press: the export publishes no
//     pressed shape for this family.
//
// Why QPushButton: exactly the argument MdFab makes — QAbstractButton already
// owns clicked/pressed/released, the keyboard activation and the
// accessibility role, and this family additionally reuses QPushButton's text
// as the label.

#include "core/MdExtendedFabTokens.h"
#include "core/MdRipple.h"
#include "core/MdTypes.h"
#include "core/MdQtCompat.h"
#include "core/QtMd3Export.h"

#include <QtCore/QPointF>
#include <QtCore/QString>
#include <QtWidgets/QPushButton>

class QFocusEvent;
class QMouseEvent;

namespace md {

class MdExtendedFabStyle;
class MdFocusRingController;

class QT_MD3_EXPORT MdExtendedFab : public QPushButton
{
    Q_OBJECT

    /// The label text is QPushButton's own text — setText() *is* setLabel().
    Q_PROPERTY(md::ExtendedFabVariant variant READ variant WRITE setVariant NOTIFY variantChanged)
    Q_PROPERTY(md::ExtendedFabSize fabSize READ fabSize WRITE setFabSize NOTIFY fabSizeChanged)
    Q_PROPERTY(bool lowered READ isLowered WRITE setLowered NOTIFY loweredChanged)
    Q_PROPERTY(QString iconName READ iconName WRITE setIconName NOTIFY iconNameChanged)
    Q_PROPERTY(md::MdIconSet iconSet READ iconSet WRITE setIconSet NOTIFY iconSetChanged)
    Q_PROPERTY(md::MdIconFamily iconFamily READ iconFamily WRITE setIconFamily
                   NOTIFY iconFamilyChanged)

public:
    explicit MdExtendedFab(QWidget *parent = nullptr);
    /// The canonical form: a leading icon plus the label.
    MdExtendedFab(const QString &iconName, const QString &label, QWidget *parent = nullptr);
    /// A label-only convenience; the spec's default shape always shows an
    /// icon, so pass iconName later to restore it.
    explicit MdExtendedFab(const QString &label, QWidget *parent = nullptr);
    ~MdExtendedFab() override;

    // --- variant / size / lowered ------------------------------------------
    ExtendedFabVariant variant() const { return m_variant; }
    void setVariant(ExtendedFabVariant variant);

    ExtendedFabSize fabSize() const { return m_size; }
    void setFabSize(ExtendedFabSize size);

    /// The `lowered-*` token rows: a lower resting elevation (level1, hovered
    /// level2). Unlike the FAB family this changes elevation only.
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
    /// True when the button neither accepts input nor paints an interactive
    /// state: `!isEnabled()`.
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- interaction state -------------------------------------------------
    /// Hover is tracked explicitly — QWidget::underMouse() is not dependable
    /// before the first enter event and is never set under the offscreen
    /// platform plugin the tests run on.
    bool isHovered() const { return m_hovered; }

    // --- tokens ------------------------------------------------------------
    /// The resolved `md.comp.extended-fab.*` set for this button, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdExtendedFabTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this button alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- controllers -------------------------------------------------------
    /// Owned by the button; the style paints from them.
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

    /// True when the button holds focus *and* that focus is keyboard focus —
    /// the `:focus-visible` rule. See MdButton for the reason-classification.
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }

    /// The painted container, in widget coordinates.
    QRectF containerRect() const;

    // --- geometry -----------------------------------------------------------
    /// Content-derived width plus the focus-indicator margin, on both axes.
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void variantChanged(md::ExtendedFabVariant variant);
    void fabSizeChanged(md::ExtendedFabSize size);
    void loweredChanged(bool lowered);
    void iconNameChanged(const QString &iconName);
    void iconSetChanged(md::MdIconSet set);
    void iconFamilyChanged(md::MdIconFamily family);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
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

    ExtendedFabVariant m_variant = ExtendedFabVariant::PrimaryContainer;
    ExtendedFabSize m_size = ExtendedFabSize::Small;
    bool m_lowered = false;
    QString m_iconName;
    MdIconSet m_iconSet = MdIconSet::Auto;
    MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    bool m_hovered = false;

    mutable MdExtendedFabTokens m_tokens;
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

#endif // MD_EXTENDED_FAB_H
