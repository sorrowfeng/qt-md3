# MD3 Coverage Matrix

> **This file is the single source of truth for the Stage 1 → Stage 2 switch.**
> Stage 2 (Qt extension components) must not begin until every Stage 1 family is
> green across all nine columns. The `TestMd3CoveragePolicy` CTest gate parses
> this file and fails the build if a Stage 2 public component appears early.

Legend: `✅` = done · `⬜` = not started · `🚧` = in progress

Column meaning (nine check columns):

| Column | Meaning |
| --- | --- |
| 变体 | Every official variant / size / shape / color style is present |
| 状态 | Every interaction state and its state-layer opacity |
| 属性 | Every documented property has a matching API (`Q_PROPERTY` + NOTIFY) |
| token | Every `md.comp.*` token is wired up |
| 动效 | Easing / duration, or spring parameters, match the spec |
| 主题 | Light / dark, seed change, contrast level, density, RTL, font switch |
| 示例页 | A dedicated page + left-nav entry in the example app |
| 测试 | Definition-of-Done item 8 (properties, signals, lifecycle, render smoke) |
| 视觉审计 | Side-by-side screenshot confirmed as `Pass` in `visual-audit.md` |

`★` = family added by M3 Expressive.

> **Spec note.** The bootstrap brief says the matrix has nine check columns, but
> its appendix E enumerates only eight (it omits theme coverage). This file adds
> `主题` as the ninth column, because Definition-of-Done item 5 is otherwise not
> represented. If the intent was a different ninth axis, change the column header
> here and the column count in `tests/TestMd3CoveragePolicy.cmake` together.

## 1.2 Actions

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Buttons | MdButton | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| ★ Button groups | MdButtonGroup | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Icon buttons | MdIconButton | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| FABs | MdFab | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Extended FABs | MdExtendedFab | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| ★ FAB menu | MdFabMenu | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| ★ Split buttons | MdSplitButton | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Segmented buttons | MdSegmentedButton | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |

### Buttons — the two rows that are still `🚧`

`主题` is in progress because light/dark and RTL are handled and verified while
seed change, contrast level, density and font switch have not yet been exercised
*against the button page*. `视觉审计` is in progress because the page has been
rendered in both modes and read against the published token values, but the
side-by-side reference comparison that `visual-audit.md` asks for has not been
recorded. Neither is a known defect; both are unfinished evidence.

Two upstream inconsistencies are deliberately preserved rather than reconciled,
and are pinned by `TestMd3Button` so they cannot drift silently:

* `md.comp.button.leading-space` is 24 px while `md.comp.button.small.leading-space`
  is 16 px, at the same 40 px height. The size-qualified token wins.
* `md.comp.button.<style>.hovered.container.elevation` is published but marked
  `@deprecated No longer part of the design spec`. It is carried in the token
  table and not painted.

See `porting-todo.md` for the `v0_192` versus `latest` export differences this
component had to resolve.

### Button groups — the two rows that are still `🚧`

Same standing as Buttons: light/dark and RTL are handled and verified, while
seed change, contrast level, density and font switch have not yet been exercised
*against the button-group page*, and the side-by-side reference comparison that
`visual-audit.md` asks for has not been recorded. Neither is a known defect;
both are unfinished evidence.

### Icon buttons — the two rows that are still `🚧`

Same standing as the first two families: light/dark and RTL are handled, while
seed change, contrast level, density and font switch have not yet been
exercised *against the icon-button page*, and the side-by-side reference
comparison has not been recorded. Neither is a known defect; both are
unfinished evidence.

Two transcription notes are pinned by `TestMd3IconButton` rather than smoothed:

* the selected colour families fall back to the plain family where the export
  publishes no `selected-*` slot (standard has no container in either state;
  outlined has no `selected.outline.color`, so the checked toggle is a filled
  inverse-surface chip and not a stroked one);
* the selected *shapes* are the published other knob — `selected-container.
  shape.round` is the square-ish corner and `.square` is full — which is the
  token form of the spec's "selected shape changes between square and round".

One upstream inconsistency is preserved rather than reconciled, pinned by
`TestMd3ButtonGroup`:

