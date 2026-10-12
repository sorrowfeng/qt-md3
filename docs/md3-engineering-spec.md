# MD3 Engineering Spec — Qt Widgets + QStyle + QPalette

> Source prompt for the Material Design 3 (Material You) port. Saved so every
> session works against the same contract. The library must remain **QStyle +
> QPalette + QPainter only** — `setStyleSheet()` / QSS is forbidden and gated
> by `TestMd3NoQss`.

## Highest principle

MD3 is not an MD2 reskin. Any of the following means an MD2 leftover that
must be fixed:

- Fixed primary colour / static palette → MD2
- Expressing every colour through `QPalette::Button` / `Window` alone → MD2
- Expressing every layer with shadows → MD2
- Every animation via `QPropertyAnimation` + `QEasingCurve::InOutQuad` → MD2
- One corner radius for every control (4 px / 8 px) → MD2
- No state layer, no tonal surface, no dynamic colour → MD2

The five non-negotiables: **dynamic colour, spring physics, shape tokens,
state layers, tonal surfaces.**

---

## 1. Dynamic Colour (QStyle + QPalette route)

Dynamic colour cannot come from QSS variables. It must run through
**QPalette + a theme engine**.

1. Implement the Material Color Utilities **HCT** colour space and scheme
   generation (reference: QSkinny `QskHctColor`, or port
   `material-color-utilities`). Input: seed colour (or wallpaper-extracted
   colours). Output: full MD3 tonal palettes — Primary / Secondary / Tertiary
   / Neutral / Neutral Variant / Error, tones 0–100.
2. Map generated tones onto **QPalette roles** where they fit:
   | QPalette | MD3 |
   | --- | --- |
   | Window | surface |
   | WindowText | onSurface |
   | Base | surfaceContainer |
   | Text | onSurface |
   | Highlight | primary |
   | HighlightedText | onPrimary |
   | Button | surfaceContainerHigh |
   | ButtonText | onSurface |
3. For roles QPalette cannot express (primaryContainer, onPrimaryContainer,
   secondaryContainer, tertiary, surfaceVariant, surfaceContainerLowest/Low/
   High/Highest, outline, outlineVariant, inverseSurface, …) keep a
   `QHash<Md3ColorRole, QColor>` in the theme engine. Controls query
   `MdTheme::instance().color(ColorRole::…)` — **never** hard-code a colour in
   `paintEvent`.
4. The theme engine must support, at run time:
   - seed change → every registered control repaints (signal / observer),
   - light / dark flip → QPalette **and** the custom role table update together,
   - surface hierarchy via the surfaceContainer family, **not** stacked shadows.

## 2. State Layer (painted in QStyle)

Hover / press / focus cannot be QSS pseudo-states. They are translucent
overlays painted in the style, clipped to the base shape.

| State | Overlay |
| --- | --- |
| hover | onSurface @ 8 % |
| focus | onSurface @ 12 % |
| pressed | onSurface @ 12 % |
| dragged | onSurface @ 16 % |

- Clip with `QPainterPath` to the base shape's corner radius — never a bare
  rectangle.
- Animate the opacity with a spring / short interpolation; no hard flips.
- Extend `QStyleOption` (or the widget) with `hovered_ / pressed_ / focused_ /
  dragged_` flags.

## 3. Spring Physics (independent integrator)

Qt Widgets has no `SpringAnimation`. Implement a standalone integrator.

```cpp
struct Spring {
    double value = 0.0;
    double target = 0.0;
    double velocity = 0.0;
    double damping = 1.0;    // 1.0 = critically damped, <1 overshoots
    double response = 0.4;   // seconds
    qint64 lastNs = 0;

    void step(qint64 nowNs);  // analytic solution, no drift
    bool settled() const;
};
```

Requirements:

- **Analytic solution** (`exp` / `cos` / `sin`), not per-frame integration —
  no long-run drift.
- **Interruptible**: changing `target` mid-flight continues from the current
  `value` and `velocity` — never jump.
- Two motion schemes:
  - **Standard** — damping ≈ 1.0, response ≈ 0.4 s (lists, ordinary transitions)
  - **Expressive** — damping ≈ 0.7–0.8, response ≈ 0.3–0.5 s, slight overshoot
    (FAB, Bottom Sheet, Dialog)
