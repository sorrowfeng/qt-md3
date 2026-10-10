#ifndef MD_MENU_H
#define MD_MENU_H

// MdMenu + MdMenuItem — the MD3 menu, over Qt popups.
//
// Compose's `DropdownMenu` is a popup surface (a `surface-container` rounded
// rect at level 2) that opens on a scale+alpha pair — scale 0.8 → 1.0 on the
// fast spatial spring, alpha 0 → 1 on fast effects, around the anchor corner —
// with a vertical layout of 48 px items and dividers. This widget ports:
//
//   * **the classic item** — 48 px tall, 12 px horizontal padding, min-width
//     112 / max-width 280, 24 px leading/trailing icons with no interaction
//     lift, the `on-surface` state layer under hover/focus/press, the
//     `secondary-container` selected side, and the **inward** secondary focus
//     ring (the export's `focus.indicator.outline.offset` is the system inner
//     offset);
//   * **the open/close animation** — the surface paints its children through
//     a scale+alpha transform around the anchor corner (Compose's
//     graphicsLayer), so the popup's geometry never changes mid-flight;
//   * **the dividers** — 1 px `surface-variant` rules with Compose's
//     `HorizontalDividerPadding` (12 px horizontal, 2 px vertical).
//
// The menu is a `Qt::Popup` window: clicking outside dismisses it, arrow keys
// walk the items, Enter/Space activates, Esc closes. Item keyboard focus is
// the `:focus-visible` kind — mouse hovering does not draw the ring.
//
// The Expressive menu families (`StandardMenu` / `VibrantMenu` /
// `SegmentedMenu`) are separate upstream token sets and are not ported.

#include "core/MdMenuTokens.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QWidget>

class QTimer;

namespace md {

class MdFocusRingController;
class MdMenuStyle;
class MdRippleController;

// ---------------------------------------------------------------------------
// MdMenuItem
// ---------------------------------------------------------------------------

/// One row of an `MdMenu`. Checkable-free: `selected` is a programmatic
/// property (the classic menu has no check state of its own — the selected
/// colours are the caller's choice).
class QT_MD3_EXPORT MdMenuItem : public QAbstractButton
{
    Q_OBJECT

    /// The item's selected side (`list-item.selected.*`).
    Q_PROPERTY(bool selected READ isSelected WRITE setSelected NOTIFY selectedChanged)
    /// The 24 px leading icon slot.
    Q_PROPERTY(QString leadingIconName READ leadingIconName WRITE setLeadingIconName NOTIFY
                   leadingIconNameChanged)
    /// The 24 px trailing icon slot (a submenu sets the cascading indicator
    /// here by convention).
    Q_PROPERTY(QString trailingIconName READ trailingIconName WRITE setTrailingIconName NOTIFY
                   trailingIconNameChanged)

public:
    explicit MdMenuItem(QWidget *parent = nullptr);
    explicit MdMenuItem(const QString &text, QWidget *parent = nullptr);
    ~MdMenuItem() override;

    bool isSelected() const { return m_selected; }
    void setSelected(bool selected);

    QString leadingIconName() const { return m_leadingIconName; }
    void setLeadingIconName(const QString &iconName);
    QString trailingIconName() const { return m_trailingIconName; }
    void setTrailingIconName(const QString &iconName);

    md::MdIconSet iconSet() const { return m_iconSet; }
    void setIconSet(md::MdIconSet set);
    md::MdIconFamily iconFamily() const { return m_iconFamily; }
    void setIconFamily(md::MdIconFamily family);

    // --- geometry ----------------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- tokens --------------------------------------------------------------------
    const MdMenuTokens &menuTokens() const;
    void setMenuTokens(const MdMenuTokens &tokens);

    // --- interaction state ---------------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool hasKeyboardFocus() const { return hasFocus() && m_focusIsKeyboard; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    // --- controllers -------------------------------------------------------------------------
    MdRippleController *rippleController() const { return m_ripple; }
    MdFocusRingController *focusRingController() const { return m_focusRing; }

signals:
    void selectedChanged(bool selected);
    void leadingIconNameChanged(const QString &iconName);
    void trailingIconNameChanged(const QString &iconName);

protected:
    bool hitButton(const QPoint &pos) const override;
    void paintEvent(QPaintEvent *event) override; // the paint filter paints
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(md::MdEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
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
    md::MdIconSet m_iconSet = MdIconSet::MaterialSymbols;
    md::MdIconFamily m_iconFamily = MdIconFamily::Outlined;

    mutable MdMenuTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;

    MdRippleController *m_ripple = nullptr;
    MdFocusRingController *m_focusRing = nullptr;
};

// ---------------------------------------------------------------------------
// MdMenu
// ---------------------------------------------------------------------------

/// The menu surface: a `Qt::Popup` carrying items and dividers, opened
/// anchored to a widget. `exec()` runs a modal loop like `QMenu::exec`.
class QT_MD3_EXPORT MdMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MdMenu(QWidget *parent = nullptr);
    ~MdMenu() override;

    // --- content -------------------------------------------------------------------
    /// Takes ownership; appends an item row.
    MdMenuItem *addItem(const QString &text);
    /// Takes ownership; appends an existing item (for caller-built rows).
    void addItem(MdMenuItem *item);
    /// Appends a 1 px divider row.
    void addDivider();

    /// The item rows in order (dividers excluded).
    QList<MdMenuItem *> items() const { return m_items; }

    // --- showing ------------------------------------------------------------------------
    /// Opens anchored below `anchor` (Compose's `MenuAnchorPosition.Below`),
    /// left-aligned with the anchor's left edge.
    void popup(const QRect &anchorRect);
    /// Opens and runs a modal loop; returns when the menu closes.
    void exec(const QRect &anchorRect);

    // --- animation readouts -----------------------------------------------------------------
    /// 0 fully closed (scale 0.8, alpha 0) to 1 fully open — the scale curve.
    qreal openProgress() const { return m_openProgress; }
    /// The alpha curve's own progress (it runs on the faster effects spring).
    qreal openAlpha() const { return m_openAlpha; }
    bool openAnimationRunning() const { return m_openTimer->isActive(); }

    // --- tokens ------------------------------------------------------------------------------
    const MdMenuTokens &menuTokens() const;
    void setMenuTokens(const MdMenuTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    /// Emitted after the menu closed (outside click, Esc or an activation).
    void closed();

protected:
    void paintEvent(QPaintEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onOpenTick();
    void onThemeChanged();

private:
    void init();
    void relayout();
    void closeAndAccept();
    void setChildrenPaintedByMenu(bool painted);

    QList<MdMenuItem *> m_items;
    QList<QWidget *> m_dividers;
    /// Rows in insertion order — items and dividers interleave as added.
    QList<QWidget *> m_rows;

    qreal m_openProgress = 1.0;
    qreal m_openAlpha = 1.0;
    qreal m_openFrom = 0.0;
    QElapsedTimer m_openClock;
    QTimer *m_openTimer = nullptr;
    bool m_opening = false;

    mutable MdMenuTokens m_tokens;
    mutable bool m_tokensDirty = true;
    bool m_hasPushedTokens = false;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_MENU_H