* `md.comp.button-group.connected.xsmall.inner-corner.corner-size` resolves to
  `corner-small` (8 px) in the `latest` export, while the spec page
  (`m3.material.io/components/button-groups/specs`) lists 4 px for extra small.
  The export wins, because tokens are the source the resolver reads; the
  disagreement is recorded here and in the `MdButtonGroupTokens.h` header
  comment rather than smoothed over.
* The square *connected outer* corner is a spec-page-only fact (the export
  publishes `container.shape: corner-full` for every connected size), so
  `ButtonGroupShape::Square` keeps the round outer corner and squares only the
  items' own shape tokens. Recorded; not silently merged.

### FABs — the two rows that are still `🚧`

Same standing as the other Actions families: light/dark and RTL are handled,
while seed change, contrast level, density and font switch have not yet been
exercised *against the FAB page*, and the side-by-side reference comparison has
not been recorded. Neither is a known defect; both are unfinished evidence.

Two transcription notes are pinned by `TestMd3Fab` rather than smoothed:

* the token export publishes **no disabled rows** — the disabled values
  (on-surface @12% container, on-surface @38% icon, level0) come from the
  official spec page's disabled state table, the same row every button family
  shows;
* no pressed container shape is published, so unlike the button/icon-button
  families a FAB does not morph on press — the ripple is the whole press
  response, which is also what the export's "Pressed (ripple)" row labels say.

### Extended FABs — the two rows that are still `🚧`

Same standing as the other Actions families: light/dark and RTL are handled,
while seed change, contrast level, density and font switch have not yet been
exercised *against the extended-FAB page*, and the side-by-side reference
comparison has not been recorded. Neither is a known defect; both are
unfinished evidence.

Three transcription notes are pinned by `TestMd3ExtendedFab` rather than
smoothed:

* the export ships **no surface colour set** for this family — six sets
  (primary / secondary / tertiary plus three `*-container`), unlike the FAB's
  four including surface. The spec page agrees, so it is a family difference,
  not an export gap;
* the base (unprefixed) metric set — 56 px, label-large, 16/12/20 rhythm —
  matches no size table: `small` is also 56 px but title-medium with a
  16/8/16 rhythm. The three explicit size tables win (56/80/96 px); the base
  rows are the legacy standard-M3 default;
* the disabled row is filled from the spec's state table exactly as the FAB
  family does (the export publishes no disabled rows), and the *deprecated*
  focus rows pointing at `*-container` roles are ignored in favour of the
  non-deprecated `focused-*` rows.

### FAB menu — the two rows that are still `🚧`

Same standing as the other Actions families: light/dark and RTL are handled,
while seed change, contrast level, density and font switch have not yet been
exercised *against the FAB-menu page*, and the side-by-side reference
comparison has not been recorded. Neither is a known defect; both are
unfinished evidence.

Three transcription notes are pinned by `TestMd3FabMenu` rather than
smoothed:

* **the export publishes no motion rows for this family** — the spec page
  says only "the FAB menu animates from the top trailing edge of the FAB".
  The reveal runs on the Compose M3 Expressive `SpatialDefault` spring with
  a 40 ms per-item stagger and a 24 px settle; that is a
  sourced-from-Compose convention, not a token fact, and is labelled as
  such on the gallery page.
* **no focus-indicator rows of its own** — the close button and items carry
  focused state-layer colours but no `focus.indicator.*` rows, so the
  indicator falls back to the shared `md-sys-state-focus-indicator` values
  (secondary, 3 px, offset 2).
* **spec-page structural facts the export cannot express** — up to six
  items, the close button always 56 dp sharing the FAB's top trailing
  corner, a recommended 4 dp FAB-to-menu gap on the web, and "inherits its
  states and specs from the baseline menu". All recorded; the menu caps
  nothing (adding a seventh item is allowed and simply beyond spec).

### Split buttons — the two rows that are still `🚧`

Same standing as the other Actions families: light/dark and RTL are handled,
while seed change, contrast level, density and font switch have not yet been
exercised *against the split-button page*, and the side-by-side reference
comparison has not been recorded. Neither is a known defect; both are
unfinished evidence.

Four transcription notes are pinned by `TestMd3SplitButton` rather than
smoothed:

