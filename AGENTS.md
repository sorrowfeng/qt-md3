# AGENTS.md — working agreement for qt-md3

This file tells humans and AI agents how work happens in this repository. Read it
before touching code.

## What this project is

`qt-md3` reproduces Google's **Material Design 3** (including **M3 Expressive**)
as native **Qt Widgets**, hand-painted with `QPainter`. The goal is not a loose
"Material-flavoured" look: token values, component behavior, states, and motion
are aligned item by item with the official design system.

- Language: C++17. Build: CMake, auto-detecting Qt 6 (min 6.5.0) or Qt 5 (min
  5.15.2).
- Namespace `md`, class prefix `Md` (`MdButton`, `MdButtonStyle`, `MdTheme`,
  `MdColorRole`).
- Painting: `QPainter` / `QProxyStyle` only. **QSS / `QStyleSheet` is forbidden**,
  enforced by the `TestMd3NoQss` gate.

## Two stages, strictly serial

| Stage | Goal | Done when |
| --- | --- | --- |
| **1 — MD3 official** | All **36** component families from <https://m3.material.io/components>, with every property, variant, size, shape, state, token, motion spec, and keyboard behavior | All nine columns green for every family in [docs/md3-coverage.md](docs/md3-coverage.md) |
| **2 — Qt extensions** | Common Qt controls MD3 has no spec for, redesigned with Stage 1 tokens and semantics | Appendix D list delivered, visually consistent with Stage 1 |

**Hard rule: do not start any Stage 2 public component until Stage 1 is 100%
green.** The `TestMd3CoveragePolicy` gate fails the build if a Stage 2 public
header appears under `src/widgets/` early. Before every task, ask: *Stage 1 or
Stage 2?* If Stage 2, prove Stage 1 is green first.

Within Stage 1, follow the official category order and do not skip around:
Actions → Communication → Containment → Navigation → Selection → Text inputs, then
the M3 Expressive cross-cutting pass.

## Authoritative sources

- Design spec: <https://m3.material.io/> (JS-rendered — use a browser, not
  `WebFetch`).
- **Token values (must be taken from here, never from memory):**
  <https://github.com/material-components/material-web/tree/main/tokens>.
- Color algorithms (official C++): <https://github.com/material-foundation/material-color-utilities>.
- Component behavior / API: <https://github.com/material-components/material-web/tree/main/docs/components>.
- Expressive behavior (spring, shape morph): Jetpack Compose Material3 source —
  `material-web` is in maintenance mode and does not implement Expressive.
- Icons: `google/material-symbols` (variable font) + `google/material-design-icons`
  (classic SVG).

## Architecture

- `src/core/` — tokens, theme, color algorithms, type scale, shape, motion, icons,
  utilities, export macro. Common enums/types live in `src/core/MdTypes.h`.
- `src/styles/` — every `Md*Style` painting class.
- `src/widgets/` — every public component (`Md*.h`).
- `src/private/` — internal helpers (not exported).
- `examples/` — the gallery app; one page per component plus a Showcase home.
- `tests/` — QTest + CTest, including the policy gates.
- `resources/` — icons, fonts, `.qrc`.
- `docs/` — status, audit, specs, TODOs.

Rendering patterns:

- **A (preferred)** — custom `QWidget` + `Md<Component>Style : MdStyleBase`, which
  intercepts Paint via an event filter and implements `drawWidget()`.
- **B** — subclass a standard Qt control (`QPushButton` / `QCheckBox` / `QMenu` /
  `QToolBar`…) and override `drawControl()` / `drawComplexControl()` in the style.
- **C** — a pure container that paints itself in `paintEvent`.

Rules:

- Every visual number comes from a theme token; no magic colors, radii, sizes,
  spacings, or durations inside components.
- Theme refresh is driven by `MdStyleBase::connectThemeUpdate<T>()`. Never scan
  all widgets on a theme switch.
- The version number has one source: the root `VERSION` file.
- The library is subproject-friendly: as an `add_subdirectory()` dependency it
  builds only the library unless examples/tests are explicitly enabled.

## Definition of Done (per component)

1. `src/widgets/Md<Component>.h/.cpp` + `src/styles/Md<Component>Style.h/.cpp`,
   explicitly listed in CMake.
2. Public API covers **all** official variants / sizes / shapes / states / slots,
   named 1:1 with the official semantics.
3. Every configurable item is a `Q_PROPERTY` with a NOTIFY signal.
4. States: enabled / hovered / focused / pressed / disabled / selected / dragged /
   loading, as applicable.
5. Themes: light / dark, seed change, contrast level, density, RTL, font switch.
6. Accessibility: Tab-reachable, visible focus ring, Space/Enter/Esc/arrow keys
   matching the spec.
7. Example page + left-nav entry, with zero style operations in the page.
8. Tests: properties & signals, lifecycle & object tree, meta-property, theme
   lifecycle, non-blank render smoke.
9. Visual audit entry in [`docs/visual-audit.md`](docs/visual-audit.md).
10. The family's nine columns updated in
    [`docs/md3-coverage.md`](docs/md3-coverage.md).
11. Docs updated: this file, `README.md`, `docs/project-status.md`.
12. One dedicated commit: `feat(<component>): <description>`.

## Pitfalls (MD3 differs sharply from Ant Design here)

- Elevation is expressed with **tonal surface**, not shadow. Overlays have no drop
  shadow unless the spec explicitly requires it.
- State layer and ripple are separate: hover / focus are static overlays; ripple
  only spreads on press.
- Every radius comes from a shape token and must support runtime tweening (shape
  morph). Never hard-code a radius.
- Colors are never literal: light / dark are tone mappings of the same
  `DynamicScheme`; changing the seed regenerates every role.
- Line height adapts to the script height class (Chinese is "medium", ~7% taller
  than Latin).
- M3 Expressive motion is spring physics, not a fixed-duration easing curve; the
  two must coexist.
- Icons are Material Symbols (variable font, four axes), not a static SVG set.
- Do not import Ant Design's semantics — variant / elevation / state layer are not
  AntD's type / size / status.

## Working rhythm

- Start each task with a "current status + next plan" report and wait for
  confirmation.
- Advance one base module or one component at a time; finish the whole DoD before
  moving on.
- Run only the CTest targets for the item you changed; do not default to a full
  run.
- When comparing against the official UI, screenshot first, then change code.
- If the official docs and the official implementation disagree, record the
  difference and state the trade-off — do not silently pick one.

## Current status

Bootstrap. Version `0.1.0`. No components yet; 0 / 36 families complete. See
[`docs/project-status.md`](docs/project-status.md) for the snapshot and
[`docs/porting-todo.md`](docs/porting-todo.md) for the work queue.

## Build

```bash
cmake --preset windows-msvc-qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```
