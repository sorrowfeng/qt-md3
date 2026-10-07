#ifndef MD_STATE_LAYER_H
#define MD_STATE_LAYER_H

// State layers.
//
// A state layer is a flat colour overlay drawn on top of a component while it
// is hovered / focused / pressed / dragged. It is a separate mechanism from
// the ripple: the ripple is a transient circle that expands on press, whereas
// a state layer is static for as long as the state holds.
//
// Opacities are the published md.sys.state tokens.

#include "MdTypes.h"
#include "QtMd3Export.h"

#include <QtGui/QColor>

namespace md {

class QT_MD3_EXPORT MdStateLayer
{
public:
    /// hover 0.08, focus 0.12, pressed 0.12, dragged 0.16.
    static qreal opacity(StateLayerKind kind);

    /// The overlay colour: `contentColor` at the state's opacity.
    static QColor overlay(const QColor &contentColor, StateLayerKind kind);

    /// Overlay colour at an explicit opacity (for interpolating in/out).
    static QColor overlayScaled(const QColor &contentColor, StateLayerKind kind, qreal scale);

    /// Composite over `base`. When several states hold at once MD3 does not
    /// stack them; the strongest single layer is used.
    static QColor over(const QColor &base,
                       const QColor &contentColor,
                       bool hovered,
                       bool focused,
                       bool pressed,
                       bool dragged);

    /// Strongest active state, or `nullptr` when none is active.
    static bool strongestActive(StateLayerKind *result,
                                bool hovered,
                                bool focused,
                                bool pressed,
                                bool dragged);
};

} // namespace md

#endif // MD_STATE_LAYER_H
