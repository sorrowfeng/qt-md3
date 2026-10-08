#include "MdBadgeStyle.h"

#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdBadge.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetrics>
#include <QtGui/QPainter>
#include <QtWidgets/QWidget>

namespace md {

namespace {

// Compose Badge.kt: the leading and trailing label padding when the text is
// too long to fit a circular badge ("1", "9+", "999+"). The export publishes
// no content-padding row — material-web does not implement the component — so
// this is a [compose] fact, recorded as such.
constexpr qreal kLabelHorizontalPadding = 4.0;

} // namespace

MdBadgeStyle::MdBadgeStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdBadgeStyle *MdBadgeStyle::shared()
{
    static QMutex mutex;
    static MdBadgeStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdBadgeStyle;
        installPaintFilter<MdBadge>(instance);
    }
    return instance;
}

bool MdBadgeStyle::isInstalled()
{
    return hasPaintFilter(&MdBadge::staticMetaObject);
}

MdBadgeStyle::Layout MdBadgeStyle::layoutFor(const MdBadge &badge, const MdBadgeTokens &tokens)
{
    Layout layout;

    const QRectF widgetRect(badge.rect());

    if (!badge.hasContent()) {
        // --- the dot form: md.comp.badge.size ------------------------------
        const QSizeF dotSize(tokens.size, tokens.size);
        const QPointF origin(widgetRect.left()
                                 + qMax<qreal>((widgetRect.width() - dotSize.width()) / 2.0, 0.0),
                             widgetRect.top()
                                 + qMax<qreal>((widgetRect.height() - dotSize.height()) / 2.0,
                                               0.0));
        layout.container = QRectF(origin, dotSize);
        const QList<qreal> radii = MdShape::resolvedRadii(tokens.shape, dotSize);
        layout.radius = radii.isEmpty() ? dotSize.width() / 2.0 : radii.first();
        layout.preferredSize = dotSize;
        return layout;
    }

    // --- the content form: md.comp.badge.large.* ---------------------------
    const MdTypeStyleSpec spec = MdTypeScale::spec(tokens.largeLabelTextType);
    const QFont font = MdTypeScale::font(tokens.largeLabelTextType, TypeEmphasis::Baseline,
                                         MdTheme::instance().scriptCategory());
    const QFontMetrics metrics(font);

    // Compose: defaultMinSize(16, 16) around a Row that pads the label 4 dp
    // on each side and centres it. The minimum is the token; the label may
    // widen ("999+") and heighten (a CJK line height) the pill past it.
    const qreal textWidth = metrics.horizontalAdvance(badge.text());
    const qreal width = qMax<qreal>(tokens.largeSize,
                                    textWidth + 2.0 * kLabelHorizontalPadding);
    const qreal height = qMax<qreal>(tokens.largeSize, metrics.height());

    const QSizeF containerSize(width, height);
    const QPointF origin(widgetRect.left()
                             + qMax<qreal>((widgetRect.width() - containerSize.width()) / 2.0,
                                           0.0),
                         widgetRect.top()
                             + qMax<qreal>((widgetRect.height() - containerSize.height()) / 2.0,
                                           0.0));
    layout.container = QRectF(origin, containerSize);
    // Both forms publish corner-full: a circle when square, a pill otherwise.
    const QList<qreal> radii =
        MdShape::resolvedRadii(badge.hasContent() ? tokens.largeShape : tokens.shape,
                               containerSize);
    layout.radius = radii.isEmpty()
                        ? qMin(containerSize.width(), containerSize.height()) / 2.0
                        : radii.first();

    const qreal lineHeight = MdTypeScale::lineHeight(tokens.largeLabelTextType,
                                                     TypeEmphasis::Baseline,
                                                     MdTheme::instance().scriptCategory());
    layout.label = QRectF(layout.container.left() + kLabelHorizontalPadding,
                          layout.container.center().y() - lineHeight / 2.0,
                          qMax<qreal>(containerSize.width() - 2.0 * kLabelHorizontalPadding, 0.0),
                          lineHeight);
    layout.preferredSize = containerSize;
    return layout;
}

void MdBadgeStyle::paintBadge(QPainter &painter, const MdBadge &badge, const MdBadgeTokens &tokens,
                              const Layout &layout)
{
    const MdTheme &theme = MdTheme::instance();

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1. The container. The shape rows are honoured (both publish corner-full
    //    in the export); the content form uses the large colour row (both are
    //    Error in the published set, but the rows are separate tokens and
    //    each is honoured).
    const QColor fill = theme.color(badge.hasContent() ? tokens.largeColor : tokens.color);
    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawPath(MdShape::roundedRect(layout.container,
                                          badge.hasContent() ? tokens.largeShape
                                                             : tokens.shape));

    // 2. The label — the whole content of the large form, in label-small on
    //    top of on-error. No state layer follows: the family has none.
    if (badge.hasContent() && layout.label.isValid()) {
        const QFont font = MdTypeScale::font(tokens.largeLabelTextType, TypeEmphasis::Baseline,
                                             theme.scriptCategory());
        painter.setFont(font);
        painter.setPen(QPen(theme.color(tokens.largeLabelTextColor), 0.0));
        painter.drawText(layout.label, Qt::AlignCenter, badge.text());
    }

    painter.restore();
}

void MdBadgeStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    auto *badge = qobject_cast<MdBadge *>(widget);
    if (painter == nullptr || badge == nullptr) {
        return;
    }
    const MdBadgeTokens &tokens = badge->tokens();
    paintBadge(*painter, *badge, tokens, layoutFor(*badge, tokens));
}

} // namespace md