* **the export publishes metric rows only** — no colour, state, focus or
  typography rows exist for `md.comp.split-button.*`. The spec page fills the
  gap in one sentence ("Split buttons use the same color schemes as standard
  buttons"), so the colour / state / focus / type rows are the button
  family's at the identity size mapping — the two Expressive scales agree on
  every height (32 / 40 / 56 / 96 / 136);
* **the inner-corner morph is the family's own behaviour** — facing corners
  rest at 4/4/4/8/12 px and spring to 8/12/12/20/20 px on hover *and* press;
  the trailing half's facing corners go to the literal 50% while selected;
  outer corners stay corner-full at every size;
* **no motion rows** — the morph runs on the button family's press spring
  (1400 / 0.9), the Compose Expressive choice, labelled as such;
* **the `arrow_drop_down` default has no glyph in the bundled classic
  49-icon set** — the gallery page sets `expand_more` explicitly; recorded
  as an icon-set gap, not a token divergence.

### Segmented buttons — the two rows that are still `🚧`

Same standing as the other Actions families: light/dark and RTL are handled,
while seed change, contrast level, density and font switch have not yet been
exercised *against the segmented-button page*, and the side-by-side reference
comparison has not been recorded. Neither is a known defect; both are
unfinished evidence.

Four transcription notes are pinned by `TestMd3SegmentedButton` rather than
smoothed:

* **the export is exactly one set** — `md.comp.outlined-segmented-button.*`
  publishes no size scale and no colour variants (40 px, label-large,
  outline 1 px, corner-full, icon 18, secondary-container selection);
* **the divider between segments is the shared stroke** — the row overlaps
  neighbours by exactly the outline width (Compose
  `Arrangement.spacedBy(-BorderWidth)`), and `itemShape` rounds only the
  first segment's inline-start corners and the last segment's inline-end
  corners; middle segments are rectangles;
* **the icon slot is always reserved** (18 px + 8 px spacing, Compose
  measure policy; the export publishes neither row) so the label never
  moves when the check scales in on selection — the scale-in runs on
  spring-fast-spatial, the spring Compose uses for it;
* **the export's pressed-state-layer opacity is the focus one** (as
  published — `pressed-state-layer-opacity: focus-state-layer-opacity`), and
  a disabled *selected* segment keeps its secondary-container fill (the
  export has no disabled-container row; Compose's disabledActive uses
  SelectedContainerColor too).

## 1.3 Communication

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Badges | MdBadge + MdBadgedBox | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Progress indicators | MdProgressIndicator | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| ★ Loading indicator | MdLoadingIndicator | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Snackbar | MdSnackbar + MdSnackbarHost | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Tooltips | MdTooltip + MdTooltipHost | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |

### Badges — the two rows that are still `🚧`

Same standing as the Actions families: light/dark and RTL are handled, while
seed change, contrast level, density and font switch have not yet been
exercised *against the badge page*, and the side-by-side reference comparison
has not been recorded. Neither is a known defect; both are unfinished evidence.

Four transcription notes are pinned by `TestMd3Badge` rather than smoothed:

* **material-web does not implement the Badge component** — there is no
  `packages/badge`. The token file
  `tokens/versions/latest/sass/_md-comp-badge.scss` is the only material-web
  fact (dot 6 px, large minimum 16 px, both corner-full, error / on-error,
  label-small text); every behaviour row is annotated `[compose]` and comes
  from androidx `Badge.kt`;
* **the export publishes no state rows at all** — a badge is not interactive:
  no hover, no press, no focus indicator, no disabled row. The widget contract
  is `Qt::NoFocus` plus `WA_TransparentForMouseEvents`, which is also what
  lets an anchored badge pass clicks through to its content (Compose: not
  wrapping the badge in a Surface "because it blocks touch propagation");
* **the anchoring offsets are Compose facts** — the dot sits on the anchor's
  top-end corner (offset 6/6), the pill's start edge 12 px inside the anchor's
  end edge and its bottom edge 14 px below the anchor's top edge, with 4 px
  label side padding;
* **one Qt adaptation, recorded rather than hidden**: Compose lets the pill
  overlap the surroundings freely, but a Qt child widget is clipped to its
  parent, so `MdBadgedBox` reserves the overhang (pill width − 12 /
  height − 14) in its own geometry — the painted result is identical, only
  the surrounding layout sees a slightly larger box.

### Progress indicators — the two rows that are still `🚧`

