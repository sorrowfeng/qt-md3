#include "MdTextFieldStyle.h"

#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTextField.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QFontMetricsF>
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
    // MD3's focused row (2 px indicator / primary outline) tracks real focus,
    // not keyboard-only focus: a mouse-clicked select must still look focused.
    // hasVisualFocus() also covers MdSelect, whose field is a NoFocus child
    // and cannot take focus itself. The separate 3 px system focus ring is a
    // different token family and is not painted on text fields.
    const MdTextFieldState state = field.isEffectivelyDisabled()
        ? MdTextFieldState::Disabled
        : (field.hasVisualFocus() ? MdTextFieldState::Focused
           : (field.isHovered()    ? MdTextFieldState::Hovered
                                   : MdTextFieldState::Enabled));

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF container = field.containerRect();
    const qreal radius = tokens.containerRadius;

    // 1 — the container.
    {
        if (tokens.outlined) {
            // Outlined: transparent fill, 1 px stroke resting / hover and the
            // focus-active-indicator-height (2 px) under focus. The stroke is
            // snapped and the floating label's gap is subtracted from the
            // path so the outline opens around the text instead of running
            // through it — that overlap is what reads as an "ugly border".
            const qreal outlineW =
                state == MdTextFieldState::Focused ? tokens.focusIndicatorHeight
                                                   : tokens.outlineWidth;
            QColor outlineColour = field.hasError()
                ? MdTheme::instance().color(tokens.errorColor)
                : resolveSlot(tokens.indicator[int(state)]);
            if (!outlineColour.isValid()) {
                outlineColour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
            }

            const QList<qreal> radii{radius, radius, radius, radius};
            QPainterPath outline = MdStyleBase::crispRoundedRectPath(
                container.adjusted(outlineW / 2.0, outlineW / 2.0,
                                   -outlineW / 2.0, -outlineW / 2.0),
                radii);

            // Label cut-out. Only the floated outlined label sits on the
            // stroke; the filled label lives inside the slab and needs none.
            if (!field.labelText().isEmpty() && field.labelFloat() > 0.5) {
                painter.save();
                painter.setFont(MdTypeScale::font(TypeStyle::BodySmall));
                const QFontMetricsF fm(painter.font());
                const QRectF label = field.labelRect();
                const qreal textW =
                    fm.horizontalAdvance(field.labelText());
                const qreal gapX = label.left();
                const qreal gapW = qBound(qreal(0.0), textW + 8.0,
                                          qMax(qreal(0.0), label.width()));
                QRectF gap(gapX, container.top() - outlineW,
                           gapW, outlineW * 2.0 + 2.0);
                QPainterPath gapPath;
                gapPath.addRect(gap);
                outline = outline.subtracted(gapPath);
                painter.restore();
            }

            painter.setPen(QPen(outlineColour, outlineW, Qt::SolidLine,
                                Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(outline);
        } else {
            // Filled: surface-container-highest slab, corner-extra-small-top
            // (4 px top only). Build the path with zero bottom radii rather
            // than clipping a fully-rounded rect — the old clip's two rects
            // unioned to the full container and left the bottom corners round.
            QColor fill = MdTheme::instance().color(tokens.containerColor);
            if (field.isEffectivelyDisabled()) {
                fill = MdTheme::instance().color(ColorRole::OnSurface);
                fill.setAlphaF(qBound(0.0, fill.alphaF() * tokens.disabledContainerOpacity, 1.0));
            }
            const QList<qreal> radii = tokens.containerRadiusTopOnly
                ? QList<qreal>{radius, radius, 0.0, 0.0}
                : QList<qreal>{radius, radius, radius, radius};
            painter.setPen(Qt::NoPen);
            painter.setBrush(fill);
            painter.drawPath(MdStyleBase::crispRoundedRectPath(container, radii));
        }

        // Hover state layer, clipped to the container shape. Focus is the
        // indicator / outline's job; press is not a text-field state.
        if (state == MdTextFieldState::Hovered && !field.isEffectivelyDisabled()) {
            const qreal scale =
                tokens.hoverStateLayerOpacity / MdStateLayer::opacity(StateLayerKind::Hover);
            const QColor overlay = MdStateLayer::overlayScaled(
                MdTheme::instance().color(ColorRole::OnSurface), StateLayerKind::Hover, scale);
            if (overlay.isValid()) {
                const QList<qreal> radii = tokens.outlined || !tokens.containerRadiusTopOnly
                    ? QList<qreal>{radius, radius, radius, radius}
                    : QList<qreal>{radius, radius, 0.0, 0.0};
                painter.setPen(Qt::NoPen);
                painter.setBrush(overlay);
                painter.drawPath(MdStyleBase::crispRoundedRectPath(container, radii));
            }
        }
    }

    // 2 — the active indicator (the bottom band). Filled only: the outlined
    // set carries the stroke as its indicator.
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
        painter.drawText(field.labelRect(), MdStyleBase::leadingAlignment(&field) | Qt::AlignVCenter, field.labelText());
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
                         MdStyleBase::leadingAlignment(&field) | Qt::AlignVCenter, field.supportingText());
    }

    // 5 — the leading / trailing icons (24 px, on-surface-variant; the
    // trailing icon takes the error colour when the field is errored).
    if (!field.leadingIconName().isEmpty()) {
        const QRectF icon = field.leadingIconRect();
        if (icon.isValid()) {
            QColor colour = resolveSlot(tokens.leadingIcon[int(state)]);
            if (!colour.isValid()) {
                colour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
            }
            MdIcon::paint(&painter, icon, field.leadingIconName(), colour);
        }
    }
    if (!field.trailingIconName().isEmpty()) {
        const QRectF icon = field.trailingIconRect();
        if (icon.isValid()) {
            QColor colour = field.hasError()
                ? MdTheme::instance().color(tokens.errorColor)
                : resolveSlot(tokens.trailingIcon[int(state)]);
            if (!colour.isValid()) {
                colour = MdTheme::instance().color(ColorRole::OnSurfaceVariant);
            }
            MdIcon::paint(&painter, icon, field.trailingIconName(), colour);
        }
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
