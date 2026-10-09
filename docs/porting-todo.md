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

- [x] §1.3 Communication — **Badges, Progress indicators, Loading indicator,
      Snackbar and Tooltips ported** (the family is closed).
- [x] §1.4 Containment — **Cards, Dialogs, Bottom sheets, Side sheets, the
      Carousel, the Divider and Lists ported** (the family is closed).

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

#### Loading indicator (ported)

`MdLoadingIndicator` + `MdLoadingIndicatorStyle` + `MdLoadingIndicatorTokens`
+ the `MdMaterialShapes` engine in core, locked by
`TestMd3LoadingIndicator`. Four facts shape the implementation:

* **The export is token-only.** material-web ships
  `_md-comp-loading-indicator.scss` (34.0.21: active indicator 38 px,
  container 48×48 corner-full, primary / on-primary-container on
  primary-container, **no state rows**) but no web component. The behaviour
  port is Compose M3 Expressive's `LoadingIndicator` (androidx-main) — the
  third family whose behaviour source is Compose (after Badge and the FAB
  menu motion).
* **The shape engine is a faithful graphics-shapes port.** `MdCubic` /
  `MdRoundedPolygon` reproduce androidx `graphics-shapes`' corner-rounding
  construction field by field — the two-step cut negotiation between
  adjacent corners, the smoothing flanking curves, the circular-arc cubic,
  `normalized()` (against the *control-point* hull, like the reference), the
  exact and max-rotation bounds, and the first-corner arc-midpoint outline
  start. The eight catalogue shapes the family needs (circle, oval, pill,
  pentagon, sunny, 4-cookie, 9-cookie, soft-burst) are transcribed from
  `MaterialShapes.kt` verbatim; the generic polygon/star/custom machinery is
  exposed so more of the catalogue can be added without touching the engine.
* **One registered divergence: the morph.** graphics-shapes' `Morph` matches
  curve *features* between two shapes before interpolating them
  (MeasuredPolygon + featureMapper, ≈1500 further lines). This port samples
  both normalized outlines radially (240 angles from the polygon centre) and
  interpolates the matching points. Every shape in the indicator's sequence
  is star-convex around its centre, so at the family's 38 px the difference
  is not observable; the divergence is pinned in the test suite and on the
  gallery page. If a later component needs morphs between shapes that are
  not star-convex (ghostish, arrow, …), the feature-matching algorithm has
  to be ported — **open question**.
* **The animation is the Compose spec as pure functions.** The morph runs on
  a 650 ms grid; each morph is the closed form of the published spring
  (dampingRatio 0.6, stiffness 200) and is left **unclamped** — the spring
  overshoots to ≈1.08 and the morph extrapolates linearly, which *is* the
  shape bounce. The draw rotation composes the in-morph quarter turn, a
  quarter-turn step per completed morph, and a 4666 ms linear global spin.
  The determinate mode walks the open circle→soft-burst morph by progress
  and sweeps −180°. Two recorded simplifications: the closed-form spring
  never freezes at the reference's visibility threshold (a sub-pixel
  difference at the snap, where the reference may hold at ≈0.99), and the
  morph path is a dense polyline rather than interpolated cubics.

#### Snackbar (ported)

`MdSnackbar` + `MdSnackbarHost` + `MdSnackbarStyle` + `MdSnackbarTokens`,
locked by `TestMd3Snackbar`. Four facts shape the implementation:

* **One published set, and the inverse roles carry it.** The 34.0.21 export
  has no variants and no per-state colour rows beyond the state layers:
  inverse-surface container (level 3, corner-extra-small), body-medium
  supporting text in inverse-on-surface, label-large action in
  inverse-primary, 24 px icon in inverse-on-surface. The export *does*
  publish hover / focus / pressed rows for both interactive elements — the
  first Communication family whose published state rows are all painted.
  The ripple colour follows the pressed row (the project's ripple rule).
* **The layout rows do not exist; Compose is the source, again.** The
  one-row and new-line layouts port `OneRowSnackbar` / `NewLineButtonSnackbar`
  (the styling-fix versions): container max width 600, start 16, the
  text-end extra spacing 8, text vertical padding 14, first line at 30, the
  new-line action's 4 px bottom padding and its 8 px end inset without a
  dismiss icon. The single-line 48 / two-line 68 heights *are* export rows.
  The action's hit region plays the role of Compose's TextButton bounds
  (label + 12 px chrome) and the dismiss icon sits in a 40 px icon-button
  chrome — both approximations are labelled in the token header.
* **MdSnackbar is the visuals only; MdSnackbarHost is the other half.**
  Compose's `SnackbarHostState.showSnackbar` owns the queue, the
  4000/10000/Indefinite durations and the rule that an actionable snackbar
  never self-dismisses; the host ports exactly that, plus the
  FadeInFadeOutWithScale transition (opacity on the effects-fast spring,
  scale 0.8→1 on the spatial-fast spring — both already in `MdMotion`).
  One open question: Compose de-dupes only the *current* request, so this
  host drops queued duplicates of the current one as its own back-pressure;
  the upstream queue semantics are unspecified.
* **A Qt child widget clips its own rect, so the level-3 shadow needs
  headroom.** The widget rectangle is the container grown by an 8 px shadow
  margin (`kShadowMargin`); the container sits inset and every layout runs
  on it. The host positions the widget rect, so the shadow survives the
  overlay.

#### Tooltip (ported)

`MdTooltip` + `MdTooltipHost` + `MdTooltipStyle` + `MdTooltipTokens`, locked
by `TestMd3Tooltip`. The Communication family's closing pair — four facts
shape it:

* **material-web ships only the token exports; Compose is the source for
  everything else.** There is no tooltip component in the current
  material-web repository — `_md-comp-plain-tooltip.scss` and
  `_md-comp-rich-tooltip.scss` (34.0.21) are the whole web story. The
  layouts port Compose M3's `Tooltip.kt` measure logic (min 40×24, plain
  max 200 with 8/4 content padding, rich max 320 with 16 px horizontal
  padding, the paddingFromBaseline rows 28 / 24 / 16, the action box 36 + 8,
  the plain 4 px vertical fallback when neither subhead nor action exists)
  and the behaviour ports `BasicTooltip.kt`.
* **The trigger rules are Compose's priority system, ported literally.**
  Mouse hover shows immediately at `UserInput` priority — no timeout; the
  pointer leaving dismisses (unless persistent). Keyboard focus and touch
  long-press (`QStyleHints::mousePressAndHoldInterval` stands in for
  `viewConfiguration.longPressTimeoutMillis`) show at the lower priority,
  which self-dismisses a non-persistent tooltip after
  `BasicTooltipDefaults.TooltipDuration` = 1500 ms. Escape dismisses; a Tab
  from a visible action-bearing anchor moves focus into the tooltip. The
  GlobalMutatorMutex is a static host pointer: a new show cancels the
  previous tooltip instantly, without its exit animation.
* **Two recorded divergences.** (1) The caret: Compose draws it OUTSIDE the
  surface into the 4 px anchor gap — overlapping the anchor by
  caret − spacing — while this port reserves an 8 px caret margin in the
  widget rect and keeps the tip exactly `kAnchorSpacing` from the anchor.
  (2) Left / right / start / end popup positioning is not ported; the host
  offers Above (the modern `Above` provider: centre→start→end, above→below,
  coerced) and Below with the rich start-aligned provider Compose still
  ships.
* **A top-level popup plus platform quirks.** The popup is a
  `Qt::ToolTip`-flagged translucent top-level; the host repositions the
  anchor to fill itself on resize (the gallery relies on it), and the Enter
  handler gates on the cursor actually being inside the anchor because some
  platforms synthesise an Enter when a window shows. Persistent default
  false (Compose `rememberTooltipState`); an actionable rich tooltip is the
  documented case for setting it true. Outside-click dismissal of a
  persistent tooltip is not ported yet.

