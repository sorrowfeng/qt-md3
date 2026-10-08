# Project Status

Last updated: 2026-10-08

## Snapshot

| Item | Value |
| --- | --- |
| Version | `0.1.0` (source of truth: [`VERSION`](../VERSION)) |
| Stage | Stage 1 — Batch 0 closed; §1.2 Actions closed; §1.3 Communication under way |
| Stage 1 families complete | `10 / 36` (§1.2 Actions × 8, Badges, Progress indicators) |
| Base modules | `21 / 21` |
| Public components | `12` (`MdButton`, `MdButtonGroup`, `MdIconButton`, `MdFab`, `MdExtendedFab`, `MdFabMenu`, `MdFabMenuItem`, `MdSplitButton`, `MdSegmentedButton`, `MdBadge`, `MdBadgedBox`, `MdProgressIndicator`) |
| Style classes | `11` (`MdStyleBase`, `MdButtonStyle`, `MdButtonGroupStyle`, `MdIconButtonStyle`, `MdFabStyle`, `MdExtendedFabStyle`, `MdFabMenuStyle`, `MdSplitButtonStyle`, `MdSegmentedButtonStyle`, `MdBadgeStyle`, `MdProgressIndicatorStyle`) |
| Example pages | `17` |
| Bundled icons | `49` classic SVGs + `4299` Material Symbols codepoints |
| Bundled fonts | `0` (opt-in; see [resources-manifest.md](resources-manifest.md)) |
| CTest entries | `23` (`TestMd3Version`, `TestMd3Tokens`, `TestMd3Contrast`, `TestMd3TemperatureCache`, `TestMd3Ripple`, `TestMd3FocusRing`, `TestMd3Icon`, `TestMd3StyleBase`, `TestMd3Button`, `TestMd3ButtonGroup`, `TestMd3IconButton`, `TestMd3Fab`, `TestMd3ExtendedFab`, `TestMd3FabMenu`, `TestMd3SplitButton`, `TestMd3SegmentedButton`, `TestMd3Badge`, `TestMd3ProgressIndicator`, `TestMd3NoQss`, `TestMd3CoveragePolicy`, `TestMd3SourceEncoding`, `TestMd3DeploymentTestBinary`, `TestMd3DeploymentExample`) |
| Supported Qt | Qt 6.5.0+ and Qt 5.15.2+ |

## What exists today

### Batch 0 — the foundation

Nothing in this list is a widget. It is the layer every component will be
measured against. All twenty-one modules are implemented and compiled in; the
outstanding behaviour gaps are listed separately below rather than glossed over.

| Module | Covers |
| --- | --- |
| `MdCore` | `md::libraryVersion()` and the generated version header |
| `MdTypes` | every shared enum, each with a `Count` sentinel, plus name helpers |
| `MdTokens` | three layers: `md.ref.*` (tones 0–100 for all six palettes), `md.sys.*` (colour, type, shape, elevation, motion, state), `md.comp.*` (global + per-instance override) |
| `MdColorMath` | CAM16 (J, C, h, M, s, Q, J\*, a\*, b\*), HCT with the 255 critical-plane solver, TonalPalette, CorePalette, CIELAB — ported from `material-color-utilities` |
| `MdColorScheme` | 49 colour roles, light and dark, including the fixed families; `baseline()` returns all six published sets (light/dark × standard/medium/high) verbatim |
| `MdColorSpec` | `ColorSpec2021`: the per-role tone solver — `ContrastCurve` (four corners at −1.0 / 0.0 / 0.5 / 1.0), `ToneDeltaPair`, the awkward-zone rule, dual backgrounds, and `MdContrast` (`ratioOfTones`, `lighter`/`darker`, `foregroundTone`) |
| `MdTemperatureCache` | Lab-temperature utilities: `rawTemperature`, `complement`, `analogous`. Feeds the `content` and `fidelity` tertiary palettes and nothing else |
| `MdDynamicColor` | all nine variants, `harmonize`, `hctHue`; every role resolved through the `ColorSpec2021` solver so all four contrast levels take effect |
| `MdTheme` | singleton with `themeAboutToChange` / `themeChanged` / `themeModeChanged` |
| `MdTypeScale` | 15 baseline + 15 emphasized styles, script-category line height |
| `MdShape` | the full corner scale, radius interpolation, path morphing |
| `MdMotion` | 10 easings, 16 durations, and the six Expressive spring slots |
| `MdElevation` | tonal surfaces 0–5; shadows opt-in only |
| `MdStateLayer` | hover 0.08 / focus 0.12 / pressed 0.12 / dragged 0.16, strongest-wins |
| `MdRipple` | press ripple geometry, timing and shape clipping; separate from the state layer by design |
| `MdFocusRing` | the 3dp indicator, 2dp gap, 8px grow-then-settle on an emphasized curve |
| `MdIcon` | Material Symbols four axes plus the classic SVG baseline |
| `MdResources` | keeps the embedded `.qrc` alive in the static archive (see the fixes section below) |
| `MdFont` | bundled font registration and global application |
| `MdDesign` | `configureHighDpi()` + `initialize(&app)` startup entry point |
| `MdStyleBase` | `QProxyStyle` base: paint filters, theme subscriptions, crisp rounded rects |

