# Porting TODO

Upstream gaps and outstanding work. Stage 1 must be green before any Stage 2 item
is started (see [md3-coverage.md](md3-coverage.md)).

## Reference sources for every port

Each component is ported against both of these, in this order:

1. **Official spec** — <https://m3.material.io/components/<component>/overview>
   (plus the `/specs` and `/accessibility` pages). Source of structure:
   variants, sizes, shapes, states, selection behaviour, keyboard behaviour,
   accessibility rules. JS-rendered; capture with a real browser, not `WebFetch`.
2. **`material-web`** — <https://github.com/material-components/material-web>.
   Source of every numeric value: `tokens/versions/latest/sass/` for the
   per-component `md.comp.*` export (and `tokens/versions/v0_192` for the
   pinned `MdTokens` reference layer), `docs/components/` for behaviour notes.
   The repo is in maintenance mode and does not implement Expressive, so where
   Expressive behaviour is not in the tokens (springs, shape morphs), the
   Jetpack Compose Material3 source is the behaviour reference.

When the spec page and the token export disagree, do not silently pick one:
record both values in this file under the component's entry and state which
one won and why (see the Button groups entry for a worked example).

For **interaction behaviour** (hover / press / focus / ripple / shape motion)
the standing comparison against the material-web demo site lives in
[`interaction-reference.md`](interaction-reference.md) — check it before
reporting a click effect as "different from the official demo", since
material-web is standard M3 while qt-md3 implements M3 Expressive.

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

      **Interaction-fidelity audit (2026-10-08).** The press animation was
      captured frame by frame (probe builds, ~50 ms steps) and compared
      against the official specs page and `material-web`'s ripple source.
      Three behaviours were wrong and are now fixed and pinned by
      `TestMd3Button` / `TestMd3IconButton`:

      * **The focus indicator follows `:focus-visible`.** The official
        components show the ring and the focused state colours for keyboard
        focus only — a pointer press that moves focus shows nothing. Qt only
        offers the focus *reason*, so `MdButton`/`MdIconButton` classify it in
        `focusInEvent` (Tab / Backtab / shortcut / other = visible; mouse /
        popup / window-activation = not) and expose `hasKeyboardFocus()`.
        Styles gate both the ring and the `Focused` state row on it.
      * **The ripple is the pressed state layer, so it takes that state's
        colour.** It was hard-coded to on-surface, which painted a *dark*
        ripple on every style; the export says
        `md.comp.button.pressed.state-layer.color` is on-primary for the
        filled button (a light ripple on the purple container) and
        on-secondary-container for tonal. The style now feeds the ripple the
        pressed row's `stateLayer` role.
      * **Press is not a flat state layer.** While pressed, the official
        `.hovered` and keyboard-`:focus-visible` flat tints keep painting and
        the press response rides entirely on the growing ripple circle.
        `MdStateLayer::strongestActive` is therefore called with
        `pressed=false`; folding press back in would darken the whole
        container the instant the pointer went down and destroy the ripple's
        spatial cue.

      Verified numerically after the fix: container lightens under the ripple
      (on-primary at the state opacity), the circle spreads from the press
      point to full coverage over 450 ms of standard easing, the corners
      spring 28 px → 12 px (medium) and back on `spring-fast-spatial`
      (1400 / 0.9), and the release fades over 375 ms linear.

