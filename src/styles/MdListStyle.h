#ifndef MD_LIST_STYLE_H
#define MD_LIST_STYLE_H

#include "core/MdListTokens.h"
#include "MdStyleBase.h"
#include "core/QtMd3Export.h"

#include <QtCore/QRectF>

class QPainter;

namespace md {

class MdList;

/// Pattern A style for `MdList`: the container background and the list-level
/// geometry. The list itself is a layout container — its only paint is the
/// container colour the export publishes (`md.comp.list.container.color` with
/// `container.shape` rounded), exactly what material-web's `:host` draws.
class QT_MD3_EXPORT MdListStyle : public MdStyleBase
{
    Q_OBJECT

public:
    explicit MdListStyle(QObject *parent = nullptr);

    /// Create-and-install accessor, the same pattern the other styles use.
    static MdListStyle *shared();

    /// True once shared() has run. Exists for the test suite.
    static bool isInstalled();

    struct Layout
    {
        /// The painted container, in widget coordinates.
        QRectF container;
        /// The corner radii (`md.comp.list.container.shape`) for that rect.
        QList<qreal> radii;
    };

    static Layout layoutFor(const MdList &list, const MdListContainerTokens &tokens);

    void drawWidget(QPainter *painter, QWidget *widget) override;

    /// The pieces of drawWidget, exposed for the test suite.
    static void paintList(QPainter &painter, const MdList &list,
                          const MdListContainerTokens &tokens, const Layout &layout);
};

} // namespace md

#endif // MD_LIST_STYLE_H
