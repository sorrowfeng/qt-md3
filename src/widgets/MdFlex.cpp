#include "MdFlex.h"

#include "styles/MdStyleBase.h"

#include <QtGui/QResizeEvent>
#include <QtCore/QChildEvent>

namespace md {

MdFlex::MdFlex(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdFlex::~MdFlex() = default;

void MdFlex::init()
{
    // Pure container: no focus, no chrome of its own (appendix D anchor).
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("flex"));

    MdStyleBase::connectThemeUpdate(this, &MdFlex::onThemeChanged);
}

void MdFlex::setDirection(MdFlexDirection direction)
{
    if (direction >= MdFlexDirection::Count) {
        return;
    }
    if (m_direction == direction) {
        return;
    }
    m_direction = direction;
    updateGeometry();
    relayout();
    emit directionChanged(m_direction);
}

void MdFlex::setJustify(MdFlexJustify justify)
{
    if (justify >= MdFlexJustify::Count) {
        return;
    }
    if (m_justify == justify) {
        return;
    }
    m_justify = justify;
    relayout();
    emit justifyChanged(m_justify);
}

void MdFlex::setAlign(MdFlexAlign align)
{
    if (align >= MdFlexAlign::Count) {
        return;
    }
    if (m_align == align) {
        return;
    }
    m_align = align;
    relayout();
    emit alignChanged(m_align);
}

void MdFlex::setGap(MdFlexSpacing gap)
{
    if (gap >= MdFlexSpacing::Count) {
        return;
    }
    if (m_gap == gap) {
        return;
    }
    m_gap = gap;
    updateGeometry();
    relayout();
    emit gapChanged(m_gap);
}

void MdFlex::setWrap(bool wrap)
{
    if (m_wrap == wrap) {
        return;
    }
    m_wrap = wrap;
    updateGeometry();
    relayout();
    emit wrapChanged(m_wrap);
}

qreal MdFlex::gapPx() const
{
    return spacingPx(m_gap);
}

qreal MdFlex::spacingPx(MdFlexSpacing spacing)
{
    // The MD3 4 dp spacing scale. Each step is a multiple of the base unit;
    // the named values match the gaps the component exports actually publish
    // (0 / 4 / 8 / 12 / 16 / 24 / 32). See the header comment and
    // docs/porting-todo.md.
    switch (spacing) {
    case MdFlexSpacing::None:
        return 0.0;
    case MdFlexSpacing::ExtraSmall:
        return 4.0;
    case MdFlexSpacing::Small:
        return 8.0;
    case MdFlexSpacing::Medium:
        return 12.0;
    case MdFlexSpacing::Large:
        return 16.0;
    case MdFlexSpacing::ExtraLarge:
        return 24.0;
    case MdFlexSpacing::Huge:
        return 32.0;
    case MdFlexSpacing::Count:
        break;
    }
    return 0.0;
}

QSize MdFlex::sizeHint() const
{
    // Without wrap the run is a single line; with wrap the cross axis grows
    // with the number of lines at the current width. Returning the unwrapped
    // box left the extra rows outside the widget (the music-player filter
    // chips at phone width).
    if (!m_wrap) {
        const qreal gap = gapPx();
        qreal main = 0.0;
        qreal cross = 0.0;
        int visible = 0;
        for (QObject *child : children()) {
            auto *widget = qobject_cast<QWidget *>(child);
            if (widget == nullptr || widget->isWindow() || widget->isHidden()) {
                continue;
            }
            QSize hint = widget->sizeHint();
            hint = hint.expandedTo(widget->minimumSizeHint());
            hint = hint.expandedTo(widget->minimumSize());
            if (hint.width() <= 0) {
                hint.setWidth(widget->width());
            }
            if (hint.height() <= 0) {
                hint.setHeight(widget->height());
            }
            hint = hint.boundedTo(widget->maximumSize());
            if (m_direction == MdFlexDirection::Row) {
                main += hint.width();
                cross = qMax(cross, qreal(hint.height()));
            } else {
                main += hint.height();
                cross = qMax(cross, qreal(hint.width()));
            }
            ++visible;
        }
        if (visible > 1) {
            main += gap * (visible - 1);
        }
        if (m_direction == MdFlexDirection::Row) {
            return QSize(qRound(main), qRound(cross));
        }
        return QSize(qRound(cross), qRound(main));
    }

    // Wrapped: preferred box is one column/row of the widest child — the
    // host stretches us to the available main axis and heightForWidth then
    // reports the wrapped cross extent. Returning the unwrapped run width
    // made scroll areas expand us to the full single line (filter chips at
    // phone width never wrapped).
    qreal widest = 0.0;
    qreal singleCross = 0.0;
    for (QObject *child : children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (widget == nullptr || widget->isWindow() || widget->isHidden()) {
            continue;
        }
        QSize hint = widget->sizeHint();
        hint = hint.expandedTo(widget->minimumSizeHint());
        hint = hint.expandedTo(widget->minimumSize());
        if (hint.width() <= 0) {
            hint.setWidth(widget->width());
        }
        if (hint.height() <= 0) {
            hint.setHeight(widget->height());
        }
        hint = hint.boundedTo(widget->maximumSize());
        if (m_direction == MdFlexDirection::Row) {
            widest = qMax(widest, qreal(hint.width()));
            singleCross = qMax(singleCross, qreal(hint.height()));
        } else {
            widest = qMax(widest, qreal(hint.height()));
            singleCross = qMax(singleCross, qreal(hint.width()));
        }
    }
    const qreal along = width() > 0
        ? (m_direction == MdFlexDirection::Row ? qreal(width()) : qreal(height()))
        : widest;
    const qreal across = wrappedExtentAt(along);
    if (m_direction == MdFlexDirection::Row) {
        return QSize(qRound(widest), qRound(across > 0 ? across : singleCross));
    }
    return QSize(qRound(across > 0 ? across : singleCross), qRound(widest));
}

bool MdFlex::hasHeightForWidth() const
{
    return m_wrap && m_direction == MdFlexDirection::Row;
}

int MdFlex::heightForWidth(int width) const
{
    if (!hasHeightForWidth() || width <= 0) {
        return sizeHint().height();
    }
    return qRound(wrappedExtentAt(qreal(width)));
}

/// Sum of children on the main axis, unwrapped (plus gaps).
qreal MdFlex::unwrappedMainExtent() const
{
    const qreal gap = gapPx();
    qreal main = 0.0;
    int visible = 0;
    for (QObject *child : children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (widget == nullptr || widget->isWindow() || widget->isHidden()) {
            continue;
        }
        QSize hint = widget->sizeHint();
        hint = hint.expandedTo(widget->minimumSizeHint());
        hint = hint.expandedTo(widget->minimumSize());
        if (hint.width() <= 0) {
            hint.setWidth(widget->width());
        }
        if (hint.height() <= 0) {
            hint.setHeight(widget->height());
        }
        hint = hint.boundedTo(widget->maximumSize());
        main += m_direction == MdFlexDirection::Row ? hint.width() : hint.height();
        ++visible;
    }
    if (visible > 1) {
        main += gap * (visible - 1);
    }
    return main;
}

/// Cross-axis extent of the wrapped run at `along` px on the main axis.
qreal MdFlex::wrappedExtentAt(qreal along) const
{
    const qreal gap = gapPx();
    const bool row = m_direction == MdFlexDirection::Row;

    QVector<qreal> mainSizes;
    QVector<qreal> crossSizes;
    for (QObject *child : children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (widget == nullptr || widget->isWindow() || widget->isHidden()) {
            continue;
        }
        QSize hint = widget->sizeHint();
        hint = hint.expandedTo(widget->minimumSizeHint());
        hint = hint.expandedTo(widget->minimumSize());
        if (hint.width() <= 0) {
            hint.setWidth(widget->width());
        }
        if (hint.height() <= 0) {
            hint.setHeight(widget->height());
        }
        hint = hint.boundedTo(widget->maximumSize());
        mainSizes.append(row ? hint.width() : hint.height());
        crossSizes.append(row ? hint.height() : hint.width());
    }
    if (mainSizes.isEmpty()) {
        return 0.0;
    }

    const qreal limit = along > 0.0 ? along : unwrappedMainExtent();
    qreal lineMain = 0.0;
    qreal lineCross = 0.0;
    qreal totalCross = 0.0;
    int lines = 0;
    for (int i = 0; i < mainSizes.size(); ++i) {
        const qreal needed = (lineMain > 0.0 ? gap : 0.0) + mainSizes.at(i);
        if (m_wrap && lineMain > 0.0 && lineMain + needed > limit) {
            totalCross += lineCross;
            if (lines > 0) {
                totalCross += gap;
            }
            ++lines;
            lineMain = mainSizes.at(i);
            lineCross = crossSizes.at(i);
            continue;
        }
        lineMain += needed;
        lineCross = qMax(lineCross, crossSizes.at(i));
    }
    totalCross += lineCross;
    if (lines > 0) {
        totalCross += gap;
    }
    return totalCross;
}

QSize MdFlex::minimumSizeHint() const
{
    // The container can always shrink to one child; wrapping is the caller's
    // escape hatch for overflow.
    return QSize(0, 0);
}

void MdFlex::childEvent(QChildEvent *event)
{
    QWidget::childEvent(event);
    if (event->type() == QEvent::ChildAdded
        || event->type() == QEvent::ChildRemoved) {
        updateGeometry();
        relayout();
    }
}

void MdFlex::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    relayout();
}