Same standing as the other families: light/dark and RTL are handled (the
linear indicator mirrors itself under RTL, the CSS `scale(-1)` contract),
while seed change, contrast level, density and font switch have not yet been
exercised *against the progress-indicator page*, and the side-by-side
reference comparison has not been recorded. Neither is a known defect; both
are unfinished evidence.

Five transcription notes are pinned by `TestMd3ProgressIndicator` rather than
smoothed:

* **the export is one merged family** — `md.comp.progress-indicator.*` with
  linear and circular segments; the two per-shape sets
  (`_md-comp-linear-progress-indicator` / `_md-comp-circular-progress-
  indicator`) are deprecated as of 34.0.21 and survive only as the source of
  the four-color rows (identical in both: primary, primary-container,
  tertiary, tertiary-container);
* **the `thick.*` rows are deprecated as a variant** ("no longer tokenized
  as a variant, but rather a sample configuration in code") — transcribed
  into the token struct for completeness, never exposed as an API;
* **the indeterminate animations are the MDC-heritage keyframes** the
  material-web internal SCSS ships: the linear two-bar 2 s cycle (translate
  0→200.611 %, scale 0.08→0.661479→0.08, per-segment beziers) and the
  circular three composed rotations (expand-arc 1333 ms, 265°↔130°;
  group-arc 8×135° over 5332 ms; a linear spin of ARCTIME×360/306 ms, right
  half delayed half a period) — pinned as *pure functions of elapsed time*;
* **the four-color cycle interpolates colours, not indices** — CSS animates
  `background`/`border-color` smoothly between keyframe marks (0/15/25/40/
  50/65/75/90 %), linear-timed for the linear shape and riding the
  indeterminate easing per segment for the circular one;
* **the registered gap: the wave rows.** The export publishes non-deprecated
  Expressive wave tokens (linear amplitude 3 / wavelength 40, indeterminate
  wavelength 20, with-wave height 10; circular amplitude 1.6 / wavelength
  15, with-wave size 48). Neither material-web (which predates Expressive)
  nor this port renders them; the token struct carries every value, and
  Compose's `WavyLinearProgressIndicator` / `WavyCircularProgressIndicator`
  are the porting source recorded in porting-todo.md.

## 1.4 Containment

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Cards | MdCard | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Dialogs | MdDialog + MdDialogHost | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Bottom sheets | MdBottomSheet + MdBottomSheetHost | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Side sheets | MdSideSheet + MdSideSheetHost | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Carousel | MdCarousel | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Divider | MdDivider | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Lists | MdList / MdListItem | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |

## 1.5 Navigation

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| App bars | MdTopAppBar / MdBottomAppBar | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| ★ Toolbars | MdToolbar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation bar | MdNavigationBar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation rail | MdNavigationRail | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation drawer | MdNavigationDrawer | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Tabs | MdTabs / MdTab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

### App bars — the two rows that are still `🚧`

Same standing as the Actions families, and for the same reasons: seed change,
contrast level, density and font switch have not yet been exercised *against the
app bar page*, and the side-by-side reference comparison has not been recorded.
Neither is a known defect; both are unfinished evidence. **RTL is a third gap,
and it is the tracked cross-cutting one**: `MdTheme::isRightToLeft()` is honoured
by the four button families and by nothing else, so an RTL app bar keeps its
navigation on the left rather than mirroring the way Compose's `placeRelative`
does. The app bar is simply the first family to say so out loud instead of
claiming the mirroring; see "RTL 全组件" under Cross-cutting items and the entry
in [porting-todo.md](porting-todo.md).

Six transcription notes are pinned by `TestMd3AppBar` and the page-28 pixel
audit rather than smoothed:

* **material-web has no production top app bar** — the component directory
  holds only a catalog stub (`catalog/src/components/top-app-bar.ts`) and an
  experimental `labs/gb/components/appbar/`. As with Divider, the numbers come
  from the export and the *layout* comes from Compose `AppBar.kt` /
  `AppBarDsl.kt` plus the spec page;
* **seven spec entries, five layouts** — the m3.material.io variant table
  lists seven, but "Center-aligned" is documented as "Use centered-text
  configuration" (so it is `MdAppBarAlignment`, not a class) and the search
  app bar is the configuration whose *centre* is a search field (so it is the
  centre slot). The baseline medium and large bars are deprecated **as
  designs** — "No subtitle support on the legacy app bar" — but remain
  published token sets and remain rendered;
* **the 16 dp edge distance is not a token row** — it is the published
  `leading-space` (4) plus the 12 px an icon button brings itself, which is
  why `titleInset()` is 16 − 4 and why retuning `leading-space` moves the
  inset, not the edge distance;
* **a two-row bar collapses its text row only** — `heightOffsetLimit` comes
  from `collapsedRowHeight`, which is read from the **small** size set for
  every variant, so medium loses 48 px and large 88 and the 64 px icon row
  never leaves the screen. A *single-row* bar's whole height is collapsible,
  i.e. it slides off instead;
* **the colour transition is eased and Oklab-interpolated** — the fraction
  runs through `FastOutLinearInEasing` (`cubic-bezier(0.4, 0, 1, 1)`, which
  is *below* linear at its midpoint: 0.32481 at x = 0.5) and then through
  `MdColorMath::lerpOklab`, because Compose's `Color.VectorConverter`
  interpolates in Oklab. The page-28 audit samples the half-collapsed medium
  bar and measures `#faf4fc`, which is the eased fraction's result and not
  the naive 0.5 average (`#f9f2fb` in sRGB, `#f8f2fb` in Oklab) — an
  end-to-end check of the whole chain;
* **the search field is not this family's** — every published `search.*` row
  is resolved (56 px, corner-full, surface-container rising to
  surface-container-highest on scroll, 8 px leading/trailing, body-large
  label), but the field itself is a text field, and this library has no text
  field yet. Gallery page 28 puts an `MdButton` in the centre slot to show the
  slot's geometry and says so. Recorded in [porting-todo.md](porting-todo.md);
* **a container lays out *containers*, not widgets** — the app bars are the
  first family since `MdButtonGroup` to place a caller's child at a token
  position, and every such child reserves its focus ring's room inside itself
  (7.5 px a side), so `sizeHint()` is the wrong thing to lay out by. The rule
  `MdButtonGroup` established is now factored out as `styles/MdChildBox.h` and
  used by both bars: the nav container sits its own 4 px from the edge (not
  11.5), the title clears it by the published 12 px inset (not 27), and two
  action buttons sit 40 px apart (not 55). Neighbouring widget rects overlap by
  `2 * 7.5` as a consequence, which is deliberate and audited in
  [porting-todo.md](porting-todo.md).

## 1.6 Selection

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Checkbox | MdCheckBox | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Chips | MdChip | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Date pickers | MdDatePicker | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Menus | MdMenu / MdMenuItem | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Radio button | MdRadioButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Sliders | MdSlider | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Switch | MdSwitch | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Time pickers | MdTimePicker | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

## 1.7 Text inputs

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Text fields | MdTextField / MdTextArea | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Search | MdSearchBar / MdSearchView | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

## Cross-cutting items

Every family must satisfy these before Stage 1 counts as complete.

| 横切能力 | 状态 |
| --- | --- |
| emphasized type style 全组件接入 | ⬜ |
| spring 物理动效全组件接入 | ⬜ |
| 35 个装饰形状 + shape morph 接入 | ⬜ |
| Expressive 变体组件更新（app bars / carousel / buttons / FAB / icon buttons / navigation bar / navigation rail / progress indicators / sliders） | ⬜ |
| light / dark × contrast level（standard / medium / high） | ⬜ |
| RTL 全组件 | ⬜ |
| 键盘与焦点顺序全组件 | ⬜ |

## Summary

| Metric | Count |
| --- | --- |
| Stage 1 families | 36 |
| Check columns per family | 9 |
| Complete families (all nine columns ✅) | 0 |
| Stage 1 status | **In progress — not started** |

## Gate: `tests/TestMd3CoveragePolicy.cmake`

1. Parses this file and checks that all 36 families are present, with no missing
   or duplicated rows, and that each family row carries exactly nine status cells.
2. While Stage 1 is not green: fails if any Stage 2 public header (per appendix D
   of the porting brief) appears under `src/widgets/`, printing the offending
   paths.
3. Allows a small, explicit, hard-coded whitelist for internal / example-level
   files; nothing is allowed implicitly.
4. Once Stage 1 is green, this rule relaxes automatically and instead reports
   Stage 2 progress.

Do not mark a column ✅ until it is genuinely complete. A missing variant recorded
as done is treated as a defect, not a rounding error.
