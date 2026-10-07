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

Track progress in [md3-coverage.md](md3-coverage.md). Order follows the official
categories: Actions → Communication → Containment → Navigation → Selection →
Text inputs, then the M3 Expressive cross-cutting pass.

### Actions 1.2 — Buttons

- [x] **`MdButton`** — 5 colour styles × 5 Expressive sizes × 2 container
      shapes, leading/trailing icons, soft-disabled, live hover / focus / press.
      `MdButtonTokens` holds the value layer, `MdButtonStyle` the painting,
      `MdButton` the widget. `TestMd3Button` pins all 50 size × style
      combinations field by field, and the gallery has an eighth page
      (`08-buttons`) covering every one of them.

      It is also the first component, which forced three base-module questions:

      * **Which token export is authoritative for component tokens?**
        The repo pins `material-web` `tokens/versions/v0_192` for `MdTokens`
        (the `_md-sys-*.scss` SVG exports). The button component tokens do not
        exist there in usable form: the five-step size scale
        (`xsmall`/`small`/`medium`/`large`/`xlarge`) is Expressive-only, and the
        legacy `md.comp.filled-button` / `md.comp.filled-tonal-button` family
        names were replaced by a base file plus five style files plus five size
        files. Buttons therefore read
        `tokens/versions/latest/sass/_md-comp-button{,-<style>,-<size>}.scss`
        (**34.0.21**). The record of which export each table came from lives in
        the header comment of `MdButtonTokens.h`, and is a per-component
        decision rather than a new global pin.
      * **The two exports disagree on `md.sys.state.*`.** v0_192 publishes
        focus 0.12 / pressed 0.12 / hover 0.08 / dragged 0.16; latest publishes
        focus 0.10 / pressed 0.10 with the same hover and dragged values.
        `MdTokens` keeps 0.12 because it is faithful to *its* pin. Recorded here
        so that nobody "fixes" it into 0.10 without also moving the pin — and so
        that nobody assumes the button's state layers and the base module's
        numbers are meant to be the same number.
      * **A focus indicator needs room the widget does not have.** Qt clips a
        child to its own rectangle, and `md.comp.button.focus.indicator` is an
        *outward* indicator: 3 px stroke at a 2 px gap, animated out to the
        8 px active width, so its outer edge sits at
        `offset + activeWidth/2 + width/2` = 7.5 px outside the container.
        `MdButton` therefore insets its container by that amount on every side
        and derives the inset from the token spec
        (`MdButtonStyle::focusRingInset`) rather than hard-coding 7.5.

      Two upstream inconsistencies are preserved, not reconciled — §九 says
      record the difference instead of quietly picking a winner:

      * `md.comp.button.leading-space` says 24 px and
        `md.comp.button.small.leading-space` says 16 px, for the same 40 px
        height. The resolver lets the size-qualified key win, and
        `theSizeSpecificPaddingWinsOverTheBaseToken` pins that.
      * `md.comp.button.<style>.hovered.container.elevation` is still published
        while carrying `@deprecated No longer part of the design spec`. It is
        kept in the table (so the table stays a faithful transcription) and not
        painted (so the widget follows the spec).

- [ ] Button groups, Icon buttons, FABs, Extended FABs, FAB menu, Split buttons,
      Segmented buttons — the rest of §1.2.

### Gallery scaffolding fixes found while building the first component

The button page is the first page with real child widgets, which exposed three
defects in `GalleryPage` that no token-only page could reach. All three are
fixed:

* `build()` is called from `measure()`, `remeasure()` and `paintEvent()` — many
  times, at changing widths. A page that allocated its widgets *inside* `build()`
  therefore created a fresh generation of children on every layout pass, all
  parked at (0, 0) and all visible. Component pages must keep a widget bank.
* `GalleryPage` overrode `hasHeightForWidth()` but never declared
  height-for-width in its **size policy**, and `QScrollArea` consults the size
  policy, not the virtual. Every page was silently pinned to exactly one
  viewport tall and nothing scrolls; the only symptom was that the bottom of a
  long page could not be reached.
* `measure()` started its cursor at the gutter while `paintEvent()` started it
  at `buildOriginY()`, and returned one gutter instead of two. Pages were
  reported a title-block short and 32 px short, and `paintEvent`'s clip rect
  then cut the last line in half.

`--screenshot` now also writes a `-full.png` per page: the page widget grown to
its own `heightForWidth()`. The window shot can only ever prove that the *top*
of a page draws, which is not enough once pages are taller than the viewport.

## Stage 2 — Qt extensions

Not started. Full list: appendix D of
[md3-qt-porting-prompts.md](md3-qt-porting-prompts.md). Blocked by the
`TestMd3CoveragePolicy` gate until Stage 1 is green.