- [x] **`MdButtonGroup`** — standard and connected variants, all five Expressive
      sizes, four selection modes (none / single / multiple / required), round
      and square shapes, horizontal and vertical orientation, live arrow-key
      navigation and focus-follows-current. The group is an *invisible
      container* — the spec gives it no colour attributes, so there is no paint
      filter for it at all; `MdButtonGroupStyle` owns geometry and corner
      shapes and `MdButtonGroup` owns the items and the selection model.
      `TestMd3ButtonGroup` (30 checks) pins the token table, the selection
      semantics, the layout arithmetic and the keyboard model.

      Three facts this component had to establish, recorded so the next
      container does not re-derive them:

      * **Neighbouring item rects overlap.** Qt clips a child to its own
        rectangle, and each item is an `MdButton` that insets itself by the
        7.5 px focus-ring margin — so two items whose *containers* are the
        token `between-space` apart have widget rects that overlap by
        `2 * 7.5 - betweenSpace`. The group places widget rects, not container
        rects, and `MdButtonGroupStyle::layoutFor` does that arithmetic.
      * **Press growth is a shared-width problem.** `pressed.item.width.
        multiplier` (15 %) grows the pressed item about its own centre; the
        items before it shift left by half the total growth and the items after
        it shift right by half. The widget's `sizeHint` carries one
        multiplier's worth of reserve so the growth never needs a resize (a
        group whose hint grew mid-press would jump), and the main-axis origin
        is measured against the *rest* preferred size so the pressed item
        swells instead of sliding.
      * **Selected inner corners are a fraction, not a shape.**
        `selected.inner-corner.corner-size: 50%` is a literal half of the cross
        extent, which no `ShapeCorner` spelling can express — so `MdButton`
        grew a per-corner override (`setCornerRadii`) that the group resolves
        per state, pressed winning over selected because no
        `selected.pressed.inner-corner` token exists.

      Upstream gaps preserved and pinned, detailed in
      [md3-coverage.md](md3-coverage.md): `connected.xsmall.inner-corner`
      (export 8 px vs spec page 4 px — export wins) and the square connected
      outer corner (spec-page-only).

      One behavioural note that is ours, not the spec's: arrow keys that are
      *not* the group's axis are consumed as no-ops. `QAbstractButton`'s
      fallback for an unhandled arrow is `focusNextPrevChild()`, which would
      silently wrap focus to the first item — worse than swallowing the key.

- [x] **`MdIconButton`** — 4 colour styles (standard / filled / tonal /
      outlined) × 5 Expressive sizes × 2 container shapes × 3 published
      padding tracks, icon-only by token arithmetic, toggle ("selected") form
      backed by Qt's checkable state. `TestMd3IconButton` pins the size table
      and all three colour families field by field.

      Two facts this component adds to the record:

      * **The colour matrix is three families, published incompletely on
        purpose.** plain (`<state>-*`), selected (`selected-<state>-*`) and
        unselected (`unselected-<state>-*`), where a missing selected slot
        means "same as the plain one" and a missing plain slot means "this
        style does not paint this part". Conflating the two absences would
        make standard grow a container, so the resolver keeps them apart and
        the test pins both.
      * **Setters must invalidate the token cache.** `tokens()` caches its
        resolved table, and a `setVariant()` / `setButtonSize()` /
        `setSpaceTrack()` that only called `update()` would repaint with the
        *previous* variant's colours and metrics — the first gallery render
        showed every icon button as standard-sized because of exactly this.
        The common button already did this right; the icon button initially
        did not, which is why it is written down here.

- [x] **`MdFab`** — 4 colour sets (surface / primary / secondary / tertiary) ×
      3 sizes (40 / 56 / 96 px, corners 12 / 16 / 28 px, icons 24 / 24 / 36) ×
      lowered / raised elevation rows, icon-only by token arithmetic (the
      labelled form is the extended FAB, a separate family). `TestMd3Fab`
      pins the size table, all four colour sets, the lowered elevation rows
      and the disabled row field by field.

      Three facts this component adds to the record:

      * **The token export publishes no disabled rows.** The disabled values
        (on-surface @12% container, on-surface @38% icon, elevation level0)
        come from the official spec page's disabled state table — the same
        disabled row every push-button family shows. Pinned by
        `disabledRowFillsTheExportGapFromTheSpecTable`.
      * **No pressed container shape, so no press morph.** The button and
        icon-button families publish `pressed.container.shape` and spring the
        corners there; the FAB family does not, and its pressed rows are
        literally labelled "(ripple)". A FAB's press response is the ripple
        alone, which is also consistent with the interaction reference
        (`docs/interaction-reference.md`).
      * **It is the Actions family that casts a real shadow.** Every colour
        set publishes a `container.elevation` (raised L3/L4, lowered L1/L2),
        so `MdFabStyle` paints `MdElevation::drawShadow` per state row — a
        lowered surface FAB visibly sits closer to the page.

      Not carried yet (recorded, not forgotten): the latest export also has
      `primary-container`-style tonal sets (`md.comp.fab.primary-container.*`)
      and the small-FAB touch-target wrapper (`48px - container-height`
      margin, for a 40 px FAB). Neither is on the official spec page's FAB
      colour/size axes; both are candidates for a follow-up pass.

