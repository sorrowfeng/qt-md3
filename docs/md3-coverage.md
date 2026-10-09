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
| ★ Toolbars | MdDockedToolbar / MdFloatingToolbar | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Navigation bar | MdNavigationBar + MdNavigationBarItem | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Navigation rail | MdNavigationRail | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Navigation drawer | MdNavigationDrawer + MdNavigationDrawerItem | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
| Tabs | MdTabs / MdTab | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |

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

### Toolbars — why the class name in the table changed, and the eight notes

The reserved name was one class, `MdToolbar`. It is now two —
`MdDockedToolbar` and `MdFloatingToolbar` — because the published component is
two: the spec's variant table lists exactly `Docked` and `Floating`, Compose
ships `HorizontalFloatingToolbar`/`VerticalFloatingToolbar` and **no** docked
component at all, and the two have mutually exclusive token sets (`corner-none`
vs `corner-full`, 16 px padding vs 8, no elevation row vs `level3`). The two
`floating` *configurations* — orientation and colour scheme — stay enums on the
one floating class, because the export's horizontal and vertical rows are exact
transposes that differ by a single number, and because both dimensions are
published per orientation there is no third class to make.

The `🚧` columns are the same unfinished evidence the App bars carry (seed
change, contrast level, density, font switch and the side-by-side comparison
have not been run against this page) plus the library-wide RTL gap: a toolbar is
large, `placeRelative`-positioned and entirely unmirrored, and this family
records that rather than claiming otherwise.

Eight transcription notes are pinned by `TestMd3Toolbar` rather than smoothed:

* **material-web implements neither variant** — the availability table says
  `Web: Unavailable`, and the export's five `_md-comp-toolbar-*.scss` files are
  the only place either exists there. As with Divider, the numbers come from the
  export and the behaviour from Compose — which for this family means *two*
  different sources: `FloatingToolbar.kt` for the floating toolbar and
  `AppBar.kt`'s `FlexibleBottomAppBar` for the docked one;
* **the docked toolbar has no Compose component** — it is realised as
  `FlexibleBottomAppBar` with `DockedToolbarTokens` substituted for the bottom
  app bar's rows. That composable *is* the docked toolbar, which is why this
  port models it as one and stops treating `FlexibleBottomAppBar` as a member of
  the App bars family;
* **the docked toolbar collapses its whole height** — `BottomAppBarLayout` sets
  `heightOffsetLimit = -placeable.height`, so `MdDockedToolbar::heightOffsetLimit()`
  is `-64`, the opposite of the two-row app bar rule above. It has no
  `arrangement` property either, because its row is `Arrangement.spacedBy(
  ContainerMaxSpacing /* 32 */, Alignment.CenterHorizontally)` and there is
  nothing for a caller to set;
* **two published rows are carried and never read** — `docked.container.min-spacing`
  (Compose reads `max` and only `max`) and `floating.container.height` (Compose's
  `ContainerSize` still points at the deprecated single row, while the export has
  superseded it with the horizontal/vertical pair that `containerCrossExtent()`
  returns). Both are recorded, not silently dropped;
* **`container.between-space` is the one row this port reads and Compose does
  not** — `FloatingToolbarTokens.ContainerBetweenSpace` is declared and
  referenced nowhere in `FloatingToolbar.kt`, because upstream's toolbar items
  are arranged by the *caller's* `Row` and Compose therefore ships no spacing of
  its own. `MdFloatingToolbar` owns the arrangement of its children, so it
  applies the published 4 px: a run of `n` slots is
  `8 + n * item + (n - 1) * 4 + 8` long. The gap was found by the page-29 pixel
  audit — a three-item pill measured 136 px where the export's own arithmetic
  says 144 — and had been resolved and asserted for a whole round while the
  layout ignored it, which is why
  `TestMd3Toolbar::betweenSpaceSeparatesTheSlots` now pins the override all the
  way through to the widget's `sizeHint()`;
* **the leading and trailing slots exist only while the toolbar is expanded** —
  Compose wraps each in `AnimatedVisibility(visible = expandedState)`, so at
  `expandedProgress == 0` they leave the layout and the content re-centres in the
  band they vacated. The pill is measured at `maxIntrinsicWidth * progress`, so
  the *widget's* bounds never move while the pill inside them shortens — the
  page-29 band that is blank at progress 0 is the state, not a bug;
* **the adjacent action button is two size sets** — 56 px with `corner-large`
  and `level1` expanded, 80 px with `corner-large-increased` and `level2`
  collapsed, so it *grows* as the toolbar shrinks, and the reserved strip is the
  expanded 56 even when the button is 80. `MdFab` publishes one size, so an
  `MdFab` handed to `setFab()` stays 56 px centred in the 80 px box: the
  toolbar's geometry is right and the gap is the FAB family's. Recorded in
  [porting-todo.md](porting-todo.md);
