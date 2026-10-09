#ifndef MD_DOCKED_TOOLBAR_H
#define MD_DOCKED_TOOLBAR_H

// MdDockedToolbar — the MD3 docked toolbar.
//
// m3.material.io: "Toolbars display frequently used actions relevant to the
// current page", and the docked toolbar is one of its two expressive variants:
//
//   | Variant          | M3 | M3 Expressive |
//   | Docked toolbar   | -- | Available     |
//   | Floating toolbar | -- | Available     |
//   | Bottom app bar   | Available | Not recommended. Use docked toolbar. |
//
// It is therefore the *replacement* for `MdBottomAppBar`, and the only thing
// that separates the two visually is the spacing and the collapse: both are
// `corner-none`, both are `surface-container`, and this one is 64 px rather
// than 80.
//
// **This widget is Compose's `FlexibleBottomAppBar`.** That composable has no
// counterpart in material-web (whose `_md-comp-toolbar-docked.scss` is the only
// place the variant exists there), and there is no `DockedToolbar` component in
// Compose either — `FlexibleBottomAppBar` reads `DockedToolbarTokens` for its
// height, padding and spacing, so it *is* the docked toolbar and this port
// names it after the design rather than after the composable.
//
// Two behaviours follow from that and are the reason this is not simply an
// `MdBottomAppBar` with a different height:
//
//   * **The spacing is the token's, not the caller's.** Compose passes
//     `Arrangement.spacedBy(ContainerMaxSpacing, CenterHorizontally)`, so the
//     row is centred with at least 32 px between items and there is no
//     arrangement property to set. The bottom app bar takes an `Arrangement`;
//     this one does not, because the published component does not.
//   * **The whole bar collapses.** `BottomAppBarLayout` sets
//     `heightOffsetLimit = -placeable.height`, so scrolling takes the entire
//     64 px, not a row of it. `heightOffset` and `heightOffsetLimit` are the
//     Compose names and are what the API exposes; a caller driving a scroll
//     area sets `heightOffset` between 0 and `heightOffsetLimit`.
//
// Children are slots: the caller hands over any widget — conventionally an
// `MdIconButton` or an `MdButton` — and the style places its *container*; see
// `MdDockedToolbarStyle` and `MdChildBox`.

#include "core/MdDockedToolbarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

namespace md {

class MdDockedToolbarStyle;

class QT_MD3_EXPORT MdDockedToolbar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(qreal heightOffset READ heightOffset WRITE setHeightOffset NOTIFY
                   heightOffsetChanged)

public:
    explicit MdDockedToolbar(QWidget *parent = nullptr);
    ~MdDockedToolbar() override;

    /// The row's children, in order from the leading edge. Null and duplicate
    /// widgets are ignored.
    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);
    void removeWidget(QWidget *widget);
    void clearWidgets();
    QList<QWidget *> widgets() const { return m_widgets; }

    // --- collapsing ---------------------------------------------------------
    /// How far the bar has been scrolled away, in px. Always in
    /// `[heightOffsetLimit(), 0]`: 0 is fully expanded, `heightOffsetLimit()`
    /// is fully collapsed.
    qreal heightOffset() const { return m_heightOffset; }
    void setHeightOffset(qreal offset);

    /// `-expandedHeight()`. Compose writes it into the scroll behaviour from
    /// the laid-out height, so it tracks the token rather than being fixed.
    qreal heightOffsetLimit() const;

    /// 0 fully expanded .. 1 fully collapsed. The docked toolbar collapses its
    /// *entire* height, so this is `-heightOffset / expandedHeight`.
    qreal collapsedFraction() const;

    /// The container height the bar wants when it is not scrolled away.
    qreal expandedHeight() const;
    /// The height the bar occupies right now.
    qreal currentHeight() const;

    /// The rectangle the bar would occupy if it were fully expanded — what the
    /// style lays its children out against, so that content does not reflow
    /// while the bar shrinks.
    QRect expandedRect() const;

    // --- tokens -------------------------------------------------------------
    const MdDockedToolbarTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides. Mutating through this accessor
    /// drops the cache, so an override takes effect on the next repaint.
    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void widgetsChanged();
    void heightOffsetChanged(qreal offset);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void syncGeometry();
    void placeChildren();

    QList<QWidget *> m_widgets;
    qreal m_heightOffset = 0.0;

    mutable MdDockedToolbarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_DOCKED_TOOLBAR_H
