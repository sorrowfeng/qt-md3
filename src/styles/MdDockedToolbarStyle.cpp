#include "MdDockedToolbarStyle.h"

#include "MdChildBox.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "widgets/MdDockedToolbar.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtCore/QSizeF>
#include <QtCore/QVector>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

MdDockedToolbarStyle::MdDockedToolbarStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdDockedToolbarStyle *MdDockedToolbarStyle::shared()
{
    static QMutex mutex;
    static MdDockedToolbarStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdDockedToolbarStyle;
        installPaintFilter<MdDockedToolbar>(instance);
    }
    return instance;
}

bool MdDockedToolbarStyle::isInstalled()
{
    return hasPaintFilter(&MdDockedToolbar::staticMetaObject);
}

MdDockedToolbarStyle::Layout MdDockedToolbarStyle::layoutFor(const MdDockedToolbar &toolbar,
                                                             const MdDockedToolbarTokens &tokens)
{
    Layout layout;

    // The children are laid out against the **expanded** geometry even while
    // the bar is collapsing, so the content does not reflow as it shrinks; the
    // widget's own reduced rect does the clipping. See the header comment.
    const QRectF expanded = QRectF(toolbar.expandedRect());
    const QRectF current = QRectF(toolbar.rect());
    layout.container = current;
    layout.radii = MdShape::resolvedRadii(tokens.containerShape, current.size());

    layout.content = QRectF(expanded.left() + tokens.containerLeadingSpace,
                            expanded.top() + tokens.contentTopSpace,
                            qMax<qreal>(expanded.width() - tokens.containerLeadingSpace
                                            - tokens.containerTrailingSpace,
                                        0.0),
                            qMax<qreal>(expanded.height() - tokens.contentTopSpace
                                            - tokens.contentBottomSpace,
                                        0.0));

    // --- the row's own children ---------------------------------------------
    // Measure each child's *container* — a 40 px icon button is a 55 px
    // widget, and advancing by that would draw the icons 15 px too far apart.
    const QList<QWidget *> children = toolbar.widgets();
    QVector<QSizeF> sizes;
    sizes.reserve(children.size());
    qreal contentWidth = 0.0;
    for (QWidget *child : children) {
        const QSizeF size = MdChildBox::measure(child).containerSize();
        sizes.append(size);
        contentWidth += size.width();
    }

    // `Arrangement.spacedBy(32, CenterHorizontally)`: the gaps are the token's
    // max-spacing, and the whole run is centred in the band. A run wider than
    // the band stays centred, i.e. `x` goes negative — Compose does the same.
    const qreal gap = tokens.containerMaxSpacing;
    const int count = sizes.size();
    const qreal totalWidth = contentWidth + (count > 1 ? gap * qreal(count - 1) : 0.0);
    qreal x = layout.content.left() + (layout.content.width() - totalWidth) / 2.0;

    layout.childBoxes.reserve(count);
    for (int i = 0; i < count; ++i) {
        const QSizeF size = sizes.at(i);
        const qreal top = layout.content.top() + (layout.content.height() - size.height()) / 2.0;
        layout.childBoxes.append(QRectF(x, top, size.width(), size.height()));
        x += size.width() + gap;
    }

    return layout;
}

void MdDockedToolbarStyle::paintDockedToolbar(QPainter &painter, const MdDockedToolbar &toolbar,
                                              const MdDockedToolbarTokens &tokens,
                                              const Layout &layout)
{
    Q_UNUSED(toolbar);
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(MdTheme::instance().color(tokens.containerColor));
    painter.drawPath(MdShape::roundedRect(layout.container, layout.radii));
    painter.restore();

    // A docked toolbar publishes no elevation row at all, so unlike the bottom
    // app bar there is nothing here that has to be carried-but-unpainted: a
    // docked toolbar is flat by the export.
}

void MdDockedToolbarStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *toolbar = qobject_cast<MdDockedToolbar *>(widget);
    if (painter == nullptr || toolbar == nullptr) {
        return;
    }
    const MdDockedToolbarTokens &tokens = toolbar->tokens();
    paintDockedToolbar(*painter, *toolbar, tokens, layoutFor(*toolbar, tokens));
}

} // namespace md
