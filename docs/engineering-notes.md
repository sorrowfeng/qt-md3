# Engineering notes

Cross-cutting lessons that apply to every future component. These come from
the interactive-fidelity audit (2026-10-08), the divider / lists / navigation
ports (2026-10-09), and the local build and research workflow. Component-specific
facts live in [porting-todo.md](porting-todo.md); the coverage matrix is
[md3-coverage.md](md3-coverage.md).

## Interaction fidelity (applies to every component)

- **Focus ring follows `:focus-visible`.** A mouse click must not show the ring
  or the Focused state tint — only keyboard focus (Tab / Backtab / shortcut)
  does. Qt classifies via `focusInEvent`'s `reason()`; a control exposes
  `hasKeyboardFocus()` and the style gates both the ring and the Focused state
  row on it. `MdButton` / `MdIconButton` are the reference implementation.
- **Ripple = pressed state layer.** The ripple colour comes from the tokens'
  pressed row `stateLayer` role (filled → `on-primary` white, tonal →
  `on-secondary-container`, outlined / text → `on-surface`). Never hard-code a
  global `OnSurface`.
- **Press is not a flat state layer.** The flat layer paints hover + keyboard
  focus only (`strongestActive` is called with `pressed=false`); the press
  response is entirely the spreading ripple circle.
- **Animated colour interpolates in Oklab, easing first.** Compose's
  `Color.VectorConverter` interpolates in Oklab, so the app-bar scroll
  transition is `lerp(container, scrolled, FastOutLinearInEasing(f))` with
  `MdColorMath::lerpOklab`. Note `FastOutLinearInEasing`
  (`cubic-bezier(0.4,0,1,1)`) is **below linear at the midpoint**
  (x=0.5 → **0.32481**). Do not interpolate at 0.5 directly, and do not
  interpolate in sRGB.

## Child layout: place containers, not widgets — use `MdChildBox`

Established by the button-group / app-bars ports. Every control that can show a
focus ring keeps that ring's room **inside its own rect** — exactly **7.5 px a
side** (`offset 2 + activeWidth 8/2 + width 3/2`), because Qt clips a child to
its own rect while the indicator paints outward. So `MdIconButton` is a 55×55
widget wrapping a **40×40** container; laying out by `sizeHint()` makes icon
gaps and margins wrong (measured 55 vs 40, 23 vs 14).

- `MdChildBox::measure(w)` sizes first, then reports `containerRect()` (a closed
  `qobject_cast` dispatch) → `geometryOn(box)` is the geometry the widget should
  get.
- `MdChildBox::resizedGeometryOn()` is the sibling for the child whose size is
  *not* its own: it resizes the widget to `box + 2 * margin` and **centres** it
  (not top-left). That is the only rule that lands the container on the box both
  for "container = widget − margin" and for "container is a fixed token size
  centred in a larger widget" (`MdFab`).
- Cost: neighbouring widget rects overlap by `2 × 7.5 = 15 px` (transparent by
  design). New components that grow a `containerRect()` must be added to
  `MdChildBox`'s dispatch.

## Testing and rendering conventions

These bit us repeatedly; they are now rules.

- **`QCOMPARE` / `QVERIFY` failure returns from the whole slot immediately**
  (it does not finish the slot). Therefore `cleanup()` must reset **every**
  mutable state that slot can touch (`setEnabled`, `setLayoutDirection`,
  `selected`, `variant`, segmented, …). Otherwise one assertion failure
  poisons every later slot and the symptom is "a string of unrelated failures".
  This bit us four times through the lists family.
- **Detect platform differences at run time; do not hard-code.** The Qt5
  offscreen plugin has no font database: `drawText` produces no ink at all
  (darkest pixel == container colour); Qt6 on the same platform does. Under
  Qt5, `QTest::mouseMove` does not synthesise an Enter event — send a
  `QEnterEvent` instead (`TestMd3Card` / `TestMd3SplitButton` show the idiom).
  For pixel text assertions, probe for real ink first and skip if there is
  none. Token-table assertions are platform-independent.
