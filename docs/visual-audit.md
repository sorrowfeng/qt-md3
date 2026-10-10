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
| Snackbars | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Tooltips | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Cards | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Dialogs | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Bottom sheets | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Side sheets | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Carousel | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Divider | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Lists | `Needs visual QA` | `Needs visual QA` | All | See below. |
| App bars | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Toolbars | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Navigation bar | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Navigation rail | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Navigation drawer | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Tabs | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Checkbox | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Chips | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Radio button | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Switch | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Menus | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Sliders | `Needs visual QA` | `Needs visual QA` | All | See below. |
| Time pickers | `Needs visual QA` | `Needs visual QA` | All | See below. |

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

### Snackbars

Page 19 (`Snackbars`) places static snapshots: a one-row message-only
snackbar, one with action + dismiss, a wrapped two-line message, and the
new-line action layout. Checked against the page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Container | inverse-surface, corner-extra-small, the level-3 shadow visible around the 8 px margin | Yes |
| Supporting text | body-medium, inverse-on-surface, vertically centred (one row) / top-14 px (new line) | Yes |
| Action | label-large inverse-primary label, hover/pressed state layers + ripple on the button-chrome region | Yes (live) |
| Dismiss icon | 24 px close glyph in inverse-on-surface, 40 px chrome flush right | Yes |
| Wrapped one-row | first line at 30 px, height ≥ 68 | Yes |
| New-line action | bottom-right, 4 px above the bottom, 8 px end inset without dismiss | Yes |
| Host transition | fade + scale (0.8→1) on enter/exit, springs | Yes (exercise) |

No side-by-side official reference exists (material-web ships no snackbar
web component; the behaviour source is Compose), so this stays
`Needs visual QA` like the other Compose-sourced families.

### Tooltips

Page 20 (`Tooltips`) places static snapshots: a short plain tooltip, a
wrapped plain at 160 px, a rich tooltip with subhead + text + action, the
same rich tooltip with the caret enabled, and a live hover demo host.
Checked against the page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Plain container | inverse-surface, corner-extra-small, NO shadow | Yes |
| Plain text | body-small, inverse-on-surface, 8/4 padding, min 40×24, wraps at 200 max | Yes |
| Rich container | surface-container, corner-medium, level-2 shadow visible around the 8 px margin | Yes |
| Subhead | title-small on-surface-variant, first baseline 28 px from the top | Yes |
| Rich text | body-medium on-surface-variant, baseline 24 px below the subhead box, 16 px bottom inset | Yes |
| Action | label-large primary label centred in its button-chrome hit region, 36 px box + 8 px bottom; hover/pressed state layers + ripple | Yes (live) |
| Text-only rich | falls back to the plain 4 px vertical padding | Yes |
| Caret | 16×8 triangle protruding 8 px past the container edge, container-coloured | Yes |
| Host transition | fade + scale (0.8→1) on enter/exit, springs; global mutex | Yes (exercise) |

No side-by-side official reference exists (material-web ships only the token
exports for this family; the layout and behaviour sources are Compose M3's
Tooltip.kt / BasicTooltip.kt), so this stays `Needs visual QA` like the other
Compose-sourced families.

### Cards

Page 21 (`Cards`) places three cards with sample content (a bold title and
wrapping supporting text, laid out by the card's own layout through the
contents margins). Checked against the page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Filled container | surface-container-highest, corner-medium, level0 — tone groups, no shadow | Yes |
| Elevated container | surface-container-low at level1, shadow ring visible below | Yes |
| Outlined container | surface with the 1 px outline-variant stroke, zero shadow | Yes |
| Clickable state rows | hover lifts filled to level1 + on-surface 0.08 layer; press ripples without lifting; Tab draws the secondary outer ring | Yes (live) |
| Contents margins | clickable cards inset the sample layout by the focus margin; non-clickable edge-to-edge | Yes |

