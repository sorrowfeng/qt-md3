# Interaction reference: material-web demo vs qt-md3

This document is the standing comparison reference for **interaction behaviour**
(hover / press / focus / ripple / shape motion). Whenever a component's click
behaviour "looks different" from the material-web demo site
(<https://material-web.dev>), check this page first: most differences are
**expected and deliberate**, not bugs.

## Why the demo looks different from qt-md3

`material-components/material-web` is in **maintenance mode**. It implements
**standard M3** (the 2021–2023 spec): press feedback is a *flat pressed state
layer + ripple*, and the container shape never changes while pressed.

qt-md3 targets **M3 Expressive**: per the official spec pages (m3.material.io,
which now label the flat *"Pressed (state layer)"* row as **[Deprecated]** in
favour of the ripple) and **Compose M3** as the behavioural source for
Expressive motion, the press response is the **ripple itself plus a shape
morph** of the container.

Both behaviours trace to authoritative Google sources; they simply belong to
two different generations of the design system. Per `AGENTS.md`, when the two
sources conflict we record the divergence and choose deliberately — this file
is that record.

## Side-by-side comparison

| Interaction | material-web (standard M3) | qt-md3 (M3 Expressive) | Status |
|---|---|---|---|
| Hover | Flat state layer, `hover-state-layer-color` @ 8% opacity | Same (flat hover layer, token-driven) | 一致 |
| Press | Flat `pressed-state-layer-color` layer **+** ripple | Ripple only (no flat pressed layer) + Expressive shape morph | 有意分歧，见上 |
| Ripple colour | Component's pressed state-layer colour (e.g. filled → `on-primary`) | Same: tokens 按压行 `stateLayer` 角色 | 一致 |
| Ripple timing | grow 450 ms, min press 225 ms, fade 375 ms, initial scale 0.2, padding 10, soft edge 75/0.35, `EASING.STANDARD` | Identical constants (`MdRipple`) | 一致 |
| Focus ring | `md-focus-ring`, keyboard-only (`:focus-visible` semantics) | `hasKeyboardFocus()` gating by `focusInEvent` reason | 一致 |
| Container shape on press | Static (`container-shape` token, never changes) | Morphs rest → pressed shape per size (8/8/12/16/16 px), `spring-fast-spatial` (1400 / 0.9) | 有意分歧，Expressive |
| Touch handling | `TOUCH_DELAY_MS = 150` before starting ripple (swipe/scroll disambiguation) | Mouse-only for now; touch delay not implemented | 已知缺口，记入 porting-todo |
| Elevation on hover/press (elevated/filled) | Elevation level tokens transitioned by CSS | Token-driven elevation levels in style layer | 一致 |

## Per-variant ripple / pressed-layer colour map

Matches material-web's token wiring and our token table:

| Variant | Ripple colour (pressed row) |
|---|---|
| Filled | `on-primary` |
| Filled tonal | `on-secondary-container` |
| Elevated | `primary` |
| Outlined | `on-surface` |
| Text | `on-surface` |

## How to verify a suspected difference

1. Read the constant in `material-web` source (local clone, e.g.
   `ripple/internal/ripple.ts`, `button/internal/_shared.scss`) — not from the
   rendered site alone.
2. Run the qt-md3 probe exe for the component, capture frames at ~50 ms
   steps around a press/release, tile them with PIL, and pixel-sample the
   frames.
3. Compare against the numbers above. Anything that differs must either be
   fixed or promoted to a recorded divergence in this file +
   `docs/porting-todo.md`.

## Component-specific notes

- **Buttons** (audited 2026-10-08): three fixes applied so qt-md3 matches the
  table above — `:focus-visible` gating, ripple colour from pressed
  state-layer role, removal of the flat pressed layer. See
  `docs/porting-todo.md` → Buttons → Interaction-fidelity audit.