### Build system and gates

- CMake auto-detects Qt 6 / Qt 5, enforces the minimum, installs an exportable
  package (`find_package(qt-md3 CONFIG REQUIRED)`), and registers the `.qrc`.
- Sixteen CTest entries, all green. One binary per module, each with its own
  `QTEST_MAIN` — a single binary hosting several `QObject` classes driven by
  `QTest::qExec` in a loop cannot honour `-o` or per-class selection, so
  `ctest -R TestMd3Icon` would silently run the wrong suite.
  - `TestMd3Tokens` — 22 assertions pinning every published number against its
    source, including the CAM16 reference values from `material-color-utilities`
    and the 49-role light/dark scheme against the current `material-web` token
    sets.
  - `TestMd3Contrast` — the colour pipeline's two references, kept apart on
    purpose. `resources/tokens/md-sys-color-*.txt` (six sets, from material-web)
    pins the *static baseline*; `resources/tokens/mcu-scheme-expectations.txt`
    (367 assertions lifted from material-color-utilities' own Swift tests) pins
    the *dynamic solver* across nine variants, both modes and three contrast
    levels. A third check pins the fact that the two are *supposed* to differ,
    because "reconciling" them is the mistake this suite exists to catch.
  - `TestMd3TemperatureCache` — `rawTemperature`, `complement` and `analogous`
    against the exact values in material-color-utilities'
    `temperature_cache_test.cc`. This module feeds `content` and `fidelity`
    tertiary and nothing else, so a defect in it is invisible from every other
    variant; it had one, and this is what catches the next.
  - `TestMd3Ripple`, `TestMd3FocusRing`, `TestMd3Icon`, `TestMd3StyleBase` —
    the interaction primitives and the Pattern A paint hub: geometry, timing,
    lifecycle, signals, meta-property lookup, and a render smoke check for each
    so a style that silently paints nothing cannot pass.
  - `TestMd3NoQss`, `TestMd3CoveragePolicy`, `TestMd3SourceEncoding` — the three
    policy gates.
  - `TestMd3DeploymentTestBinary`, `TestMd3DeploymentExample` — run a unit-test
    binary and the example with `PATH` reduced to the Windows directories and
    `QT_PLUGIN_PATH` / `QT_QPA_PLATFORM` cleared, which is the environment a
    double-click produces. They fail if the Qt runtime deployment is incomplete,
    the one failure mode no other test can see.
  - `TestMd3Version` — the library links, loads and reports its version.
- The example is an eight-page gallery under `examples/gallery/`, built into
  `build/` and never committed. `--screenshot <dir>` renders every page to PNG
  and quits, which is how the pages are checked for a non-blank result: a page
  whose `paintEvent` draws nothing still exits 0, so "the window opened" proves
  nothing. It writes two images per page — the window, and the page widget
  grown to its own `heightForWidth()` — because the window shot can only ever
  prove that the *top* of a page draws.

### Stage 1 §1.2 Actions (all eight families) + §1.3 Badges and Progress indicators

| Component | Covers |
| --- | --- |
| `MdButton` | `QPushButton` subclass, 5 colour styles (elevated / filled / tonal / outlined / text) × 5 Expressive sizes (xsmall 32 → xlarge 136 px) × 2 container shapes, leading / trailing icons, mnemonic text, soft-disabled. Six `Q_PROPERTY`s with NOTIFY. |
| `MdButtonTokens` | the `md.comp.button.*` value layer: size-driven metrics, both shape slots, the pressed-shape spring, and the per-variant, per-state colour slots including the disabled opacities. Read from `tokens/versions/latest/sass` (34.0.21) — see [porting-todo.md](porting-todo.md) for why this one component uses a different export than `MdTokens`. |
| `MdButtonStyle` | Pattern A style: layout and painting, registered in the paint hub so one style serves the whole family. Also owns `focusRingInset()`, which derives the 7.5 px focus margin from the focus-indicator tokens instead of hard-coding it. |
| `MdButtonGroup` | `QWidget` container for 2–n `MdButton` items — standard (press grows an item 15 % on `spring-fast-spatial` and shifts the neighbours) and connected (2 px gap, per-side corners) variants, five Expressive sizes, four selection modes (none / single / multiple / required), round / square shapes, horizontal / vertical orientation, arrow-key navigation with focus-follows-current. Eight `Q_PROPERTY`s with NOTIFY. |
| `MdButtonGroupTokens` | the `md.comp.button-group.{standard,connected}.<size>` value layer: heights, between-space, the 15 % press multiplier, the spring constants, the connected corner ladder and the literal 50 % selected inner corner. |
| `MdButtonGroupStyle` | a style with a deliberately *empty* `drawWidget()` — the spec calls the group an invisible container with no colour attributes, so it owns geometry (`layoutFor`), per-item corner radii (`itemRadii`) and margins only. |
| `MdIconButton` | `QPushButton` subclass, 4 colour styles (standard / filled / tonal / outlined) × 5 Expressive sizes × 2 shapes × 3 published padding tracks, icon-only by token arithmetic, toggle form backed by Qt's checkable state with the `selected-*` / `unselected-*` colour families and the swapped selected corner shapes. Nine `Q_PROPERTY`s with NOTIFY. |
| `MdIconButtonTokens` | the `md.comp.icon-button.*` value layer: three colour families (plain / selected / unselected) over five states, the five-size metric table, the five shape slots and the press spring. |
| `MdFab` | `QPushButton` subclass, 4 colour sets (surface / primary / secondary / tertiary) × 3 sizes (small 40 / medium 56 / large 96 px, corners 12 / 16 / 28 px) × lowered / raised elevation rows, icon-only by token arithmetic, the one Actions family with a real per-state shadow. Six `Q_PROPERTY`s with NOTIFY. |
| `MdFabTokens` | the `md.comp.fab.*` value layer: the three-size metric table, the four colour sets (flat interactive rows), the lowered elevation rows and the spec-filled disabled row — the export publishes none. |
| `MdExtendedFab` | `QPushButton` subclass, 6 colour sets (primary / secondary / tertiary + three `*-container`, no surface) × 3 sizes (small 56 / medium 80 / large 96 px, corners 16 / 20 / 28 px, icons 24 / 28 / 36) × lowered / raised elevation rows, icon + label on a content-derived width (leading + icon + gap + label + trailing, not a token). Six `Q_PROPERTY`s with NOTIFY. |
| `MdExtendedFabTokens` | the `md.comp.extended-fab.*` value layer: the three-size metric + type-scale table, the six colour sets (flat interactive rows reusing the FAB family structs), the lowered elevation rows and the spec-filled disabled row. |
| `MdFabMenu` | `QWidget` container: an anchor `MdFab`, a 56 px close button sharing its top trailing corner, and up to six list items staggering in top-down on the Compose `SpatialDefault` spring (40 ms stagger) — the export publishes no motion rows. `Q_PROPERTY`s for variant / expanded / anchor icon, `itemActivated(int, QString)`. |
| `MdFabMenuItem` | `QPushButton` subclass for both element shapes (close button / list item), each with its own published token rows; `reveal` property drives the staggered open animation (opacity + 24 px settle). |
| `MdFabMenuTokens` | the `md.comp.fab-menu.*` value layer: the common spacing rows (8 / 4), both elements' metric + colour + elevation tables across the three colour groups, and the spec-filled disabled row. |
| `MdSplitButton` | one `QWidget` with two press targets: a leading half (icon + label) and a trailing half (the dropdown icon), separated by the published 2 px gap. The facing corners morph on hover / press (4/4/4/8/12 -> 8/12/12/20/20 px per size) and go to the literal 50% while the trailing half is selected; outer corners stay a full pill. Left/Right walk the halves, Up/Down are swallowed, Space/Enter activate the focused half. |
| `MdSplitButtonTokens` | the `md.comp.split-button.<size>.*` metric layer (five rows verbatim, inner corners resolved from the shape scale) plus the button family's colour / type / focus rows at the identity size mapping — the spec page: "the same color schemes as standard buttons". |
| `MdSegmentedButton` | one `QWidget` with N press targets: segments share one 40 px pill outline, neighbours overlap by exactly the outline width so the shared edge is the divider, `itemShape` rounds only the row's ends, and the check scales in on a reserved 18 px icon slot (spring-fast-spatial) so the label never moves. Single-choice (radio) by default, multi-choice per segment. |
| `MdSegmentedButtonTokens` | the single `md.comp.outlined-segmented-button.*` set verbatim (no size scale, no colour variants) with the Compose-sourced rows the export lacks (8 px icon spacing, 12 px content padding, spring-fast-spatial check motion) — every borrow labelled in the header. |
| `MdBadge` | `QWidget` (non-interactive by contract: `Qt::NoFocus`, transparent for mouse events), one token set with two forms decided by content — the 6 px dot and the minimum-16 px pill (label-small on-error text, 4 px Compose side padding). Empty accessible name falls back to "badge". One `Q_PROPERTY` (`text`) with NOTIFY. |
| `MdBadgedBox` | the anchor container: content widget plus one `MdBadge` positioned with the Compose offsets (dot 6/6, pill 12/14); reserves the pill overhang (width − 12 / height − 14) in its own geometry because a Qt child is clipped to its parent — the one deliberate divergence from Compose's free overlap, recorded in the header. |
| `MdBadgeTokens` | the `md.comp.badge.*` value layer verbatim (dot 6, large 16, both corner-full, error / on-error, label-small); no state rows exist in the export — non-interactivity is a published fact, not a gap. |
| `MdBadgeStyle` | Pattern A style: the quietest painter in the library — a corner-full container and, in the content form, one label-small text. No state layer, no ripple, no focus indicator. |
| `MdProgressIndicator` | `QWidget` (non-interactive by contract), one merged family with two shapes: linear (determinate with the 4 px gap + stop indicator, buffer with the scaled track and scrolling 2 px dots, the MDC 2 s two-bar indeterminate) and circular (determinate arc from 12 o'clock, the three-composed-rotation indeterminate). Determinate value changes transition over the published 250/500 ms curves; `fourColor` rides the deprecated four-color sets. Six `Q_PROPERTY`s with NOTIFY. |
| `MdProgressIndicatorTokens` | the merged `md.comp.progress-indicator.*` value layer verbatim — base + linear + circular rows, the deprecated base metrics, the deprecated `thick.*` rows (transcribed, never exposed as an API) and the four-color roles from the deprecated per-shape sets. The Expressive wave rows are carried but not rendered (the family's registered gap). |
| `MdProgressIndicatorStyle` | Pattern A style plus the indeterminate math as *pure functions of elapsed time* (`linearIndeterminateFrame`, `circularIndeterminateFrame`, `fourColorAt`) so the tests pin the keyframes field by field without an event loop. Owns a hand-rolled cubic-bezier solver — `QEasingCurve` has no public per-t evaluation. |

## Known gaps, recorded rather than hidden

1. **35 Expressive decorative shape paths are not ported.**
   `MdShape::decorativeShapeCount()` returns `0`. The shape *scale* and shape
   *morphing* are complete; the decorative path library is not.
2. **`ScriptCategory::Large` and `ExtraLarge` line-height multipliers.**
   Only `Small` (1.00) and `Medium` (1.07) are sourced. `Large` and
   `ExtraLarge` currently fall back to `Medium`'s 1.07.
3. **Variable font axes need Qt 6.7+.**
   On Qt 6.5/6.6 `MdIcon::axesSupported()` returns false and only the named
   weights are honoured.
4. **The progress-indicator wave rows are carried but not rendered.**
   The merged export publishes non-deprecated Expressive wave tokens
   (amplitude/wavelength, with-wave container sizes); neither material-web
   nor this port renders them. Compose's `WavyLinearProgressIndicator` /
   `WavyCircularProgressIndicator` are the porting source; see
   [porting-todo.md](porting-todo.md).

`ContrastLevel` used to head this list. It is closed: `MdColorSpec` solves every
role against its own `ContrastCurve` and `ToneDeltaPair`, and all four levels
(reduced / standard / medium / high) take effect. See
[porting-todo.md](porting-todo.md) for what that required, including the
distinction between the published static baseline and the dynamic solver that
had to be drawn before the numbers could be verified at all.

Everything above is in [`porting-todo.md`](porting-todo.md) with its source
question attached.

## Fixed while closing Batch 0

Three defects were found by actually running the result rather than by reading
it. They are recorded because each one was invisible to the compiler and to
review.

1. **The embedded resources were never linked in.**
   CMake's AUTORCC put `qrc_qt-md3.cpp` inside `libqt-md3.a`, but nothing
   referenced its initialiser, so the linker dropped the object and every
   `:/qt-md3/...` lookup failed — the icon codepoint table, the classic SVG
   baseline and the font directory. `MdResources::ensure()` is the reference
   that keeps it; `MdIcon` and `MdFont` call it from the accessors that read a
   resource. This affected any application linking the static library, not just
   the tests.
2. **`MdRippleController::currentFrame()` reported a painted ripple while idle.**
   A caller painting on `frame.valid` rather than on `isActive()` got a phantom
   blot in the top-left corner before the first press. Idle now yields an
   invalid frame, and `MdRipple::geometryFor()` refuses a non-positive extent
   instead of clamping a negative one to zero.
3. **A UTF-8 em dash was decoded as Latin-1 in the gallery**, rendering as
   mojibake. Fixed at the call site and gated: `TestMd3SourceEncoding` now
   rejects the Latin-1 decoders outright and validates that every source file is
   well-formed UTF-8.

## Fixed while closing the contrast levels

The `ContrastLevel` work surfaced a defect that had been sitting under two other
symptoms, and it is worth recording because of how quiet it was.

4. **`MdTemperatureCache` read the three Lab components out of order.**
   `MdTonalPalette::labFromArgb` returns `{L*, a*, b*}` — but through a generic
   `MdVec3{a, b, c}`. `rawTemperature` then read `.a` where a\* was meant (it got
   L\*) and `.b` where b\* was meant (it got a\*). Every number still looked like
   a plausible temperature and stayed inside its documented range; the whole
   warm/cool axis was simply rotated by about a quadrant. The only visible
   symptom was that the `content` and `fidelity` variants produced strange
   tertiary hues — 24 of material-color-utilities' 367 scheme assertions failed,
   all of them on `tertiary` / `tertiary-container` for those two variants and
   nothing else, which is exactly the fingerprint of this module.

   Two things came out of it. `MdVec3` is fine as a generic triple but not as a
   colour-space result, so `labFromArgb` now returns a named `MdLab{l, a, b}`
   and the mistake cannot be repeated. And the module moved out of an anonymous
   namespace inside `MdDynamicColor.cpp` into `MdTemperatureCache`, with
   `TestMd3TemperatureCache` pinning it against the exact values in
   `temperature_cache_test.cc`. It had no test before, which is why a fully
   rotated colour-temperature axis went unnoticed.

   Worth noting how it was found: not by reading the code, but by comparing two
   upstream references against each other. The MCU fixture said the solver was
   wrong for two variants; the published token sets said the baseline was a
   different thing entirely. Working out which of those was the real signal is
   what narrowed it to this module.

## Fixed while building the first component

The button page is the first gallery page with real child widgets, and that is
what found these. None of them is button-specific; all three were sitting in the
gallery scaffolding, unreachable from the seven token-only pages.

5. **`build()` allocated its widgets.** `build()` is called from `measure()`,
   `remeasure()` and `paintEvent()` — many times, at changing widths, because
   that is how one code path serves both measurement and painting. A page that
   created its children inside it therefore produced a fresh generation on every
   layout pass, each parked at (0, 0) and each visible, while the layout code
   only ever moved the newest one. The screen showed buttons at the right
   positions *and* several dozen more underneath them. Component pages now keep
   a widget bank and reuse the same widgets across every rebuild.
6. **`QScrollArea` never gave a page its height.** `GalleryPage` overrode
   `hasHeightForWidth()` — but `QScrollArea` asks
   `sizePolicy().hasHeightForWidth()`, not the virtual, so every page was
   pinned to exactly one viewport tall and no page ever scrolled. It looked
   correct, because the first screenful always renders; the only symptom was
   that the bottom of a long page could not be reached. Fixed by declaring
   height-for-width in the size policy, where the scroll area can see it.
7. **`measure()` and `paintEvent()` disagreed about where the body starts.**
   `paintEvent` hands `build()` its first coordinate at `buildOriginY()`,
   *below* the title and subtitle; `measure()` handed it the bare gutter.
   Measurement also returned one gutter where the value wanted is a *widget*
   height and needs two. Every page therefore reported itself one title-block
   plus 32 px too short, and `paintEvent`'s clip rect then cut the last line of
   text in half. Both now use one source, which is what the file's own "same
   code path" claim requires.

## What is next

Stage 1 §1.3 Communication, in official order — Badges and Progress
indicators are complete:

**Loading indicator, snackbars, tooltips.** Then on to §1.4 Containment.

Each component lands as its own commit and must satisfy all twelve Definition of
Done items before the next one starts, including an independent gallery page, a
side-by-side visual audit in [visual-audit.md](visual-audit.md), and the
corresponding row in [md3-coverage.md](md3-coverage.md). Buttons is the first
row with cells still in progress: `主题` (seed / contrast / density / font
switch not yet exercised against the page) and `视觉审计` (rendered and read in
both modes, reference comparison not yet recorded).

See [`md3-qt-porting-prompts.md`](md3-qt-porting-prompts.md) for the full brief,
the component order, and the Definition of Done.
