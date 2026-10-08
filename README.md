# qt-md3

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6%20%7C%205-green.svg)](https://www.qt.io)
[![CMake](https://img.shields.io/badge/CMake-3.16+-blue.svg)](https://cmake.org)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com)
[![Release](https://img.shields.io/github/v/release/sorrowfeng/qt-md3)](https://github.com/sorrowfeng/qt-md3/releases)
[![GitHub Stars](https://img.shields.io/github/stars/sorrowfeng/qt-md3?style=social)](https://github.com/sorrowfeng/qt-md3/stargazers)
[![Last Commit](https://img.shields.io/github/last-commit/sorrowfeng/qt-md3)](https://github.com/sorrowfeng/qt-md3/commits/main)

English | [简体中文](README.zh-CN.md)

`qt-md3` is a C++ component library that reproduces **Material Design 3**
(including **M3 Expressive**) as native **Qt Widgets**. Everything is
hand-painted with `QPainter` / `QProxyStyle` — no QSS, no `QStyleSheet`.

The goal is not a loose "Material-flavoured" look. Token values, component
behavior, states, and motion are aligned item by item with the official design
system:

- Design system: <https://m3.material.io/>
- Token truth (numeric values are taken from here, never from memory):
  <https://github.com/material-components/material-web/tree/main/tokens>
- Color algorithms: <https://github.com/material-foundation/material-color-utilities>

## Status

> **This repository is in the bootstrap phase.** Version `0.1.0`. The build
> system, the token layer, the interaction primitives, an example gallery and the
> policy gates exist, and the first two components have landed.

| Metric | Value |
| --- | --- |
| Stage 1 MD3 families complete | `9 / 36` — **all of §1.2 Actions**, plus Badges (the first §1.3 Communication family) |
| Foundation modules | `21` |
| Public components | `11` (`MdButton`, `MdButtonGroup`, `MdIconButton`, `MdFab`, `MdExtendedFab`, `MdFabMenu`, `MdFabMenuItem`, `MdSplitButton`, `MdSegmentedButton`, `MdBadge`, `MdBadgedBox`) |
| CTest entries | `22` (all green) |
| Stage 2 (Qt extensions) | Not started — gated until Stage 1 is green |

Progress is tracked in [docs/md3-coverage.md](docs/md3-coverage.md), the single
source of truth for the Stage 1 → Stage 2 switch. Current state:
[docs/project-status.md](docs/project-status.md).

## Two stages, strictly serial

1. **Stage 1 — MD3 official components.** All **36** component families from
   <https://m3.material.io/components>, with every property, variant, size,
   shape, state, token, motion spec, and keyboard behavior.
2. **Stage 2 — Qt desktop extensions.** Common Qt controls that MD3 has no spec
   for, redesigned with Stage 1 tokens and semantics.

Stage 2 does not start until every Stage 1 family is green across all nine
columns. A CTest gate (`TestMd3CoveragePolicy`) fails the build if a Stage 2
public component appears early.

<details>
<summary>The 36 Stage 1 families</summary>

Buttons · Button groups · Icon buttons · FABs · Extended FABs · FAB menu ·
Split buttons · Segmented buttons · Badges · Progress indicators · Loading
indicator · Snackbar · Tooltips · Cards · Dialogs · Bottom sheets · Side sheets ·
Carousel · Divider · Lists · App bars · Toolbars · Navigation bar · Navigation
rail · Navigation drawer · Tabs · Checkbox · Chips · Date pickers · Menus ·
Radio button · Sliders · Switch · Time pickers · Text fields · Search

</details>

## Design principles

- **Tokens over literals.** Every color, radius, size, spacing, and duration comes
  from the theme. No magic values inside components.
- **Tonal elevation, not shadow.** Layering is expressed with tonal surfaces;
  shadows appear only where the spec explicitly requires them.
- **State layer ≠ ripple.** Hover and focus are static overlays; ripple only
  spreads on press and clips to the current shape.
- **Shape morph.** Radii come from shape tokens and tween at runtime.
- **Dynamic color.** Light and dark are tone mappings of one `DynamicScheme`;
  changing the seed color regenerates every role.
- **Script-aware typography.** Line height adapts to the language script height
  class (Chinese is taller than Latin).
- **Expressive motion.** Spring physics coexists with classic easing curves.
- **No QSS.** Painting is `QPainter` only, enforced by a gate.

## Requirements

- Qt **6.5.0+** or Qt **5.15.2+** (`Core`, `Widgets`, `Svg`; `Test` for tests).
- CMake **3.16+**.
- A C++17 compiler.

## Build

```bash
cmake --preset windows-msvc-qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```

Or configure manually:

```bash
cmake -S . -B build -DQT_MD3_BUILD_EXAMPLES=ON -DQT_MD3_BUILD_TESTS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

The library is static by default; pass `-DBUILD_SHARED_LIBS=ON` for a shared
build. On Windows every executable gets the Qt runtime, the C++ runtime and the
`platforms/` + `styles/` plugins copied next to it, so the example in `build/`
runs by double-click without a Qt-aware `PATH`. `cmake --install` additionally
uses `windeployqt`. Pass `-DQT_MD3_DEPLOY_EXAMPLE=OFF` to skip both.

## Install and consume

```bash
cmake --install build --config Release
```

```cmake
find_package(qt-md3 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qt-md3::qt-md3)
```

The installed package re-checks the minimum Qt version on the consumer side.

## Project layout

```text
src/core/       tokens, theme, color algorithms, type scale, shape, motion, icons
src/styles/     Md*Style painting classes
src/widgets/    public components (Md*.h)
src/private/    internal helpers (not exported)
examples/       the component gallery app
tests/          QTest + CTest, including the policy gates
resources/      icons, fonts, .qrc
docs/           status, audit, specs, TODOs
```

## Policy gates

| Gate | Enforces |
| --- | --- |
| `TestMd3NoQss` | No QSS anywhere in `src/`, `examples/`, `tests/`, `resources/` |
| `TestMd3CoveragePolicy` | All 36 families tracked; no Stage 2 components before Stage 1 is green |
| TestMd3SourceEncoding | Every source file is valid UTF-8; no UTF-8 literal is decoded as Latin-1 |
| `TestMd3Deployment*` | Every executable starts with no Qt on `PATH` — the double-click case |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development flow, the component
Definition of Done, and the build/test commands. Please also read
[AGENTS.md](AGENTS.md) and the [Code of Conduct](CODE_OF_CONDUCT.md).

## License

[MIT](LICENSE). Ported or bundled third-party assets (Material Design tokens,
Material Color Utilities, Material Symbols, Roboto) keep their own licenses — see
[NOTICE.md](NOTICE.md). This is an independent community project, not affiliated
with or endorsed by Google LLC or The Qt Company Ltd.
