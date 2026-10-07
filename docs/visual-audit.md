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
