# Porting TODO

Upstream gaps and outstanding work. Stage 1 must be green before any Stage 2 item
is started (see [md3-coverage.md](md3-coverage.md)).

## Stage 1 — base modules (current)

- [ ] `MdTokens`: reference / system / component token layers, with global and
      per-instance override APIs
- [ ] `MdColorScheme`: full color-role enum + light / dark schemes (including the
      `*-fixed` roles)
- [ ] `MdDynamicColor`: HCT / CAM16, tonal palettes, core palette, dynamic scheme,
      material dynamic colors, contrast, blend, dislike, temperature, quantize,
      score; variants tonalSpot / vibrant / expressive / content / fidelity /
      monochrome / neutral; ColorSpec 2025 + 2026; contrast standard / medium / high
- [ ] `MdTheme`: singleton with lifecycle signals
- [ ] `MdTypeScale`: 30 styles with script-height-aware line height
- [ ] `MdShape`: shape scale + 35 decorative shapes + shape morph
- [ ] `MdMotion`: easing / duration tokens + spring solver
- [ ] `MdStateLayer`, `MdRipple`, `MdElevation`, `MdFocusRing`
- [ ] `MdIcon`: Material Symbols variable font + classic SVG baseline
- [ ] `MdStyleBase`: `QProxyStyle` base with paint-filter / theme hooks
- [ ] `MdFont` + `MdDesign::initialize(&app)`
- [ ] Resources under `resources/` + `.qrc` + manifest
- [ ] Token-value test against the `material-web` token sources
- [ ] Light / dark render test for every color role
- [ ] Fixed-seed dynamic-color test against the official implementation

## Stage 1 — components

Track progress in [md3-coverage.md](md3-coverage.md). Order follows the official
categories: Actions → Communication → Containment → Navigation → Selection →
Text inputs, then the M3 Expressive cross-cutting pass.

## Stage 2 — Qt extensions

Not started. Full list: appendix D of
[md3-qt-porting-prompts.md](md3-qt-porting-prompts.md). Blocked by the
`TestMd3CoveragePolicy` gate until Stage 1 is green.