* **`container.elevation` is `level3` in the export and unused in Compose** —
  Compose's own constants are `ElevationTokens.Level0` with a "TODO read from
  token". This port paints the published row and ramps it with
  `expandedProgress`, which is Compose's own behaviour, so a collapsed pill
  casts nothing; the with-FAB constants Compose uses instead (`level1` expanded,
  `level0` collapsed, no token rows at all) are recorded rather than copied.

### Navigation bar, rail, drawer — one export, two coexisting families (and one that is not)

Navigation is the second family (after Toolbars) where the "one component =
one token family" assumption breaks, and the first where the break is
**two live families in one export version**. The bar ships
`md.comp.navigation-bar.*` (80 px tall, a 64x32 pill, the selected label
`on-surface`) *and* `md.comp.nav-bar.*` + `nav-bar-item-{vertical,horizontal}`
(64 px, 56x32, the selected label `secondary`) at the same 34.0.21; the rail
does the same (`navigation-rail` vs `nav-rail-collapsed` / `-expanded` /
`nav-rail` / `nav-rail-item*`); the drawer ships only one family. The spec's
own words are that the flexible bar and rail *replace* the baseline ones and
that the **expanded navigation rail replaces the navigation drawer** — but
nothing was removed, and Compose keeps both generations as separate classes
(`NavigationBar` / `ShortNavigationBar`, `NavigationRail` /
`WideNavigationRail`). So the port carries both under one `variant` per
component, and `TestMd3Navigation` pins both token tables in one test file.

The judgment method — read the Compose implementation body, not just its
token file, because migration residue lies — is recorded in
`AGENTS.md`/memory: Compose's `NavigationBarTokens.ContainerHeight = 64` sits
under a `// TODO` while the behaviour reads `TallContainerHeight` (80), and
the export and the spec agree on 80. Four divergences are pinned for the bar:

* **the 80 height** — export + behaviour body + spec beat the TODO'd token
  row;
* **the baseline pill is 64 wide, not 56** — two sources publish 64; Compose's
  baseline reuses the flexible family's 56;
* **the item gap is 8, not the export's `0px`** — Compose's hard-coded
  `spacedBy(8.dp)`; a gap between items is behaviour, and behaviour wins;
* **the level2 elevation is carried and not painted** — the spec's "no
  shadow", same grounds as the bottom app bar's.

The rail's rows are where the two sources actually meet: the flexible rail's
export tables match Compose's `WideNavigationRail*Tokens` line for line, and
the items are *the bar's* — Compose says outright that
`WideNavigationRailItem` and `ShortNavigationBarItem` wrap the one
`NavigationItem` composable, so the port pushes the rail's item rows into the
shared `MdNavigationBarItem` (three rows join for the rail's sake: the
horizontal item's 8 px icon-label gap, the baseline's 56x56 no-label pill,
the expanded item's `label-large`). Expanded is a **state**, not a variant:
one container, 96 collapsed and content-driven 220–360 expanded, the items
flipping Top/Start at the edge; the baseline family publishes no expanded
rows and refuses `setExpanded(true)`.

The drawer is the deliberate exception: Compose's `NavigationDrawerItem` is
*not* a wrapper over the shared item, because the geometry differs in kind —
the pill **is** the item, a full-width 56 px row whose container colour is
the selected state, no width animation, a badge slot — and the export's
colour table diverges from the bar's in three pinned places (every active row
`on-secondary-container`; the inactive pressed state layer is the one-row
special case `on-secondary-container` where hover and focus read
`on-surface`; the label `label-large`). So it gets its own
`MdNavigationDrawerItem`. Three of its rows are carried and not painted: the
scrim (a window overlay a child widget cannot cover — `scrimColor()` /
`scrimOpacity()` are on the tokens for a host; the export's
`neutral-variant20` is recorded against the library's `neutral0` Scrim
role), the modal level1 elevation, and the large-badge rows (a text is what
they describe; Compose's badge is an arbitrary composable slot).

The `🚧` columns carry the same unfinished evidence as every other family
(seed change, contrast level, density, font switch and the side-by-side
comparison have not been run against pages 30–32), plus the library-wide RTL
gap: the drawer's `corner-large-end` mirrors its radii with the layout
direction and its item content rows re-read it, but the containers are not
`placeRelative`-mirrored — recorded, not claimed.

### Tabs — the two families' indicators, and the three pinned divergences

Tabs is the last Navigation family, and it is a two-family export like the
bar and the rail: `md.comp.primary-navigation-tab.*` and
`md.comp.secondary-navigation-tab.*` at the same 34.0.21, differing in
indicator height (3 vs 2), indicator shape (rounded on top vs square), the
indicator's width semantics (the selected tab's *content* width vs the whole
tab), and the colour tables (primary keeps `active.*` / `inactive.*` rows
where the inactive **pressed** layer is `primary`; secondary publishes one
shared `on-surface` table). material-web ships neither family as a component,
so the numbers come from the export and the behaviour from Compose's
`TabRow.kt` / `Tab.kt` — with Flutter's M3 defaults (generated from the same
token database) as the third vote where the two disagree.