#### Card (ported)

`MdCard` + `MdCardStyle` + `MdCardTokens`, locked by `TestMd3Card`. The
Containment family's opening surface — four facts shape it:

* **material-web ships no card web component** (token export only), so the
  visuals come from the three `_md-comp-<variant>-card.scss` sets (34.0.21)
  and the behaviour from Compose M3's `Card.kt`. Compose publishes exactly
  two shapes per variant — a plain surface and a clickable one — modelled
  here as one widget with a `clickable` switch.
* **Press never raises.** The 34.0.21 export's "Pressed (ripple)" rows
  repeat the resting elevation (filled level0, elevated level1); the press
  response is the ripple alone, the same rule the button families follow.
  Dragged climbs highest (filled/outlined level3, elevated level4) and
  paints its own on-surface state layer at the dragged opacity; the state
  is driven by an explicit setter for external drag frameworks.
* **The elevation ladder animates.** Compose animates elevation through the
  theme's motion scheme; this port runs the same transition as a 200 ms
  standard-easing tween (registered divergence — the scheme's spring spec
  is not published in the static export) and snaps when the card becomes
  disabled, as Compose does. `MdElevation::drawShadowDp` was added so the
  paint can follow the interpolated dp.
* **Disabled compositing is Compose arithmetic.** The export publishes
  disabled container rows (filled → surface-variant, elevated → surface,
  outlined unchanged); the paint composites each at 0.38 *over the enabled
  container colour*, content drops to 0.38, and the outlined stroke fades
  to 0.12. Recorded divergence: Compose composites the disabled outline
  over the *elevated* card's container colour (an over-eager copy); this
  port composites over the card's own container.

Also of note: contents margins always match the painted container (the
focus margin for a clickable card, zero otherwise), so a layout installed
on the card fills the container exactly; Compose publishes no default
content padding and neither does this class.

#### Dialog (ported)

`MdDialog` + `MdDialogHost` + `MdDialogStyle` + `MdDialogTokens`, locked by
`TestMd3Dialog`. Four facts shape it:

* **material-web ships no dialog web component** (token export only), so the
  visuals come from `_md-comp-dialog.scss` (34.0.21) and the behaviour from
  Compose M3's `AlertDialog.kt`. The full-screen-dialog token export
  (`_md-comp-full-screen-dialog.scss`) exists but Compose implements no M3
  full-screen dialog to port behaviour from — recorded, not ported.
* **The action slots are real Text buttons, not self-drawn regions** — the
  recorded counter-divergence to the snackbar: there the inverse-primary
  action colour no `MdButton` variant could express; here the export's
  action rows (label-large in primary at the standard opacities) ARE the
  text button's own tokens, and Compose documents that its slots are
  TextButtons which use their own colours. Embedding gets ripple, state
  layers and `:focus-visible` for free; the token table still transcribes
  the export's rows as the family's record.
