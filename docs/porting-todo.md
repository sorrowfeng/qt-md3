# Porting TODO

Upstream gaps and outstanding work. Stage 1 must be green before any Stage 2 item
is started (see [md3-coverage.md](md3-coverage.md)).

## Stage 1 — base modules

All nineteen modules are implemented. What follows is what is *not* finished
inside them, recorded with the question that has to be answered before it can be.

- [x] `MdTokens` — reference / system / component layers, global and per-instance
      override
- [x] `MdColorScheme` — 49 roles, light / dark, including the `*-fixed` family
- [x] `MdTheme` — singleton with the `themeAboutToChange` / `themeChanged` /
      `themeModeChanged` lifecycle
- [x] `MdTypeScale` — 30 styles with script-category line height
- [x] `MdMotion` — easing and duration tokens plus the six Expressive springs
- [x] `MdStateLayer`, `MdRipple`, `MdElevation`, `MdFocusRing`
- [x] `MdIcon` — Material Symbols four axes plus the classic SVG baseline
- [x] `MdResources` — keeps the embedded `.qrc` linked in when the library is a
      static archive
- [x] `MdStyleBase` — `QProxyStyle` base with paint-filter / theme hooks
- [x] `MdFont` + `MdDesign::initialize(&app)`
- [x] Resources under `resources/` + `.qrc` + manifest
- [x] Token-value audit against the published token sources (`TestMd3Tokens`)
- [x] Light / dark render of every colour role (`TestMd3Tokens` compares all 49
      roles in both modes against `_md-sys-color.scss`)
- [x] Fixed-seed dynamic colour against the official implementation
      (`TestMd3Tokens` pins CAM16 against `dart/test/hct_test.dart` and the
      palette tables against `ColorSpec2021`)
- [x] Runnable example gallery under `examples/gallery/`, with `--screenshot`
      for a per-page non-blank render check

### Gaps still open inside Batch 0

- [x] **`ContrastLevel` is applied.**
      `MdDynamicScheme::create()` takes reduced / standard / medium / high and
      `MdColorSpec2021` solves every role against its own `ContrastCurve` and
      `ToneDeltaPair`, so the levels genuinely move the tones.
      Source of truth: `material-color-utilities` `ColorSpec2021.java`,
      `dynamiccolor/ContrastCurve.java`, `ToneDeltaPair.java`,
      `utils/Contrast.java`, verified by `TestMd3Contrast`.

      Two things had to be separated to get here, and the separation is the
      interesting part of the change:

      * **The published `md.sys.color` sets are not the solver's output.**
        They are authored static data on the M3 reference palette, where the
        primary family carries the seed's own chroma. The dynamic solver
        rebuilds the family at the chroma `ColorSpec2021` prescribes per
        variant, so tonal-spot's primary for the default purple is `#65558f`
        while the published baseline primary is `#6750a4`. Both are correct in
        their own layer. `MdColorScheme::baseline()` now returns the published
        sets verbatim — all six of them, light/dark × standard/medium/high —
        and the solver is verified against Google's own scheme assertions
        instead. They are pinned so that `dynamicTonalSpotIsNotTheStaticBaseline`
        fails if anyone "reconciles" them.
      * **`ContrastLevel::Reduced` exists.** It is material-color-utilities'
        contrast level `-1.0`. No `md.sys.*` token names it because it only
        exists in the dynamic pipeline, so `baseline()` falls back to the
        standard set for it rather than inventing tones.

### Verification fixtures for the colour pipeline

The colour modules are checked against two vendored reference sets rather than
against the implementation's own output, because a self-referential check
cannot detect a shared misunderstanding:

- `resources/tokens/md-sys-color-*.txt` — the six published token sets, from
  material-web `tokens/versions/latest/sass`. Regenerate with
  `tools/update-sys-color-fixtures.py`, which also takes `--emit-cpp` to
  regenerate `MdColorScheme.cpp`'s role tables so the 288 rows are never
  hand-typed.
- `resources/tokens/mcu-scheme-expectations.txt` — 367 assertions lifted from
  material-color-utilities' own Swift tests, covering nine variants, both modes
  and three contrast levels. Regenerate with
  `tools/update-mcu-scheme-fixtures.py`.

Both generators are checked in, and both need a checkout of the upstream source
to run. Vendoring a pinned copy under `docs/reference/` so they can run offline
is still outstanding.

- [ ] **35 Expressive decorative shape paths.**
      `MdShape::decorativeShapeCount()` returns `0`. The shape scale and shape
      morphing are complete; the decorative path library is not.
      What is needed: the 35 path definitions from the M3 Expressive shape
      library, as normalised paths plus their interpolation pairs. They are not
      in `material-web` (which has no Expressive support) and are not published
      as data anywhere machine-readable, so each one has to be traced from the
      specification and reviewed visually. That is a task of its own, and
      inventing plausible-looking shapes would be worse than having none.

- [ ] **`ScriptCategory::Large` and `ExtraLarge` line-height multipliers.**
      Only `Small` (1.00) and `Medium` (1.07) are sourced from the M3 type
      specification. `Large` and `ExtraLarge` currently fall back to `Medium`'s
      1.07 and are marked as unsourced in the code.
      What is needed: a source that publishes the per-script-category
      multipliers for Devanagari / Bengali / Tamil (large) and Tibetan /
      Mongolian (extra-large).

- [ ] **`ColorSpec2025` and `ColorSpec2026`.**
      The brief names both. `MdDynamicColor` currently implements the
      `ColorSpec2021` palette and tone tables, which is what `material-web`
      v0_192 reproduces and therefore what the token audit can verify. The newer
      specs change several tone assignments (notably around `surface` and the
      tertiary palettes) and need their own tables plus their own audit.

- [ ] **Variable font axes below Qt 6.7.**
      `MdIcon` applies FILL / wght / GRAD / opsz through
      `QFont::setVariableAxis`, which is Qt 6.7+. On Qt 6.5 and 6.6
      `MdIcon::axesSupported()` returns false and only the named weights are
      honoured. The declared minimum stays at 6.5.0, so this is a documented
      capability difference rather than a bug.

## Stage 1 — components

Not started. Track progress in [md3-coverage.md](md3-coverage.md). Order follows
the official categories: Actions → Communication → Containment → Navigation →
Selection → Text inputs, then the M3 Expressive cross-cutting pass.

## Stage 2 — Qt extensions

Not started. Full list: appendix D of
[md3-qt-porting-prompts.md](md3-qt-porting-prompts.md). Blocked by the
`TestMd3CoveragePolicy` gate until Stage 1 is green.
