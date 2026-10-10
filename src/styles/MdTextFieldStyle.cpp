#include "MdTextFieldStyle.h"

#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTextField.h"

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

MdTextFieldStyle::MdTextFieldStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTextFieldStyle *MdTextFieldStyle::shared()
{
    static QMutex mutex;
    static MdTextFieldStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTextFieldStyle;
        installPaintFilter<MdTextField>(instance);
    }
    return instance;
}

bool MdTextFieldStyle::isInstalled()
{
    return hasPaintFilter(&MdTextField::staticMetaObject);
}

void MdTextFieldStyle::paintTextField(QPainter &painter, const MdTextField &field,
                                      const MdTextFieldTokens &tokens)
{
    const MdTextFieldState state = field.isEffectivelyDisabled()
        ? MdTextFieldState::Disabled
        : (field.hasKeyboardFocus() ? MdTextFieldState::Focused
           : (field.isHovered()    ? MdTextFieldState::Hovered
                                   : MdTextFieldState::Enabled));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 — the container.
    {
        const QRectF container = field.containerRect();
        if (tokens.outlined) {
            painter.setPen(QPen(resolveSlot(tokens.indicator[int(state)]), tokens.outlineWidth));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(container.adjusted(0.5, 0.5, -0.5, -0.5), tokens.containerRadius,
                                    tokens.containerRadius);
        } else {
            QColor fill = MdTheme::instance().color(tokens.containerColor);
            if (field.isEffectivelyDisabled()) {
                fill = MdTheme::instance().color(ColorRole::OnSurface);
                fill.setAlphaF(qBound(0.0, fill.alphaF() * tokens.disabledContainerOpacity, 1.0));
            }
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            if (tokens.containerRadiusTopOnly) {
                QPainterPath path;
                path.addRoundedRect(container, tokens.containerRadius, tokens.containerRadius);
                // Square the bottom corners — corner-extra-small-top.
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

    // 2 — the active indicator (the bottom band).
    if (!tokens.outlined) {
        const MdNavigationColourSlot &slot = tokens.indicator[int(state)];
        const QColor colour = field.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(slot);
        if (colour.isValid()) {
            const qreal height = state == MdTextFieldState::Focused ? tokens.focusIndicatorHeight
                                 : state == MdTextFieldState::Hovered
                                         ? tokens.hoverIndicatorHeight
                                         : tokens.indicatorHeight;
            painter.setPen(Qt::NoPen);
            painter.setBrush(colour);
            painter.drawRect(QRectF(field.containerRect().left(),
                                    field.containerRect().bottom() - height,
                                    field.containerRect().width(), height));
        }
    }

    // 3 — the floating label.
    if (!field.labelText().isEmpty()) {
        const bool floated = field.labelFloat() > 0.5;
        const QFont font = MdTypeScale::font(floated ? TypeStyle::BodySmall : TypeStyle::BodyLarge);
        painter.setFont(font);
        QColor labelColour = field.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(tokens.label[int(state)]);
        if (!labelColour.isValid()) {
            labelColour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
        }
        painter.setPen(labelColour);
        painter.drawText(field.labelRect(), Qt::AlignLeft | Qt::AlignVCenter, field.labelText());
    }

    // 4 — the supporting text.
    if (!field.supportingText().isEmpty()) {
        const QFont font = MdTypeScale::font(TypeStyle::BodySmall);
        painter.setFont(font);
        QColor supportingColour = field.hasError()
            ? MdTheme::instance().color(tokens.errorColor)
            : resolveSlot(tokens.supportingText[int(state)]);
        if (!supportingColour.isValid()) {
            supportingColour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
        }
        painter.setPen(supportingColour);
        painter.drawText(QRectF(field.containerRect().left() + 16.0,
                                field.containerRect().bottom() + 4.0,
                                field.containerRect().width() - 32.0, 16.0),
                         Qt::AlignLeft | Qt::AlignVCenter, field.supportingText());
    }

    painter.restore();
}

void MdTextFieldStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdTextField *field = qobject_cast<MdTextField *>(widget);
    if (!field || !painter) {
        return;
    }
    paintTextField(*painter, *field, field->textFieldTokens());
}

} // namespace md