- [x] **`MdExtendedFab`** — 6 colour sets (primary / secondary / tertiary +
      three `*-container`, **no surface**) × 3 sizes (56 / 80 / 96 px, corners
      16 / 20 / 28 px, icons 24 / 28 / 36, labels title-medium / title-large /
      headline-small) × lowered / raised elevation rows. Width is derived —
      leading space + icon + icon-label space + label + trailing space — which
      is the entire point of an "extended" FAB. `TestMd3ExtendedFab` pins the
      size table, all six colour sets, the lowered elevation rows and the
      disabled row field by field.

      Three facts this component adds to the record:

      * **No surface colour set.** The FAB family publishes surface / primary
        / secondary / tertiary; the extended FAB ships six sets and none of
        them surface-based. The spec page agrees, so this is a family
        difference, not an export gap.
      * **The base metric set matches no size table.** The unprefixed
        `_md-comp-extended-fab.scss` publishes 56 px, label-large and a
        16/12/20 rhythm, while the `small` table is also 56 px but
        title-medium with a 16/8/16 rhythm. The three explicit size tables
        win; the base rows are the legacy standard-M3 default.
      * **Deprecated focus rows ignored.** The colour sets carry *deprecated*
        `focus-icon-color`/`focus-state-layer-color` rows pointing at the
        `*-container` roles alongside the non-deprecated `focused-*` rows
        pointing at the interactive roles; the latter are used, matching the
        FAB family's reading.

      Elevation rows are word-for-word the FAB family's (raised L3/L4,
      lowered L1/L2), and — unlike the FAB — lowered changes elevation only:
      there is no lowered container colour in this family.

- [x] **`MdFabMenu`** — 3 colour groups (primary / secondary / tertiary,
      each published twice: the close button takes the pure colour, the
      list items the container colour), an anchor FAB + a 56 px close
      button sharing its top trailing corner + up to six staggered list
      items. `TestMd3FabMenu` pins both elements' rows, the three groups,
      the spacing and the disabled row field by field.

      Facts this component adds to the record:

      * **No motion rows in the export.** The spec page's only motion
        statement is "the FAB menu animates from the top trailing edge of
        the FAB". The reveal here uses the Compose M3 Expressive
        `SpatialDefault` spring, a 40 ms per-item stagger and a 24 px
        settle — recorded as a sourced-from-Compose convention, labelled as
        such on the gallery page, not a token fact.
      * **No focus-indicator rows of its own.** Focused state-layer colours
        exist; `focus.indicator.*` rows do not, so the shared
        `md-sys-state-focus-indicator` fallback (secondary, 3 px, offset 2)
        applies.
      * **No disabled rows** — the spec's disabled state table fills them
        (on-surface @12% container, @38% content, level0), as in every
        family since the buttons.
      * **Structural facts from the spec page**: up to six items, the close
        button always 56 dp, a recommended 4 dp FAB-to-menu gap on web, and
        "inherits its states and specs from the baseline menu". The
        container enforces none of these caps (a seventh item renders
        fine); the facts are recorded here and in the token header.

