# Project Status

Last updated: 2026-10-09

## Snapshot

| Item | Value |
| --- | --- |
| Version | `0.1.0` (source of truth: [`VERSION`](../VERSION)) |
| Stage | Stage 1 — Batch 0 closed; §1.2 Actions closed; §1.3 Communication closed; §1.4 Containment closed; §1.5 Navigation closed (App bars, Toolbars, Navigation bar, Navigation rail, Navigation drawer and Tabs all in); §1.6 Selection opened |
| Stage 1 families complete | `27 / 36` (§1.2 Actions × 8, Badges, Progress indicators, Loading indicator, Snackbar, Tooltips, Cards, Dialogs, Bottom sheets, Side sheets, Carousel, Divider, Lists, App bars, Toolbars, Navigation bar, Navigation rail, Navigation drawer, Tabs, Checkbox) |
| Base modules | `21 / 21` |
| Public components | `40` (`MdButton`, `MdButtonGroup`, `MdIconButton`, `MdFab`, `MdExtendedFab`, `MdFabMenu`, `MdFabMenuItem`, `MdSplitButton`, `MdSegmentedButton`, `MdBadge`, `MdBadgedBox`, `MdProgressIndicator`, `MdLoadingIndicator`, `MdSnackbar`, `MdSnackbarHost`, `MdTooltip`, `MdTooltipHost`, `MdCard`, `MdDialog`, `MdDialogHost`, `MdBottomSheet`, `MdBottomSheetHost`, `MdSideSheet`, `MdSideSheetHost`, `MdCarousel`, `MdDivider`, `MdList`, `MdListItem`, `MdTopAppBar`, `MdBottomAppBar`, `MdDockedToolbar`, `MdFloatingToolbar`, `MdNavigationBar`, `MdNavigationBarItem`, `MdNavigationRail`, `MdNavigationDrawer`, `MdNavigationDrawerItem`, `MdTabs`, `MdTab`, `MdCheckBox`) |
| Style classes | `34` (`MdStyleBase`, `MdButtonStyle`, `MdButtonGroupStyle`, `MdIconButtonStyle`, `MdFabStyle`, `MdExtendedFabStyle`, `MdFabMenuStyle`, `MdSplitButtonStyle`, `MdSegmentedButtonStyle`, `MdBadgeStyle`, `MdProgressIndicatorStyle`, `MdLoadingIndicatorStyle`, `MdSnackbarStyle`, `MdTooltipStyle`, `MdCardStyle`, `MdDialogStyle`, `MdBottomSheetStyle`, `MdSideSheetStyle`, `MdCarouselStyle`, `MdDividerStyle`, `MdListStyle`, `MdListItemStyle`, `MdTopAppBarStyle`, `MdBottomAppBarStyle`, `MdDockedToolbarStyle`, `MdFloatingToolbarStyle`, `MdNavigationBarStyle`, `MdNavigationBarItemStyle`, `MdNavigationRailStyle`, `MdNavigationDrawerStyle`, `MdNavigationDrawerItemStyle`, `MdTabsStyle`, `MdTabStyle`, `MdCheckBoxStyle`) |
| Example pages | `34` |
| Bundled icons | `49` classic SVGs + `4299` Material Symbols codepoints |
| Bundled fonts | `0` (opt-in; see [resources-manifest.md](resources-manifest.md)) |
| CTest entries | `38` (`TestMd3Version`, `TestMd3Tokens`, `TestMd3Contrast`, `TestMd3TemperatureCache`, `TestMd3Ripple`, `TestMd3FocusRing`, `TestMd3Icon`, `TestMd3StyleBase`, `TestMd3Button`, `TestMd3ButtonGroup`, `TestMd3IconButton`, `TestMd3Fab`, `TestMd3ExtendedFab`, `TestMd3FabMenu`, `TestMd3SplitButton`, `TestMd3SegmentedButton`, `TestMd3Badge`, `TestMd3ProgressIndicator`, `TestMd3LoadingIndicator`, `TestMd3Snackbar`, `TestMd3Tooltip`, `TestMd3Card`, `TestMd3Dialog`, `TestMd3BottomSheet`, `TestMd3SideSheet`, `TestMd3Carousel`, `TestMd3Divider`, `TestMd3List`, `TestMd3AppBar`, `TestMd3Toolbar`, `TestMd3Navigation`, `TestMd3Tabs`, `TestMd3CheckBox`, `TestMd3NoQss`, `TestMd3CoveragePolicy`, `TestMd3SourceEncoding`, `TestMd3DeploymentTestBinary`, `TestMd3DeploymentExample`) |
| Supported Qt | Qt 6.5.0+ and Qt 5.15.2+ |

## What exists today

### Batch 0 — the foundation

Nothing in this list is a widget. It is the layer every component will be
measured against. All twenty-one modules are implemented and compiled in; the
outstanding behaviour gaps are listed separately below rather than glossed over.