* **The slots map onto Qt by kind**: headline and supporting text are
  painted by the style (the snackbar idiom), the icon slot is any `QWidget`
  (Compose's icon slot is any composable), actions are the embedded
  buttons. Compose's RTL `AlertDialogFlowRow` trick is restated plainly:
  one row puts the confirm rightmost, wrapping puts the confirm on the
  first row and the dismiss on the second, both end-aligned.
* **The host is the Compose `BasicAlertDialog` half**: scrim (black at
  0.32, md.sys.color.scrim — not a dialog row), centring with the
  280..560 width clamp, Escape and scrim clicks running the
  `onDismissRequest` flow with the fade on the effects-fast spring —
  dialogs fade only, no scale. Recorded divergence: modality stops at the
  host's parent — a child-widget port cannot run the platform modal loop,
  so the overlay swallows interaction within its parent only. Compose's
  flag-gated "precision pointer" sizing (20/16 paddings, 20 sp title) is
  not ported.

#### Bottom sheets (ported)

`MdBottomSheet` + `MdBottomSheetHost` + `MdBottomSheetStyle` +
`MdBottomSheetTokens`, locked by `TestMd3BottomSheet`. Four facts shape it:

* **material-web ships no sheet web component** (token export only), so the
  visuals come from `_md-comp-sheet-bottom.scss` (34.0.21) and the behaviour
  from Compose M3's `BottomSheet.kt` / `BottomSheetScaffold.kt` /
  `ModalBottomSheet.kt` / `SheetDefaults.kt`. One token set covers both
  presentations — the export's modal and standard elevation rows both resolve
  to level 1 — and the port keeps Compose's split: the sheet surface carries
  the anchors and the drag, the host is the `ModalBottomSheet` wrapper (scrim,
  Escape / scrim click, `onDismissRequest`).
* **The anchor math is the port**, verbatim: Hidden at the parent's bottom;
  PartiallyExpanded at the peek height (standard) or
  `fullHeight - min(fullHeight/2, sheetHeight/2)` (modal — Compose's
  deterministic rule, flag-on by default); Expanded at
  `max(0, fullHeight - sheetHeight)`. The standard kind skips Hidden by
  default (`skipHiddenState = true`), the modal kind always has it. Show
  animations run the spatial-default spring, hide the fast-effects spring,
  exactly the `showMotionSpec` / `hideMotionSpec` assignment.
* **Recorded divergences**: (1) no velocity fling — Compose's
  `anchoredDraggable` weighs the 125 dp/s velocity threshold and dampens
  inside the 125 dp boundary zone; the Qt port settles drags positionally
  against the 56 px threshold and clamps (both constants are transcribed into
  the token struct as the record). (2) The export's drag-handle opacity row
  (0.4, deprecated per b/278783477) is transcribed but NOT applied — Compose's
  `BottomSheetDefaults.DragHandle` paints the full colour. (3) The published
  Hidden-state shape (`minimized.container.shape` = corner-none) is recorded
  only — Compose publishes `HiddenShape` but `BottomSheetImpl` keeps the
  expanded shape constantly, so the paint follows the implementation. (4) The
  focus-indicator rows are recorded, not painted — Compose's drag handle is a
  clickable Box with no focus ring. (5) The Expressive standalone drag handle
  (`_md-comp-drag-handle.scss`: a 48 px handle growing to 52 x 12 when
  pressed) is a separate token set, not a sheet row — recorded, not ported.
  (6) Modality stops at the host's parent, the shared child-widget caveat.
* **Geometry management is opt-in.** A statically-placed sheet (a gallery
  snapshot, a fixed scaffold slot) never jumps to its anchors when the parent
  resizes — the host calls `setGeometryManaged(true)` to take over
  anchoring/centring/the hidden slide. The widget rect carries the level-1
  shadow margin on the top/left/right with a FLUSH bottom edge (a sheet is
  edge-to-edge with the parent's bottom), and `dismissed()` fires when the
  slide-down settles — Compose's `invokeOnCompletion` timing, not at request
  time.

#### Side sheets (ported)

`MdSideSheet` + `MdSideSheetHost` + `MdSideSheetStyle` +
`MdSideSheetTokens`, locked by `TestMd3SideSheet`. Four facts shape it:

* **The behaviour source is MDC-Android, not Compose.** material-web ships
  only the token export (`_md-comp-sheet-side.scss`, 34.0.21) and Compose M3
  ships no side sheet at all — the official overview's availability table
  lists Android Views as the ONE available implementation. The two-state
  machine (Hidden / Expanded plus the transient dragging/settling), the
  Left/Right delegate geometry (`hiddenOffset` = the parent's width,
  `expandedOffset` = `max(0, parentWidth - childWidth - innerMargin)`, and
  the mirrored pair for the left edge), the `isReleasedCloseToInnerEdge`
  midpoint settle, and the dialog wrapper (cancel on hide) all transcribe
  from `SideSheetBehavior.java` / `RightSheetDelegate.java` /
  `SideSheetDialog.java`; the measurements (24 px content padding, the 256 px
  container width, the 16 px detached margin) come from
  m3.material.io/components/side-sheets/specs.
* **Recorded divergences**: (1) no velocity-weighted settle — MDC projects
  the release position through the 0.1 hide friction against the 0.5 hide
  threshold and weighs the 500 px/s significant velocity; the Qt port
  settles on the midpoint rule positionally (all three constants are
  transcribed into the token struct as the record). (2) The spec overview's
  M2→M3 diff says "modal side sheets have a 16dp corner radius" but the
  token export and MDC both publish `corner-large-start` (the 24 px large
  radius on the start pair) — the paint follows the token export + MDC, the
  16 dp note is recorded. (3) The detached presentation (16 px margins all
  around, `detached.container.shape` = corner-large, the
  `Widget.Material3.SideSheet.Detached` style) is recorded, not ported. (4)
  The Expressive floating sheet (`_md-comp-sheet-floating.scss`) is a
  separate token set — recorded, not ported. (5) MDC's coplanar sibling
  layout (`updateCoplanarSiblingLayoutParams` — the content beside a docked
  sheet reflows to make room as it slides) needs a coordinating parent
  layout the port's overlay model does not have — recorded. (6) The
  focus-indicator rows and the action rows (primary label-text, the state
  layer opacities) are recorded, not painted — the sheet surface paints
  none of them; the content is the caller's widgets. (7) Modality stops at
  the host's parent, the shared child-widget caveat.
* **The corner-large-start pair is edge-aware.** The published shape names
  the START pair, which faces the app content: for a right-docked sheet the
  TL / BL pair rounds (the style builds {r, 0, 0, r}), for a left-docked
  sheet the TR / BR pair ({0, r, r, 0}) — the LeftSheetDelegate's mirror,
  written as one layout function with an edge parameter.
* **Geometry management is opt-in**, the bottom sheet's guard: a
  statically-placed sheet never jumps to its anchors when the parent
  resizes; the host calls `setGeometryManaged(true)`. The widget rect
  carries the level-1 shadow margin on the top/bottom/inner edges with a
  FLUSH docked edge, and `dismissed()` fires when the slide-out settles —
  MDC's `SideSheetDialog` cancel timing (the behavior reports HIDDEN).

#### Carousel (ported)

`MdCarousel` + `MdCarouselStyle` + `MdCarouselTokens`, locked by
`TestMd3Carousel`. The heaviest math port of Stage 1 so far:

* **The behaviour source is the whole Compose M3 carousel package** —
  `Carousel.kt` / `Strategy.kt` / `Keylines.kt` / `KeylineList.kt` /
  `Arrangement.kt` (~2,600 lines), material-web again shipping only the token
  export (`_md-comp-carousel-item.scss`, 34.0.21) and no web component. The
  port transcribes the model verbatim: an *arrangement* (the
  `findLowestCostArrangement` / `fit` / `calculateLargeSize` trio, the medium
  size solved as `(large + small) / 2` and the ±10% medium flex), the
  *keylines* (offsets accumulating by each slot's own size, unadjusted
  offsets by the focal size — the end-to-end scroll model), the *steps* (one
  per non-anchor slot, each moving that slot across the focal range), and
  the per-item interpolation (the item's visible width and position lerp
  from its two surrounding keylines — the resize-as-it-scrolls signature).
  The multi-browse counts, the tiny-container small relaxation, the surplus
  trimming against the real item count and the 10 px anchors all follow
  `multiBrowseKeylineList`.
* **The port is pinned against Compose's own unit tests** — the
  380/186/8 case reproduces `MultiBrowseTest.adjustsForItemSpacing`
  exactly (five keylines, unadjusted offsets −101 / 93 / 287 / 481 / 675,
  the large item unresized, the trailing small at the 56 px cap), and the
  100/200 and 512/3 cases reproduce the other two test bodies.
* **Recorded divergences**: (1) item spacing — Compose defaults
  `CarouselDefaults.ItemSpacing` to 0.dp while the specs page publishes
  "Padding between elements 8dp"; the port takes the spec value and records
  the conflict. (2) The specs page's leading/trailing 16 px padding is the
  caller's margin — Compose's contentPadding shift path
  (`createShiftedKeylineListForContentPadding`) is recorded, not ported.
  (3) Only the multi-browse strategy is ported; hero, center-aligned hero,
  uncontained and full-screen are separate keyline functions in Compose and
  are recorded, not ported. (4) The snap settle runs the effects-default
  spring against the nearest item boundary (Compose Pager's page snap); the
  velocity-weighted fling is not ported. (5) The focus-indicator rows and
  the item-z-order (Compose's `zIndex = 1 / (1 + distance)`, approximated
  by raise order) are recorded. (6) Item content is mouse-transparent — the
  carousel owns the drag; per-item interaction (Compose's clickable items)
  is the caller's business, recorded.

#### Divider (ported)

`MdDivider` + `MdDividerStyle` + `MdDividerTokens`, locked by
`TestMd3Divider`. The smallest token export in the library — exactly two
rows (`thickness: 1px`, `color: outline-variant`, 34.0.21) — so the port is
mostly arrangement:

* **Three sources, two vocabularies for "inset".** The m3.material.io
  measurements name "inset" (left 16dp, right 0) and "middle-inset" (both
  16dp); material-web publishes the attributes `[inset-start]`, `[inset-end]`
  and `[inset]` — where `[inset]` pads **both** edges. `InsetMode`
  (None / Start / End / Both) covers every combination so both vocabularies
  are reachable; the naming conflict is recorded here rather than resolved
  silently.
* **The 16px inset is not a published token** — material-web hardcodes it as
  `padding-inline-start/end: 16px` in `divider/internal/_divider.scss`. The
  port tokenises it as `md.comp.divider.inset` so themes can retune it; the
  hardcoding is the divergence.
* **Start/end are logical inline edges** — material-web spells the padding
  `padding-inline-*`, so an RTL layout swaps the physical sides. The widget
  mirrors on `LayoutDirectionChange`; pinned by a test.
* **The vertical form is `[compose]`** — Compose publishes
  `VerticalDivider` (`fillMaxHeight().width(thickness)`); material-web has
  no vertical form (CSS layout does not need one). The thickness and colour
  parameter overrides mirror Compose's `thickness` / `color` parameters.
* **The hairline is painted device-pixel aligned** — the line rect is
  snapped outward onto whole device pixels and drawn without antialiasing,
  so the 1px token thickness stays one crisp physical pixel instead of
  straddling two at half coverage.
* **Non-interactivity is the contract**: no state rows are published at all;
  `Qt::NoFocus`, `WA_TransparentForMouseEvents`, no disabled form.
* **Recorded, not ported**: the spec's usage spacing rows ("space between
  divider & supporting-text 4dp", "right/bottom margin 8dp") are caller
  layout guidance, not component tokens.

#### Lists (ported)

`MdList` + `MdListItem` + their two styles + `MdListTokens` / the container
tokens, locked by `TestMd3List`. Unlike the Divider, lists *do* have a
material-web implementation, so the export supplies every number and the
behaviour comes from Compose; the split is:

* **The container owns selection and navigation, the item owns its paint.**
  `MdList` carries the roving tab stop, the arrow / Home / End keys, the wrap
  policy (`wrapNavigation ?? true`, material-web's default) and the three
  selection modes; `MdListItem` only knows how to paint its own state. That is
  Compose's `selectable` overload split and material-web's `ListController`.
* **The line count is derived, never set** [compose] `ListItemType`: three when
  there is both an overline and a supporting text *or* the supporting text
  wraps (`isSupportingMultilineHeuristic`), two when there is either, one
  otherwise. The container height takes the 56 / 72 / 88 token floor, so a tall
  content stack grows past it.
* **The shape ladder is the behaviour**, not a token rewrite
  [compose] `ListItemShapes.shapeForInteraction`: pressed > dragged > selected
  > focused > hovered > base. `MdListTokens::resolve` therefore reports the
  export *verbatim* even for the Standard variant; "Standard is square" is
  applied by `MdListItemStyle::shapeFor`, which short-circuits to
  `container.shape` (corner-none).
* **Two disabled shape rows, not one.** The export publishes both
  `disabled.container.expressive.shape` (corner-extra-small) and
  `selected.disabled.container.expressive.shape` (corner-large). Compose has no
  disabled branch at all — selection would win there — and lands on the same
  two values, which is a useful cross-check on the reading.
* **The selected interaction rows drop the icons to on-surface** while the label
  keeps on-secondary-container. That asymmetry is the export's, not a slip.
* **`segmentedShapes(index, count)`** [compose]: the first item's *top* corner
  pair and the last item's *bottom* pair take the list's own
  `container.shape`; a single item takes all four; middle items are untouched.
  A count of 1 is a real segment, not "not segmented".
* **The focus ring is inward and follows the container's radii.**
  `focusRingInset` returns 0: the ring is drawn *inside* the item, so a list
  needs no outside margin (contrast the card's 7.5 px outward ring). The
  thickness and the offset come from the component's own
  `md.comp.list.focus.indicator.*` rows; the published
  `focus.indicator.outline.offset` is a CSS `outline-offset` of -3 px, which
  with a 3 px outline is the inward variant's zero gap — derived in
  `MdListTokens::focusRingGap()` rather than hard-coded.

Divergences recorded rather than resolved:

* **The between-space is 12 px.** The export and Compose both say 12
  (`ItemBetweenSpace`); material-web's `_list-item.scss` hardcodes `gap: 16px`
  and loses, two sources against one hardcode.
* **The trailing element's right padding is the token's 16 px**, not the spec's
  24 dp. The spec value is recorded; the token wins.
* **The container's 8 px vertical padding is not a token row** — it is
  `padding: 8px 0` in `list/internal/_list.scss`. Carried as a value so a theme
  can retune it; the export publishes no key to override it through, so
  `MdListContainerTokens::resolve` deliberately does not look one up.
* **The dragged level-4 elevation is carried but not painted.** A list item's
  container *is* its whole widget rect, so a shadow drawn around it is clipped
  away by Qt, and a parent-side shadow would be covered by the neighbouring
  items' opaque containers. Compose renders a dragged item in an overlay above
  the list; `MdList` lays items out in place, so the dragged state is expressed
  through its shape row and its state layer instead. A future overlay host can
  turn the row on without touching the tokens.
* **The focus ring follows the container radii**, where material-web hardcodes
  `md-focus-ring { shape: 8px }`. Compose and the spec both describe a ring that
  follows the component's shape, and this port's item has real per-state radii,
  so the hardcode is the one that loses.
* **material-web fades the whole disabled item** (`opacity: 0.38`), where the
  export publishes per-element opacities and a container colour only for the
  *selected* family. Compose is a third reading and has no `selectedDisabled`
  rows at all — its `ListItemColors.containerColor` takes
  `!enabled -> disabledContainerColor`, and `disabledContainerColor` is the
  *plain* container, so a disabled-and-selected item would lose its
  secondary-container entirely. This port follows the export (and lands where
  material-web does): a disabled selected item keeps the selected container and
  composites on-surface at 0.38 over it, while a disabled *unselected* item's
  container is untouched.
* **Resting and hovered items show no shape** — the container colour is
  `surface`, and a surface container on a surface list has no visible outline.
  This is true of material-web as well; the gallery page says so explicitly,
  because it is easy to misread as a missing feature.

#### App bars (ported)

`MdTopAppBar` + `MdBottomAppBar` + their two styles + `MdAppBarTokens` +
`MdAppBarScrollBehavior`, locked by `TestMd3AppBar` and by the page-28 pixel
audit. material-web ships **no production top app bar** — only a catalog stub
and an experimental `labs/gb/components/appbar/` — so this family follows the
Divider split: the export supplies every number, Compose `AppBar.kt` /
`AppBarDsl.kt` supplies the layout and the scroll state machine, and the spec
page supplies the taxonomy. Compose's `AppBar*Tokens.kt` were cross-checked
against the export and agree row for row, deprecated rows included.

* **Seven spec entries, five layouts.** "Center-aligned" is documented as "Use
  centered-text configuration", so it is `MdAppBarAlignment`; the search app
  bar is the configuration whose *centre* is a search field, so it is the
  `centerWidget` slot. No sixth and seventh classes.
* **Baseline medium and large are deprecated as designs but still published.**
  The spec says so outright ("No subtitle support on the legacy app bar") and
  points at the flexible replacements. Both are still ported and still
  rendered: a port that could not draw a 2024 app bar would not be a port.
  Their `subtitle.font` rows are carried verbatim and never reachable —
  `supportsSubtitle()` is false for them.
* **The 16 dp edge distance is not a token row.** It is the published
  `leading-space` (4) plus the 12 px an icon button brings itself, so
  `titleInset()` is `edgeSpace − leadingSpace` = 12 and a theme that retunes
  `leading-space` moves the inset rather than the edge distance. The 24 px
  medium title bottom and 28 px large title bottom are likewise Compose
  constants with no token row.
* **`collapsedRowHeight` is read from the small set for every variant.** That
  is Compose's own arithmetic (`MediumAppBarCollapsedHeight` and
  `LargeAppBarCollapsedHeight` are both `AppBarSmallTokens.ContainerHeight`),
  and it is what makes a medium bar lose 48 px and a large bar 88 rather than
  collapsing to nothing. A *single-row* bar is the opposite case: its whole
  height is collapsible, so it slides off the screen.
* **The colour transition is a step on one row and a ramp on two.** Compose:
  `overlappedFraction > 0.01f` asks for the scrolled colour outright on a
  single-row bar (the 0 → 1 is the `animateColorAsState` spring), while a
  two-row bar reads `collapsedFraction` continuously — "changes color at the
  same rate the app bar expands or collapse". Both are reproduced.
* **The transition fraction is eased, and the interpolation is Oklab.**
  `containerColor(f) = lerp(container, scrolled, FastOutLinearInEasing(f))`,
  and Compose's colour lerp goes through `Color.VectorConverter`, i.e. Oklab.
  `MdColorMath::lerpOklab` is new for this: it is the first *animated colour*
  in the library, so it is the first place the interpolation space is
  observable. Interpolating in sRGB would land on a different midpoint.
* **`consumeScroll` reduces Compose's two-hook protocol to one entry point.**
  Compose divides the work between `onPreScroll` (Pinned takes nothing,
  EnterAlways takes everything, ExitUntilCollapsed takes the collapsing
  direction only — "Don't intercept if scrolling down") and `onPostScroll`.
  A Qt host has no nested-scroll protocol to plug into, so both hooks live
  behind `consumeScroll(dy, contentAtStart)`. The over-consumption is
  deliberate and matches Compose: a non-pinned mode answers with the **whole**
  delta as soon as the bar moves at all, not with the part it absorbed.
* **A container lays out *containers*, not widgets — and that needed a shared
  helper.** Every qt-md3 component that can show a focus indicator reserves the
  ring's room *inside* itself, because Qt clips a child to its own rectangle and
  the indicator is an outward one. `MdIconButton` is therefore a 55 × 55 widget
  around a 40 × 40 container and `MdFab` a 71 × 71 one around a 56 × 56, both
  with an exact 7.5 px margin on every side (`offset 2 + activeWidth 8 / 2 +
  width 3 / 2`). `MdButtonGroup` was the first component to have to place such a
  child at a token position and established the rule — *place the container, not
  the widget, and accept that two neighbouring widget rects then overlap by
  `2 * margin − gap`* — doing the arithmetic inline in its own layout. The app
  bars are the second and third, so the rule is now factored out as
  **`styles/MdChildBox.h`**: `measure()` sizes a child and reports where it
  paints, `geometryOn()` returns the geometry that puts that box on a target.

  Measured on the gallery page, before → after: the bottom bar's first icon
  draws 23 px from its edge → **14** (4 content padding + 8 icon padding + the
  glyph's own inset), and its three icons sit 55 px apart → **40**; the top
  bar's navigation draws 23 px in → **15**, its title starts at 63 → **48**
  (`max(12, 4 + 40) + 4`), and its two action buttons sit 55 px apart → **40**
  with the last container flush against the published 4 px trailing inset. Both
  bars' first revisions had the bug; the page-28 pixel audit is what caught it.

  The dispatch inside `measure()` is a closed list of the components that have a
  `containerRect()` — the eight a slot can realistically hold. **Add a component
  there when it grows one**, or a container will silently lay it out by its
  `sizeHint` again.
* **Neighbouring children's widget rects overlap, deliberately.** Two 40 px icon
  buttons whose containers touch are two 55 px widgets overlapping by 15 px. The
  overhang is transparent — a container draws nothing outside itself — and the
  alternative is worse: reserving the margin as real spacing moves the visible
  icons 15 px apart from what the tokens say. Compose does not have the problem
  at all, because a focus indicator there is an overlay drawn outside the layout
  bounds rather than something the layout has to make room for. The one
  observable consequence is that the *input* region of a child is its widget,
  not its container, so a click in the 7.5 px band either side of a shared edge
  goes to the child that is later in the stacking order. Recorded rather than
  hidden; the alternative is a parent-side mouse router, which is a library-wide
  decision and not one family's to make.
* **The bottom app bar's FAB is placed by its container, not by its widget.**
  The default medium FAB is a 71 × 71 widget wrapped around a 56 × 56 container
  at (7.5, 7.5), so laying the widget out by its `sizeHint` parked the painted
  disc 25 px from the trailing edge instead of 16 and 20 px from the top instead
  of 12. The first revision did exactly that and the page-28 pixel audit caught
  it; `MdChildBox` plus a resize-before-measure is the fix. The residual
  half-pixel (the widget's position is integral, the offset is not) is the
  floor, not a shortcut. `TestMd3AppBar` grew a *real*-`MdFab` case for it,
  because the layout test's fixed-size probe could not reach it.
* **RTL is not mirrored.** Both widgets handle `LayoutDirectionChange` and
  relayout, and both used to carry a comment claiming the styles "read logical
  edges" — they do not: the layouts place physical ones, so an RTL app bar keeps
  its navigation on the left where Compose's `placeRelative` would move it
  right. `MdTheme::isRightToLeft()` is honoured by `MdButtonStyle`,
  `MdButtonGroupStyle`, `MdSegmentedButtonStyle` and `MdSplitButtonStyle` and by
  nothing else in the library, so this is a library-wide gap rather than an app
  bar defect. The misleading comments are gone and the gap is recorded here; the
  coverage row's `主题` cell stays `🚧` partly because of it.
* **The bottom app bar's content band has no bottom padding.** The export's
  `ContentPadding` is `start 4 / top 4 / end 4`; the row's contents therefore
  sit 2 px below the container's true centre and the band runs to the bottom
  edge. Reproduced, not corrected.
* **`container.elevation` (level 2) is carried and not painted** — a bottom
  app bar's shadow falls *above* its own rect, which Qt clips away for a
  widget laid out in place. Same call the list family made for its dragged
  elevation. Compose draws it because a Compose layout does not clip.
* **The search field is not this family's.** Every published `search.*` row is
  resolved (56 px, corner-full, `search.label.color` on-surface-variant,
  container surface-container rising to surface-container-highest on scroll,
  8 px leading and trailing, body-large label), but the field itself is a text
  field and this library has no text field yet. Gallery page 28 puts an
  `MdButton` in the centre slot to show the slot's geometry and says so in the
  page copy; the field belongs to ★ Text fields.
* **`FlexibleBottomAppBar` is ★ Toolbars, not App bars.** Compose's flexible
  bottom bar reads `DockedToolbarTokens` — `FlexibleContentPadding` and
  `FlexibleBottomAppBarHeight` are both docked-toolbar rows — and material-web
  ships `_md-comp-toolbar-docked.scss` for it. Noted here so the next family
  claims its own component rather than inheriting it by accident.
* **Docs gap noticed while writing this entry**: `docs/project-status.md`'s
  "What exists today" component table still stops at `MdDialogHost`, so the
  §1.4 families ported after the Dialog (bottom sheets, side sheets, carousel,
  Divider, Lists) have module rows missing there even though their coverage
  rows and porting notes are present. Left unfixed rather than back-filled
  from memory; the App bars rows were appended.

#### Toolbars (ported)

`MdDockedToolbar` + `MdFloatingToolbar` + `MdDockedToolbarStyle` +
`MdFloatingToolbarStyle` + `MdDockedToolbarTokens` + `MdFloatingToolbarTokens`,
locked by `TestMd3Toolbar` and by the page-29 pixel audit. material-web
implements **neither** variant — the spec's own availability table says
`Web: Unavailable`, and the export's five `_md-comp-toolbar-*.scss` files
(docked 36 rows, floating 75, floating-fab 51, standard 87, vibrant 87) are the
only place either exists there. So, as with Divider and the app bars, the export
supplies every number and Compose supplies the behaviour — and for this family
that means *two* different behaviour sources, because the two variants have
nothing in common structurally:

* **Docked → `AppBar.kt`'s `FlexibleBottomAppBar`.** Compose has no
  `DockedToolbar` composable; `DockedToolbarTokens` is read by
  `FlexibleBottomAppBar` for its height (64), its content padding (16 / 16) and
  its arrangement (`Arrangement.spacedBy(ContainerMaxSpacing /* 32 */,
  Alignment.CenterHorizontally)`). That composable *is* the docked toolbar, so
  this port models it as one — which also settles the note the App bars entry
  left behind ("`FlexibleBottomAppBar` is ★ Toolbars, not App bars").
* **Floating → `FloatingToolbar.kt`.** Its `HorizontalFloatingToolbar` and
  `VerticalFloatingToolbar` are the two published layouts; the file is 2069
  lines and covers five private layouts between them.

Twelve divergences and gaps were found and are recorded rather than smoothed:

* **The reserved action-button strip uses the *expanded* size.** Compose's
  `Layout` reports `width = toolbarMaxWidth + toolbarToFabGap + FabSizeRange.
  start` and never revisits it, so a collapsed toolbar's 80 px button is placed
  at `width - 80`, which is 16 px *inside* `width - 64`. The overlap is real and
  is reproduced; `TestMd3Toolbar` asserts it.
* **`MdFab` cannot take the 80 px size.** The floating toolbar's action button
  is 56 px expanded and 80 px collapsed with different icons (24 / 28) and
  corners (`corner-large` / `corner-large-increased`), and `setFab()` does ask
  the child for that size — `MdChildBox::resizedGeometryOn` resizes the widget
  and centres it on the token box. But `MdFabStyle::layoutFor` derives its
  container from `MdFabTokens::containerWidth/Height` rather than from the
  widget's own rect, so an `MdFab` handed in stays 56 px, centred in the 80 px
  box, with a 24 px icon and the wrong corner. The toolbar's geometry is
  correct; what is missing is a size set on the FAB family. A plain widget
  fills the box correctly, and `TestMd3Toolbar` asserts both halves.
* **`container.elevation` is published and unused.** The export carries
  `md.comp.toolbar.floating.container.elevation: level3`; Compose's
  `FloatingToolbarDefaults.ContainerExpandedElevation` is
  `ElevationTokens.Level0` with a literal `// TODO read from token`. This port
  paints the published row, ramped by `expandedProgress` so a collapsed pill
  casts nothing — which is Compose's own behaviour applied to the export's own
  value. Compose's *with-FAB* constants (`level1` expanded, `level0` collapsed,
  no token rows behind either) are recorded, not copied.
* **Two published rows are carried and never read.** `docked.container.
  min-spacing` (4): Compose's arrangement reads `max` and only `max`.
  `floating.container.height`: Compose's `ContainerSize` still points at the
  deprecated single row while the export has superseded it with
  `horizontal.container.height` / `vertical.container.width`, which is what
  `containerCrossExtent()` returns. All three are 64, so nothing observable
  differs until a theme moves one — which is what makes silently dropping the
  row the wrong call.
* **The selected group publishes colours but no opacities and no disabled
  row.** Both are gaps in the export rather than transcription slips: none of
  the four scss files carries a `selected.*.state-layer.opacity`, and the
  `disabled` rows exist only unselected. The port carries the unselected
  opacities across and uses the unselected disabled colours for
  `selected[Disabled]`, which is where Compose's `!enabled -> disabled`
  precedence lands anyway.
* **Qt clips a child to its widget, Compose clips it to the pill.** Compose's
  floating toolbar wraps its content in `graphicsLayer { clip = true; shape =
  shape }`, so a collapsing pill progressively clips the slots inside it. In Qt
  the pill is a shape drawn *inside* the widget, and a child widget is clipped
  to the widget's rectangle, so an unclipped child would float over the page
  behind the pill. `MdFloatingToolbar::placeChildren()` therefore **hides** a
  child whose container has left the pill: the same reveal, discretised per
  item instead of per pixel. Gallery page 29's "expanded and collapsed" bands
  are the evidence.
* **`floating.container.between-space` is read here and nowhere in Compose.** The
  export publishes the row (4 px) and so does Compose — but
  `FloatingToolbarTokens.ContainerBetweenSpace` is referenced nowhere in
  `FloatingToolbar.kt`, because a floating toolbar's items are arranged by the
  *caller's* `Row` and upstream ships no spacing at all. `MdFloatingToolbar`
  owns the arrangement of its children, so it applies the row: a run of `n`
  slots is `8 + n * item + (n - 1) * 4 + 8` long. This one was a **real defect
  for a whole round** — the row was resolved, carried and asserted by
  `floatingTokenTable` while `MdFloatingToolbarStyle::layoutFor` advanced its
  cursor by each container and by nothing else, so a three-item pill measured
  136 px where the export's arithmetic says 144. Found by the page-29 pixel
  audit, not by reasoning, and the lesson is in the entry's own shape: a token
  test cannot see a row that nothing reads.
  `TestMd3Toolbar::betweenSpaceSeparatesTheSlots` now pins the override through
  to `sizeHint()` so it cannot happen again.
* **The pill's level3 shadow has nowhere to land when there is no action
  button.** `container.elevation` is resolved and painted
  (`MdElevation::drawShadowDp`, ramped by `expandedProgress`), but a floating
  toolbar without a FAB has a widget rectangle *identical* to its pill, and Qt
  clips painting to the widget — so the shadow is invisible: sampling 8 px below
  the standard pill's edge returns the page background exactly. With a FAB the
  widget is 80 px across against the pill's 64, and there the shadow does show
  in the 8 px of slack (rows 1549…1553 under the pill's rounded edge measure a
  152 → 233 grey falloff). The row stays painted because it lands wherever the
  widget has room; the same Qt clip is why the lists' drag shadow is carried
  rather than drawn.
* **Qt clips the docked toolbar's row where Compose overflows it.**
  `BottomAppBarLayout` ends with
  `layout(placeable.width, height.roundToInt()) { placeable.place(0, 0) }`, so at
  `heightOffset = -32` the row keeps y = 0 inside a 32 px box — the glyphs are
  *not* re-centred, and page 29 shows their upper slivers (9 ink rows where the
  expanded bar has 20). A Compose app would additionally paint the overflow over
  whatever is behind the bar, since `Modifier.layout` reports a smaller height
  without an implicit clip; a Qt child cannot be painted outside its widget at
  all, so this port cuts instead of overflowing. The *position* is Compose's; the
  *cut* is Qt's, and it is the docked half of the same divergence the floating
  pill's hidden slots carry.
* **A toolbar has no item size, and this library has no 48 px icon button.**
  Neither the 36 `docked` nor the 75 `floating` exported rows contains an item
  height or width, so the pill's length is its children's — every toolbar
  advances by whatever container its child publishes, which is also what
  Compose's intrinsic measurement does. The spec's composition is
  `8 + 48 + 8 = 64`, i.e. a 48 px item in the 48 px band, but
  `MdIconButtonTokens`' ladder is the Expressive one (32 / 40 / 56 / 96 / 136:
  xsmall, small, medium, large, xlarge) and has no 48, so page 29 uses the 40 px
  default and every pill is 8 px narrower per item than the spec's arithmetic.
  Closing this is an `MdIconButton` question (a size set with a 48, or an
  interactive-size concept beside the container), not a toolbar one — recorded
  here because it is the toolbar that shows the difference. It is the icon-button
  family's sibling of the `MdFab` 80 px gap above.
* **RTL is not mirrored**, as everywhere else in this library:
  `MdTheme::isRightToLeft()` reaches only the four button families, and neither
  toolbar reads it. A toolbar is the largest `placeRelative`-positioned
  component in the set, so the gap is proportionally the most visible here.
  `TestMd3Toolbar::rtlIsNotMirrored` pins the current behaviour so a future RTL
  pass has to change it deliberately.
* **`MdToolbarFabPosition` is one enum, not two.** Compose splits the concept
  into `FloatingToolbarHorizontalFabPosition {Start, End}` and
  `FloatingToolbarVerticalFabPosition {Top, Bottom}`; both are the same two
  places on the main axis and both defaults are the axis' end, so the port has
  a single `{Start, End}` enum documented as the main axis.

One shared primitive came out of this family:
`MdChildBox::resizedGeometryOn()` — `geometryOn()`'s sibling for the one child
whose size is *not* its own. It sizes the widget to `box + 2 * margin` and
**centres** it on the box, rather than aligning top-lefts, because that is the
one rule that lands the container on the box for both a component that fills
`widget - margin` and one whose container is a fixed token size centred inside a
larger widget (which is exactly what `MdFab` is).

#### Navigation bar, rail and drawer (ported)

`MdNavigationBar` + `MdNavigationBarItem` + `MdNavigationBarTokens` +
`MdNavigationBarStyle` + `MdNavigationBarItemStyle`, `MdNavigationRail` +
`MdNavigationRailTokens` + `MdNavigationRailStyle`, `MdNavigationDrawer` +
`MdNavigationDrawerItem` + `MdNavigationDrawerTokens` + `MdNavigationDrawerStyle`
+ `MdNavigationDrawerItemStyle`, locked by `TestMd3Navigation` (38 slots) and by
the page 30-32 gallery shots. material-web implements none of the three as a
product — `labs/navigationbar`, `labs/navigationdrawer` and
`labs/gb/components/navbar` are the only code there — so the export supplies
every number and Compose supplies the behaviour, from *five* files this time
(`NavigationBar.kt`, `ShortNavigationBar.kt`, `NavigationItem.kt`,
`NavigationRail.kt` / `WideNavigationRail.kt`, `NavigationDrawer.kt`).

**Navigation is the family where "one component = one token family" breaks
open.** The bar ships two live families in one export version (34.0.21) —
`md.comp.navigation-bar.*` and `md.comp.nav-bar.*` + items — the rail the same,
and the drawer one. The spec says the flexible bar and rail replace the baseline
ones and the expanded rail replaces the drawer; nothing was removed. Both
bar/rail generations ride one `variant`; the judgment method (read the
implementation body, not the token file — migration residue lies) is in
`AGENTS.md` and memory. The pinned divergences and gaps:

* **Compose's own token file lies about the bar height.**
  `NavigationBarTokens.ContainerHeight = 64` sits under a `// TODO`, while the
  behaviour reads `NavigationBarHeight = NavigationBarTokens.TallContainerHeight`
  (80) and the export and the spec agree on 80. 80 it is — the first source in
  this library whose *own tokens* were overruled by its own code.
* **The baseline pill is 64 wide, Compose's is 56.** Two sources publish 64;
  Compose's baseline reuses the flexible family's 56. The pill paddings are
  derived (`(64 - 24) / 2 = 20`), which is why the baseline bar's selected pill
  looks wider than Compose's screenshot of it.
* **The item gap is 8, not the export's `0px`.** Compose hard-codes
  `Arrangement.spacedBy(8.dp)`; a gap between items is behaviour, and behaviour
  wins. Same shape as the toolbar's `between-space` call, opposite direction.
* **The bar's level2 elevation is carried and not painted** — the spec's
  "Differences from M2: no shadow". The drawer's modal level1 and the rail's
  modal level2 fall outside a child widget's rect and are carried the same way.
* **The drawer's scrim is carried, not painted.** `scrim-color:
  neutral-variant20` / `scrim-opacity: 0.4` describe an overlay over the
  drawer's *parent*, which a child widget cannot paint. `scrimColor()` /
  `scrimOpacity()` expose them for a host. Divergence recorded: the library's
  `ColorRole::Scrim` resolves from `neutral0` and there are no per-component
  colour rows, so the semantic role is what is carried, not the export's tone.
* **The drawer's item is deliberately not the shared expressive item.**
  Compose's `NavigationDrawerItem` is independent because the geometry differs
  in kind — the pill IS the item, a full-width 56 px row whose container colour
  is the selected state, no width animation, a badge slot. Its colour table
  diverges from the bar's in three pinned places: every active row
  `on-secondary-container`, the inactive *pressed* state layer the one-row
  special case (`on-secondary-container` where hover and focus read
  `on-surface`), and `label-large`. The badge is a text (the
  `large-badge-label-*` rows are what the export publishes); Compose's badge is
  an arbitrary composable slot.
* **The rail's items are the bar's.** Compose says outright that
  `WideNavigationRailItem` and `ShortNavigationBarItem` wrap the one
  `NavigationItem` composable, so the rail pushes its family's item rows into
  `MdNavigationBarItem`. Three rows join the shared item for the rail's sake:
  the horizontal item's 8 px icon-label gap (`horizontalIconLabelSpace`), the
  baseline's 56x56 no-label pill (`noLabelIndicatorHeight`), and the expanded
  item's `label-large` (`horizontalLabelTextType`).
* **Expanded is a state, not a variant.** The flexible rail has one container
  with two widths — 96 collapsed, content-driven 220-360 expanded (the widest
  item plus 20 px of trailing room, clamped) — and expanding flips every item
  Top/Start with no interpolation, which is Compose's own boolean. The baseline
  rail publishes no expanded rows and refuses `setExpanded(true)`.
* **Four spacing rows are Compose hard-codes.** The baseline rail publishes no
  spacing rows at all; Compose's `NavigationRail.kt` hard-codes 4 between items,
  4 of item vertical padding and 8 after the header. The drawer's content row
  (`start 16 / icon 24 / gap 12 / end 24`) and `ItemPadding` (horizontal 12,
  where 336 = 360 - 24 comes from) are hard-codes too, and the drawer's
  headline-to-content 12 is Compose *sample* spacing — none are token rows, all
  are recorded in the token headers as the behaviour's numbers.
* **`corner-large-end` has no enum.** Directional shapes are carried as the
  base radius with the pair placement done in the style's `Layout.radii`
  (end pair rounded, leading edge square, mirrored in RTL) — the bottom
  sheet's `corner-extra-large-top` precedent. The drawer is also the one
  Navigation container that does re-read the layout direction (its item content
  rows swap leading/trailing); the containers themselves are still not
  `placeRelative`-mirrored, the library-wide RTL gap.
* **A layout function must never measure.** Found by the rail tests:
  `MdNavigationRailStyle::layoutFor` runs on the *paint* path, and
  `MdChildBox::measure` resizes its subject — so a layout that measured would
  drag every item back to its hint on each repaint and leave it there. The
  rule now: layout reads `sizeHint()`; `measure` only ever appears in a
  placement pass where every measure is immediately followed by its
  `setGeometry`. Written into `AGENTS.md`-adjacent memory and pinned by
  `TestMd3Navigation::railFlexibleExpands`.
* **Two struct-default and width-source defects the tests flushed out.** A
  multi-family token struct must write every family's literals explicitly
  (the flexible bar inherited the struct's baseline defaults and silently
  resolved 80 px tall), and a component's `boxes()` must place trailing
  content against the content width, not the widget width, if the same
  arithmetic answers `sizeHint()` on an unsized widget (the drawer item's
  badge landed at a 640 x 480 top-level's trailing edge).

#### Tabs (ported)

`MdTabs` + `MdTab` + `MdTabsTokens` + `MdTabsStyle` + `MdTabStyle`, locked by
`TestMd3Tabs` (21 slots) and by the page 33 gallery shot. material-web
implements neither family as a product, so the export supplies every number
(`_md-comp-primary-navigation-tab.scss` and `-secondary-navigation-tab.scss`)
and Compose supplies the behaviour (`TabRow.kt`, `Tab.kt`) — with Flutter's M3
defaults (generated from the same token database, `tabs.dart`'s
`_TabsPrimaryDefaultsM3` / `_TabsSecondaryDefaultsM3`) as the third vote. The
pinned divergences and gaps:

* **The icon+label height: 64, not Compose's hard-coded 72.** The export and
  the spec page agree on `with-icon-and-label-text.container.height: 64px`,
  and Compose *declares* `IconAndLabelTextContainerHeight = 64` — but its
  `Tab.kt` `LargeTabHeight` hard-codes `72.dp` with no TODO, an M2-era
  residue. Three sources beat one hard-coded number; 64 it is.
* **The icon+label content block is centred, not baseline-aligned.** Compose
  aligns the pair on the text baseline (`SingleLineTextBaselineWithIcon` 14,
  `IconDistanceFromBaseline` 20sp); Qt has no cross-widget baseline
  alignment, so the icon + 4 + label block is vertically centred in the 64.
  With a single-line `title-small` label Compose's own arithmetic lands the
  block within a few pixels of centre. Recorded, not silently merged.
* **The secondary indicator is 2, not the 3 Compose renders.** The secondary
  export publishes `active-indicator-height: 2px`; Compose's
  `SecondaryNavigationTabTokens` declares no height row, so its
  `SecondaryIndicator` falls back to the *primary's* 3 through a default
  parameter. The export's own row wins.
* **The fixed row's indicator centring is missing upstream.** Compose's
  `ScrollableTabRowImpl` places the indicator at
  `max(0, (tabWidth - indicatorWidth) / 2)` explicitly; its fixed
  `TabRowImpl` places it at the tab's start with no centring step, which
  would left-align a content-width indicator inside an equal-share tab and
  contradict every official rendering. Flutter's M3 defaults
  (`TabBarIndicatorSize.label` primary / `.tab` secondary) centre both.
  This port centres both; the Compose omission is recorded.
* **No motion rows and no disabled rows.** The indicator's offset/width ride
  the spatial default spring and the content colours cross-fade on the
  effects ones (in `EffectsDefault`, out `EffectsFast` — `TabTransition`'s
  own `if (false isTransitioningTo true)` split), all behaviour, labelled as
  such. Disabled content is the system 0.38 alpha, as in the navigation
  families.
* **The keyboard walk is a convention, not a published row.** Compose's row
  is a `selectableGroup` and its tests walk Left/Right; the spec page spells
  nothing out. `MdTabs` implements Left/Right (wrapping), Home/End.
* **A child paints over its parent: the tab's ripple covers the indicator's
  3 px during a press.** Compose places the indicator on top of the tabs; a
  Qt child widget always draws above its parent's `paintEvent`. The overlap
  is a transient press-state edge; recorded rather than re-architected.
* **The divider rows are deprecated but painted.** Compose's default tab row
  still paints a `HorizontalDivider`; the export's `divider.*` rows are
  carried (`surface-variant`, 1 px) and painted.
* **RTL: not mirrored.** Same library-wide gap as every Navigation family —
  the row, its scroll and its indicator keep the LTR placement under an RTL
  layout direction.

#### Checkbox (ported)

`MdCheckBox` + `MdCheckBoxTokens` + `MdCheckBoxStyle`, locked by
`TestMd3CheckBox` (24 slots) and by the page 34 gallery shot. The export
(`_md-comp-checkbox.scss`) supplies every number; Compose's `Checkbox.kt`
supplies the behaviour, read along its `isCheckboxStylingFixEnabled` path (the
fix is current). The pinned divergences and gaps:

* **Compose's unchecked ripple is transparent — an upstream oversight.** With
  the styling fix on, `CheckboxColors.indicatorColor(Off)` returns
  `uncheckedBoxColor`, which the default colours set to `Color.Transparent`,
  so an unchecked checkbox would ripple invisibly. The export's state-layer
  rows win: an unchecked press ripples `primary` (the row the box is about to
  earn), a checked press `on-surface`, an error press `error`.
* **The deprecated rows are carried, not read.** `unselected.*.icon.color`,
  `disabled.selected.icon.*` and `disabled.unselected.icon.*` predate the
  rendering rework ("Checkbox changed how rendering was specified"); the
  current model colours the check from `selected.icon.color` even at rest and
  folds the 0.38 into the container-opacity rows. The unselected checkmark
  table is filled `on-surface` for the record; nothing reads it (no glyph is
  painted when unchecked). The error `outline-width` rows are marked
  "redundant" upstream — the base width rows carry the same numbers, so they
  are recorded here as comments rather than table columns.
* **The error variant publishes no disabled rows.** `error.*` rows cover the
  enabled interactions only; a disabled error box falls back to the base
  disabled rows through the `boxFor`/`outlineFor`/`checkmarkFor`/
  `stateLayerFor` accessors.
* **The focus ring follows the component's radii, not Compose's 25 %.** The
  export publishes no `focus-indicator.shape` row, so material-web's rule
  applies (the ring's radii follow the box's, offset by the gap — 4 px on a
  2 px box). Compose overrides with `RoundedCornerShape(25)` ≈ 5.5 px on the
  ring's 22 px box. The difference is under 2 px at this size; the
  `focusRingRadiusPercent` row is carried for the record.
* **The touch target is the whole widget; Qt's native one is not.** Qt 6.9's
  `QCheckBox::hitButton` defers to the platform style's indicator rect — with
  no text it puts the clickable sliver at the left edge and the 48 px target
  is dead everywhere else. `MdCheckBox` overrides `hitButton` to the whole
  rect (the MD3 target is the whole 48 px).
* **The size holds while disabled.** Compose applies
  `minimumInteractiveComponentSize` only when `onClick != null`, so a disabled
  checkbox shrinks to the 18 px canvas and reshuffles the layout around it.
  This port keeps 48 px in both states for layout stability; recorded, not
  silently merged.
* **The hover/focus colour changes do not animate.** Compose wraps every
  resolved colour in `animateColorAsState`, so a hover's outline lift from
  `on-surface-variant` to `on-surface` also fades. This port animates the
  selection fade (the effects springs) and applies interaction-state colours
  directly — the same standing as the tab's colour fade, and visually
  negligible at these deltas.
* **The ripple's geometry is bounded where Compose's is unbounded.** Compose's
  checkbox ripple is `bounded = false` with a fixed radius of
  `StateLayerSize / 2`; this port runs the standard bounded ripple in the 40 px
  state-layer bounds, clipped to its circle — the growth curve past the clip
  is invisible, so the visible result matches.
* **Text is not painted.** MD3's checkbox has no label; Compose's `Checkbox`
  has none either. Qt's `QCheckBox` carries a `text` property that this widget
  ignores — a label is the caller's widget, as in the gallery.
* **RTL: not mirrored.** Nothing to mirror in the box itself, but the shared
  focus-ring and state-layer paths are not RTL-audited — the library-wide gap.

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

The app bar page then exposed a fourth defect, in the screenshot hook itself: it
sampled a page after a single `processEvents()`, so anything that animates
towards its resting state on first show — the app bar's scroll colour springing
from surface to surface-container, a progress indicator's indeterminate track, a
sheet sliding in — was documented at the *start* of its animation. `settleAnimations()`
now pumps the event loop for a bounded budget (400 ms; the longest of those
springs, stiffness 1600 and critically damped, settles inside 200 ms) before each
grab. Qt's timers are wall-clock based, which is why the short sleep between
pumps is what actually advances them. Without it, page 28's "scrolled" sample
bar would have been captured as surface — i.e. indistinguishable from the
"at rest" bar it is meant to contrast with.

A fifth followed from the same page, and is the reason the example grew a
`--language` option. The gallery's copy is written in both languages
(`L("中文", "English")`), but a screenshot of the *Chinese* copy on this machine
is a wall of tofu boxes: the bundled font set carries no CJK coverage and
neither does the offscreen platform plugin's fallback. The page still rendered,
so the pixel audit could read its geometry, but no human could audit it. The
hook can now be asked for `--language en`, which is how every measurement on
this page was read. While producing that screenshot the app bar page turned out
to have a real localisation bug of its own — three of its row tables carried an
`…Zh` / `…En` pair and read only the Chinese one, so the 136 dp and 152 dp
flexible rows drew their subtitle as four tofu boxes in English mode. Fixed with
a `copyOrEmpty()` helper that returns an empty string when either copy is
absent, rather than falling back to the wrong language. No other page had the
pattern.

## Stage 2 — Qt extensions

Not started. Full list: appendix D of
[md3-qt-porting-prompts.md](md3-qt-porting-prompts.md). Blocked by the
`TestMd3CoveragePolicy` gate until Stage 1 is green.
