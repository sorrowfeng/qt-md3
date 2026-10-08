# Visual Audit

Side-by-side comparison of the official Material Design 3 pages against the Qt
example app, per component and per state.

One component has been rendered and read; none has yet been compared against a
captured official screenshot.

## Method

1. Capture the official reference (not committed) into `build/ref/`:

   ```bash
   npx playwright screenshot --wait-for-timeout=4000 \
     --viewport-size "1280,900" \
     "https://m3.material.io/components/xxx/overview" \
     build/ref/<component>.png
   ```

   `m3.material.io` is a JS-rendered site; wait for rendering to finish, or read
   the DOM instead of taking a full-page screenshot.

2. Capture the Qt side (not committed) into `build/qt/`, covering light / dark and
   each state.

3. Build a side-by-side image (official left, Qt right) for light and dark.

4. Attribute each difference to the specific component across at least: token
   values, color-role mapping, shape-token radius, spacing / padding, font weight
   / size / line height, state-layer opacity, tonal elevation, shadow usage, icon
   axes, motion curve / spring, hit area, and missing variants.

5. Only fix the component itself; container and page-margin differences belong to
   the owning component's review.

## Status values

| Status | Meaning |
| --- | --- |
| `Pass` | Screenshot compared, no component-body difference |
| `Needs visual QA` | States covered but screenshots still pending |
| `Needs fix` | Component differences remain |
| `Blocked` | Cannot capture, or no official reference available |

## Matrix

| 组件族 | Light | Dark | 状态 | Notes |
| --- | --- | --- | --- | --- |
| Buttons | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Button groups | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Icon buttons | `Needs visual QA` | `Needs visual QA` | All | See below. |
| FABs | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Extended FABs | `Needs visual QA` | `Needs visual QA` | All | See below. |
| FAB menu | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Split buttons | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Segmented buttons | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Badges | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Progress indicators | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Loading indicators | `Needs visual QA` | `Needs visual QA` | All | See below. |

### Badges

Rendered via `qt-md3-example --screenshot` in light and dark modes (page
`16-badges`) and read against `tokens/versions/latest/sass/
_md-comp-badge.scss` plus androidx `Badge.kt` — material-web does not
implement the component.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Dot form | 6 px circle, error, corner-full, on the anchor's top-end corner | Yes |
| Content form | minimum 16 px pill, label-small on-error text, 4 px side padding, widening for "99+" | Yes |
| Anchoring | dot offset 6/6 [compose]; pill 12/14 with the overhang reserved by `MdBadgedBox` | Yes |
| Non-interactivity | no state layer, no ripple, no focus ring; clicks pass through | Yes (live) |
| Both modes | error / on-error track the scheme; the dot and pills stay legible on dark surfaces | Yes |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### Split buttons