- **Self-drawn shadows / outward strokes of in-place components get clipped by
  Qt.** A component whose container *is* its whole widget rect (list items,
  and later navigation-bar / toolbar items) cannot draw a shadow outside
  itself, and a parent painting it is covered by the neighbours' opaque
  containers. Compose solves this with an overlay layer. Per project rule:
  *carry the token, do not paint a decorative no-op, write the gap into
  porting-todo.md*. **Floating toolbar is the cleanest case (2026-10-09):**
  with no action buttons the control rect == the pill and the level-3 shadow
  probe reads the page background (diff=0); with a FAB the control is 80 wide
  and the pill 64, so the 8 px band above and below lets the shadow land.
  **Keep painting the row when there is room**, but record where it is
  invisible.
- **A published token row nobody reads is invisible to token tests**
  (the toolbars lesson, 2026-10-09): `floating.container.between-space` was
  parsed, asserted as 4.0 in `floatingTokenTable`, and described in the header
  as "item spacing" — and the layout never read it. The only symptom is a
  pixel measurement (three-item pill measured 136 where the export arithmetic
  wants 144). So: ① an assertion that "this library reads this row" must go
  through **override → `sizeHint()` / layout**, not just the struct field;
  ② when a token row lands in code, ask "who reads it?" — if the answer is
  nobody, either wire it up or explicitly mark it "carried, not read" and list
  it in porting-todo.md. **Compose can have the same shape** (the same row is
  referenced 0 times in `FloatingToolbar.kt` because it leaves the arrangement
  to the caller's `Row`) — so "Compose does not read it" does **not** mean "we
  should not either": this library's control lays out its own children, so it
  must read the row itself.
- **Two QtTest traps on this machine (Windows offscreen):** ① the test exe
  writes nothing to stdout — run with `-o <file>,txt` to read assertions (a
  bare run can look like `exit=127` / ctest `0xc0000374` while the assertion is
  in the file); ② **`QWidget` destruction deletes children** — a stack probe
  adopted with `addWidget()` that is declared *before* its container is
  double-destroyed when the container dies first → heap corruption. Rule:
  **declare the container first, then the probe**
  (`TestMd3Toolbar::verticalIsTheTranspose` learned this; it is in a comment).
- **Gallery pages must not stretch "content-sized" controls.**
  `MdFloatingToolbar` is `QSizePolicy::Fixed`; if the gallery always fills the
  band, the pill stays at the leading edge (length from content) but the `End`
  button is flung to the far right. Rule: **branch on
  `sizePolicy().horizontalPolicy() == QSizePolicy::Expanding`** to decide
  "band" versus "sizeHint".
- **The icon-button size ladder is Expressive's 32 / 40 / 56 / 96 / 136 — there
  is no 48.** So the spec's `8 + 48 + 8 = 64` cannot be reproduced here (the
  toolbar has no item-size row either; item width comes entirely from the
  child) and each pill item is 8 px narrower. The gap to fix is icon-button,
  not the toolbar.
- **There is no colour-override mechanism in the library**
  (`MdCompTokenParse.h` only parses length/shape; there is no `parseColor`
  anywhere). A gallery page cannot recolour a container, so demos must use real
  state (selected / dragged / disabled) to make shape and colour visible.
- **Gallery screenshot hooks (app bars, 2026-10-09):** ① a single
  `processEvents()` before the animation settles grabs the spring's *start*
  frame — use `settleAnimations(400ms)` (8 ms stepping the event loop; Qt
  timers use wall-clock); ② `examples/main.cpp` has **`--language <tag>`** —
  local offscreen Chinese is tofu boxes while English is readable, so the
  visual audit can run in either language. **Gallery copy must be bilingual
  `L(zh,en)` with both halves filled** (page 28 shipped only the Chinese half
  and drew tofu in English mode).
- **`MdTheme::isRightToLeft()` is a library-wide gap.** Only the four button
  families read it; nothing else mirrors RTL. New components must not claim
  "mirrored" — record it honestly in porting-todo.md.

## How to pick between the two token-export families

MD3's bar / rail publish **two families inside the same export version**;
naming distinguishes them, not age. Baseline uses `navigation-*` names (bar 80
/ rail 80); M3 Expressive uses another set (bar `nav-bar` 64 with an extra
horizontal-item variant; rail `nav-rail-collapsed` 96 /
`nav-rail-expanded` 220–360). The drawer has only `navigation-drawer`; its
Expressive replacement is the **expanded navigation rail**.

