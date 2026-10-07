# Contributing

Thanks for helping improve `qt-md3`. The project reproduces Google's Material
Design 3 (including M3 Expressive) as native Qt Widgets, hand-painted with
`QPainter` — **no QSS, no `QStyleSheet`**, ever.

## Before you start: which stage are we in?

The project is delivered in two strictly serial stages:

- **Stage 1 — MD3 official components.** All 36 component families from
  <https://m3.material.io/components>, with *every* variant, size, shape, color
  style, slot, state, token, motion spec, and keyboard behavior.
- **Stage 2 — Qt desktop extensions.** Common Qt controls that MD3 has no spec
  for, redesigned with Stage 1 tokens and semantics.

The single source of truth for stage switching is
[docs/md3-coverage.md](docs/md3-coverage.md). **Stage 2 work must not start until
every Stage 1 family is green across all nine columns.** A CTest gate
(`TestMd3CoveragePolicy`) fails the build if a Stage 2 public header appears
early.

## Development flow

1. Work from `dev` for normal changes. `main` holds releases.
2. Keep changes scoped to the component, style, example page, test, and docs
   affected by the issue.
3. Follow the existing `src/core`, `src/styles`, `src/widgets`, `examples`
   layering.
4. Prefer **Pattern A** (custom `QWidget` + `Md<Component>Style` inheriting
   `MdStyleBase`) for new components, unless Pattern B (`QProxyStyle`
   `drawControl` / `drawComplexControl`) or Pattern C (self-painting container)
   is clearly the right fit.
5. Every visual number (color, radius, font size, spacing, duration) must come
   from a theme token. No magic values in components.

## Build

Use a CMake preset that matches your environment:

```powershell
cmake --preset windows-msvc-qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```

For Qt5 on Windows, point `QT5_CMAKE_PREFIX_PATH` at the Qt 5 `lib/cmake`
directory when CMake would otherwise pick up Qt6:

```powershell
$env:QT5_CMAKE_PREFIX_PATH = "C:/Qt/5.15.2/msvc2019_64/lib/cmake"
cmake --preset windows-msvc-qt5-debug
cmake --build --preset qt5-debug
ctest --preset qt5-debug
```

Minimum supported versions: **Qt 6.5.0** or **Qt 5.15.2**. CMake enforces this
at configure time and in the installed package's consumer check.

## Tests

Run only the targeted tests for what you changed; do not default to a full run.

```powershell
cmake --build build --config Debug --target qt-md3-example
ctest --test-dir build -C Debug -R "TestMd3Version|TestMd3CoveragePolicy" --output-on-failure
```

Two policy gates run on every change:

- `TestMd3NoQss` — scans `src/`, `examples/`, `tests/`, `resources/` and fails if
  QSS is used to style any component.
- `TestMd3CoveragePolicy` — validates [docs/md3-coverage.md](docs/md3-coverage.md)
  and blocks premature Stage 2 components.

## Component Definition of Done

A component is only "done" when all of these hold:

1. `src/widgets/Md<Component>.h/.cpp` + `src/styles/Md<Component>Style.h/.cpp`,
   explicitly listed in CMake.
2. Public API covers **every** variant / size / shape / state / slot from the
   official docs, named 1:1 with the official semantics.
3. Every configurable item is a `Q_PROPERTY` with a `NOTIFY` signal.
4. States covered: enabled / hovered / focused / pressed / disabled / selected /
   dragged / loading (as applicable).
5. Themes covered: light / dark, seed-color change, contrast level, density,
   RTL, font switch.
6. Accessibility: Tab-reachable, visible focus ring, Space/Enter/Esc/arrow
   behavior matching the official spec.
7. Example app: a dedicated page + left-nav entry, with **zero** style
   operations (`QPalette` / `setFont` / `setStyleSheet` are forbidden in pages).
8. Tests: properties & signals, lifecycle & object tree, meta-property read /
   write, theme lifecycle, and a non-blank render smoke check.
9. Visual audit: official screenshot vs Qt screenshot side-by-side across
   light/dark and default/hover/focus/pressed/disabled, recorded in
   [docs/visual-audit.md](docs/visual-audit.md).
10. `docs/md3-coverage.md` updated: the row's nine columns for that family.
11. Docs updated: `AGENTS.md`, `README.md`, `docs/project-status.md`.
12. A single dedicated commit, message format `feat(<component>): <description>`.

## Visual audit

Capture the official page first, then the Qt page; attribute every difference to
the specific component (never tweak across components). Reference screenshots go
under `build/ref/` and `build/qt/` and are never committed.

## Pull requests

- Describe the user-visible behavior change.
- Mention Qt5 / Qt6 coverage when relevant.
- Include screenshots for visual changes.
- Keep generated build output out of `main` and `dev`.
- Update `docs/md3-coverage.md` when a family's columns change.
