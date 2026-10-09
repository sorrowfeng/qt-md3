#ifndef MD_NAVIGATION_DRAWER_STYLE_H
#define MD_NAVIGATION_DRAWER_STYLE_H

// MdNavigationDrawerStyle — the layout and paint of `MdNavigationDrawer`.
//
// A start-anchored sheet: an optional headline (`title-small` in
// `on-surface-variant`), an optional divider, then the full-width items at
// the behaviour's 12 px side inset. Two rows are carried and not painted —
// the scrim (a window overlay a child widget cannot cover; the colour and
// opacity are on the tokens for a host) and the modal container's level1
// elevation (outside a child sheet's rect) — the same grounds as the bar's
// level2 and the rail's modal level2. See docs/porting-todo.md.
//
// The `corner-large-end` shape has no directional enum member: the token
// carries the base Large radius and `Layout.radii` puts it on the **end**
// pair (a start-anchored sheet's leading edge stays square), mirrored in
// RTL — the bottom-sheet's `corner-extra-large-top` precedent.

#include "MdStyleBase.h"
#include "core/MdNavigationDrawerTokens.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdNavigationDrawer;

/// Pattern A style for `MdNavigationDrawer`.
class QT_MD3_EXPORT MdNavigationDrawerStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdNavigationDrawerStyle(QObject *parent = nullptr);

    static MdNavigationDrawerStyle *shared();
    static bool isInstalled();

    struct Layout
    {
        QRectF container;
        /// The end-pair radii, mirrored for RTL — see the header note.
        QList<qreal> radii;
        /// The headline's box, or empty when there is no headline.
        QRectF headlineBox;
        /// The divider's box, or empty when the divider is not shown.
        QRectF dividerBox;
        /// Where each item goes, in `items()` order — full-width rows at the
        /// behaviour's `itemPadding` inset.
        QList<QRectF> itemBoxes;
    };

    static Layout layoutFor(const MdNavigationDrawer &drawer, const MdNavigationDrawerTokens &tokens);

    static void paintNavigationDrawer(QPainter &painter, const MdNavigationDrawer &drawer,
                                      const MdNavigationDrawerTokens &tokens, const Layout &layout);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_NAVIGATION_DRAWER_STYLE_H
