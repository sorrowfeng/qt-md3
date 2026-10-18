# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] — 2026-10-18

First public release. Material Design 3 (including M3 Expressive) as native
Qt Widgets, hand-painted with `QPainter` / `QProxyStyle` — no QSS.

### Added

#### Stage 1 — MD3 official components (36 families)

All component families from <https://m3.material.io/components>, ported with
token-true values, variants, sizes, shapes, states, motion specs and keyboard
behavior:

- **Actions (§1.2)** — Buttons, Button groups, Icon buttons, FABs, Extended
  FABs, FAB menu, Split buttons, Segmented buttons.
- **Communication (§1.3)** — Badges, Progress indicators, Loading indicator,
  Snackbar, Tooltips.
- **Containment (§1.4)** — Cards, Dialogs, Bottom sheets, Side sheets,
  Carousel, Divider, Lists.
- **Navigation (§1.5)** — App bars, Toolbars, Navigation bar, Navigation rail,
  Navigation drawer, Tabs.
- **Selection (§1.6)** — Checkbox, Chips, Date pickers, Menus, Radio button,
  Sliders, Switch, Time pickers.
- **Text inputs (§1.7)** — Text fields, Search.

Each family ships `src/widgets/Md*.h` + `src/styles/Md*Style`, a gallery page,
a dedicated CTest binary, and a visual-audit entry.

#### Stage 2 — Qt desktop extensions (55 public headers)

Common Qt controls that MD3 has no spec for, redesigned with Stage 1 tokens and
semantics (appendix D closed):

`MdScrollBar`, `MdSelect`, `MdMessageBox`, `MdScrollArea`, `MdTable`,
`MdAvatar`, `MdWindow`, `MdFileDialog`, `MdInputNumber`, `MdScaffold`,
`MdMenuBar`, `MdStatusBar`, `MdSplitter`, `MdPagination`, `MdEmptyState`,
`MdSkeleton`, `MdSteps` / `MdStepper`, `MdUpload`, `MdRate`, `MdFlex`,
`MdGrid`, `MdSpace`, `MdCollapse`, `MdNotification`, `MdTree` /
`MdTreeSelect`, `MdStatistic`, `MdPopover` / `MdPopconfirm`, `MdAutoComplete`,
`MdColorPicker`, `MdDockWidget` / `MdDockManager`, `MdDescriptions`,
`MdTimeline`, `MdImage`, `MdWatermark`, `MdTransfer`, `MdMentions`,
`MdBackTop`, `MdInputDialog`, `MdStackedWidget`, `MdResult`, `MdBreadcrumb`,
`MdQRCode`, `MdLog`, `MdCascader`, `MdPlainTextEdit`, `MdMasonry`,
`MdDrawer`, `MdTour` / `MdCoachMark`, `MdRibbon`, `MdNav` / `MdNavItem`,
`MdAffix` / `MdAnchor`.

#### Foundation (Batch 0)

- Twenty-one core modules: `MdCore`, `MdTypes`, `MdTokens`, `MdColorMath`,
  `MdColorScheme`, `MdDynamicColor`, `MdTheme`, `MdTypeScale`, `MdShape`,
  `MdMotion`, `MdElevation`, `MdStateLayer`, `MdRipple`, `MdFocusRing`,
  `MdIcon`, `MdFont`, `MdDesign`, `MdStyleBase`, and the supporting colour /
  temperature layers.
- **Dynamic colour** — the full `material-color-utilities` pipeline (CAM16 /
  HCT, 255 critical-plane solver, `ColorSpec2021` tone solver, nine scheme
  variants, four contrast levels). Light and dark are tone mappings of one
  `DynamicScheme`; changing the seed regenerates every role. Six published
  baseline schemes are available verbatim.
- **M3 Expressive springs** — six spring slots alongside the ten classic
  easings; shape morph and press springs run on spring physics, not fixed
  duration curves.
- **Material Symbols icons** — Outlined / Rounded / Sharp variable fonts
  (FILL / wght / GRAD / opsz) register through a generated qrc when present;
  otherwise the 49-icon classic Material Icons SVG set is the fallback. The
  TTF binaries themselves stay out of git (size policy) — drop them into
  `resources/fonts/` or run `tools/update-icons.sh --fonts`.
- **RTL** — twelve families mirrored (buttons through toolbars and the
  showcase chrome); remaining gaps are recorded in
  [docs/porting-todo.md](docs/porting-todo.md).
- **QR encoder** — `MdQRCode` with a real encoder (not a bitmap stub).

#### Build, gates, gallery

- CMake auto-detects Qt 6 (min 6.5.0) / Qt 5 (min 5.15.2), installs an
  exportable package (`find_package(qt-md3 CONFIG REQUIRED)`), and deploys
  the Qt runtime next to Windows executables.
- Policy gates: `TestMd3NoQss` (no QSS anywhere), `TestMd3CoveragePolicy`
  (blocks premature Stage 2), `TestMd3SourceEncoding` (UTF-8 sources).
- Deployment gates: the example and test binaries start with no Qt on `PATH`.
- 94 CTest entries, all green, covering properties & signals, lifecycle,
  theme lifecycle, meta-properties, render smoke, and the policy gates.
- Gallery app (`build/examples/qt-md3-example`): 55 pages — foundation
  modules, one page per Stage 1 family, Stage 2 pages 48–55, dynamic colour,
  and three product showcase screens (Mail / Music / Settings).
  `--screenshot <dir>` renders every page to PNG and doubles as a render
  check. `--theme light|dark`, `--dynamic`, `--language <tag>`.
- Qt 5 / Qt 6 portability via `MdQtCompat` and the nine rules in
  [CONTRIBUTING.md](CONTRIBUTING.md). CI matrix: Linux Qt 5.15 ASan/UBSan,
  Linux Qt 5.15 shared Release, Windows Qt 5.15, Windows Qt 6.7, macOS Qt 6.

### Known gaps

Recorded in [docs/porting-todo.md](docs/porting-todo.md) rather than glossed
over: a library-wide RTL audit beyond the twelve mirrored families, the
remaining visual-QA passes against official captures, and `MdTextArea` as a
separate multiline widget.

## [Unreleased]

[0.1.0]: https://github.com/sorrowfeng/qt-md3/releases/tag/v0.1.0
[Unreleased]: https://github.com/sorrowfeng/qt-md3/compare/v0.1.0...HEAD
