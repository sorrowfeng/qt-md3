#include "MdSearchTokens.h"

namespace md {

namespace {

/// The state-layer table: on-surface under hover and press, absent otherwise.
void fillStateLayers(MdSearchTokens *tokens)
{
    for (int i = 0; i < searchStateCount; ++i) {
        const MdSearchState state = MdSearchState(i);
        switch (state) {
        case MdSearchState::Hovered:
            tokens->stateLayer[i] = {ColorRole::OnSurface, tokens->hoverStateLayerOpacity};
            break;
        case MdSearchState::Pressed:
            tokens->stateLayer[i] = {ColorRole::OnSurface, tokens->pressedStateLayerOpacity};
            break;
        case MdSearchState::Disabled:
            tokens->stateLayer[i] = {ColorRole::OnSurface, 0.0};
            break;
        case MdSearchState::Enabled:
        case MdSearchState::Count:
            tokens->stateLayer[i] = {ColorRole::OnSurface, 0.0};
            break;
        }
    }
}

} // namespace

MdSearchTokens MdSearchTokens::resolve(MdSearchSurface surface)
{
    MdSearchTokens tokens;

    switch (surface) {
    case MdSearchSurface::Bar:
        tokens.containerHeight = 56.0;
        tokens.containerRadius = 28.0;
        break;
    case MdSearchSurface::DockedView:
        tokens.containerHeight = 56.0;
        tokens.containerRadius = tokens.dockedRadius;
        tokens.containerColor = tokens.viewBackgroundColor;
        break;
    case MdSearchSurface::FullScreenView:
        tokens.containerHeight = 72.0;
        tokens.containerRadius = tokens.fullScreenRadius;
        tokens.containerColor = tokens.viewBackgroundColor;
        break;
    case MdSearchSurface::Count:
        break;
    }

    fillStateLayers(&tokens);
    return tokens;
}

} // namespace md