- [x] §1.2 Actions — all eight families ported (buttons, button groups,
      icon buttons, FABs, extended FABs, FAB menu, split buttons, segmented
      buttons).

#### Split buttons (ported)

`MdSplitButton` + `MdSplitButtonStyle` + `MdSplitButtonTokens`, locked by
`TestMd3SplitButton`. Four sourced facts shape the implementation:

* **The export publishes metric rows only.** No colour, state, focus or
  typography rows exist for `md.comp.split-button.*`. The spec page fills the
  gap in one sentence: "Split buttons use the same color schemes as standard
  buttons ... shown in the following token module." So the colour / state /
  focus / type rows are the button family's, resolved at the *identity* size
  mapping — the two Expressive scales agree on every height (32 / 40 / 56 /
  96 / 136). Recorded in the token header and pinned by
  `heightMatchedButtonSetIsUsedForTypography`.
* **The one behaviour the buttons don't have is the inner-corner morph.** The
  facing corners rest at 4 / 4 / 4 / 8 / 12 px (per size) and spring to
  8 / 12 / 12 / 20 / 20 px on hover *and* press; the trailing half's facing
  corners go to the literal 50% while selected. The outer corners stay
  corner-full at every size. The export publishes no motion rows, so the
  morph runs on the button family's press spring (1400 / 0.9) — the Compose
  Expressive choice, labelled as such.
* **Two press targets, one control.** Each half has its own ripple, its own
  state layer and its own focused colour row; the focus indicator is one ring
  around the whole split (`:focus-visible` semantics, keyboard focus only).
  Left/Right walk the halves, Up/Down are swallowed, Space/Enter activate the
  focused half.
* **The trailing icon default is `arrow_drop_down`** per the spec ("the
  trailing button should always have a menu icon") — but the bundled classic
  49-icon set has no arrow_drop_down glyph, so the gallery page sets
  `expand_more` explicitly. Recorded as an icon-set gap, not a token
  divergence.

#### Segmented buttons (ported)

`MdSegmentedButton` + `MdSegmentedButtonStyle` + `MdSegmentedButtonTokens`,
locked by `TestMd3SegmentedButton`. The export publishes exactly one set —
no size scale, no colour variants — so the whole family rides on
`md.comp.outlined-segmented-button.*` (40 px, label-large, 1 px outline,
corner-full, icon 18, secondary-container selection). Four facts the
implementation carries:

* **the divider is the shared stroke**: segments overlap by exactly the
  outline width (Compose lays the row out with
  `Arrangement.spacedBy(-BorderWidth)`), so each segment paints its *full*
  outline and shared edges stack into one 1 px line — no separate divider
  element exists;
* **`itemShape`** [compose]: the first segment rounds its inline-start
  corners, the last its inline-end corners, middle segments are rectangles;
* **the icon slot is always reserved** (18 + 8 px, [compose]; the export
  publishes neither row, nor the 12 px content padding) so the check scales
  in without moving the label; custom icons crossfade with it;
* **the export's pressed state-layer opacity is the focus one** (as
  published), and there are no motion rows — the check's scale-in runs on
  spring-fast-spatial (Compose `FastSpatial`), labelled as such.

A disabled selected segment keeps its secondary-container fill: the export
has no disabled-container row and Compose's `disabledActiveContainerColor`
is `SelectedContainerColor`.

- [ ] §1.3 Communication — **Badges and Progress indicators ported**; loading
      indicator, snackbars and tooltips remain.

#### Badges (ported)

`MdBadge` + `MdBadgedBox` + `MdBadgeStyle` + `MdBadgeTokens`, locked by
`TestMd3Badge`. The quietest family so far — four facts shape it:

