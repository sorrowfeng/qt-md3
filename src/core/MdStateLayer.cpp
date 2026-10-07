#include "MdStateLayer.h"

#include "MdTokens.h"

namespace md {

qreal MdStateLayer::opacity(StateLayerKind kind)
{
    return MdSystemTokens::stateLayerOpacity(kind);
}

QColor MdStateLayer::overlay(const QColor &contentColor, StateLayerKind kind)
{
    return overlayScaled(contentColor, kind, 1.0);
}

QColor MdStateLayer::overlayScaled(const QColor &contentColor, StateLayerKind kind, qreal scale)
{
    QColor result = contentColor;
    result.setAlphaF(qBound(0.0, opacity(kind) * scale, 1.0));
    return result;
}

bool MdStateLayer::strongestActive(StateLayerKind *result,
                                   bool hovered,
                                   bool focused,
                                   bool pressed,
                                   bool dragged)
{
    // Ordered by descending token opacity: dragged > focus == pressed > hover.
    if (dragged) {
        if (result) {
            *result = StateLayerKind::Dragged;
        }
        return true;
    }
    if (focused) {
        if (result) {
            *result = StateLayerKind::Focus;
        }
        return true;
    }
    if (pressed) {
        if (result) {
            *result = StateLayerKind::Pressed;
        }
        return true;
    }
    if (hovered) {
        if (result) {
            *result = StateLayerKind::Hover;
        }
        return true;
    }
    return false;
}

QColor MdStateLayer::over(const QColor &base,
                          const QColor &contentColor,
                          bool hovered,
                          bool focused,
                          bool pressed,
                          bool dragged)
{
    StateLayerKind kind = StateLayerKind::Hover;
    if (!strongestActive(&kind, hovered, focused, pressed, dragged)) {
        return base;
    }

    const QColor overlayColor = overlay(contentColor, kind);
    const qreal alpha = overlayColor.alphaF();
    return QColor::fromRgbF(base.redF() * (1.0 - alpha) + overlayColor.redF() * alpha,
                            base.greenF() * (1.0 - alpha) + overlayColor.greenF() * alpha,
                            base.blueF() * (1.0 - alpha) + overlayColor.blueF() * alpha,
                            base.alphaF());
}

} // namespace md
