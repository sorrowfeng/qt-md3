#ifndef MD_LIST_ITEM_STYLE_H
#define MD_LIST_ITEM_STYLE_H

#include "core/MdFocusRing.h"
#include "core/MdListTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QRectF>
#include <QtCore/QSizeF>

class QPainter;

namespace md {

class MdListItem;

/// Pattern A style for `MdListItem`: the layout, the shape morph and the
/// painting for the `md.comp.list.list-item.*` family, registered in the
/// paint hub so the first item construction installs it application-wide.
class QT_MD3_EXPORT MdListItemStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdListItemStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdListItemStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    /// The measured item: the container, its per-corner radii, every slot and
    /// the alignment verdict.
    struct Layout
    {
        /// The painted container, in widget coordinates.
        QRectF container;
        /// Per-corner radii (top-left, top-right, bottom-right, bottom-left)
        /// after the state's shape and the segmented override.
        QList<qreal> radii;
        /// The leading slot (empty when there is no leading content).
        QRectF leading;
        /// The trailing slot (empty when there is no trailing content).
        QRectF trailing;
        /// The text column: overline above headline above supporting text.
        QRectF overline;
        QRectF headline;
        QRectF supporting;
        /// Trailing supporting text / trailing meta text.
        QRectF trailingSupporting;
        /// True when the content is top-aligned rather than centred — the
        /// spec's 88dp rule.
        bool topAligned = false;
        /// 1, 2 or 3 lines, derived from the content [compose].
        int lines = 1;
    };

    static Layout layoutFor(const MdListItem &item, const MdListTokens &tokens);

    /// 1, 2 or 3 — Compose's `ListItemType` rule: three-line when there is
    /// both an overline and supporting text, or a multiline supporting text;
    /// two-line when there is either; else one line.
    static int lineCount(const MdListItem &item);

    /// The container height a layout reports: the token minimum for the line
    /// count, or the content height plus the vertical padding, whichever is
    /// larger.
    static qreal measuredHeight(const MdListItem &item, const MdListTokens &tokens,
                                qreal contentWidth);

    /// The shape the item paints with *now*: the expressive morph when the
    /// variant morphs, the flat baseline shape otherwise.
    ///
    /// Priority [compose] `shapeForInteraction`: pressed > dragged > selected >
    /// focused > hovered > base, with the export's two disabled rows inserted
    /// after `dragged` (corner-extra-small unselected, corner-large selected).
    static ShapeCorner shapeFor(const MdListItem &item, const MdListTokens &tokens);

    /// The per-corner radii after `segmentedShapes` [compose]: the first
    /// item's top pair and the last item's bottom pair take the list's
    /// `container.shape`, the middle items keep the item shape.
    static QList<qreal> radiiFor(const MdListItem &item, const MdListTokens &tokens,
                                 const QSizeF &size);

    /// The interaction state the paint uses now for its *colour* row.
    ///
    /// Deliberately a different order from `shapeFor`: disabled comes first
    /// here, which is what Compose's `ListItemColors.containerColor` /
    /// `contentColor` do (`!enabled -> …, dragged -> …, selected -> …`), while
    /// its `shapeForInteraction` has no disabled branch at all. The two are
    /// faithful to their own sources; do not "fix" one to match the other.
    static MdListState stateFor(const MdListItem &item);

    /// The ring's parameters: the inward variant, the token thickness and the
    /// token colour, with the gap derived from `focus.indicator.outline.offset`.
    static MdFocusRingSpec focusRingSpec(const MdListTokens &tokens);

    /// The focus ring's reserved *outside* margin. Zero for a list item — the
    /// ring is drawn inward, so it needs no room from the parent.
    static qreal focusRingInset(const MdListTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite.
    static void paintListItem(QPainter &painter, const MdListItem &item,
                              const MdListTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_LIST_ITEM_STYLE_H
