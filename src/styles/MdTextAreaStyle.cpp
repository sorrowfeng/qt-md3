#include "MdTextAreaStyle.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTextArea.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

QColor resolveSlot(const MdNavigationColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (!colour.isValid()) {
        return QColor();
    }
    colour.setAlphaF(qBound(0.0, colour.alphaF() * slot.opacity, 1.0));
    return colour;
}

} // namespace

MdTextAreaStyle::MdTextAreaStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTextAreaStyle *MdTextAreaStyle::shared()
{
    static QMutex mutex;
    static MdTextAreaStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTextAreaStyle;
        installPaintFilter<MdTextArea>(instance);
    }
    return instance;
}

bool MdTextAreaStyle::isInstalled()
{
    return hasPaintFilter(&MdTextArea::staticMetaObject);
}

void MdTextAreaStyle::paintTextArea(QPainter &painter, const MdTextArea &area,
                                    const MdTextFieldTokens &tokens)
{
    const MdTextFieldState state = area.isEffectivelyDisabled()
        ? MdTextFieldState::Disabled
        : (area.hasKeyboardFocus() ? MdTextFieldState::Focused
           : (area.isHovered()    ? MdTextFieldState::Hovered
                                  : MdTextFieldState::Enabled));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 — the container.
    {
        const QRectF container = area.containerRect();
        if (tokens.outlined) {
            painter.setPen(QPen(resolveSlot(tokens.indicator[int(state)]), tokens.outlineWidth));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(container.adjusted(0.5, 0.5, -0.5, -0.5), tokens.containerRadius,
                                    tokens.containerRadius);
        } else {
            QColor fill = MdTheme::instance().color(tokens.containerColor);
            if (area.isEffectivelyDisabled()) {
                fill = MdTheme::instance().color(ColorRole::OnSurface);
                fill.setAlphaF(qBound(0.0, fill.alphaF() * tokens.disabledContainerOpacity, 1.0));
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            if (tokens.containerRadiusTopOnly) {
                QPainterPath path;
                path.addRoundedRect(container, tokens.containerRadius, tokens.containerRadius);
                QPainterPath clip;
                clip.addRect(container.adjusted(0.0, 0.0, 0.0, -tokens.containerRadius));
                clip.addRect(container.adjusted(0.0, tokens.containerRadius, 0.0, 0.0));
                painter.setClipPath(clip);
                painter.drawPath(path);
                painter.setClipping(false);
            } else {
                painter.drawRoundedRect(container, tokens.containerRadius, tokens.containerRadius);
            }
        }
    }

    // 2 — the active indicator (under the last line).
    if (!tokens.outlined) {
        const MdNavigationColourSlot &slot = tokens.indicator[int(state)];
        const QColor colour = area.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(slot);
        if (colour.isValid()) {
            const qreal height = state == MdTextFieldState::Focused ? tokens.focusIndicatorHeight
                                 : state == MdTextFieldState::Hovered
                                         ? tokens.hoverIndicatorHeight
                                         : tokens.indicatorHeight;
            painter.setPen(Qt::NoPen);
            painter.setBrush(colour);
            painter.drawRect(QRectF(area.containerRect().left(),
                                    area.containerRect().bottom() - height,
                                    area.containerRect().width(), height));
        }
    }

    // 3 — the floating label.
    if (!area.labelText().isEmpty()) {
        const bool floated = area.labelFloat() > 0.5;
        const QFont font = MdTypeScale::font(floated ? TypeStyle::BodySmall : TypeStyle::BodyLarge);
        painter.setFont(font);
        QColor labelColour = area.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(tokens.label[int(state)]);
        if (!labelColour.isValid()) {
            labelColour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
        }
        painter.setPen(labelColour);
        painter.drawText(area.labelRect(), MdStyleBase::leadingAlignment(&area) | Qt::AlignVCenter, area.labelText());
    }

    // 4 — the supporting text.
    if (!area.supportingText().isEmpty()) {
        const QFont font = MdTypeScale::font(TypeStyle::BodySmall);
        painter.setFont(font);
        QColor supportingColour = area.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(tokens.supportingText[int(state)]);
        if (!supportingColour.isValid()) {
            supportingColour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
        }
        painter.setPen(supportingColour);
        painter.drawText(QRectF(area.containerRect().left() + 16.0,
                                area.containerRect().bottom() + 4.0,
                                area.containerRect().width() - 32.0, 16.0),
                         MdStyleBase::leadingAlignment(&area) | Qt::AlignVCenter, area.supportingText());
    }

    painter.restore();
}

void MdTextAreaStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdTextArea *area = qobject_cast<MdTextArea *>(widget);
    if (!area || !painter) {
        return;
    }
    paintTextArea(*painter, *area, area->textFieldTokens());
}

} // namespace md
