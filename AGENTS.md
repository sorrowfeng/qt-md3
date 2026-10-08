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

**Every component port consults both of these first** — the official spec for
structure and behaviour, and the `material-web` token export for every numeric
value. If the two disagree, record the disagreement in `docs/porting-todo.md`
and say which one won and why; never silently pick one.

- Design spec: <https://m3.material.io/> (JS-rendered — use a browser, not
  `WebFetch`). Per-component pages live under
  <https://m3.material.io/components/<component>/overview> (plus `/specs` and
  `/accessibility`).
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
- `examples/` — the gallery app.
  - `examples/gallery/` — the shell and the pages. Example code, deliberately
    named `Gallery*` rather than `Md*` so it can never be mistaken for a
    library component or picked up by the coverage gate. Every page reads its
    colours, fonts, radii and durations from the theme at paint time; there is
    no QSS, no `QWidget::setPalette` and no `QWidget::setFont` anywhere in it.
  - One page per component plus an Overview home.
  - A page that owns real child widgets must keep them in a **bank** and only
    record their content-local rects in `build()`. `build()` is called from
    `measure()`, `remeasure()` and `paintEvent()` — many times, at changing
    widths — so allocating widgets inside it creates a new generation of visible
    children on every layout pass, all parked at (0, 0).
  - `GalleryPage::contentRectForCurrentSize()` / `buildOriginY()` / `remeasure()`
    give such a page the same numbers `paintEvent` uses, so its children can be
    placed from `resizeEvent()` and `showEvent()` rather than guessed at.
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

Version `0.1.0`. **Stage 1 Batch 0 (the foundation modules) is closed** — all
twenty-one modules are implemented, and `ContrastLevel` is applied rather than
stored, so reduced / standard / medium / high genuinely move the tones.

Stage 1 components have started: **`MdButton`, `MdButtonGroup`,
`MdIconButton`, `MdFab` and `MdExtendedFab` are complete** — Buttons (5 colour
styles × 5 Expressive sizes × 2 shapes, leading/trailing icons, soft-disabled,
live states), Button groups (standard / connected, five sizes, four selection
modes, round / square, horizontal / vertical, arrow-key navigation), Icon
buttons (4 styles × 5 sizes × 2 shapes × 3 padding tracks, toggle form with
the selected colour families and swapped selected corners), FABs (4 colour
sets × 3 sizes × lowered / raised, the one Actions family with a real shadow)
and extended FABs (6 colour sets × 3 sizes × lowered / raised, content-derived
width), which is `5 / 36` families. The next families are the rest of §1.2 —
FAB menu, split buttons, segmented buttons.

Before starting a component, read
[`docs/project-status.md`](docs/project-status.md) for the module inventory and
[`docs/porting-todo.md`](docs/porting-todo.md) for the open gaps with the
question attached to each.

### Base modules (all in `src/core/`, plus `MdStyleBase` in `src/styles/`)

`MdCore` · `MdTypes` · `MdTokens` · `MdColorMath` · `MdColorScheme` ·
`MdDynamicColor` · `MdTheme` · `MdTypeScale` · `MdShape` · `MdMotion` ·
`MdElevation` · `MdStateLayer` · `MdRipple` · `MdFocusRing` · `MdIcon` ·
`MdFont` · `MdDesign` · `MdStyleBase`

Two invariants worth knowing:

- Every enum in `MdTypes.h` ends with a `Count` sentinel. Keep it that way so
  callers can enumerate a set without hard-coding its size.
- Every published number is transcribed from an authoritative source and cited
  in a comment. If a value cannot be sourced, leave it out and record it in
  `porting-todo.md` — do not estimate it.

## Build

The build directory is `build/` and is gitignored. Build output is never
committed; only sources, resources and docs are.

```bash
cmake --preset windows-msvc-qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```

With the Qt 6 MinGW kit (this is what the reference checkouts use):

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe \
  -DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/mingw_64 \
  -DQT_MD3_BUILD_EXAMPLES=ON -DQT_MD3_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

`-G Ninja` needs `ninja` on `PATH` at configure time; `-DCMAKE_CXX_COMPILER`
removes the need to put the MinGW bin directory on `PATH` at all. A fresh tree
configured without one of those two fails with "unable to find a build program"
or "CMAKE_CXX_COMPILER not set", which looks alarming and means neither more
nor less than that.

The gallery binary lands at `build/examples/qt-md3-example.exe`. The build
copies the Qt modules, the C++ runtime, the `platforms/` + `styles/` plugins and
a `qt.conf` next to it (see `cmake/QtMd3Deploy.cmake`), so it runs by
double-click and needs no Qt-aware `PATH`. `tools/run-gallery.sh` is a
convenience wrapper around the same binary; it only touches `PATH` for builds
made with `-DQT_MD3_DEPLOY_EXAMPLE=OFF`.

```bash
tools/run-gallery.sh                        # open the gallery
tools/run-gallery.sh --theme dark --dynamic # other startup modes
tools/run-gallery.sh --screenshot build/shots   # render every page to PNG, then quit
```

`--screenshot` exits non-zero if any page fails to write, so it doubles as a
render check: a page whose `paintEvent` draws nothing still starts and quits
cleanly, and only the PNG proves otherwise. It writes two images per page:
`<nn>-<slug>.png` is the window, and `<nn>-<slug>-full.png` is the page widget
grown to its own `heightForWidth()`. Use the `-full` one — the window shot can
only ever show the first screenful, and most pages are taller than the viewport.

Note that a screenshot taken under `QT_QPA_PLATFORM=offscreen` renders every
glyph as a tofu box on Windows — that is the offscreen platform having no font
database, not a library fault. Capture from the native platform when the text
matters. In a GUI-subsystem build `qWarning`/`qInfo` also go to the debugger
rather than the terminal; set `QT_ASSUME_STDERR_HAS_CONSOLE=1` to see them.
