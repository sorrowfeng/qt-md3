# Project Status

Last updated: 2026-10-07

## Snapshot

| Item | Value |
| --- | --- |
| Version | `0.1.0` (source of truth: [`VERSION`](../VERSION)) |
| Stage | Stage 1 — Batch 0 (foundation modules) present, colour pipeline complete |
| Stage 1 families complete | `0 / 36` |
| Base modules | `21 / 21` |
| Public components | `0` |
| Style classes | `1` (`MdStyleBase`) |
| Example pages | `7` |
| Bundled icons | `49` classic SVGs + `4299` Material Symbols codepoints |
| Bundled fonts | `0` (opt-in; see [resources-manifest.md](resources-manifest.md)) |
| CTest entries | `13` (`TestMd3Version`, `TestMd3Tokens`, `TestMd3Contrast`, `TestMd3TemperatureCache`, `TestMd3Ripple`, `TestMd3FocusRing`, `TestMd3Icon`, `TestMd3StyleBase`, `TestMd3NoQss`, `TestMd3CoveragePolicy`, `TestMd3SourceEncoding`, `TestMd3DeploymentTestBinary`, `TestMd3DeploymentExample`) |
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
- Thirteen CTest entries, all green. One binary per module, each with its own
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
- The example is a seven-page gallery under `examples/gallery/`, built into
  `build/` and never committed. `--screenshot <dir>` renders every page to PNG
  and quits, which is how the pages are checked for a non-blank result: a page
  whose `paintEvent` draws nothing still exits 0, so "the window opened" proves
  nothing.

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

## What is next

Batch 0 is closed. The next step is Stage 1 § 1.2, in official order:

**Buttons** — 5 colour styles × 5 sizes × 3 shapes, with all eight interaction
states, then icon buttons, FABs, extended FABs, FAB menu, button groups, split
buttons and segmented buttons.

Each component lands as its own commit and must satisfy all twelve Definition of
Done items before the next one starts, including an independent gallery page, a
side-by-side visual audit in [visual-audit.md](visual-audit.md), and the
corresponding row in [md3-coverage.md](md3-coverage.md).

See [`md3-qt-porting-prompts.md`](md3-qt-porting-prompts.md) for the full brief,
the component order, and the Definition of Done.