void MdFlex::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange
        || event->type() == QEvent::StyleChange) {
        relayout();
    }
}

void MdFlex::onThemeChanged()
{
    relayout();
}

void MdFlex::relayout() const
{
    auto *self = const_cast<MdFlex *>(this);

    // Resolve a child's main/cross extent the way a layout would: sizeHint,
    // raised to the minimum hints and the hard minimumSize, clamped to
    // maximumSize. A bare QWidget's sizeHint() is (-1,-1) even after
    // setFixedSize(), so falling through to minimumSize() is what makes
    // fixed-size chips land at the published spacing.
    auto extentOf = [](QWidget *item, QSize *resolved) {
        QSize hint = item->sizeHint();
        hint = hint.expandedTo(item->minimumSizeHint());
        hint = hint.expandedTo(item->minimumSize());
        if (hint.width() <= 0) {
            hint.setWidth(item->width());
        }
        if (hint.height() <= 0) {
            hint.setHeight(item->height());
        }
        hint = hint.boundedTo(item->maximumSize());
        *resolved = hint;
    };

    QVector<QWidget *> items;
    for (QObject *child : self->children()) {
        auto *widget = qobject_cast<QWidget *>(child);
        if (widget != nullptr && !widget->isWindow() && !widget->isHidden()) {
            items.append(widget);
        }
    }
    if (items.isEmpty()) {
        return;
    }

    const qreal gap = gapPx();
    const bool row = m_direction == MdFlexDirection::Row;
    const qreal mainExtent = row ? width() : height();
    const qreal crossExtent = row ? height() : width();

    // Measure each child on the main axis.
    QVector<qreal> mainSizes;
    QVector<qreal> crossSizes;
    qreal total = 0.0;
    for (QWidget *item : items) {
        QSize hint;
        extentOf(item, &hint);
        const qreal main = row ? hint.width() : hint.height();
        const qreal cross = row ? hint.height() : hint.width();
        mainSizes.append(main);
        crossSizes.append(cross);
        total += main;
    }

    // Pack into lines (wrap) or a single line.
    struct Line {
        int first = 0;
        int last = 0; // exclusive
        qreal total = 0.0;
    };
    QVector<Line> lines;
    Line line;
    line.first = 0;
    line.total = 0.0;
    for (int i = 0; i < items.size(); ++i) {
        const qreal needed = (line.last > line.first ? gap : 0.0) + mainSizes.at(i);
        if (m_wrap && line.last > line.first
            && line.total + needed > mainExtent) {
            line.last = i;
            lines.append(line);
            line.first = i;
            line.last = i;
            line.total = 0.0;
        }
        if (line.last > line.first) {
            line.total += gap;
        }
        line.total += mainSizes.at(i);
        line.last = i + 1;
    }
    if (line.last > line.first) {
        lines.append(line);
    }
    if (lines.isEmpty()) {
        return;
    }

    const qreal lineGap = gap;
    const bool rtl = self->layoutDirection() == Qt::RightToLeft && row;

    // Cross-axis position of each line.
    qreal crossUsed = 0.0;
    for (int li = 0; li < lines.size(); ++li) {
        // Tallest child on this line sets the line thickness.
        qreal thickness = 0.0;
        for (int i = lines.at(li).first; i < lines.at(li).last; ++i) {
            thickness = qMax(thickness, crossSizes.at(i));
        }
        crossUsed += thickness;
        if (li + 1 < lines.size()) {
            crossUsed += lineGap;
        }
    }

    qreal crossCursor = 0.0;
    switch (m_align) {
    case MdFlexAlign::Center:
        crossCursor = qMax(0.0, (crossExtent - crossUsed) / 2.0);
        break;
    case MdFlexAlign::End:
        crossCursor = qMax(0.0, crossExtent - crossUsed);
        break;
    case MdFlexAlign::Start:
    case MdFlexAlign::Stretch:
    case MdFlexAlign::Count:
        break;
    }

    for (int li = 0; li < lines.size(); ++li) {
        const Line &run = lines.at(li);
        const int count = run.last - run.first;
        qreal lineThickness = 0.0;
        for (int i = run.first; i < run.last; ++i) {
            lineThickness = qMax(lineThickness, crossSizes.at(i));
        }

        qreal free = mainExtent - run.total;
        qreal lead = 0.0;
        qreal between = gap;
        switch (m_justify) {
        case MdFlexJustify::Start:
            lead = 0.0;
            break;
        case MdFlexJustify::Center:
            lead = qMax(0.0, free / 2.0);
            break;
        case MdFlexJustify::End:
            lead = qMax(0.0, free);
            break;
        case MdFlexJustify::SpaceBetween:
            lead = 0.0;
            if (count > 1) {
                between = gap + free / qreal(count - 1);
            }
            break;
        case MdFlexJustify::SpaceAround:
            if (count > 0) {
                const qreal share = free / qreal(count);
                lead = share / 2.0;
                between = gap + share;
            }
            break;
        case MdFlexJustify::SpaceEvenly:
            if (count > 0) {
                const qreal share = free / qreal(count + 1);
                lead = share;
                between = gap + share;
            }
            break;
        case MdFlexJustify::Count:
            break;
        }

        qreal mainCursor = lead;
        for (int i = run.first; i < run.last; ++i) {
            qreal itemMain = mainSizes.at(i);
            qreal itemCross = crossSizes.at(i);
            qreal crossOffset = 0.0;

            if (m_align == MdFlexAlign::Stretch) {
                itemCross = lineThickness;
            } else {
                switch (m_align) {
                case MdFlexAlign::Center:
                    crossOffset = (lineThickness - itemCross) / 2.0;
                    break;
                case MdFlexAlign::End:
                    crossOffset = lineThickness - itemCross;
                    break;
                case MdFlexAlign::Start:
                case MdFlexAlign::Stretch:
                case MdFlexAlign::Count:
                    break;
                }
            }

            QRectF geometry;
            if (row) {
                const qreal x = rtl ? mainExtent - mainCursor - itemMain : mainCursor;
                geometry = QRectF(x, crossCursor + crossOffset, itemMain, itemCross);
            } else {
                geometry = QRectF(crossCursor + crossOffset, mainCursor, itemCross, itemMain);
            }
            items.at(i)->setGeometry(geometry.toRect());

            mainCursor += itemMain + between;
        }

        crossCursor += lineThickness + lineGap;
    }
}

} // namespace md
