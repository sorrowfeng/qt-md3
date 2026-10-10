#ifndef MD_SEARCH_BAR_STYLE_H
#define MD_SEARCH_BAR_STYLE_H

// MdSearchBarStyle — the paint of one search bar.
//
// Layers, in paint order:
//
//   1. the **container** — the 56 px pill (surface-container-high at level 3
//      with corner-full) or the view surface (docked corner-extra-large /
//      full-screen corner-none on surface-container-low).
//   2. the **state layer** — on-surface under hover / press.
//   3. the **leading icon** — the 24 px search glyph, on-surface.

#include "MdStyleBase.h"
#include "core/MdSearchTokens.h"
#include "core/QtMd3Export.h"

class QPainter;
class QWidget;

namespace md {

class MdSearchBar;

/// Pattern A style for `MdSearchBar`.
class QT_MD3_EXPORT MdSearchBarStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdSearchBarStyle(QObject *parent = nullptr);

    static MdSearchBarStyle *shared();
    static bool isInstalled();

    static void paintSearchBar(QPainter &painter, const MdSearchBar &bar,
                               const MdSearchTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;
};

} // namespace md

#endif // MD_SEARCH_BAR_STYLE_H
