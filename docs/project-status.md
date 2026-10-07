# Project Status

Last updated: 2026-10-07

## Snapshot

| Item | Value |
| --- | --- |
| Version | `0.1.0` (source of truth: [`VERSION`](../VERSION)) |
| Stage | Bootstrap — repository scaffolding only |
| Stage 1 families complete | `0 / 36` |
| Public components | `0` |
| Style classes | `0` |
| Example pages | `0` (shell only) |
| Bundled icons | `0` |
| Bundled fonts | `0` |
| CTest entries | 3 (`TestMd3Version`, `TestMd3NoQss`, `TestMd3CoveragePolicy`) |
| Supported Qt | Qt 6.5.0+ and Qt 5.15.2+ |

## What exists today

- CMake build system that auto-detects Qt 6 / Qt 5, enforces the minimum version,
  installs an exportable package (`find_package(qt-md3 CONFIG REQUIRED)`), and
  optionally runs `windeployqt` for the example on Windows.
- `core/QtMd3Export.h` (`QT_MD3_EXPORT`) and CMake-generated
  `core/QtMd3Version.h`, plus a minimal `md::libraryVersion()` API.
- The example shell (`qt-md3-example`) that links the library and opens an empty
  gallery window.
- Two policy gates and one smoke test wired into CTest.
- The coverage matrix ([md3-coverage.md](md3-coverage.md)) with all 36 families
  tracked as not started.

## What is next

The next batch is the **Stage 1 base modules** (no UI components yet). They define
the ceiling for everything that follows:

1. `MdTokens` — the three-layer `md.ref.*` / `md.sys.*` / `md.comp.*` token system
2. `MdColorScheme` — full color-role enum + light / dark schemes
3. `MdDynamicColor` — port of `material-color-utilities` (HCT / CAM16, tonal
   palettes, dynamic schemes, contrast / blend / quantize)
4. `MdTheme` — singleton with `themeMode`, `seedColor`, `contrastLevel`,
   `density`, `direction`, and lifecycle signals
5. `MdTypeScale` — 30 type styles with script-height-aware line height
6. `MdShape` — shape scale + 35 Expressive decorative shapes + shape morph
7. `MdMotion` — easing / duration tokens + Expressive spring solver
8. `MdStateLayer`, `MdRipple`, `MdElevation`, `MdFocusRing`
9. `MdIcon` — Material Symbols variable font + classic SVG baseline
10. `MdStyleBase` — `QProxyStyle` base with the paint-filter / theme-update hooks
11. `MdFont` + `MdDesign::initialize(&app)`
12. Resource drop under `resources/` with `.qrc` and a manifest
13. Base-module tests, including token values compared against the `material-web`
    token sources

See [`docs/md3-qt-porting-prompts.md`](md3-qt-porting-prompts.md) for the full
bootstrap brief, the component order, and the Definition of Done.