No side-by-side official reference exists (material-web ships only the token
exports for this family; the behaviour source is Compose M3's Card.kt), so
this stays `Needs visual QA` like the other Compose-sourced families.

### Dialogs

Page 22 (`Dialogs`) places two static dialog snapshots and one live demo
(a real `MdDialogHost` raised over the page). Checked against the page
render:

| What | Expected | Rendered |
| --- | --- | --- |
| Container | surface-container-high, corner-extra-large, level-3 shadow ring visible | Yes |
| Basic slots | centred title (headline-small), wrapped supporting text (body-medium), end-aligned text actions | Yes |
| Action order | dismiss left of confirm, both flush to the end padding | Yes |
| Text-only | no icon/title — the supporting text starts at the top padding | Yes |
| Live host | the demo button raises the scrim + centred dialog; Escape / scrim click dismiss | Yes (live) |

No side-by-side official reference exists (material-web ships only the token
export; the behaviour source is Compose M3's AlertDialog.kt), so this stays
`Needs visual QA` like the other Compose-sourced families.

### Bottom sheets

Page 23 (`Bottom sheets`) places three static sheet snapshots and one live
demo (a real `MdBottomSheetHost` raised over the page). Checked against the
page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Container | surface-container-low, extra-large radius on the TOP pair only, bottom edge flush, level-1 shadow | Yes |
| Drag handle | 32 x 4 centred pill in on-surface-variant, 22 px touch padding above and below | Yes |
| Handle-less | the modal snapshot without the handle — the content area starts at the container top | Yes |
| Partial vs expanded | standard snapshots at 164 px and 240 px content heights — same surface, only the anchor differs | Yes |
| Live host | the demo button raises the scrim (black 0.32) + bottom-anchored sheet; drag / Escape / scrim click settle the anchors | Yes (live) |

No side-by-side official reference exists (material-web ships only the token
export; the behaviour source is Compose M3's BottomSheet.kt and
SheetDefaults.kt), so this stays `Needs visual QA` like the other
Compose-sourced families.

### Side sheets

Page 24 (`Side sheets`) places three static sheet snapshots and one live
demo (a real `MdSideSheetHost` raised over the page). Checked against the
page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Modal container (right) | surface-container-low, level-1 shadow, the LARGE radius on TL/BL (the start pair facing the content), the right edge flush | Yes |
| Standard sheet | surface at level 0, square corners, a 1 px outline divider on the content-facing edge — against the page's own surface only the divider reads, which is the token-faithful rendering | Yes |
| Left-docked modal | the mirror: the radius pair on TR/BR | Yes |
| Live host | the demo button raises the scrim (black 0.32) + the right-docked full-height sheet; drag / Escape / scrim click run the cancel flow | Yes (live) |

No side-by-side official reference exists (material-web ships only the token
export and Compose ships no side sheet; the behaviour source is MDC-Android's
sidesheet package), so this stays `Needs visual QA` like the other
MDC-sourced families.

### Carousel

Page 25 (`Carousel`) places two static carousel snapshots at different scroll
offsets. Checked against the page render:

| What | Expected | Rendered |
| --- | --- | --- |
| Rest snapshot | the large item flush with the container start, the extra-large (28) corners, the small item riding the trailing anchor partially cut off | Yes |
| Mid-scroll snapshot | the large item shrunk along the keyline curve, the next item grown toward it — the resize-as-it-scrolls deformation visible between the two shots | Yes |
| Container colour | surface at level 0 (no shadow at rest) with the colour-block contents clipped to the rounded mask | Yes |

No side-by-side official reference exists (material-web ships only the token
export; the behaviour source is the Compose M3 carousel package, verified
against Compose's own MultiBrowseTest expectations), so this stays
`Needs visual QA` like the other Compose-sourced families.

### Divider

Page 26 (`Divider`) places the four horizontal inset modes, three vertical
snapshots and one 4 px thickness/colour override. Verified against the page
render with pixel sampling (the 1 px hairlines are too fine to eyeball):

| What | Expected | Rendered |
| --- | --- | --- |
| Full-width | the hairline spanning the entire content column, outline-variant | Yes |
| Inset start | the line starting 16 px in, flush at the end (the spec's "inset" measurement) | Yes (24 device px at dpr 1.5) |
| Inset end | flush at the start, ending 16 px short (material-web's `[inset-end]`) | Yes |
| Middle inset | 16 px padded on both sides (the spec's "middle-inset") | Yes |
| Vertical | the full-height column and the two padded columns (48 / 32 / 16 logical px tall) | Yes |
| 4 px override | the thickness parameter honoured with the custom colour, crisp edges (no AA blur) | Yes |

### Lists

Page 27 (`List`) places six `MdList` widgets: the Standard/Expressive pair, the
56/72/88 line-count ladder, the three leading-slot kinds, a segmented set, and
the interaction states. Audited by pixel sampling of the page render, because
the shape ladder is a *corner radius* — a value no eye can read off a
screenshot.

The measurement that carries the family is the Standard/Expressive pair: two
lists with identical content and identical states, so the selected item's
container is the same colour, the same width and the same height in both, and
the shape is the only variable.

| What | Expected | Rendered |
| --- | --- | --- |
| Standard, selected | a 56 dp container of secondary-container with **all four corners square** (corner-none) | Yes — the selected band is 1494x84 device px at dpr 1.5 and all four corner pixels sampled 1 px inside are the container colour |
| Expressive, selected | the same band with all four corners rounded (corner-large, 16 dp) | Yes — identical band, and all four sampled corner pixels are the page surface instead |
| Selected container colour | secondary-container, identical in both lists | Yes — `#e8def8` exactly in both |
| Segmented gap | 2 px between items (`segmented.gap`) | Yes — 3 device px of surface between the two selected bands |
| Dragged | on-surface at 0.16 over the surface, corner-large | Yes — `#dad4db`, the composite of `#1d1b20` at 0.16 over `#fef7ff`, with rounded corners |
| Selected + disabled | on-surface at 0.38 over the selected container | Yes — `#9b94a6`, the composite of `#1d1b20` at 0.38 over `#e8def8`, with rounded corners |
| Disabled, unselected | the container unchanged; only the content fades | Yes — the container pixel is the bare surface, as the export's missing `disabled.container` row requires |
| Avatar slot | a 40 dp corner-full disc in primary-container, with the label centred | Yes |
| Rendering of the resting shape | none — a surface container on a surface list has no visible shape, in this port and in material-web alike | By design; the page says so in prose, so the demo leans on the selected / dragged / disabled rows |

Not verifiable in this environment: the official material-web rendering of a
list is a live custom element whose shadow DOM needs a browser; the source of
truth for every number above is the `md.comp.list.*` export, which the token
table in `TestMd3List` pins field by field. The family therefore stays
`Needs visual QA` like the other Compose-sourced ones.

### App bars

Page 28 (`App bars`) places nine `MdTopAppBar`s — the five size layouts across
seven heights, the two-row collapse ladder at 0/0.5/1, the two alignments, and
the scroll-colour pair — plus three `MdBottomAppBar`s (Start / Center /
SpaceBetween with a docked FAB). Audited by pixel sampling of the page render
at DPR 1 (the window is 1280 × 860 and the page column 994 px, so one device
pixel is one logical pixel here). Read from the `--language en` render, because
the bundled font set has no CJK coverage and a Chinese shot is a wall of tofu
boxes — legible to an assertion, useless to a reader.

Two measurements carry the family, and both were **wrong on the first pass**.

**The container geometry.** Every child that can show a focus indicator
reserves 7.5 px a side inside its own widget, so a layout that places the
*widget* on a token position draws the container inset by that margin:

| What | Token says | First pass | Rendered |
| --- | --- | --- | --- |
| Bottom bar, first icon's ink from the bar's edge | 4 (content) + 8 (icon padding) + ink ≈ 14 | 23 *(measured)* | **14** |
| Bottom bar, three icons centre-to-centre | 40 (container) + 0 (`iconButtonSpace`) | 55 *(measured)* | **40.5** |
| Top bar, navigation ink from the bar's edge | ≈ 15 | 23 *(measured)* | **15** |
| Top bar, title's left edge | `max(12, 4 + 40) + 4` = 48 | 63 *(derived — the old code measured the nav's 55 px `sizeHint`)* | **48**, ink at 49 |
| Top bar, two actions centre-to-centre | 40 | 55 *(derived)* | **40** (last container flush at the 4 px trailing inset) |

The fix is the rule `MdButtonGroup` established and `styles/MdChildBox.h` now
shares: place the container, not the widget. `TestMd3AppBar`'s
`containersArePlacedNotWidgets` pins it with *real* `MdIconButton`s, which the
fixed-size probes cannot stand in for.

**The heights.** A bar at rest paints surface on a surface page — that is the
spec's own behaviour, not a rendering failure — so the seven size bands cannot
be found by colour. They were measured instead through the navigation glyph,
whose three 2 px marks sit at the row's vertical centre: consecutive glyph
centres are `height + 12` apart, and the six spacings that fit on the page give
76, 124, 164, 124, 148 and 132 px, i.e. **64, 112, 152, 112, 136 and 120** —
the small, medium, large, medium-flexible, medium-flexible-with-subtitle and
large-flexible heights, each exact. The seventh (large-flexible-with-subtitle,
152) follows the same rule.

**The scroll colour.** The page's "scrolled" sample is a single-row bar pushed
past `overlappedFraction > 0.01`, so it takes surface-container outright; the
half-collapsed medium bar sits at `collapsedFraction = 0.5` and reproduces the
whole chain — `FastOutLinearInEasing` then Oklab:

| Fraction | Expected | Rendered |
| --- | --- | --- |
| 0.5, eased then Oklab-interpolated | `#faf4fc` | Yes — the half-collapsed medium band, 88 px tall |
| 0.5, naive sRGB average | `#f9f2fb` | No — ruled out by the pixel |
| 0.5, naive Oklab average | `#f8f2fb` | No — ruled out by the pixel |
| 1.0 | surface-container `#f3edf7` | Yes — 64 px on both the medium and the large bar |

The half-collapsed bars are 88 px tall (medium: `112 − 48/2`) and the fully
collapsed ones 64 px, which is the `collapsedRowHeight`-from-the-small-set
arithmetic made visible.

| What | Expected | Rendered |
| --- | --- | --- |
| The five size layouts | 64 / 112 / 152 / 112 / 136 / 120 / 152 | Yes — measured through the 12 px-separated navigation glyph centres, see above |
| Two-row collapse keeps the icon row | 112 → 88 → 64 and 152 → 108 → 64, never to zero | Yes — 88, 64, 76 + 17 (split by the title glyphs), 64 |
| Leading title alpha at 0.5 | `TopTitleAlphaEasing(0.5)` ≈ 0.03, i.e. all but invisible | Yes |
| Bottom app bar height | 80 for all three arrangements | Yes — 80, 80, 80 |
| Bottom bar's content band | 2 px below the container's centre (there is no bottom padding) | Yes — glyph centre 2626.5 against a band centre 2625.5, i.e. the band's own 2 px offset plus the glyph's ink asymmetry |
| Docked FAB container | 56 × 56, 16 px from the trailing edge, 12 px from the top | Yes — the primary fill measures 55 × 55 with one antialiased pixel on the rounded edge, so the container spans 954..1009 and 2810..2865, i.e. 16 and 12 |
| Resting top-bar container | invisible — `container.color` is surface on a surface page, which is the spec's own behaviour | By design; the page says so, and the scroll section exists precisely so the container is visible somewhere |
| Two-row widget rects overlap | two 40 px containers touching are two 55 px widgets overlapping by 15 px | By design, recorded in porting-todo.md |

Not verifiable in this environment: material-web has **no production top app
bar** (only a catalog stub and an experimental `labs/gb/` one), and the spec
page's six measurement diagrams are served from `lh3.googleusercontent.com`,
which is unreachable from here on any proxy. The source of truth for every
number above is therefore the `md.comp.app-bar.*` export cross-checked against
Compose's `AppBar*Tokens.kt` (which agree row for row up to the two deprecated
subtitle rows), plus `AppBar.kt` for the layout and the state machine. The
family stays `Needs visual QA`.

### Toolbars

Page 29 (`Toolbars`) places two `MdDockedToolbar`s — expanded and at
`heightOffset = -32` — three floating toolbars for the orientation × colour
matrix, three more at `expandedProgress` 1 / 0.5 / 0 with a leading and a
trailing action, and three with an action button (FAB at `End`, at `Start`, and
a vertical toolbar with it at the bottom). Audited the same way as the app bars:
pixel sampling of the `--language en` render at DPR 1, the page column being
994 px wide (x 32…1025), so one device pixel is one logical pixel. Sampling is
by **exact token colour** rather than by difference-from-background, because the
level3 pill shadow is faint enough to fall under a `diff > 6` threshold and the
bounding boxes would otherwise be the pill's, not the shadow's.

Every number below is the export's own arithmetic, and every one of them was
**wrong on the first pass in a way only the pixels could show**.

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Docked container | 994 × 64, `corner-none` | Yes — `#f3edf7` across the whole band |
| Docked at `heightOffset = -32` | 994 × **32** (`heightOffsetLimit = -placeable.height`) | Yes — 32, the whole bar, not one row |
| Docked item pitch | 40 (`containerHeight`) + **32** (`containerMaxSpacing`) = 72 | Yes — glyph centres 457.5 / 529.5 / 601.5 |
| Docked run centring | 3 × 40 + 2 × 32 = 184 centred in the 961 px band (48…1009) → first container at 436.5 | Yes — first glyph centre 457.5 = 436.5 + 20 + ink asymmetry |
| Floating pill, horizontal | 8 + 40 + **4** + 40 + **4** + 40 + 8 = 144 × 64 | Yes — 144 (first pass 136, see below) |
| Floating pill, vertical | the same numbers transposed | Yes — 64 × 144, exact |
| Five slots (`leading + 3 + trailing`) | 8 + 5 × 40 + 4 × 4 + 8 = 232 | Yes — 232 |
| …at `expandedProgress = 0.5` | 232 × 0.5 = 116, **trailing edge fixed** | Yes — right edge 264 = 32 + 232 and the pill sweeps left |
| …at `expandedProgress = 0` | no extent at all, and the leading / trailing slots have left the layout | Yes — nothing but the row's label |
| Pill with an action button | pill 8 + 40 + 4 + 40 + 8 = 100, strip = 8 + **56** (the *expanded* FAB) = 64, widget 164 × 80 | Yes — pill 32…132, widget trailing edge 196 |
| The action button at `End`, expanded | box = `outerMainEnd - 56` = 140…196 | Yes — the `#6750a4` circle measures 141…195, i.e. 56 with one antialiased pixel |
| The same at `Start` | box 32…88, pill begins after the strip at 96 | Yes — pill 96…195 |
| The same on a vertical toolbar | `Bottom` is the axis' end, so the box is `outerMainEnd - 56` | Yes — circle 45…99 × 1806…1860 |
| The same, collapsed | box = `outerMainEnd - 80` = 116, i.e. **16 px inside** the pill's 132 anchor | Pinned by `TestMd3Toolbar::floatingFabStripAndSizeSets`; not visible on the page (see below) |

**The finding that changed the component.** The three-item pill measured 136 px.
The export says `container.between-space: 4`, so it should have been 144 — and
`MdFloatingToolbarStyle::layoutFor` had been advancing its cursor by each slot's
container and by nothing else. The row was resolved, carried in the token struct
and asserted by `floatingTokenTable`, while the layout ignored it: a token test
cannot see a row that nothing reads. Compose is no help here, because it does
not read the row either — `FloatingToolbarTokens.ContainerBetweenSpace` is
referenced nowhere in `FloatingToolbar.kt`, since upstream arranges a toolbar's
items in the *caller's* `Row`. This widget owns its children's arrangement, so
the published gap is applied, and `TestMd3Toolbar::betweenSpaceSeparatesTheSlots`
now pins it from the override down to `sizeHint()`. The pitch is visible in the
render: 44 px between neighbouring glyph centres, in a five-slot row as well as
a three-slot one.

**The half-collapsed docked bar is not a bug, and the source says so.** At
`heightOffset = -32` the bar is 32 px tall while its 20 rows of glyph ink become
9 (rows 319…327 of a 296…327 band — the glyphs' upper slivers, at the band's
bottom edge). That is `AppBar.kt` line for line:

```kotlin
val height = (placeable.height + heightOffset).coerceAtLeast(0f)
layout(placeable.width, height.roundToInt()) { placeable.place(0, 0) }
```

The row stays at y = 0 inside a shorter box; nothing re-centres it. A Compose
app would additionally paint the overflow over whatever is behind the bar, since
`Modifier.layout` reports a smaller height without an implicit clip — Qt cannot
paint a child outside its widget at all, so this port cuts instead of
overflowing. Same discretisation as the floating pill's slots, recorded in
[porting-todo.md](porting-todo.md).

**Qt clips the pill's shadow away.** `container.elevation` is `level3` in the
export and this port paints it (`MdElevation::drawShadowDp`, ramped by
`expandedProgress`), but a floating toolbar with **no** action button has a
widget rectangle identical to its pill and Qt clips painting to the widget, so
the shadow has nowhere to land: sampling 8 px below the standard pill's bottom
edge returns the page background exactly (`#fef7ff`, `diff = 0`), and the only
non-background pixels beyond the pill's outline are its own antialiased edge.
With a FAB the widget is 80 px across against the pill's 64, so there are 8 px of
slack above and below — and there the shadow does land: at x = 50, which the
pill's rounded end has already left at that row (the fill begins at x ≈ 56), rows
1549…1553 measure 152 → 174 → 196 → 216 → 233 grey, a five-row falloff under the
pill's edge, while the row above (inside the fill) is the pill's own
`#EADDFF`. So the row is carried and painted, visible exactly where the widget
has room for it; the lists' drag shadow carries the same note.

**There is no item size to be faithful to.** The pill's *length is its
children's*: the export publishes no item height or width for a toolbar (all 36
`docked` and 75 `floating` rows were re-read to check), so a toolbar advances by
whatever container its child publishes — the same intrinsic-measurement rule
Compose uses. Page 29 uses the library's 40 px default icon button, because the
`MdIconButton` ladder is the Expressive one (32 / 40 / 56 / 96 / 136) and there
is no 48; a 48 px item would make the band exactly `64 − 8 − 8` and reproduce
the export's own "8 + 48 + 8 = 64" composition. Every pill on the page is
therefore 8 px narrower per item than the spec's arithmetic. That is a fact about
the child component, not about this layout, and it is why the audit compares the
pill against *its own* children rather than against a nominal 168.

**Clamping at 0.5.** Of the five slots in the half-progress row only the fourth
and fifth survive — glyph ink at 186…199 (`delete`) and 235…238 (`more_vert`) —
because the pill spans 148…264 (relative 116…232): the leading, first-content and
second-content containers (relative 8…48, 52…92, 96…136) all start before the
pill does, while the third content container (relative 140…180, absolute
172…212) and the trailing one (relative 184…224, absolute 216…256) sit inside
it. The rule is "fully inside or hidden", which is what makes an overlapping slot
disappear rather than be cut. It is a discretisation of Compose's
`graphicsLayer { clip = true; shape = shape }`, forced by Qt clipping a child to
its widget rather than to a shape drawn inside it.

Not verifiable in this environment: m3.material.io's `/components/toolbars`
pages are a client-side SPA whose content arrives only in the browser, and the
spec's own availability table marks both variants `Web: Unavailable`, so there
is no official web rendering to compare against — only the headless-Edge capture
of the taxonomy (Variant / Baseline / Configuration / Anatomy) and the export's
five token files. The family therefore stays `Needs visual QA`.

### Navigation bar

Page 30 (`Navigation bar`) puts the two published families side by side — a
baseline bar and a flexible bar, four items each with the first selected —
then the same flexible bar in `EqualWeight` and `Centered`, then a `Start`
row. Audited by pixel sampling the offscreen smoke render (which carries its
own uniform scale of ≈1.04, so the numbers below are ratios, not absolute
pixels — every one lands on the token value under that single scale):

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Baseline container height | 80 | Yes — 82-83 at the smoke scale, `#f3edf7` (surface-container) across the band |
| Flexible container height | 64 | Yes — 65, the same container colour |
| Baseline pill | 64 x 32, `corner-full`, `secondary-container` | Yes — 66 x 33, `#e8def8`, icon ink `#4a4458` (on-secondary-container) |
| Flexible pill | 56 x 32 | Yes — 59 x 34 |
| Centered band | one tenth per side at four items | Yes — the second row's pill sits visibly inboard of the first's |
| Start pill height | 40 (16 + icon 24 + 8 a side) | Yes — 41; the pill wraps icon and label, its width content-driven |
| Selected label cut | emphasized (prominent) | Yes — the selected label is visibly heavier |

`itemLayout` is the bar's own property — the page learned that the hard way:
setting `iconPosition` on the items *before* `addItem` was silently
overwritten by `applyToItem`, which is the design (the bar owns the
arrangement) working as intended.

### Navigation rail

Page 31 (`Navigation rail`) puts the baseline and flexible rails side by side
in one band (FAB header, four items, first selected), then the flexible rail
expanded, then the modal one. Same smoke-scale caveat:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Baseline rail width / pill | 80 wide, pill 56 x 32 centred | Yes — pill 58 x 33, centred in the column |
| Flexible collapsed width / pill | 96 wide, pill 56 x 32, top inset 44 | Yes — the flexible column's FAB sits visibly lower than the baseline's |
| Expanded pill | `Start` arrangement, 56 tall, width = 16 + icon + 8 + label + 16 | Yes — 58 tall, width 79 at the offscreen font's label advance |
| Expanded item form | icon beside label, labels always on | Yes — every item shows its label, not just the selected one |
| Modal container | `surface-container` / level2 / `corner-large` | Yes — the third column is visibly tinted against the page |
| Item pitch, collapsed | item + 4, from the family's top inset | Yes — the four pills stack at the 4 px rhythm |

The expanded width is the content's between 220 and 360, and the flip is a
state switch — the page holds one rail per state rather than animating, which
is what Compose's boolean does too.

### Navigation drawer

Page 32 (`Navigation drawer`) shows the standard sheet (headline, divider,
four items, first selected with a `24` badge) and the modal one. Same
smoke-scale caveat:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Standard container | 360 wide, `surface` at level0, `corner-large-end` | Yes — `#fef7ff`, the end pair rounded, the leading edge square |
| Modal container | `surface-container-low` at level1 | Yes — `#f7f2fa` against the page's `#fef7ff` |
| Item pill | 336 = 360 - 2 x 12, full width of the inset, 56 tall, `corner-full` | Yes — 350 x 58, one row per destination, no gaps between rows |
| Pill colour | `secondary-container` when selected, nothing when not | Yes — `#e8def8` on the selected row only |
| Badge | trailing text at the 24 px inset, `large-badge-label-*` | Yes — the `24` hangs inside the pill's trailing edge |
| Headline / divider | `title-small` in `on-surface-variant`, then the outline rule | Yes — both push the items down by their own height plus the recorded gaps |

Not verifiable in this environment, as with every Navigation family member:
m3.material.io's navigation pages are a client-side SPA, and the availability
table marks the flexible rows `Web: Unavailable` — there is no official web
rendering to compare against, only the headless-Edge captures of the taxonomy
and the export's token files. The three families therefore stay
`Needs visual QA`.

### Tabs

Page 33 (`Tabs`) shows the primary and secondary rows with icons, the 64 dp
icon+label row, a scrollable row with the last tab selected, and the leading
icon row. The page-33 smoke shot was read against the export's token values
at the usual ~1.04 smoke scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Primary indicator | 3 px tall, rounded on top, 24 px content width under tab 0 | Yes — 3 rows of `#6750a4`, a 23 px run (24 / 1.04) centred under the first tab |
| Secondary indicator | 2 px tall, square, the whole tab | Yes — 2 rows of `#6750a4` spanning the full tab width |
| Icon+label row | 64 dp tall; the indicator still 3 px | Yes — the band grows, the indicator keeps its height |
| Scrollable row | 52 px edge padding, 90 px minimum tab, the last tab scrolled towards the centre | Yes — the run is offset, not pinned to the start edge |
| Divider | `surface-variant` `#e7e0ec`, 1 px, under the indicator | Yes in the offscreen probes; the smoke scale blurs the single row into a blend, as with every 1 px rule |
| Content colours | selected `primary` `#6750a4`, unselected `on-surface-variant` | Yes — the selected tab's label and icon lift to `#6750a4` |

The official side-by-side comparison is the same standing as the rest of the
family: m3.material.io is a client-side SPA with no server-rendered markup to
diff, so the family stays `Needs visual QA` until the reference captures are
taken.

### Checkbox

Page 34 (`Checkbox`) shows the three check states, the disabled trio, the
error pair and two live boxes. The page-34 smoke shot was read against the
export's token values at the usual ~1.04 smoke scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Unchecked box | 18 px, 2 px corners, 2 px `on-surface-variant` outline, no fill | Yes — outline only, centre transparent |
| Checked box | one `primary` fill (border collapsed into it), an `on-primary` check | Yes — `#6750a4` fill, white check through (0.25, 0.5) → (0.4, 0.65) → (0.75, 0.3) |
| Indeterminate | the same fill, the check gravitated flat onto the centre line | Yes — a horizontal `on-primary` dash from 0.25 to 0.75 of the box |
| Disabled, checked | the whole box `on-surface` @ 0.38, the check `surface` at full strength | Yes — the greyed fill with a light check |
| Disabled, unchecked | the 2 px outline in `on-surface` @ 0.38 | Yes — the pale outline, no fill |
| Error | container `error` `#b3261e`, check `on-error`; unchecked outline `error` | Yes — the red pair |
| State layer | 40 px circle behind the box, `primary`/`on-surface` pressed by side | Visible on press only — the live boxes ripple `primary` unchecked |

The pixel probes in `TestMd3CheckBox` also caught the one trap this page's
arithmetic hides: the check's second leg passes within a stroke width of the
box centre, so a naive "centre pixel = fill" sample reads the check's
`on-primary` instead. The probes sample above the path's topmost point.

The official side-by-side comparison is the same standing as the rest of the
matrix: m3.material.io is a client-side SPA with no server-rendered markup to
diff, so the family stays `Needs visual QA` until the reference captures are
taken.

### Chips

Page 35 (`Chips`) shows the four families flat, the elevated trio, the
selection pair, the input avatar, the disabled/dragged row and two live
chips. The page-35 smoke shot was read against the export's token values at
the usual ~1.04 smoke scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Four families | 32 px tall, corner-small pills; assist `on-surface` + `primary` icon, suggestion muted `on-surface-variant`, filter/input grey until selected | Yes — the four pills read distinctly, input carries the trailing `close` |
| Elevated | `surface-container-low` fill with the level-1 resting shadow | Yes — the three pills lift off the band |
| Selected | `secondary-container` fill, `on-secondary-container` content, no outline | Yes — both `Selected` pills purple, the flat outline gone |
| Avatar | a 24 px corner-full circle displacing the leading icon slot | Yes — the dark circle before "Alex" |
| Disabled | the selected container `on-surface` @ 0.12, content @ 0.38; the unselected flat one transparent with its outline at 0.12 | Yes — the two greyed pills, the selected one still filled faintly |
| Dragged | level 4 shadow + the 0.16 state layer | Yes — the `Dragged` pill lifts visibly above its band |

The pixel-probe standing of the family matches the checkbox's: the 1 px flat
outline hugs the top edge (the stroke is inset half a width), so sampling at
y = 0.5 rounds to the first empty row and a naive edge sample reads
transparent — the test samples row 0.

The official side-by-side comparison is the same standing as the rest of the
matrix: m3.material.io is a client-side SPA with no server-rendered markup to
diff, so the family stays `Needs visual QA` until the reference captures are
taken.

### Radio button

Page 36 (`Radio button`) shows the two selection sides with a group member,
the disabled pair and a live three-button group. The page-36 smoke shot was
read against the export's token values at the usual ~1.04 smoke scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Unselected | a 20 px circle stroked 2 px in `on-surface-variant`, transparent centre | Yes — the bare ring, no dot |
| Selected | the same ring in `primary` with a 5 px radius dot (12 px diameter minus the stroke inset) | Yes — the `primary` dot inside the ring |
| Disabled | both sides `on-surface` @ 0.38 | Yes — the pale pair, the selected one still dotted |
| Live group | three interactive siblings, one exclusive group per section | Yes — clicking one moves the dot |

The page-36 build itself caught the one trap this family's group semantics
hide: without per-section `QButtonGroup`s every radio on the page shares the
page parent's autoExclusive group, so checking the *disabled* sample silently
unchecked the *selected* sample two sections above — the first screenshot
rendered three bare rings and no selected dot. The page now wraps each row in
its own exclusive group.

The official side-by-side comparison is the same standing as the rest of the
matrix: m3.material.io is a client-side SPA with no server-rendered markup to
diff, so the family stays `Needs visual QA` until the reference captures are
taken.

### Switch

Page 37 (`Switch`) shows the two sides plus the icon thumbs, the disabled
pair and two live switches. The page-37 smoke shot was read against the
export's token values at the usual ~1.04 smoke scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Unselected | a 52×32 `surface-container-highest` track, 2 px `outline` border, a 16 px `outline` thumb at the left inset (8 px in) | Yes — the grey track, the small thumb left |
| Selected | a `primary` track, no border, a 24 px `on-primary` thumb at the far bound (right inset 4 px) | Yes — the purple track, the white thumb right |
| With icon | a 24 px thumb even unchecked (Compose's `hasContent` branch), the 16 px check in the icon row's colour | Yes — the check rides the thumb, track's colour unchecked / `primary` selected |
| Disabled | the track @ 0.12, the unselected handle @ 0.38, the icon @ 0.38 — the **selected handle `surface` @ 1** | Yes — the pale pair, the selected one's white thumb at full strength |
| Live | two interactive switches, the ripple riding the thumb | Yes — pressing snaps the thumb 2 px inward and blooms at the thumb |

The official side-by-side comparison is the same standing as the rest of the
matrix: m3.material.io is a client-side SPA with no server-rendered markup to
diff, so the family stays `Needs visual QA` until the reference captures are
taken.

### Menus

Page 38 (`Menu`) shows the embedded surface (four rows, the second selected,
a divider between the second and third, the fourth disabled) and the three
row costumes (leading icon, cascading arrow, disabled). The page-38 smoke
shot was read against the export's token values at the usual ~1.04 smoke
scale:

| Row | Arithmetic | Rendered |
| --- | --- | --- |
| Surface | a `surface-container` corner-extra-small block, level-2 shadow, 8 px above the first row and below the last | Yes — the grey rounded block with the soft shadow |
| Selected row | a `secondary-container` row block with `on-secondary-container` text, 48 px tall, 12 px padding | Yes — the purple second row |
| Divider | a 1 px `surface-variant` rule inset 12 px each side, 2 px above and below | Yes — the hairline between the second and third rows |
| Disabled row | `on-surface` @ 0.38 text | Yes — the pale fourth row |
| Row costumes | a 24 px leading icon 8 px from the text; the trailing `arrow_forward` in `on-surface-variant`; the disabled row's faded text | Yes — all three paint |

The official side-by-side comparison shares the matrix's standing: the spec
site is a client-side SPA, so the family stays `Needs visual QA` until the
reference captures are taken.

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
