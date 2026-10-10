#include "MdTextFieldTokens.h"

namespace md {

namespace {

/// The colour tables. The export publishes the resting / hover / focus rows
/// plus a disabled row; the pressed state is absent (the field is a text
/// input, not a button). The focus rows carry the primary label / indicator.
void fillTables(MdTextFieldTokens *tokens)
{
    for (int i = 0; i < textFieldStateCount; ++i) {
        const MdTextFieldState state = MdTextFieldState(i);
        const bool disabled = state == MdTextFieldState::Disabled;

        switch (state) {
        case MdTextFieldState::Enabled:
            tokens->indicator[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->label[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->supportingText[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->leadingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->trailingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            break;
        case MdTextFieldState::Hovered:
            tokens->indicator[i] = {ColorRole::OnSurface, 1.0};
            tokens->label[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->supportingText[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->leadingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->trailingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            break;
        case MdTextFieldState::Focused:
            tokens->indicator[i] = {ColorRole::Primary, 1.0};
            tokens->label[i] = {ColorRole::Primary, 1.0};
            tokens->supportingText[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->leadingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            tokens->trailingIcon[i] = {ColorRole::OnSurfaceVariant, 1.0};
            break;
        case MdTextFieldState::Disabled:
            tokens->indicator[i] = {ColorRole::OnSurface, tokens->disabledContentOpacity};
            tokens->label[i] = {ColorRole::OnSurface, tokens->disabledContentOpacity};
            tokens->supportingText[i] = {ColorRole::OnSurface, tokens->disabledContentOpacity};
            tokens->leadingIcon[i] = {ColorRole::OnSurface, tokens->disabledContentOpacity};
            tokens->trailingIcon[i] = {ColorRole::OnSurface, tokens->disabledContentOpacity};
            break;
        case MdTextFieldState::Count:
            break;
        }
    }
}

} // namespace

MdTextFieldTokens MdTextFieldTokens::resolve(MdTextFieldVariant variant)
{
    MdTextFieldTokens tokens;
    const bool outlined = variant == MdTextFieldVariant::Outlined;

    tokens.outlined = outlined;
    // The outlined set publishes no container colour — the field sits on the
    // page surface with a 1 px outline. ColorRole has no Transparent member;
    // the paint treats an invalid slot as no fill, so the outlined container
    // keeps its Count row and paints nothing.
    tokens.containerColor =
        outlined ? ColorRole::Count : ColorRole::SurfaceContainerHighest;
    tokens.containerRadiusTopOnly = !outlined;

    fillTables(&tokens);
    return tokens;
}

} // namespace md
