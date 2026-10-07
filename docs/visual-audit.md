# Visual Audit

Side-by-side comparison of the official Material Design 3 pages against the Qt
example app, per component and per state.

No components have been audited yet — Stage 1 has not started.

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
| _(empty — no components yet)_ | | | | |
