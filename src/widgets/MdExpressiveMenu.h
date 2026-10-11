#ifndef MD_EXPRESSIVE_MENU_H
#define MD_EXPRESSIVE_MENU_H

// MdExpressiveMenu + MdExpressiveMenuItem — the MD3 Expressive menu family,
// `md.comp.menus.*` (Standard / Vibrant) at export 34.0.21.
//
// ## Why this is a sibling of `MdMenu`, not a mode of it
//
// `MdMenu` is the classic `md.comp.menu.*` surface: 48 px rows, corner-
// extra-small container, a `secondary-container` selected side. The Expressive
// family is a disjoint token namespace with 44 px rows, a corner-large
// container, a 2 px group gap and its own colour families (Standard /
// Vibrant). Folding both into one widget would mean one `MdMenuTokens` struct
// carrying two incompatible geometries; the library already keeps separate
// token sets as separate widgets (the toolbars, the search surfaces). So this
// is `MdExpressiveMenu` over the same `Qt::Popup` model, and the classic
// `MdMenu` is untouched.
//
// ## What this ports
//
//   * **the shared Expressive geometry** — 44 px items, 2 px gap, 2 px group
//     padding, 20 px icons, 16 px side spaces, 12 px icon↔text;
//   * **the corner-large surface** with the per-position item shapes
//     (selected / first / last → corner-medium, middle → corner-extra-small,
//     pressed → corner-large);
//   * **the two colour families** — Standard (surface-container-low base) and
//     Vibrant (tertiary-container base, with the icon interaction lift);
//   * **`:focus-visible` keyboard focus** on the inward secondary ring.
//
// ## Not ported (recorded in porting-todo)
//
//   * **SegmentedMenu** — the latest sass export publishes Standard + Vibrant
//     only; the third family is not invented;
//   * **the horizontal / icon-only layouts** — a separate geometry family
//     (`horizontal.container.shape: corner-full`);
//   * **the Expressive open motion** — the provided export carries no motion
//     rows for this family; the popup opens without a flight.

#include "core/MdExpressiveMenuTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QWidget>

namespace md {

class MdFocusRingController;
class MdExpressiveMenuStyle;
class MdRippleController;

// ---------------------------------------------------------------------------
// MdExpressiveMenuItem
// ---------------------------------------------------------------------------

/// One row of an `MdExpressiveMenu`. `selected` is a programmatic property
/// (the family has no check state of its own).
class QT_MD3_EXPORT MdExpressiveMenuItem : public QAbstractButton
{
    Q_OBJECT

    /// The item's selected side (`menu-item.selected.*`).
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    /// The 20 px leading icon slot (`menu-item.leading-icon.size`).
    Q_PROPERTY(QString leadingIconName READ leadingIconName WRITE setLeadingIconName NOTIFY
                   leadingIconNameChanged)
    /// The 20 px trailing icon slot.
    Q_PROPERTY(QString trailingIconName READ trailingIconName WRITE setTrailingIconName NOTIFY
                   trailingIconNameChanged)
    /// The body-small supporting line under the label.
    Q_PROPERTY(QString supportingText READ supportingText WRITE setSupportingText NOTIFY
                   supportingTextChanged)
    /// The label-large trailing supporting line.
    Q_PROPERTY(QString trailingSupportingText READ trailingSupportingText WRITE
                   setTrailingSupportingText NOTIFY trailingSupportingTextChanged)

public:
    explicit MdExpressiveMenuItem(QWidget *parent = nullptr);
    explicit MdExpressiveMenuItem(const QString &text, QWidget *parent = nullptr);
    ~MdExpressiveMenuItem() override;

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    QString leadingIconName() const { return m_leadingIconName; }
    void setLeadingIconName(const QString &iconName);
    QString trailingIconName() const { return m_trailingIconName; }
    void setTrailingIconName(const QString &iconName);
    QString supportingText() const { return m_supportingText; }
    void setSupportingText(const QString &text);
    QString trailingSupportingText() const { return m_trailingSupportingText; }
    void setTrailingSupportingText(const QString &text);

    md::MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(md::MdIconSet set);
    md::MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(md::MdIconFamily family);

    // --- geometry ----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- tokens --------------------------------------------------------------------
    const MdExpressiveMenuTokens &menuTokens() const;
    void setMenuTokens(const MdExpressiveMenuTokens &tokens);
    /// Push the resolved tokens of `variant` down onto this item (the menu
    /// does this for every row it owns).
    void applyVariant(MdExpressiveMenuVariant variant);

    // --- interaction state ---------------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- slot geometry the style reads ---------------------------------------------------
    /// The row's shape role inside a group of `count` rows at `index`.
    ShapeCorner shapeInGroup(int index, int count) const;

    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

signals:
    void selectedChanged(bool selected);
    void leadingIconNameChanged(const QString &iconName);
    void trailingIconNameChanged(const QString &iconName);
    void supportingTextChanged(const QString &text);
    void trailingSupportingTextChanged(const QString &text);

protected:
    bool hitButton(const QPoint &pos) const override;
    void paintEvent(QPaintEvent *event) override; // the paint filter paints
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();

    bool m_selected = false;
    bool m_hovered = false;
    bool m_focusIsKeyboard = false;
    QString m_leadingIconName;
    QString m_trailingIconName;
    QString m_supportingText;
    QString m_trailingSupportingText;
    md::MdIconSet m_iconSet = MdIconSet::MaterialSymbols;
    md::MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    mutable MdExpressiveMenuTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

// ---------------------------------------------------------------------------
// MdExpressiveMenu
// ---------------------------------------------------------------------------

/// The Expressive menu surface: a `Qt::Popup` carrying items and dividers.
class QT_MD3_EXPORT MdExpressiveMenu : public QWidget
{
    Q_OBJECT

    /// The colour family (`Standard` / `Vibrant`).
    Q_PROPERTY(md::MdExpressiveMenuVariant variant READ variant WRITE setVariant NOTIFY
                   variantChanged)

public:
    explicit MdExpressiveMenu(QWidget *parent = nullptr);
    ~MdExpressiveMenu() override;

    // --- variant -------------------------------------------------------------------
    MdExpressiveMenuVariant variant() const { return m_variant; }
    void setVariant(MdExpressiveMenuVariant variant);

    // --- content -------------------------------------------------------------------
    /// Takes ownership; appends an item row.
    MdExpressiveMenuItem *addItem(const QString &text);
    /// Takes ownership; appends an existing item.
    void addItem(MdExpressiveMenuItem *item);
    /// Appends a 1 px divider row.
    void addDivider();

    QList<MdExpressiveMenuItem *> items() const { return m_items; }

    // --- showing ------------------------------------------------------------------------
    void popup(const QRect &anchorRect);
    void exec(const QRect &anchorRect);

    // --- tokens ------------------------------------------------------------------------------
    const MdExpressiveMenuTokens &menuTokens() const;
    void setMenuTokens(const MdExpressiveMenuTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void variantChanged(md::MdExpressiveMenuVariant variant);
    /// Emitted after the menu closed (outside click, Esc or an activation).
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void relayout();
    void closeAndAccept();
    void pushTokensToItems();

    QList<MdExpressiveMenuItem *> m_items;
    QList<QWidget *> m_dividers;
    /// Rows in insertion order — items and dividers interleave as added.
    QList<QWidget *> m_rows;

    MdExpressiveMenuVariant m_variant = MdExpressiveMenuVariant::Standard;

    mutable MdExpressiveMenuTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_EXPRESSIVE_MENU_H
