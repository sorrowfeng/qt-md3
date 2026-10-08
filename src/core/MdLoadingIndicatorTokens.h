#ifndef MD_LOADING_INDICATOR_TOKENS_H
#define MD_LOADING_INDICATOR_TOKENS_H

// The `md.comp.loading-indicator.*` token set, export 34.0.21.
//
// Every number and every role below is transcribed from the authoritative
// source, never from memory:
//
//   material-components/material-web
//     tokens/versions/latest/sass/_md-comp-loading-indicator.scss
//
// Token export version 34.0.21. The export is an **Expressive-only family**:
// material-web ships the token rows but no web component, so the behaviour
// port (morph sequence, spring, rotations) comes from Compose M3 Expressive
// (androidx.compose.material3.LoadingIndicator, androidx-main), recorded in
// docs/porting-todo.md.
//
// Facts of this family worth keeping visible:
//
//   * `container.color` (secondary-container) is **deprecated** — "in favor
//     of a distinct variant which uses a different color mapping with, and
//     without container". That variant is what the export's `contained.*`
//     rows describe and what Compose ships; the deprecated row is
//     transcribed here for completeness and never read by the painter.
//   * The container shape is `corner-full` on a square — the container is a
//     circle; the active indicator lives inside it at
//     active-indicator.size / min(container.width, container.height)
//     (Compose's ActiveIndicatorScale).
//   * Like the progress-indicator families, the export publishes **no state
//     rows at all** — a loading indicator is not interactive.

#include "MdTokens.h"
#include "MdTypes.h"
#include "QtMd3Export.h"

namespace md {

/// Everything needed to paint and lay out one loading indicator.
struct QT_MD3_EXPORT MdLoadingIndicatorTokens
{
    // --- metrics (md.comp.loading-indicator.*) ------------------------------
    /// `active-indicator.size` — the morphing shape's target size.
    qreal activeIndicatorSize = 38.0;
    /// `container.width` / `container.height` — the 48 px square the shape
    /// spins inside.
    qreal containerWidth = 48.0;
    qreal containerHeight = 48.0;
    /// `container.shape` — `corner-full`, i.e. the container is a circle.
    ShapeCorner containerShape = ShapeCorner::Full;

    // --- colours (md.comp.loading-indicator.*) ------------------------------
    /// `active-indicator.color` — the uncontained variant's shape colour.
    ColorRole activeIndicatorColor = ColorRole::Primary;
    /// `contained.active-indicator.color`.
    ColorRole containedActiveIndicatorColor = ColorRole::OnPrimaryContainer;
    /// `contained.container.color`.
    ColorRole containedContainerColor = ColorRole::PrimaryContainer;
    /// `container.color` — **deprecated** (see the file comment); the
    /// secondary-container row superseded by the contained variant.
    ColorRole deprecatedContainerColor = ColorRole::SecondaryContainer;

    /// Build the published set for one variant.
    ///
    /// `overrides` is consulted through the `md.comp.*` key namespace, the
    /// variant-qualified key winning over the base one:
    ///
    ///   md.comp.loading-indicator.contained.container.width  (contained, wins)
    ///   md.comp.loading-indicator.container.width            (base fallback)
    ///
    /// Colours are not overridable — same rule as the other families,
    /// recorded in docs/porting-todo.md. Lengths accept both spellings.
    static MdLoadingIndicatorTokens resolve(
        LoadingIndicatorVariant variant, const MdComponentTokens *overrides = nullptr);
};

} // namespace md

#endif // MD_LOADING_INDICATOR_TOKENS_H
