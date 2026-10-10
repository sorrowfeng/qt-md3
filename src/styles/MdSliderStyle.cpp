#include "MdSliderStyle.h"

#include "core/MdElevation.h"
#include "core/MdFocusRing.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdSlider.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// A `MdNavigationColourSlot` as a paintable colour. An absent slot resolves
/// to an *invalid* colour, which the paint treats as transparent.
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

MdSliderStyle::MdSliderStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdSliderStyle *MdSliderStyle::shared()
{
    static QMutex mutex;
    static MdSliderStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdSliderStyle;
        installPaintFilter<MdSlider>(instance);
    }
    return instance;
}

bool MdSliderStyle::isInstalled()
{
    return hasPaintFilter(&MdSlider::staticMetaObject);
}

MdFocusRingSpec MdSliderStyle::focusRingSpec(const MdSliderTokens &tokens)
{
    MdFocusRingSpec spec;
    spec.width = tokens.focusIndicatorThickness;
    spec.activeWidth = 8.0;
    spec.outwardOffset = tokens.focusIndicatorOuterOffset;
    spec.inward = false;
    // Around the 40 px state-layer circle — Compose's `CircleShape` contract.
    spec.shape = ShapeCorner::Full;
    return spec;
}

void MdSliderStyle::paintSlider(QPainter &painter, const MdSlider &slider,
                                const MdSliderTokens &tokens)
{
    const bool disabled = slider.isEffectivelyDisabled();
    const MdSliderInteraction state = slider.interactionState();
    const QRectF track = slider.trackRect();
    const QRectF handleStart = slider.handleRect(true);
    const QRectF handleEnd = slider.handleRect(false);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 + 2 — the tracks. Both are full pills; the active one is clipped to
    // the span the handles cover.
    const qreal trackHeight = tokens.trackHeight;
    const qreal radius = qMin(trackHeight / 2.0, tokens.trackEndRadius);
    auto trackPath = [&](const QRectF &span) {
        QPainterPath path;
        path.addRoundedRect(span, radius, radius);
        return path;
    };

    // Inactive: the full track.
    {
        const QColor colour = resolveSlot(tokens.inactiveTrack[int(state)]);
        if (colour.isValid()) {
            painter.fillPath(trackPath(track), colour);
        }
    }

    // Active: from the track start to the handle centre (or between the two
    // handles in range form).
    {
        const QColor colour = resolveSlot(tokens.activeTrack[int(state)]);
        QRectF span = track;
        if (slider.isRange()) {
            span.setLeft(handleStart.center().x());
            span.setRight(handleEnd.center().x());
        } else {
            span.setRight(handleEnd.center().x());
        }
        if (colour.isValid() && span.width() > 0.0) {
            painter.fillPath(trackPath(span), colour);
        }
    }

    // 3 — tick marks and stop indicators.
    if (slider.hasTicks() && tokens.tickMarkSize > 0.0 && track.width() > 0.0) {
        const qreal ticks = slider.step() > 0.0
            ? qMax(0.0, (slider.max() - slider.min()) / slider.step())
            : 0.0;
        painter.setPen(Qt::NoPen);
        for (int i = 0; i <= int(ticks + 0.5); ++i) {
            const qreal f = ticks > 0.0 ? i / ticks : 0.0;
            const qreal x = track.left() + f * track.width();
            const bool active = slider.isRange()
                ? (x >= handleStart.center().x() && x <= handleEnd.center().x())
                : (x <= handleEnd.center().x());
            const QColor colour = resolveSlot(
                active ? tokens.activeTickMark[int(state)] : tokens.inactiveTickMark[int(state)]);
            if (!colour.isValid()) {
                continue;
            }
            painter.setBrush(colour);
            painter.drawEllipse(QPointF(x, track.center().y()), tokens.tickMarkSize / 2.0,
                                tokens.tickMarkSize / 2.0);
        }
    }
    {
        painter.setPen(Qt::NoPen);
        const QColor startColour = resolveSlot(tokens.inactiveStopIndicator[int(state)]);
        if (startColour.isValid()) {
            painter.setBrush(startColour);
            painter.drawEllipse(
                QPointF(track.left() + tokens.stopIndicatorTrailingSpace, track.center().y()),
                tokens.stopIndicatorSize / 2.0, tokens.stopIndicatorSize / 2.0);
        }
        const QColor endColour = resolveSlot(tokens.activeStopIndicator[int(state)]);
        if (endColour.isValid()) {
            painter.setBrush(endColour);
            painter.drawEllipse(
                QPointF(track.right() - tokens.stopIndicatorTrailingSpace, track.center().y()),
                tokens.stopIndicatorSize / 2.0, tokens.stopIndicatorSize / 2.0);
        }
    }

    // 4 — the handles. The active handle paints its level-1 shadow.
    auto paintHandle = [&](const QRectF &rect, bool active) {
        if (active && !disabled) {
            MdElevation::drawShadowDp(&painter, rect, rect.width() / 2.0, 1.0,
                                      MdTheme::instance().color(ColorRole::Shadow));
        }
        const QColor colour = resolveSlot(tokens.handle[int(state)]);
        if (!colour.isValid()) {
            return;
        }
        painter.setPen(Qt::NoPen);
        painter.setBrush(colour);
        painter.drawRoundedRect(rect, rect.width() / 2.0, rect.width() / 2.0);
    };
    if (slider.isRange()) {
        paintHandle(handleStart, slider.hasKeyboardFocus());
    }
    paintHandle(handleEnd, true);

    // 5 — the value indicator.
    if (slider.isLabeled() && slider.labelReveal() > 0.01) {
        const QString text = slider.valueLabel().isEmpty()
            ? QString::number(slider.isRange() ? slider.valueEnd() : slider.value())
            : slider.valueLabel();
        const QFont font = MdTypeScale::font(TypeStyle::LabelLarge);
        const QFontMetricsF fm(font);
        const qreal labelHeight = tokens.valueIndicatorMinSize;
        const qreal labelWidth =
            qMax(labelHeight, fm.horizontalAdvance(text) + 2.0 * tokens.handlePadding);
        const QRectF target(handleEnd.center().x() - labelWidth / 2.0,
                            handleEnd.top() - tokens.valueIndicatorBottomSpace - labelHeight,
                            labelWidth, labelHeight);
        const QPointF origin(target.center().x(), target.bottom());
        painter.save();
        painter.translate(origin);
        painter.scale(slider.labelReveal(), slider.labelReveal());
        painter.translate(-origin);
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.valueIndicatorContainer));
        painter.drawRoundedRect(target, labelHeight / 2.0, labelHeight / 2.0);
        painter.setFont(font);
        painter.setPen(MdTheme::instance().color(tokens.valueIndicatorLabel));
        painter.drawText(target, Qt::AlignCenter, text);
        painter.restore();
    }

    // 6 — the focus ring, keyboard focus only.
    if (slider.hasKeyboardFocus()) {
        const QRectF layer = slider.stateLayerRect(false);
        MdFocusRing::paint(&painter, layer, QList<qreal>(),
                           MdTheme::instance().color(tokens.focusIndicatorColor),
                           focusRingSpec(tokens));
    }

    painter.restore();
}

void MdSliderStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdSlider *slider = qobject_cast<MdSlider *>(widget);
    if (!slider || !painter) {
        return;
    }
    paintSlider(*painter, *slider, slider->sliderTokens());
}

} // namespace md