| Module | Covers |
| --- | --- |
| `MdCore` | `md::libraryVersion()` and the generated version header |
| `MdTypes` | every shared enum, each with a `Count` sentinel, plus name helpers |
| `MdTokens` | three layers: `md.ref.*` (tones 0–100 for all six palettes), `md.sys.*` (colour, type, shape, elevation, motion, state), `md.comp.*` (global + per-instance override) |
| `MdColorMath` | CAM16 (J, C, h, M, s, Q, J\*, a\*, b\*), HCT with the 255 critical-plane solver, TonalPalette, CorePalette, CIELAB — ported from `material-color-utilities` |
| `MdColorScheme` | 49 colour roles, light and dark, including the fixed families; `baseline()` returns all six published sets (light/dark × standard/medium/high) verbatim |
| `MdColorSpec` | `ColorSpec2021`: the per-role tone solver — `ContrastCurve` (four corners at −1.0 / 0.0 / 0.5 / 1.0), `ToneDeltaPair`, the awkward-zone rule, dual backgrounds, and `MdContrast` (`ratioOfTones`, `lighter`/`darker`, `foregroundTone`) |
| `MdTemperatureCache` | Lab-temperature utilities: `rawTemperature`, `complement`, `analogous`. Feeds the `content` and `fidelity` tertiary palettes and nothing else |
| `MdDynamicColor` | all nine variants, `harmonize`, `hctHue`; every role resolved through the `ColorSpec2021` solver so all four contrast levels take effect |
| `MdTheme` | singleton with `themeAboutToChange` / `themeChanged` / `themeModeChanged` |
| `MdTypeScale` | 15 baseline + 15 emphasized styles, script-category line height |
| `MdShape` | the full corner scale, radius interpolation, path morphing |
| `MdMotion` | 10 easings, 16 durations, and the six Expressive spring slots |
| `MdElevation` | tonal surfaces 0–5; shadows opt-in only |
| `MdStateLayer` | hover 0.08 / focus 0.12 / pressed 0.12 / dragged 0.16, strongest-wins |
| `MdRipple` | press ripple geometry, timing and shape clipping; separate from the state layer by design |
| `MdFocusRing` | the 3dp indicator, 2dp gap, 8px grow-then-settle on an emphasized curve |
| `MdIcon` | Material Symbols four axes plus the classic SVG baseline |
| `MdResources` | keeps the embedded `.qrc` alive in the static archive (see the fixes section below) |
| `MdFont` | bundled font registration and global application |
| `MdDesign` | `configureHighDpi()` + `initialize(&app)` startup entry point |
| `MdStyleBase` | `QProxyStyle` base: paint filters, theme subscriptions, crisp rounded rects |

### Build system and gates

- CMake auto-detects Qt 6 / Qt 5, enforces the minimum, installs an exportable
  package (`find_package(qt-md3 CONFIG REQUIRED)`), and registers the `.qrc`.
