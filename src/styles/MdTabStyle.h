#ifndef MD_TAB_STYLE_H
#define MD_TAB_STYLE_H

// MdTabStyle — the paint of one tab.
//
// The tab has no container of its own: it is content (an icon, a label) over
// the row's surface, with the interaction feedback painted *over* that
// surface. Three things are this file's to decide:
//
//   * **the state layer spans the whole tab** — a tab has no pill to clip to,
//     and Compose's `selectable` indication is a bounded ripple over the whole
//     composable. Hover and keyboard focus are the flat layers; the press is
//     the ripple, as in the button families.
//   * **the press ripple's colour is the *active* colour** — Compose builds
//     `ripple(bounded = true, color = selectedContentColor)` so that the press
//     on an inactive tab shows the colour it is about to earn. The rows this
//     resolves are the active side's `state-layer` ones (`primary` in the
//     primary family, `on-surface` in the secondary).
//   * **the content colours ride the fade** — the icon and label resolve from
//     the active / inactive tables by the tab's `colourProgress()`, the
//     Oklab-interpolated spring progress that `TabTransition` drives.

#include "MdStyleBase.h"
#include "core/MdFocusRing.h"
#include "core/MdTabsTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>

class QPainter;
class QWidget;

namespace md {

class MdTab;

/// Pattern A style for `MdTab`.
class QT_MD3_EXPORT MdTabStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdTabStyle(QObject *parent = nullptr);

    static MdTabStyle *shared();
    static bool isInstalled();

    /// The focus-indicator spec for this family: an *inward* ring at the tab's
    /// bounds, like the navigation item's.
    static MdFocusRingSpec focusRingSpec(const MdTabsVariantTokens &tokens);

    /// The state the tab is painted in. `Disabled` first, matching Compose's
    /// `!enabled -> ... -> selected` ordering.
    static MdNavigationItemState stateFor(const MdTab &tab);

    static void paintTab(QPainter &painter, const MdTab &tab, const MdTabsVariantTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_TAB_STYLE_H
