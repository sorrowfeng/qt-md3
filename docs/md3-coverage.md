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
| Icon buttons | MdIconButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| FABs | MdFab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Extended FABs | MdExtendedFab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| ★ FAB menu | MdFabMenu | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| ★ Split buttons | MdSplitButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Segmented buttons | MdSegmentedButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

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

## 1.3 Communication

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Badges | MdBadge | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Progress indicators | MdProgressIndicator | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| ★ Loading indicator | MdLoadingIndicator | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Snackbar | MdSnackbar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Tooltips | MdTooltip | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

## 1.4 Containment

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Cards | MdCard | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Dialogs | MdDialog | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Bottom sheets | MdBottomSheet | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Side sheets | MdSideSheet | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Carousel | MdCarousel | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Divider | MdDivider | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Lists | MdList / MdListItem | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

## 1.5 Navigation

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 主题 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| App bars | MdTopAppBar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| ★ Toolbars | MdToolbar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation bar | MdNavigationBar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation rail | MdNavigationRail | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation drawer | MdNavigationDrawer | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Tabs | MdTabs / MdTab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

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