- Sixteen CTest entries, all green. One binary per module, each with its own
  `QTEST_MAIN` — a single binary hosting several `QObject` classes driven by
  `QTest::qExec` in a loop cannot honour `-o` or per-class selection, so
  `ctest -R TestMd3Icon` would silently run the wrong suite.
  - `TestMd3Tokens` — 22 assertions pinning every published number against its
    source, including the CAM16 reference values from `material-color-utilities`
    and the 49-role light/dark scheme against the current `material-web` token
    sets.
  - `TestMd3Contrast` — the colour pipeline's two references, kept apart on
    purpose. `resources/tokens/md-sys-color-*.txt` (six sets, from material-web)
    pins the *static baseline*; `resources/tokens/mcu-scheme-expectations.txt`
    (367 assertions lifted from material-color-utilities' own Swift tests) pins
    the *dynamic solver* across nine variants, both modes and three contrast
    levels. A third check pins the fact that the two are *supposed* to differ,
    because "reconciling" them is the mistake this suite exists to catch.
  - `TestMd3TemperatureCache` — `rawTemperature`, `complement` and `analogous`
    against the exact values in material-color-utilities'
    `temperature_cache_test.cc`. This module feeds `content` and `fidelity`
    tertiary and nothing else, so a defect in it is invisible from every other
    variant; it had one, and this is what catches the next.
  - `TestMd3Ripple`, `TestMd3FocusRing`, `TestMd3Icon`, `TestMd3StyleBase` —
    the interaction primitives and the Pattern A paint hub: geometry, timing,
    lifecycle, signals, meta-property lookup, and a render smoke check for each
    so a style that silently paints nothing cannot pass.
  - `TestMd3NoQss`, `TestMd3CoveragePolicy`, `TestMd3SourceEncoding` — the three
    policy gates.
  - `TestMd3DeploymentTestBinary`, `TestMd3DeploymentExample` — run a unit-test
    binary and the example with `PATH` reduced to the Windows directories and
    `QT_PLUGIN_PATH` / `QT_QPA_PLATFORM` cleared, which is the environment a
    double-click produces. They fail if the Qt runtime deployment is incomplete,
    the one failure mode no other test can see.
  - `TestMd3Version` — the library links, loads and reports its version.
- The example is a 33-page gallery under `examples/gallery/`, built into
  `build/` and never committed. `--screenshot <dir>` renders every page to PNG
  and quits, which is how the pages are checked for a non-blank result: a page
  whose `paintEvent` draws nothing still exits 0, so "the window opened" proves
  nothing. It writes two images per page — the window, and the page widget
  grown to its own `heightForWidth()` — because the window shot can only ever
  prove that the *top* of a page draws.

### Stage 1 §1.2 Actions (all eight families) + §1.3 Badges, Progress indicators, Loading indicator, Snackbar and Tooltips + §1.4 Cards + §1.5 App bars and Toolbars

| Component | Covers |
| --- | --- |
| `MdButton` | `QPushButton` subclass, 5 colour styles (elevated / filled / tonal / outlined / text) × 5 Expressive sizes (xsmall 32 → xlarge 136 px) × 2 container shapes, leading / trailing icons, mnemonic text, soft-disabled. Six `Q_PROPERTY`s with NOTIFY. |
| `MdButtonTokens` | the `md.comp.button.*` value layer: size-driven metrics, both shape slots, the pressed-shape spring, and the per-variant, per-state colour slots including the disabled opacities. Read from `tokens/versions/latest/sass` (34.0.21) — see [porting-todo.md](porting-todo.md) for why this one component uses a different export than `MdTokens`. |
| `MdButtonStyle` | Pattern A style: layout and painting, registered in the paint hub so one style serves the whole family. Also owns `focusRingInset()`, which derives the 7.5 px focus margin from the focus-indicator tokens instead of hard-coding it. |
| `MdButtonGroup` | `QWidget` container for 2–n `MdButton` items — standard (press grows an item 15 % on `spring-fast-spatial` and shifts the neighbours) and connected (2 px gap, per-side corners) variants, five Expressive sizes, four selection modes (none / single / multiple / required), round / square shapes, horizontal / vertical orientation, arrow-key navigation with focus-follows-current. Eight `Q_PROPERTY`s with NOTIFY. |
| `MdButtonGroupTokens` | the `md.comp.button-group.{standard,connected}.<size>` value layer: heights, between-space, the 15 % press multiplier, the spring constants, the connected corner ladder and the literal 50 % selected inner corner. |
| `MdButtonGroupStyle` | a style with a deliberately *empty* `drawWidget()` — the spec calls the group an invisible container with no colour attributes, so it owns geometry (`layoutFor`), per-item corner radii (`itemRadii`) and margins only. |
| `MdIconButton` | `QPushButton` subclass, 4 colour styles (standard / filled / tonal / outlined) × 5 Expressive sizes × 2 shapes × 3 published padding tracks, icon-only by token arithmetic, toggle form backed by Qt's checkable state with the `selected-*` / `unselected-*` colour families and the swapped selected corner shapes. Nine `Q_PROPERTY`s with NOTIFY. |
| `MdIconButtonTokens` | the `md.comp.icon-button.*` value layer: three colour families (plain / selected / unselected) over five states, the five-size metric table, the five shape slots and the press spring. |
| `MdFab` | `QPushButton` subclass, 4 colour sets (surface / primary / secondary / tertiary) × 3 sizes (small 40 / medium 56 / large 96 px, corners 12 / 16 / 28 px) × lowered / raised elevation rows, icon-only by token arithmetic, the one Actions family with a real per-state shadow. Six `Q_PROPERTY`s with NOTIFY. |
| `MdFabTokens` | the `md.comp.fab.*` value layer: the three-size metric table, the four colour sets (flat interactive rows), the lowered elevation rows and the spec-filled disabled row — the export publishes none. |
| `MdExtendedFab` | `QPushButton` subclass, 6 colour sets (primary / secondary / tertiary + three `*-container`, no surface) × 3 sizes (small 56 / medium 80 / large 96 px, corners 16 / 20 / 28 px, icons 24 / 28 / 36) × lowered / raised elevation rows, icon + label on a content-derived width (leading + icon + gap + label + trailing, not a token). Six `Q_PROPERTY`s with NOTIFY. |
| `MdExtendedFabTokens` | the `md.comp.extended-fab.*` value layer: the three-size metric + type-scale table, the six colour sets (flat interactive rows reusing the FAB family structs), the lowered elevation rows and the spec-filled disabled row. |
| `MdFabMenu` | `QWidget` container: an anchor `MdFab`, a 56 px close button sharing its top trailing corner, and up to six list items staggering in top-down on the Compose `SpatialDefault` spring (40 ms stagger) — the export publishes no motion rows. `Q_PROPERTY`s for variant / expanded / anchor icon, `itemActivated(int, QString)`. |
| `MdFabMenuItem` | `QPushButton` subclass for both element shapes (close button / list item), each with its own published token rows; `reveal` property drives the staggered open animation (opacity + 24 px settle). |
| `MdFabMenuTokens` | the `md.comp.fab-menu.*` value layer: the common spacing rows (8 / 4), both elements' metric + colour + elevation tables across the three colour groups, and the spec-filled disabled row. |
| `MdSplitButton` | one `QWidget` with two press targets: a leading half (icon + label) and a trailing half (the dropdown icon), separated by the published 2 px gap. The facing corners morph on hover / press (4/4/4/8/12 -> 8/12/12/20/20 px per size) and go to the literal 50% while the trailing half is selected; outer corners stay a full pill. Left/Right walk the halves, Up/Down are swallowed, Space/Enter activate the focused half. |
| `MdSplitButtonTokens` | the `md.comp.split-button.<size>.*` metric layer (five rows verbatim, inner corners resolved from the shape scale) plus the button family's colour / type / focus rows at the identity size mapping — the spec page: "the same color schemes as standard buttons". |
| `MdSegmentedButton` | one `QWidget` with N press targets: segments share one 40 px pill outline, neighbours overlap by exactly the outline width so the shared edge is the divider, `itemShape` rounds only the row's ends, and the check scales in on a reserved 18 px icon slot (spring-fast-spatial) so the label never moves. Single-choice (radio) by default, multi-choice per segment. |
| `MdSegmentedButtonTokens` | the single `md.comp.outlined-segmented-button.*` set verbatim (no size scale, no colour variants) with the Compose-sourced rows the export lacks (8 px icon spacing, 12 px content padding, spring-fast-spatial check motion) — every borrow labelled in the header. |
| `MdBadge` | `QWidget` (non-interactive by contract: `Qt::NoFocus`, transparent for mouse events), one token set with two forms decided by content — the 6 px dot and the minimum-16 px pill (label-small on-error text, 4 px Compose side padding). Empty accessible name falls back to "badge". One `Q_PROPERTY` (`text`) with NOTIFY. |
| `MdBadgedBox` | the anchor container: content widget plus one `MdBadge` positioned with the Compose offsets (dot 6/6, pill 12/14); reserves the pill overhang (width − 12 / height − 14) in its own geometry because a Qt child is clipped to its parent — the one deliberate divergence from Compose's free overlap, recorded in the header. |
| `MdBadgeTokens` | the `md.comp.badge.*` value layer verbatim (dot 6, large 16, both corner-full, error / on-error, label-small); no state rows exist in the export — non-interactivity is a published fact, not a gap. |
| `MdBadgeStyle` | Pattern A style: the quietest painter in the library — a corner-full container and, in the content form, one label-small text. No state layer, no ripple, no focus indicator. |
| `MdProgressIndicator` | `QWidget` (non-interactive by contract), one merged family with two shapes: linear (determinate with the 4 px gap + stop indicator, buffer with the scaled track and scrolling 2 px dots, the MDC 2 s two-bar indeterminate) and circular (determinate arc from 12 o'clock, the three-composed-rotation indeterminate). Determinate value changes transition over the published 250/500 ms curves; `fourColor` rides the deprecated four-color sets. Six `Q_PROPERTY`s with NOTIFY. |
| `MdLoadingIndicator` | `QWidget` (non-interactive by contract), the Expressive shape-morphing family: plain and contained variants, the seven-shape indeterminate loop (650 ms morph grid, closed-form spring with the bounce kept, quarter-turn steps over the 4666 ms linear spin) and the determinate circle→soft-burst walk with the −180° sweep. Backed by the `MdMaterialShapes` engine (a faithful graphics-shapes port) and the radial morph — the one registered divergence. Four `Q_PROPERTY`s with NOTIFY. |
| `MdProgressIndicatorTokens` | the merged `md.comp.progress-indicator.*` value layer verbatim — base + linear + circular rows, the deprecated base metrics, the deprecated `thick.*` rows (transcribed, never exposed as an API) and the four-color roles from the deprecated per-shape sets. The Expressive wave rows are carried but not rendered (the family's registered gap). |
| `MdProgressIndicatorStyle` | Pattern A style plus the indeterminate math as *pure functions of elapsed time* (`linearIndeterminateFrame`, `circularIndeterminateFrame`, `fourColorAt`) so the tests pin the keyframes field by field without an event loop. Owns a hand-rolled cubic-bezier solver — `QEasingCurve` has no public per-t evaluation. |
| `MdSnackbar` | `QWidget`, the inverse-surface one-row / new-line layouts ported from Compose's measure policies: single-line height max(48, content), wrapped first line at 30 px, two-line minimum 68; the action and the dismiss icon are internal regions painting this export's own hover / focus / pressed rows with hit-region-first geometry. Visuals only — placement and durations are the host's. Five `Q_PROPERTY`s with NOTIFY. |
| `MdSnackbarHost` | the `SnackbarHostState` half: queue, Auto/Short 4000 / Long 10000 / Indefinite durations (Auto pins Indefinite when an action exists), bottom-centre placement inside the 12 px margin and the FadeInFadeOutWithScale enter/exit (opacity on the effects-fast spring, scale 0.8→1 on the spatial-fast spring). |
| `MdSnackbarTokens` | the single `md.comp.snackbar.*` set verbatim: inverse-surface container (level 3, corner-extra-small), body-medium inverse-on-surface text, label-large inverse-primary action, 24 px inverse-on-surface dismiss icon, and the hover / focus / pressed rows for BOTH interactive elements. |
| `MdSnackbarStyle` | Pattern A style with the one-row / new-line measure policies as pure functions (every rect pinned by tests) and `paintSnackbar` shared with the host's fade/scale transform; `kShadowMargin` keeps the level-3 shadow inside a child widget's clip. |
| `MdTooltip` | `QWidget` for both exports: plain (inverse-surface, corner-extra-small, no elevation, body-small inverse-on-surface) and rich (surface-container at level 2, corner-medium, title-small subhead + body-medium text in on-surface-variant, label-large primary action) plus the opt-in 16×8 Expressive caret. The rich action is an internal region painting the export's own hover / focus / pressed rows; `:focus-visible` semantics like the button families. Five `Q_PROPERTY`s with NOTIFY. |
| `MdTooltipHost` | the `TooltipBox` half on Qt's widget tree: wraps an anchor widget, shows a top-level tooltip popup on hover (Compose's UserInput priority — never self-dismisses, pointer leaving hides it), on keyboard focus / touch long-press / programmatically (1500 ms auto-hide unless persistent, `BasicTooltipDefaults.TooltipDuration`), Escape dismisses, the GlobalMutatorMutex keeps exactly one tooltip on screen, and the enter/exit is the same fade + scale pair. Above / Below placement with the plain `Above` provider (centre→start→end, above→below, coerced) and the rich start-aligned provider. |
| `MdTooltipTokens` | the `md.comp.plain-tooltip.*` + `md.comp.rich-tooltip.*` sets verbatim, the rich action's three state rows, and the Compose-port layout constants (min 40×24, plain max 200, rich max 320, content 8/4, rich horizontal 16, subhead baseline 28, text 24/16, action 36/8, anchor spacing 4, caret 16×8, duration 1500 ms). |
| `MdTooltipStyle` | Pattern A style with the plain / rich measure logic and the two popup-position providers as pure functions, the caret triangle painting, and `paintTooltip` shared with the host's fade/scale transform; the rich container keeps the shadow-margin idiom for the level-2 rings. |
| `MdCard` | `QWidget` container for the three exports with Compose's clickability split: non-clickable cards paint the enabled row edge-to-edge (no interaction source, no focus), clickable cards add the hover / keyboard-focus state layers, the press ripple (press never raises — the "Pressed (ripple)" row repeats the resting elevation), the `:focus-visible` ring with its reserved contents margin, Space/Enter activation and the `clicked()` signal. Children fill the painted container through the contents margins. The elevation ladder *animates* (standard easing 200 ms; disabled snaps), and an explicit `dragged` setter paints the dragged row for external drag frameworks. Three `Q_PROPERTY`s with NOTIFY. |
| `MdCardTokens` | the three `md.comp.<variant>-card.*` sets verbatim — per-state container / state-layer / outline roles and the elevation ladders (filled L0→L1→L3, elevated L1→L2→L4, outlined L0→L1→L3), the exported disabled rows (filled → surface-variant, elevated → surface, outlined container unchanged) and Compose's disabled constants (0.38 container / 0.38 content / 0.12 outline). |
| `MdCardStyle` | Pattern A style: the layout as a pure function (container = widget rect minus the focus margin for clickable cards, zero for plain surfaces), shadow via the elevation ladder's animated dp, the disabled compositing exactly as Compose does it (disabled colour over the enabled container), and `paintCard` shared with the test suite. |
| `MdDialog` | `QWidget` carrying Compose's four AlertDialog slots: the headline and supporting text are painted by the style (the snackbar idiom), the icon slot is any `QWidget` handed to `setIconWidget`, and the action slots are real Text-variant `MdButton` children — the export's action rows are the text button's own tokens, so ripple and `:focus-visible` come free (the recorded divergence from the snackbar's self-drawn action). Escape emits `dismissed()`. Two `Q_PROPERTY`s with NOTIFY. |
| `MdDialogTokens` | the single `md.comp.dialog.*` export verbatim (34.0.21) — container surface-container-high at level3 with the extra-large corner, headline-small on-surface, body-medium on-surface-variant, 24 px secondary icon, label-large primary action with the hover/focus/pressed state rows (0.08/0.12/0.12), the deprecated-but-published divider rows, Compose's private layout constants (24 px padding, icon 16 / title 16 / text 24 bottoms, 8 px action spacing, 280..560 width clamp, 0.32 scrim) and the shadow-margin idiom. |
| `MdDialogStyle` | Pattern A style: the layout as a pure function of a `ContentSpec` (icon centred, title start-aligned, text wrapped, one or two end-aligned action rows restating Compose's RTL FlowRow trick — confirm rightmost, wrapping confirm-above-dismiss), `paintDialog` shared with the test suite, painter-opacity carrying the host's fade. |
| `MdDialogHost` | the Compose `BasicAlertDialog` half: an overlay across its parent painting the scrim (black 0.32, faded by the transition), the dialog centred and width-clamped into 280..560, Escape and scrim clicks running the `onDismissRequest` flow (`dismissed()` emitted at request time), fade-in/out on the effects-fast spring — dialogs fade only, no scale. |
| `MdTopAppBar` | `QWidget`, the five published size layouts (small 64 / medium 112 / large 152 / medium-flexible 112 and 136 / large-flexible 120 and 152) × 2 alignments, with a navigation slot, a centre slot (the search app bar is this configuration, not a sixth layout) and an action row. A two-row bar collapses only its *text row* — the 64 px icon row never leaves — so medium loses 48 px and large 88. Surface, subtitle, badge-free: this family publishes no hover / press / focus rows at all, because its interaction *is* scrolling. Five `Q_PROPERTY`s with NOTIFY. |
| `MdBottomAppBar` | `QWidget`, the single 80 px row with the export's `start 4 / top 4 / end 4` content band (no bottom value, faithfully), four arrangements and an optional docked FAB placed by its *container* — 16 px from the trailing edge, 12 px from the top. Three `Q_PROPERTY`s with NOTIFY. |
| `MdAppBarTokens` | the `md.comp.app-bar.*` value layer: the common set (avatar 32, icons 24, leading/trailing 4, search 8, the surface → surface-container / level0 → level2 scroll rows) over all five size sets verbatim, plus `md.comp.bottom-app-bar.*`. Carries the derived geometry the export leaves implicit — `titleInset()` = 16 − 4, the medium 24 / large 28 title bottoms, and `collapsedRowHeight`, which is read from the **small** set whatever the variant (that is what makes a medium bar stop at 64). |
| `MdAppBarScrollBehavior` | the `TopAppBarState` half as a `QObject`: the coerced `heightOffset`, `collapsedFraction`, `overlappedFraction` (verbatim, special case included), the three modes (Pinned / EnterAlways / ExitUntilCollapsed with Compose's own pre- vs post-scroll division of labour), settle-and-snap on the `spring-effects-default`, and `followScrollBar()` for a plain `QScrollBar` host. |
| `MdTopAppBarStyle` | Pattern A style: the layout as a pure function (nav centred in the leading row, title aligned against the bar and *then* pushed past the nav and action boxes — Compose's own order, and measured against the *containers* the slots paint rather than the widgets' `sizeHint`s), the colour transition as `lerp(container, scrolled, FastOutLinearInEasing(fraction))` interpolated in **Oklab**, and the two title alphas (`TopTitleAlphaEasing` for the small title, `1 − collapsedFraction` for the expanded one). |
| `MdBottomAppBarStyle` | Pattern A style: content band, the FAB's container target rect, the actions band that stops at it, and the per-child container box each of the row's own children belongs on — already distributed by the arrangement and already vertically centred, so the widget's placement pass has no arithmetic left to do. |
| `MdDockedToolbar` | `QWidget`, the docked variant: one 64 px row, `corner-none`, surface-container, its content 16 px in from each end with the items separated by the token's 32 px maximum and the run centred (there is no `arrangement` property, because the published component has none). `heightOffset` collapses the **whole** bar — `heightOffsetLimit()` is `-64`, not a row — and the children stay where they were, so the widget's shorter rect clips them, which is `BottomAppBarLayout`'s own `place(0, 0)` inside a shrinking box. `addWidget` / `insertWidget` / `removeWidget` / `clearWidgets`. One `Q_PROPERTY` with NOTIFY. |
| `MdFloatingToolbar` | `QWidget`, the floating variant: a `corner-full`, level3 pill with 8 px of content padding and the published 4 px between its slots, `orientation` (horizontal / vertical, an exact transpose), `colorScheme` (standard / vibrant), leading and trailing slots that exist only while expanded, a content run, an optional action button (`End` — the axis' end — by default) whose two size sets are 56 expanded and 80 collapsed, and `expansionProgress` / `expanded` driven by the effects spring. A child whose container has left the pill is hidden, Qt's clip standing in for Compose's `graphicsLayer { clip = true }`. Five `Q_PROPERTY`s with NOTIFY. |
| `MdDockedToolbarTokens` | the `md.comp.toolbar.docked.*` value layer verbatim (height 64, leading / trailing 16, max-spacing 32, `corner-none`, surface-container), plus the published-but-unread `min-spacing` (Compose reads `max` and only `max`) and the two invented `content.top-space` / `bottom-space` keys, which are not token rows because the published row centres its content rather than padding it. |
| `MdFloatingToolbarTokens` | four published sets: `md.comp.toolbar.floating.*` (with the deprecated single `container.height` and `external-padding` rows carried unread beside the horizontal / vertical pair), `floating.fab.*` (both size sets — expanded 56 / icon 24 / `corner-large` / level1 and collapsed 80 / icon 28 / `corner-large-increased` / level2), and `standard.*` / `vibrant.*` with five states × selected and unselected, including the export's own two gaps (no selected state-layer opacities, no selected disabled row). Plus Compose's `ScrollDistanceThreshold` 40. |
| `MdDockedToolbarStyle` | Pattern A style: the layout as a pure function — the container is the widget's rect, but the content band and the child boxes are laid out against the **expanded** rect, so a collapse never reflows the row — and the `spacedBy(32, centred)` distribution that lets a too-wide run overflow both ends rather than compress. |
| `MdFloatingToolbarStyle` | Pattern A style: one axis-generic layout serving both orientations (every value written in main / cross), the pill measured at `maxIntrinsicWidth * expandedProgress` while the outer bounds stay put, the action strip reserved at the **expanded** FAB size, the FAB's two size sets and the level3 shadow ramped by progress. |
| `MdNavigationBar` | `QWidget`, both published families under one `variant`: the baseline `navigation-bar` one (80 px, a 64x32 pill, the selected label on-surface) and the flexible `nav-bar` one (64 px, 56x32, secondary). Items divide the band equally with the behaviour's 8 px gap (Compose's `spacedBy(8.dp)`, not the export's 0px row), and the flexible family's `Centered` arrangement insets the band by the fraction `(100 - 10 * (count + 3)) / 2 / 100`. The level2 elevation is carried and not painted (the spec's "no shadow"). `addItem` / `insertItem` / `removeItem` / `clearItems` / `currentIndex`, `EqualWeight` / `Centered`, and the flexible family's `itemLayout` (`Top` / `Start`) pushed onto every item. Three `Q_PROPERTY`s with NOTIFY. |
| `MdNavigationBarItem` | the shared expressive item — a checkable `QPushButton` whose pill opens behind the icon on a width-only `FastSpatial` spring, the label riding the same progress when `alwaysShowLabel` is off. Two arrangements: `Top` (icon above the label) and `Start` (icon beside it, the pill wrapping both, the label painted in the pill's own content colour). The press re-maps into the pill (`MappedInteractionSource`'s Qt equivalent), the `:focus-visible` ring draws inward on the pill, and the boxes are exposed for the tests. Eight `Q_PROPERTY`s with NOTIFY. |
| `MdNavigationBarTokens` | both families in one variant array, each resolve branch writing its own literal defaults (inheriting the struct's baseline numbers would silently publish the short family tall). Carries the four pinned divergences in its header; the item colour fill is exported as `fillNavigationItemColours` and shared with the rail. |
| `MdNavigationBarStyle` / `MdNavigationBarItemStyle` | Pattern A styles: the bar's `layoutFor` divides the band and applies the Centered fraction; the item's paint draws the animated pill, the state layer clipped to it, the ripple as the pressed layer, the icon and label, and the inward focus ring last. |
| `MdNavigationRail` | `QWidget`, the same two-family split: the baseline (80 px, no expanded rows — `setExpanded(true)` is refused) and the flexible one, whose `expanded` **state** animates the width on a spatial spring (96 collapsed, content-driven between the published 220 and 360, `FastSpatial` when modal) and flips every item between its Top pill and its Start pill with a label. The header is a foreign widget the rail places. The modal rows are the drawer's Expressive replacement. Three `Q_PROPERTY`s with NOTIFY. |
| `MdNavigationRailTokens` | the baseline `navigation-rail.*` rows (with Compose's three hard-coded spacings 4 / 4 / 8 carried as the behaviour's numbers and the published 56x56 no-label pill) and the flexible `nav-rail-collapsed` / `-expanded` / `nav-rail` / `nav-rail-item*` rows, whose tables match Compose's `WideNavigationRail*Tokens` line for line; the item field is the shared `MdNavigationBarVariantTokens` pushed into the bar's item. |
| `MdNavigationRailStyle` | Pattern A style: the collapsed / expanded spacing switch (4 vs the expanded family's 0), the Top / Start item flip at the expanded edge, and the content-driven expanded width as a pure function clamped into the published bounds. **Layout functions read `sizeHint()` and never measure** — this family is where the measure-in-paint defect was found and the rule was written. |
| `MdNavigationDrawer` | `QWidget`, the family's one file with two container row groups (`variant`: modal `surface-container-low` at level1, standard `surface` at level0), an optional `title-small` headline and divider, and items placed by **direct geometry** — the pill is the item, its width the container minus the behaviour's 2 x 12 `ItemPadding`, nothing to centre. The scrim and the modal elevation are carried and not painted; `scrimColor()` / `scrimOpacity()` expose them for a host overlay. Four `Q_PROPERTY`s with NOTIFY. |
| `MdNavigationDrawerItem` | an independent checkable `QPushButton` — deliberately *not* the shared expressive item, because the pill **is** the item: a full-width, 56 px-minimum row whose container colour is the selected state, no width animation, a trailing badge text from the `large-badge-label-*` rows. Its content row is Compose's hard-coded `start 16 / icon 24 / gap 12 / end 24`. Six `Q_PROPERTY`s with NOTIFY. |
| `MdNavigationDrawerTokens` | the `md.comp.navigation-drawer.*` set verbatim, the item's own colour table (every active row `on-secondary-container`; the inactive pressed state layer the one-row special case; `label-large`), and the carried scrim rows with the export's `neutral-variant20` recorded against the library's `neutral0` Scrim role. |
| `MdNavigationDrawerStyle` / `MdNavigationDrawerItemStyle` | Pattern A styles: the sheet's `corner-large-end` radii on the end pair, mirrored in RTL (the bottom-sheet precedent), the headline and divider boxes, and the item's paint — the full-width pill, the state layer and ripple clipped to it, the left-aligned label and badge, the inward focus ring. |
| `MdChildBox` | the shared answer to "where does a child widget actually paint": `measure()` sizes a child and reports the container inside it, `geometryOn()` returns the geometry that puts that container on a target box, and `resizedGeometryOn()` is the sibling for the one child whose size is *not* its own — it resizes the widget to `box + 2 * margin` and **centres** it on the box, which is the only rule that lands the container on the box both for a component that fills `widget - margin` and for one whose container is a fixed token size inside a larger widget. Every component that can show a focus indicator reserves 7.5 px a side inside itself (Qt clips a child to its own rect, so the room cannot live outside it), which makes `sizeHint()` the wrong thing for a *container* to lay a child out by. `MdButtonGroup` established the rule inline; `MdChildBox` is it factored out, and both app bars, both toolbars and the FAB menu use it. |
| `MdTabs` | `QWidget`, both published families under one `variant` (primary: a 3 px indicator rounded on top, animated to the selected tab's *content* width; secondary: a 2 px square indicator spanning the whole tab) and one `layout` (Fixed divides the width evenly; Scrollable keeps a 52 px edge padding, a 90 px minimum tab, and scrolls the selection towards the centre on the spatial spring). The row paints the container, the deprecated divider and the indicator; the indicator's first placement lands without a spring. `addTab` / `insertTab` / `removeTab` / `clearTabs` / `currentIndex`, a Left/Right (wrapping) and Home/End keyboard walk. Three `Q_PROPERTY`s with NOTIFY. |
| `MdTab` | a checkable `QPushButton` in three content shapes: text-only and icon-only at 48 px, icon-above-label at 64 px, and the `LeadingIconTab`'s icon-beside-label at 48 px. The content colours cross-fade on the effects springs (in `EffectsDefault`, out `EffectsFast`) through an Oklab interpolation; the press ripple is coloured with the *active* side's pressed colour - the primary family's `inactive.pressed` one-row special case; `indicatorContentWidth()` is the arithmetic the primary indicator animates to. The `:focus-visible` ring draws inward. Seven `Q_PROPERTY`s with NOTIFY. |
| `MdTabsTokens` | both `md.comp.primary-navigation-tab.*` and `md.comp.secondary-navigation-tab.*` resolved verbatim, plus Compose's behaviour constants (`HorizontalTextPadding` 16, `TextDistanceFromLeadingIcon` 8, `ScrollableTabRowEdgeStartPadding` 52, `ScrollableTabRowMinTabWidth` 90, the 24 px indicator floor). Carries the three pinned divergences in its header: the 64-vs-72 icon+label height, the secondary indicator's 2-vs-3, and the fixed row's missing centring. |
| `MdCheckBox` | a `QCheckBox` whose whole surface is repainted: the 18 px box (2 px corners) centred in the 48 px touch target with the 40 px circular state layer behind it. The check is one path through Compose's fractions, revealed along its length on the spatial default spring; the indeterminate dash is the same path gravitated onto the centre line (`Off → Indeterminate` snaps the gravitation, an undo runs `snap(delay 100)`); the box, border and check colours cross-fade between the unselected/selected tables on the effects springs and **snap** into disabled. `hitButton` is the whole rect — Qt's native hit area is the style's indicator sliver, dead at the touch target's centre. `error` is a `Q_PROPERTY`; tristate cycles Off → On → Indeterminate → Off. |
| `MdCheckBoxTokens` | the `md.comp.checkbox.*` set: the four colour tables indexed `[selection][interaction]` (with the unselected-pressed `primary` / selected-pressed `on-surface` special cases), the error overlay tables with the disabled fallback baked into the accessors, the system focus-indicator rows (outer offset 2, thickness 3, secondary) and Compose's behaviour constants (the 100 ms snap delay, the 48 px touch target, the check fractions, the carried 25 % ring shape). The deprecated rendering-model rows are carried and read by nothing. |
| `MdCheckBoxStyle` | the paint: the circular hover/focus state layer, the ripple clipped to that circle and coloured with the pressed row, Compose's `drawBox` port (one fill when the border and fill resolve equal; otherwise an inset fill plus the stroke) and the `drawCheck` port (a partial polyline through the gravitated fractions, square cap), then the outward secondary focus ring around the box. |
| `MdColorMath::lerpOklab` | new shared primitive: Oklab interpolation with a linear alpha, added because every *animated colour* in Compose goes through `Color.VectorConverter`, which interpolates in Oklab. Interpolating the app bar's container in sRGB instead lands on a different midpoint. |

## Known gaps, recorded rather than hidden

1. **35 Expressive decorative shape paths are not ported.**
   `MdShape::decorativeShapeCount()` returns `0`. The shape *scale* and shape
   *morphing* are complete; the decorative path library is not.
2. **`ScriptCategory::Large` and `ExtraLarge` line-height multipliers.**
   Only `Small` (1.00) and `Medium` (1.07) are sourced. `Large` and
   `ExtraLarge` currently fall back to `Medium`'s 1.07.
3. **Variable font axes need Qt 6.7+.**
   On Qt 6.5/6.6 `MdIcon::axesSupported()` returns false and only the named
   weights are honoured.
4. **The progress-indicator wave rows are carried but not rendered.**
   The merged export publishes non-deprecated Expressive wave tokens
   (amplitude/wavelength, with-wave container sizes); neither material-web
   nor this port renders them. Compose's `WavyLinearProgressIndicator` /
   `WavyCircularProgressIndicator` are the porting source; see
   [porting-todo.md](porting-todo.md).
5. **RTL mirroring is not library-wide.**
   `MdTheme::isRightToLeft()` is honoured by `MdButtonStyle`,
   `MdButtonGroupStyle`, `MdSegmentedButtonStyle` and `MdSplitButtonStyle` and
   by nothing else, so cards, dialogs, sheets, lists, carousels, snackbars,
   tooltips and now the app bars lay themselves out the same way in an RTL
   locale. It is a tracked cross-cutting item in
   [md3-coverage.md](md3-coverage.md) ("RTL 全组件"); the App bars family is the
   first to state the gap in its own code rather than carry a comment claiming
   a mirroring that does not happen.

`ContrastLevel` used to head this list. It is closed: `MdColorSpec` solves every
role against its own `ContrastCurve` and `ToneDeltaPair`, and all four levels
(reduced / standard / medium / high) take effect. See
[porting-todo.md](porting-todo.md) for what that required, including the
distinction between the published static baseline and the dynamic solver that
had to be drawn before the numbers could be verified at all.

Everything above is in [`porting-todo.md`](porting-todo.md) with its source
question attached.

## Fixed while closing Batch 0

Three defects were found by actually running the result rather than by reading
it. They are recorded because each one was invisible to the compiler and to
review.

1. **The embedded resources were never linked in.**
   CMake's AUTORCC put `qrc_qt-md3.cpp` inside `libqt-md3.a`, but nothing
   referenced its initialiser, so the linker dropped the object and every
   `:/qt-md3/...` lookup failed — the icon codepoint table, the classic SVG
   baseline and the font directory. `MdResources::ensure()` is the reference
   that keeps it; `MdIcon` and `MdFont` call it from the accessors that read a
   resource. This affected any application linking the static library, not just
   the tests.
2. **`MdRippleController::currentFrame()` reported a painted ripple while idle.**
   A caller painting on `frame.valid` rather than on `isActive()` got a phantom
   blot in the top-left corner before the first press. Idle now yields an
   invalid frame, and `MdRipple::geometryFor()` refuses a non-positive extent
   instead of clamping a negative one to zero.
3. **A UTF-8 em dash was decoded as Latin-1 in the gallery**, rendering as
   mojibake. Fixed at the call site and gated: `TestMd3SourceEncoding` now
   rejects the Latin-1 decoders outright and validates that every source file is
   well-formed UTF-8.

## Fixed while closing the contrast levels

The `ContrastLevel` work surfaced a defect that had been sitting under two other
symptoms, and it is worth recording because of how quiet it was.

4. **`MdTemperatureCache` read the three Lab components out of order.**
   `MdTonalPalette::labFromArgb` returns `{L*, a*, b*}` — but through a generic
   `MdVec3{a, b, c}`. `rawTemperature` then read `.a` where a\* was meant (it got
   L\*) and `.b` where b\* was meant (it got a\*). Every number still looked like
   a plausible temperature and stayed inside its documented range; the whole
   warm/cool axis was simply rotated by about a quadrant. The only visible
   symptom was that the `content` and `fidelity` variants produced strange
   tertiary hues — 24 of material-color-utilities' 367 scheme assertions failed,
   all of them on `tertiary` / `tertiary-container` for those two variants and
   nothing else, which is exactly the fingerprint of this module.

   Two things came out of it. `MdVec3` is fine as a generic triple but not as a
   colour-space result, so `labFromArgb` now returns a named `MdLab{l, a, b}`
   and the mistake cannot be repeated. And the module moved out of an anonymous
   namespace inside `MdDynamicColor.cpp` into `MdTemperatureCache`, with
   `TestMd3TemperatureCache` pinning it against the exact values in
   `temperature_cache_test.cc`. It had no test before, which is why a fully
   rotated colour-temperature axis went unnoticed.

   Worth noting how it was found: not by reading the code, but by comparing two
   upstream references against each other. The MCU fixture said the solver was
   wrong for two variants; the published token sets said the baseline was a
   different thing entirely. Working out which of those was the real signal is
   what narrowed it to this module.

## Fixed while building the first component

The button page is the first gallery page with real child widgets, and that is
what found these. None of them is button-specific; all three were sitting in the
gallery scaffolding, unreachable from the seven token-only pages.

5. **`build()` allocated its widgets.** `build()` is called from `measure()`,
   `remeasure()` and `paintEvent()` — many times, at changing widths, because
   that is how one code path serves both measurement and painting. A page that
   created its children inside it therefore produced a fresh generation on every
   layout pass, each parked at (0, 0) and each visible, while the layout code
   only ever moved the newest one. The screen showed buttons at the right
   positions *and* several dozen more underneath them. Component pages now keep
   a widget bank and reuse the same widgets across every rebuild.
6. **`QScrollArea` never gave a page its height.** `GalleryPage` overrode
   `hasHeightForWidth()` — but `QScrollArea` asks
   `sizePolicy().hasHeightForWidth()`, not the virtual, so every page was
   pinned to exactly one viewport tall and no page ever scrolled. It looked
   correct, because the first screenful always renders; the only symptom was
   that the bottom of a long page could not be reached. Fixed by declaring
   height-for-width in the size policy, where the scroll area can see it.
7. **`measure()` and `paintEvent()` disagreed about where the body starts.**
   `paintEvent` hands `build()` its first coordinate at `buildOriginY()`,
   *below* the title and subtitle; `measure()` handed it the bare gutter.
   Measurement also returned one gutter where the value wanted is a *widget*
   height and needs two. Every page therefore reported itself one title-block
   plus 32 px too short, and `paintEvent`'s clip rect then cut the last line of
   text in half. Both now use one source, which is what the file's own "same
   code path" claim requires.

## What is next

Stage 1 §1.5 Navigation is closed — all six families are in, ending with Tabs
(`MdTabs` / `MdTab`, both published families, fixed and scrollable). The
section that opens is §1.6 Selection, in official order:

**§1.6 Selection.** Checkbox, Chips, Date pickers, Menus, Radio button,
Sliders, Switch, Time pickers. The first family is **Checkbox** — material-web
implements it (`packages/checkbox`), Compose's `Checkbox.kt` is the behaviour
source, and the export publishes the full `md.comp.checkbox.*` set including
the state layers. Chips follow as the section's largest family (four exported
sets: assist / filter / input / suggestion). §1.7 Text inputs (Text fields,
Search) closes Stage 1.

Each component lands as its own commit and must satisfy all twelve Definition of
Done items before the next one starts, including an independent gallery page, a
side-by-side visual audit in [visual-audit.md](visual-audit.md), and the
corresponding row in [md3-coverage.md](md3-coverage.md). Buttons is the first
row with cells still in progress: `主题` (seed / contrast / density / font
switch not yet exercised against the page) and `视觉审计` (rendered and read in
both modes, reference comparison not yet recorded).

See [`md3-qt-porting-prompts.md`](md3-qt-porting-prompts.md) for the full brief,
the component order, and the Definition of Done.