Authoritative order (all three steps; do not skip):

1. **Read which set Compose's `*Tokens.kt` consumes** — that is behavioural
   truth. Compose leaves migration debris ("declared but unread", e.g.
   `NavigationBarTokens.ContainerHeight = 64` is TODO'd while the
   implementation reads `TallContainerHeight = 80`), so confirm against the
   implementation body, not just the token file.
2. **Read material-web's `tokens/versions/v0_192/_md-comp-*.scss`** — those are
   the stable values the components it *has* implemented consume; cross-check
   against `versions/latest` to see which set is current.
3. **Screenshot the spec site for the taxonomy** (Variant / Baseline /
   Configuration and the "M3 Expressive update" section). The Availability
   table writes `Web: Unavailable` directly, which shows at a glance whether
   material-web ever implemented it (both formal families of that component
   were not; the implementation lived only under `labs/`).

## Local build and research notes

Reference project for engineering conventions (naming, CI, install/consumer,
gating): `D:/Project/GitProject/qt-ant-design`. Most scaffolding here is
rewritten from its counterparts.

- MSVC is limited on this machine: CMake 4.0.1 has no VS18 generator, and the
  sandbox blocks `reg.exe`, so `vcvars64.bat` cannot locate the Windows SDK.
- The working local route is `C:/Qt/Tools/mingw1310_64` GCC 13.1 + Qt 6.9.1
  `mingw_64` + `C:/Qt/Tools/Ninja`, with
  `-DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/mingw_64`. A fresh tree configured without
  `ninja` on `PATH` or without `-DCMAKE_CXX_COMPILER` fails with "unable to
  find a build program" / "CMAKE_CXX_COMPILER not set" — that is all it means.
- Running tests / examples needs `C:/Qt/Tools/mingw1310_64/bin` and
  `C:/Qt/6.9.1/mingw_64/bin` on `PATH` (missing MinGW runtime → load failure).
- The local Qt 5.15.2 verification route is `C:/Qt/5.15.2/mingw81_64` +
  `C:/Qt/Tools/mingw810_64` + Ninja in a separate `build-qt5` directory. Run
  that before every component commit — it reproduces all the Qt5 CI failures
  early.
- Proxies drift; do not hard-code. Seen `9426` → `12574` in
  `HTTP(S)_PROXY` while git config says `127.0.0.1:7889` / `7890`. **Pushing
  `main` / `dev` with the proxy bypassed is the reliable route:**
  `git -c http.proxy= -c https.proxy= push`. curl to
  raw.githubusercontent.com is often 404 / blocked; fetch material-web sources
  with
  `gh api repos/.../contents/<path> --jq .content | base64 -d`.
  Occasional `schannel handshake` failures: verify remote state with `gh api`.
  Direct (`curl --noproxy '*'`) reaches `m3.material.io`, while
  `lh3.googleusercontent.com` / `raw.githubusercontent.com` are blocked.
- **Scraping m3.material.io (toolbars, 2026-10-09):** the site is a pure
  client-side Angular SPA (`<mio-root>`, `<noscript>` says JS is required) —
  curl / WebFetch get an empty shell. The content endpoint
  `/guide-page-content` is a parameterless GET that only ever returns the root
  page ("Get Started") and ignores path / query / Referer / cookie — stop
  wasting time there. The only working path is a screenshot:
  `msedge.exe --headless --disable-gpu --hide-scrollbars --window-size=1440,<h>
  --screenshot="C:/abs/path.png" --virtual-time-budget=16000
  --proxy-server="direct://"`. **Three hard constraints:** ① both the output
  path and `--user-data-dir` must be **Windows absolute paths** (Edge is a
  native binary; it does not understand `/tmp/...`); ② `--dump-dom` is always
  SIGTERM'd on this machine — only `--screenshot` works; ③ one Bash call runs
  **one Edge instance only** (a second one in a loop / retry gets killed), and
  about one attempt in four succeeds — retry alone. Lower page sections are
  lazy-loaded: give a tall viewport, raise `--virtual-time-budget`, or switch
  `--user-data-dir` on retry.