* **material-web does not implement the Badge component.** There is no
  `packages/badge`; the token file
  `tokens/versions/latest/sass/_md-comp-badge.scss` is the only material-web
  fact (dot 6 px, large minimum 16 px, both corner-full, error / on-error,
  label-small label). Every behaviour row is annotated `[compose]` and comes
  from androidx `Badge.kt` — the second family (after FAB menu's motion) with
  no web behaviour source at all.
* **The export publishes no state rows at all.** A badge is not interactive:
  no hover, no press, no focus indicator, no disabled row. The widget
  contract is `Qt::NoFocus` + `WA_TransparentForMouseEvents`, which is what
  lets an anchored badge pass clicks through — Compose achieves the same by
  not wrapping the badge in a Surface ("it blocks touch propagation behind
  it").
* **The anchoring offsets are Compose facts.** The dot sits on the anchor's
  top-end corner (offset 6/6); the pill's start edge sits 12 px inside the
  anchor's end edge and its bottom edge 14 px below the anchor's top edge,
  with 4 px label side padding.
* **One Qt adaptation, recorded rather than hidden.** Compose lets the pill
  overlap the surroundings freely; a Qt child widget is clipped to its
  parent. `MdBadgedBox` therefore reserves the overhang (pill width − 12 /
  height − 14) in its own geometry — the painted result is identical to
  Compose's, only the surrounding layout sees a slightly larger box.

#### Progress indicators (ported)

`MdProgressIndicator` + `MdProgressIndicatorStyle` + `MdProgressIndicatorTokens`,
locked by `TestMd3ProgressIndicator`. Five facts shape the implementation:

* **The export is one merged family.** `md.comp.progress-indicator.*` (base +
  linear + circular) supersedes the two per-shape sets, deprecated as of
  34.0.21. The deprecated sets survive as the *only* source of the four-color
  rows (primary / primary-container / tertiary / tertiary-container, identical
  in both) — carried for the `fourColor` API with that provenance. The base
  set's four metric rows are also individually `@deprecated`; transcribed,
  not read by the painter.
* **The `thick.*` rows are deprecated as a variant** — "no longer tokenized
  as a variant, but rather a sample configuration in code". Transcribed into
  the token struct for completeness, never exposed as an API.
* **The indeterminate animations are the MDC-heritage keyframes** the
  material-web internal SCSS ships (cited to
  `mdc-linear-progress/_linear-progress.scss`): the linear two-bar 2 s cycle
  (translate 0→200.611 %, scale 0.08→0.661479→0.08, each segment its own
  bezier) and the circular three composed rotations — expand-arc 1333 ms
  (265°↔130°), the eased 135° group steps over 4× that period, and a linear
  spin of ARCTIME×360/306 ms, the right half delayed half a period. All
  pinned as pure functions of elapsed time, which required a hand-rolled
  cubic-bezier solver (`QEasingCurve` has no public per-t evaluation).
* **The four-color cycle interpolates colours, not indices** — CSS animates
  the background/border colour smoothly between keyframe marks
  (0/15/25/40/50/65/75/90 %), linear-timed for the linear shape and riding
  the indeterminate easing per segment for the circular one.
* **The registered gap: the wave rows.** The export publishes non-deprecated
  Expressive wave tokens — linear amplitude 3 px / wavelength 40 px,
  indeterminate wavelength 20 px, with-wave height 10 px; circular amplitude
  1.6 px / wavelength 15 px, with-wave size 48 px. Neither material-web
  (which predates Expressive) nor this port renders them. **Open question:
  port the wave rendering from Compose's `WavyLinearProgressIndicator` /
  `WavyCircularProgressIndicator`** (amplitude animation on indeterminate,
  wave phase shift, the with-wave container sizes). Until then the token
  struct carries every value and the painter draws the non-wave baseline.

Two smaller divergences, both recorded: the circular geometry follows
Compose (stroke centred so the ring spans exactly `size`; material-web's
percentage-stroke CSS resolves differently) and the circular determinate
track is painted (the merged export's `track.color`; material-web's legacy
`.track` stroke is transparent). RTL mirrors the linear indicator, the CSS
`scale(-1)` contract.

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