Four transcription notes are pinned by `TestMd3Tabs` rather than smoothed:

* **the icon+label container height is 64, not 72** — the export, the spec
  page and Compose's own `PrimaryNavigationTabTokens.IconAndLabelText-
  ContainerHeight` all say 64, while `Tab.kt`'s `LargeTabHeight` hard-codes
  `72.dp` in the behaviour (an M2-era residue with no TODO). Three sources
  against one hard-coded number: 64 wins, and the tab's content block is
  centred (Qt has no cross-widget baseline alignment; Compose's baseline
  arithmetic lands within a few pixels of centre — recorded, not silently
  merged);
* **the secondary indicator is 2, not 3** — the secondary export publishes
  `active-indicator-height: 2px`, but Compose's
  `SecondaryNavigationTabTokens` declares no height row at all, so its
  `SecondaryIndicator` borrows the *primary's* 3 through a default parameter.
  The export's own row wins;
* **the indicator centres in the tab** — Compose's scrollable row places the
  indicator at `max(0, (tabWidth - indicatorWidth) / 2)` explicitly, but its
  fixed `TabRowImpl` places it at the tab's start with no centring step.
  Flutter's M3 defaults (`TabBarIndicatorSize.label` for the primary family
  and `.tab` for the secondary) centre both, and a left-aligned content-width
  indicator contradicts every official rendering. The centring wins; the
  fixed-row omission is recorded in porting-todo.md;
* **no motion rows and no disabled rows** — the indicator's offset and width
  animate on the spatial default spring and the content colours cross-fade on
  the effects ones (in `EffectsDefault`, out `EffectsFast` — Compose's
  `TabTransition`), all labelled as behaviour rather than tokens; the
  disabled content is the system 0.38 alpha over the unselected colour, the
  same rule the navigation families apply. The press ripple's colour is the
  *active* side's pressed colour — Compose builds
  `ripple(color = selectedContentColor)` "because we want to show the color
  before the item is considered selected" — which is the primary family's
  `inactive.pressed.state-layer.color = primary` one-row special case.

The `🚧` columns carry the same unfinished evidence as every other family
(seed change, contrast level, density, font switch and the side-by-side
comparison have not been run against page 33), plus the library-wide RTL gap:
the row is not `placeRelative`-mirrored — recorded, not claimed.

### Checkbox — one export, one deprecated rendering model, and Compose's transparent ripple

The checkbox is the first Selection family, and the first whose export carries
a **deprecated rendering model inside the same version**: `unselected.*.icon.*`
and `disabled.*.icon.*` rows still publish, marked "Checkbox changed how
rendering was specified" — the current model colours the check from
`selected.icon.color` and folds the 0.38 into the *container* opacity rows.
The rows are carried into the tables for the record and read by nothing.

Five transcription notes are pinned by `TestMd3CheckBox` rather than smoothed:

* **the state-layer special cases** — `unselected.pressed.state-layer.color`
  is `primary` (the colour the box is about to earn) and
  `selected.pressed.state-layer.color` is `on-surface`; the error variant
  presses `error`. The same shape of special case the tabs' inactive pressed
  layer carries.
* **Compose's unchecked ripple is a bug.** With the styling fix on,
  `indicatorColor(Off)` returns the *transparent* unchecked box fill — an
  unchecked checkbox would ripple invisibly. The export's state-layer rows
  win; the divergence is recorded in porting-todo.md.
* **indeterminate is a selected state with a gravitation of its own.** The
  dash is the check path lerped onto the centre line (Compose's
  `crossCenterGravitation`), not a second glyph: `Off → Indeterminate` snaps
  the gravitation (the dash draws in from nothing), `On ↔ Indeterminate`
  springs the morph, and anything → `Off` holds the old visual for the 100 ms
  `SnapAnimationDelay` and then snaps it away.
* **the check proportions follow the styling fix** — 0.25/0.5 → 0.4/0.65 →
  0.75/0.3, revealed along the path's length (Compose's
  `pathMeasure.getSegment`).
* **the focus ring is the *outward* variant** — `md.sys.state.focus-indicator`
  outer offset 2, thickness 3, secondary, around the 18 px box. The export
  publishes no shape row, so material-web's rule applies (the ring follows the
  box's radii); Compose overrides with a 25 % rounded rect — a difference of
  under 2 px at this size, recorded.

The `🚧` columns carry the same unfinished evidence as every other family,
plus the library-wide RTL gap: a checkbox has nothing to mirror, but the
focus-ring and state-layer code paths are shared with families that do.

## 1.6 Selection

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Checkbox | MdCheckBox | ✅ | ✅ | ✅ | ✅ | ✅ | 🚧 | ✅ | ✅ | 🚧 |
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