Rendered via `qt-md3-example --screenshot` in light mode (page
`14-split-buttons`) and read against `tokens/versions/latest/sass/
_md-comp-split-button-<size>.scss` plus the spec page's colour rule.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Two halves + gap | leading content pill + 2 px between-space + trailing dropdown pill, outer corners at height/2 | Yes |
| Inner corners at rest | 4 / 4 / 4 / 8 / 12 px by size — nearly square facing edges | Yes |
| Colour rows | the button family's five variants (elevated / filled / tonal / outlined / text), selection adds a state layer only | Yes |
| Trailing selected | facing corners at the literal 50%, sealing the gap | Yes |
| Disabled | on-surface @ 0.12 container, @ 0.38 content (the button family's row) | Yes |
| Focus ring | one ring around the whole split, keyboard focus only | Yes (live) |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### FAB menu

Rendered via `qt-md3-example --screenshot` in light mode (page
`13-fab-menus`) and read against `tokens/versions/latest/sass/
_md-comp-fab-menu{,-<variant>-container,-<variant>-close-button}.scss`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Close button | 56 × 56, icon 20, corner-full, elevation L3 resting / L4 hovered | Yes (live) |
| List items | 56 tall, icon 24, 24/8/24 rhythm, corner-full, title-medium label, L0 throughout | Yes |
| Colour groups | close = pure primary / secondary / tertiary, items = the matching `*-container`, on-* content | Yes |
| Spacing | 8 px close-to-first-item, 4 px between items, right-aligned under the anchor corner | Yes |
| Open animation | staggered fade + 24 px settle, top-down, from the anchor corner | Yes (live; Compose-sourced motion, no export rows) |
| Anchor crossfade | FAB hides, close button appears in the same top trailing place | Yes (live) |
| Disabled | on-surface @ 0.12 container, @ 0.38 content, no shadow, no state layer | Yes |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### Segmented buttons

Rendered via `qt-md3-example --screenshot` in light mode (page
`15-segmented-buttons`) and read against
`_md-comp-outlined-segmented-button.scss` plus androidx `SegmentedButton.kt`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| One set | 40 px tall, label-large, 1 px outline, corner-full ends | Yes |
| Divider | neighbours overlap by the outline width; shared edge is a single 1 px stroke | Yes |
| itemShape | first rounds inline-start, last rounds inline-end, middle rectangles | Yes |
| Selection | secondary-container fill, on-secondary-container content, check scales in on a reserved 18 px slot | Yes |
| Multi choice | independent toggles, same geometry | Yes |
| Disabled | content @ 0.38, outline @ 0.12; a selected segment keeps its container fill | Yes |
| Focus ring | one ring around the focused segment, keyboard focus only | Yes (live) |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### Extended FABs

Rendered via `qt-md3-example --screenshot` in light mode (page
`12-extended-fabs`) and read against `tokens/versions/latest/sass/
_md-comp-extended-fab{,-<size>,-<variant>}.scss`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container heights | 56 / 80 / 96 px | Yes |
| Container corners | corner-large 16 · corner-large-increased 20 · corner-extra-large 28 | Yes |
| Icon sizes | 24 / 28 / 36 px | Yes |
| Label type scale | title-medium / title-large / headline-small, per size | Yes |
| Colour sets | primary / secondary / tertiary (on-* content) · primary/secondary/tertiary-container (on-*-container content); no surface set | Yes |
| Content-derived width | leading + icon + gap + label + trailing, per size rhythm (16/8/16 · 26/12/26 · 28/16/28) | Yes |
| Elevation shadow | raised L3 resting, L4 hovered; lowered L1 / L2, no lowered container colour | Yes (live) |
| Press response | ripple only — no pressed shape, no flat pressed layer | Yes (live) |
| Disabled | on-surface @ 0.12 container, @ 0.38 icon and label, no shadow, no state layer | Yes |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### FABs

Rendered via `qt-md3-example --screenshot` in light mode (page `11-fabs`) and
read against `tokens/versions/latest/sass/_md-comp-fab{,-<variant>,-<size>}.scss`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container sizes | 40 / 56 / 96 px | Yes |
| Container corners | corner-medium 12 · corner-large 16 · corner-extra-large 28 | Yes |
| Icon sizes | 24 / 24 / 36 px | Yes |
| Colour sets | surface-container-high + primary icon · primary · secondary · tertiary, on-* icons | Yes |
| Elevation shadow | raised L3 resting, L4 hovered; real layered shadow | Yes (live) |
| Lowered | L1 resting / L2 hovered; surface variant swaps to surface-container-low | Yes |
| Press response | ripple only — no pressed shape, no flat pressed layer | Yes (live) |
| Disabled | on-surface @ 0.12 container, @ 0.38 icon, no shadow, no state layer | Yes |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### Icon buttons

Rendered via `qt-md3-example --screenshot` in light mode (page
`10-icon-buttons`) and read against `tokens/versions/latest/sass/
_md-comp-icon-button{,-<style>,-<size>}.scss`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container heights | 32 / 40 / 56 / 96 / 136 px | Yes |
| Icon sizes | 20 / 24 / 24 / 32 / 40 px | Yes |
| Square default track | leading + icon + trailing == height at every size | Yes |
| Styles | standard bare glyph · filled primary chip · tonal secondary-container chip · outlined outline-variant stroke | Yes |
| Selected toggle (filled) | surface-container chip unchecked, primary checked | Yes |
| Selected corners | the knobs swap — selected round is the square-ish corner | Yes |
| Selected outlined | inverse-surface chip, stroke dropped | Yes |
| Padding tracks | default square · narrow · wide, same height | Yes |
| Disabled | container @ 0.1, icon @ 0.38, outlined keeps its stroke | Yes |
| Press morph | corners to the pressed shape on spring-fast-spatial | Yes (live) |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`.

### Button groups

Rendered via `qt-md3-example --screenshot` in light mode (page `09-button-groups`)
and read against `tokens/versions/latest/sass/_md-comp-button-group-{standard,
connected}-<size>.scss` plus the spec page captured at `build/ref/bg-specs.txt`.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container heights | 32 / 40 / 56 / 96 / 136 px | Yes |
| Standard between-space | 18 / 12 / 8 / 8 / 8 px | Yes |
| Connected between-space | 2 px at every size | Yes |
| Connected outer corners | full at every size | Yes |
| Connected inner corners | small / extra-small / small / large / large-increased | Yes |
| Selected inner corner (connected) | 50 % of the cross extent — the segment ends read as a pill | Yes |
| Press growth (standard) | item width +15 %, neighbours shifted, spring not easing | Yes (live) |
| Selection modes | none keeps nothing, single wraps, multiple toggles independently, required refuses to empty | Yes (live) |
| Vertical orientation | column with a uniform cross extent that grows past the token height when a label needs it | Yes |
| Item colours | none of the group's own — every colour belongs to the items | Yes (no paint filter exists) |

Same standing as Buttons: the official reference screenshots have not been
captured side by side, so this stays `Needs visual QA` rather than `Pass`.

### Buttons

Rendered via `qt-md3-example --screenshot` in both modes and read against the
values in `tokens/versions/latest/sass/_md-comp-button{,-<style>,-<size>}.scss`.
The `-full.png` output is the one that matters: it is the page grown to its own
`heightForWidth()`, so it shows all 32 buttons rather than the first third.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container heights | 32 / 40 / 56 / 96 / 136 px | Yes |
| Icon box | 20 / 20 / 24 / 32 / 40 px | Yes |
| Leading and trailing padding | 12 / 16 / 24 / 48 / 64 px | Yes |
| Outline width | 1 / 1 / 1 / 2 / 3 px | Yes |
| Type style | label-large ×2, title-medium, headline-small, headline-large | Yes |
| Round shape | corner-full at every size | Yes |
| Square shape | 12 / 12 / 16 / 28 / 28 px | Yes |
| Elevated | surface-container-low + primary, level 1 | Yes |
| Filled | primary + on-primary | Yes |
| Tonal | secondary-container + on-secondary-container | Yes |
| Outlined | no container, outline-variant stroke, on-surface-variant label | Yes |
| Text | no container, no stroke, primary label | Yes |
| Disabled | on-surface @ 0.1 container, @ 0.38 label and icon; outlined keeps its stroke | Yes |

Not yet done, and the reason this is `Needs visual QA` rather than `Pass`: the
official reference screenshots have not been captured, so the comparison is
against the *token files* rather than against the rendered official component.
Differences that a token file cannot reveal — optical spacing, how the icon
sits on the label baseline, whether the pressed shape reads as the intended
morph — are therefore unverified.

Known and deliberate: the button container is inset 7.5 px inside the widget, so
the widget's `sizeHint()` is 15 px taller and wider than the token container.
That is `md.comp.button.focus.indicator`'s outward geometry, which Qt would clip
at the widget edge otherwise. It is a containment difference, not a component
difference, and it is why a raw widget-size comparison against the official
component will always show +15 px in both axes.

### Progress indicators

Rendered via `qt-md3-example --screenshot` in light and dark modes (page
`17-progress-indicators`) and read against
`_md-comp-progress-indicator{,-linear,-circular}.scss` plus the
MDC-heritage keyframes in the material-web internal SCSS.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Linear determinate | 4 px, primary bar on a secondary-container track, both corner-full | Yes |
| Gap + stop indicator | 4 px gap then the 4 px round dot, trailing-space 0; both vanish at value 1 | Yes |
| Linear buffer | track scaled to the buffer fraction, scrolling 2 px dots beyond it | Yes |
| Linear indeterminate | full track + the two-bar MDC 2 s keyframes (screenshot catches one frame; bars sweep 0→200.611 %) | Yes |
| Circular determinate | 40 px, 4 px stroke, arc from 12 o'clock clockwise, track full circle | Yes |
| Circular indeterminate | the three composed rotations — two half-plane arcs expanding/rotating | Yes (frame-checked against the pure functions) |
| Four-color | primary → primary-container → tertiary → tertiary-container, interpolated | Yes (both shapes) |
| Non-interactivity | no state layer, no ripple, no focus ring | Yes (live) |
| Both modes | colours track the scheme | Yes |

Same standing as the other families: no side-by-side official reference yet,
so this stays `Needs visual QA` rather than `Pass`. The Expressive wave
rendering is the family's registered gap (see porting-todo.md) and is called
out on the page itself.

### Loading indicators

Rendered via `qt-md3-example --screenshot` in light and dark modes (page
`18-loading-indicators`) and read against
`_md-comp-loading-indicator.scss` plus the Compose M3 Expressive
`LoadingIndicator` / `MaterialShapes` sources — the export is token-only, so
there is no official web rendering to compare against at all.

Checked against the published values:

| What | Expected | Rendered |
| --- | --- | --- |
| Container | 48 px corner-full disc; only the contained variant paints it (primary-container) | Yes |
| Active indicator | 38 px target (ActiveIndicatorScale 38/48), primary / on-primary-container | Yes |
| Indeterminate | the seven-shape morph loop, one 650 ms spring morph each, bounce kept (overshoot ≈1.08) | Yes (frame-checked against the pure functions) |
| Rotation | quarter-turn step per morph over the 4666 ms linear spin | Yes (pure-function checked) |
| Determinate | circle(rot 18°) → soft-burst by progress, sweeping −180° | Yes (0/25/50/75/100 % snapshots on the page) |
| Non-interactivity | no state layer, no ripple, no focus ring | Yes (live) |
| Both modes | colours track the scheme | Yes |

Shape-engine ground truth: the eight MaterialShapes render recognisably
against the catalogue's reference images (soft-burst's 10 soft points,
9-cookie's rounded lobes, the 45°-tilted pill, the tilted oval). The morph's
radial-interpolation divergence is structural (see porting-todo.md) and
cannot diverge visually for these star-convex shapes.

## 2026-10 comparison re-check (official vs ported)

Triggered by the observation that the ported components "do not look
particularly like" the official renderings. Method: material-web.dev pages
captured with a headless browser (the button page screenshot succeeded; the
icon-button / fab / progress token tables came through as text when the
network flapped), the qt-md3 gallery re-rendered from the current tree, both
normalised to logical px (the gallery screenshots are captured at DPR 1.5),
and side-by-side sheets composed under `build/audit/` (not committed).

### Finding 1 — the text font was missing (fixed)

`resources/fonts/` was empty: every label rendered in the platform sans
(Segoe UI on Windows), not the Roboto that `md.ref.typeface` names for both
the brand and the plain slots. This was the single largest source of visual
drift — letterforms, metrics and the 500-weight emphasis were all wrong.

Fix: Roboto Regular / Medium / Bold (the type scale's whole baseline weight
set) are now committed and loaded from `:/qt-md3/fonts/` — see
resources-manifest.md. CJK stays on the system fallback by design.

### Finding 2 — token-level geometry agrees with the official tables

Cross-checked against the token defaults the official pages publish:

| Family | Official default | qt-md3 | Verdict |
| --- | --- | --- | --- |
| Buttons | 40 px height, corner-full, label-large | same (token-locked) | agree |
| Icon buttons | 40×40, icon 24, corner-full, outlined 1 px | same | agree |
| FAB | container corner-large, small corner-medium, large corner-extra-large (16/12/28), icons 24/24/36, surface-container-high + primary icon, lowered/raised | same | agree |
| Linear progress | 4 px track & indicator, track surface-container-highest, corner-none, stop indicator + gap | same | agree |
| Circular progress | 48 px, 8.3333 % stroke (percentage model) | Compose-derived geometry — deliberate divergence, recorded in porting-todo.md | recorded |

### Finding 3 — the remaining differences are the documented ones

* **Standard M3 vs M3 Expressive**: material-web is in maintenance mode and
  renders the standard spec (pressed state layer + ripple); this library
  implements Expressive (pressed ripple + shape morph). interaction-reference.md
  documents this as intentional, component by component.
* **No official live reference exists** for extended FABs, FAB menus, split
  buttons, segmented buttons and badges — material-web never implemented
  them; those families were ported against the token exports and Compose M3,
  and their audits remain token/frame-based.
* **Colour**: the official demo pages ship their own custom schemes (the
  button page's gold/brown theme), so hue comparisons against them are
  meaningless; the baseline purple here is the published reference palette.