- Every interaction feedback (button press, chip select, switch toggle),
  container transform (expand/collapse) and page transition runs on a Spring.
  `QEasingCurve` is not a substitute.

## 4. Shape tokens in QStyle

| Token | Radius |
| --- | --- |
| ExtraSmall | 4 dp |
| Small | 8 dp |
| Medium | 12 dp |
| Large | 16 dp |
| ExtraLarge | 28 dp |
| Full | capsule |

Assignment: Button (Filled/Outlined/Tonal) **Full** · Card **Medium** ·
Dialog **ExtraLarge** · Bottom Sheet **ExtraLarge** (top corners) ·
Chip **Small** · FAB **Large** · IconButton **Full**.

- Draw with `QPainterPath::addRoundedRect`, not a QSS border-radius mindset.
- Shape morphs (press) are Spring-driven, not instant.

## 5. Typography (Material 2021)

Display L/M/S · Headline L/M/S · Title L/M/S · Body L/M/S · Label L/M/S.

Each token: `QFont` family, pixelSize (dp-scaled for DPI), weight, lineHeight,
letterSpacing. Controls take fonts from
`MdTheme` / `MdTypeScale::font(TypeStyle::…)`. **Never** hard-code `QFont`.
The MD2 H1–H6 / Subtitle / Body1/2 naming is banned.

## 6. Elevation

1. Elevation is **tonal**: surfaceContainerLowest < Low < surfaceContainer <
   High < Highest.
2. Shadows are auxiliary only, tinted, low alpha, blur growing with level —
   never a pure-black drop shadow.
3. In dark mode elevation is *entirely* the surfaceContainer ladder; do not
   paint black shadows.

## 7. Component checklist

| Control | Requirements |
| --- | --- |
| Button (5 variants) | Full radius, state layer, spring press scale 0.96→1.0 |
| IconButton | Full radius, centred glyph, circular state-layer clip |
| FAB / Extended FAB | Large radius, Expressive spring expand/collapse, aux shadow |
| Card (3 variants) | Medium radius, tonal surface, hover state layer |
| Chip (4 families) | Small radius, spring selection, state layer |
| Switch / Checkbox / Radio | Spring thumb / check motion, MD3 colour roles |
| Slider | Spring thumb, split active/inactive track colours |
| Top App Bar | Tonal surface, spring-driven elevation change on scroll |
| Navigation Bar / Rail / Drawer | Spring-sliding selection indicator, state layer |
| Bottom Sheet | Expressive spring drag + snap, interruptible |
| Dialog | ExtraLarge radius, Expressive spring enter (scale + opacity) |
| Menu / Dropdown | Small radius, spring open, state layer |
| Progress Indicator | Spring-driven value, no instant jumps |

## 8. Output contract

Every code generation / modification:

1. List the MD3 tokens touched (colour roles, shape, type, motion).
2. Explain why this is MD3 and not MD2.
3. Ship compilable C++ / Qt Widgets code that does not break the existing
   structure.
4. Append a self-check:
   - dynamic colour (HCT + QPalette extension)?
   - state layer?
   - spring integrator?
   - MD3 shape tokens?
   - Material 2021 typography?
   - tonal surface instead of shadow?
   - any QSS leftover?
   - any MD2 leftover?
5. If existing code is MD2-style, name the problem first, then the migration.

---

## Mapping onto this repository

| Spec area | Implementation |
| --- | --- |
| HCT / CAM16 | `src/core/MdColorMath` |
| Scheme + tonal palettes | `src/core/MdColorScheme`, `MdDynamicColor` |
| Theme engine + role table | `src/core/MdTheme`, `ColorRole` in `MdTypes.h` |
| State layer | `src/core/MdStateLayer`, painted by each `Md*Style` |
| Spring | `src/core/MdMotion` (`MdSpring`, Standard / FastSpatial / Effects) |
| Shape tokens | `src/core/MdShape` (`ShapeCorner`) |
| Typography | `src/core/MdTypeScale` (`TypeStyle`) |
| Elevation | `src/core/MdElevation` + tonal `ColorRole::SurfaceContainer*` |
| No QSS | `TestMd3NoQss` gate |
