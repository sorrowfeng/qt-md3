# Material Design 3 → Qt 移植提示词包

> 用途：为新项目（Qt 版 Material Design 3 组件库）准备可复用的 AI 提示词。
> 参照 `qt-ant-design` 已验证的工程惯例与工作流编写，可整体复制到新仓库的 `docs/prompts/` 下。
> 生成日期：2026-10-07
>
> **核心原则（贯穿全文）：先把 MD3 官方组件（含全部属性/变体/状态/token/动效）100% 移植完，再补充 MD3 规范里没有、但 Qt 常用控件。阶段一未全绿之前不启动阶段二。**

---

## 0. 项目命名建议

### 推荐主名

| 项 | 值 |
| --- | --- |
| 仓库 / CMake 项目 | `qt-md3` |
| C++ 命名空间 | `md` |
| 类前缀 | `Md` → `MdButton`、`MdButtonStyle`、`MdTheme`、`MdColorRole` |
| 库 target | `qt-md3`，消费方 `find_package(qt-md3 CONFIG REQUIRED)` |
| 安装头文件目录 | `install/include/qt-md3/` |
| 导出宏 | `QT_MD3_EXPORT` |
| 版本头 | `core/QtMd3Version.h` |

推荐理由：

- `md` / `md-` 是 Material 自己的官方前缀——token 是 `md.sys.color.primary`，Web Component 是 `md-filled-button`。用 `Md` 做类前缀，人和 AI 都能一眼建立映射。
- 短。这个库最终会有 100+ 组件，`MdNavigationRailStyle` 比 `MaterialNavigationRailStyle` 在代码里轻得多。
- 避开命名冲突：**`qt-material-widgets` 是已停止维护的 Material Design 2 项目**（laserpants 原作，最后一个 commit 在 2020 年），现存的都只是各种 fork。取名 `qt-material-design` 容易被误认成它的延续。

### 备选

| 方案 | 前缀 | 适用场景 |
| --- | --- | --- |
| `qt-material-design` | `Material` | 最自解释，与 Android 官方 MDC 的 `MaterialButton`/`MaterialCardView` 命名一致。缺点是类名较长，且要注意与 `qt-material-widgets` 区分 |
| `qt-m3` | `M3` | 更短、带 Expressive 版本指向。缺点是 `m3` 在搜索引擎里撞车严重（BMW M3、Apple M3） |

> 下文提示词按 `qt-md3` + `Md` 写。**若改用其他方案，全局替换即可**：
> `qt-md3` → 项目名、`Md` → 前缀、`md` → 命名空间、`QT_MD3_EXPORT` → 导出宏。

---

## 1. 移植优先级总原则（先读这一节）

### 1.1 两次交付，严格串行

| 阶段 | 目标 | 完成判据 |
| --- | --- | --- |
| **阶段一：MD3 官方组件全量覆盖** | 把 `m3.material.io/components` 上的**全部 36 个组件族及其全部属性、变体、尺寸、形状、状态、token、动效、键盘行为**完整移植为 Qt 控件 | `docs/md3-coverage.md` 中每个组件族的九列检查项全部 ✅ |
| **阶段二：Qt 常用控件补充** | 补充 MD3 规范里没有、但 Qt 桌面开发必需的常用控件，**外观与行为一律用 MD3 的 token、语义与动效重新设计** | 附录 D 清单全部落地，且视觉上与阶段一组件同源 |

### 1.2 硬性规则

1. **阶段一未达 100% 之前，不得开始阶段二的任何公开组件。** 不允许"顺手先做个 Table"这类提前消费。
2. 阶段一内部**按 MD3 官方分类顺序推进**（Actions → Communication → Containment → Navigation → Selection → Text inputs），不跳序、不挑软柿子。
3. 「移植完成」的判据不是"能画出来"，而是**属性齐全**：官方文档列出的每一个变体、尺寸、形状、颜色 style、插槽、状态、交互、键盘行为，都要有对应的公开 API 与视觉实现。少一个变体就不算完成。
4. 阶段一允许为支撑 MD3 组件而实现**内部 helper**（弹层容器、滚动条、文本排版器、popup 定位等），但不导出为公开组件，不计入阶段二。
5. 阶段一与阶段二之间**唯一的例外**：示例程序需要一个窗口壳体来承载组件画廊。把它作为**内部/示例级**实现即可（可用原生 `QMainWindow` 或最小自绘壳体），不作为阶段二的公开组件提前交付。

### 1.3 覆盖率矩阵与门禁

从第一天起维护 `docs/md3-coverage.md`，以官方 36 个组件族为行、九列为检查项。这份文件是**阶段切换的唯一依据**，骨架见附录 E。

另外加一条 CTest 门禁 `TestMd3CoveragePolicy.cmake`：解析覆盖率矩阵，**阶段一未全绿时，若 `src/widgets/` 中出现了附录 D 清单里的阶段二公开组件头文件，测试直接失败并打印冲突文件**。这样"先做完 MD3"就从一个口头约定变成了构建期强制约束。

---

## 2. 权威参考来源（已核对，直接写进提示词）

| 类别 | 来源 | 说明 |
| --- | --- | --- |
| 设计规范总入口 | <https://m3.material.io/> | 官方指南。**JS 渲染站点，WebFetch 抓不到正文，必须用 Playwright 截图/取 DOM** |
| 设计 token | <https://m3.material.io/foundations/design-tokens/overview> | `md.ref.*` / `md.sys.*` / `md.comp.*` 三层结构 |
| 颜色系统 | <https://m3.material.io/styles/color/system/overview> | 色角色、tonal palette、dynamic color、contrast level |
| 字体 | <https://m3.material.io/styles/typography/type-scale-tokens> | 30 个 type style（15 baseline + 15 emphasized）、语言脚本高度类别 |
| 形状 | <https://m3.material.io/styles/shape/shape-scale-tokens> | shape scale 圆角 token + 35 个 Expressive 装饰形状 |
| 高度 | <https://m3.material.io/styles/elevation/overview> | 0–5 级，**用 tonal surface 表达而非阴影** |
| 动效 | <https://m3.material.io/styles/motion/overview> | easing/duration token + M3E spring 物理 |
| 组件索引 | <https://m3.material.io/components> | 36 个组件族清单（**阶段一的全部范围**，见附录 A） |
| M3 Expressive | <https://m3.material.io/blog/building-with-m3-expressive> | 2025-05 发布：5 个新组件 + spring 动效 + 35 形状 + emphasized 字体 |
| token 真值（权威） | <https://github.com/material-components/material-web/tree/main/tokens> | `_md-sys-color.scss` / `_md-sys-typescale.scss` / `_md-sys-shape.scss` / `_md-sys-elevation.scss` / `_md-sys-motion.scss` + `versions/` 下的 JSON。**数值以此为准，不要凭记忆写** |
| 颜色算法（官方 C++） | <https://github.com/material-foundation/material-color-utilities> | HCT / CAM16 / TonalPalette / CorePalette / DynamicScheme / MaterialDynamicColors / contrast / blend / quantize。**`cpp/` 是官方 C++ 实现（CMake 包），可直接移植** |
| 组件行为与 API | <https://github.com/material-components/material-web/tree/main/docs/components> | 每个组件的变体、状态、API 说明 |
| Expressive 行为参考 | androidx 的 Compose Material3 源码 | `material-web` 已进入 maintenance mode 且**未实现 Expressive**，Expressive 行为以 Compose 为准 |
| 图标 | `google/material-symbols`、`google/material-design-icons` | Material Symbols（可变字体，fill/weight/grade/optical size 四轴）+ 经典 Material Icons SVG |

---

## 3. 提示词 A：新项目开工（Bootstrap）

> 在一个空仓库里第一次使用。整个提示词一次性粘贴。

```text
你正在从零搭建 `qt-md3` 项目：一个以 Qt Widgets 手绘方式复刻 Material Design 3（含 M3 Expressive）的 C++ 组件库。
目标不是"Material 风格"的近似模仿，而是逐项对齐官方设计系统的 token 数值、组件行为、状态与动效。

## 最高优先级：两次交付，严格串行

- **阶段一**：先把 m3.material.io/components 上全部 36 个组件族及其**全部属性、变体、尺寸、形状、状态、token、动效、键盘行为**完整移植。
- **阶段二**：阶段一 100% 覆盖之后，才补充 MD3 规范里没有、但 Qt 桌面开发必需的常用控件，且**一律用阶段一的 token 与 MD3 语义重新设计**。

阶段一未全绿之前，不得开始阶段二的任何公开组件。判断依据是 `docs/md3-coverage.md` 覆盖率矩阵。

## 第 0 步：先读资料，再写代码

用 Playwright 或 WebFetch 读取以下官方资料（注意 m3.material.io 是 JS 渲染站点，WebFetch 取不到正文，需用浏览器抓取或直接读其 token 源文件）：

设计规范
- 总入口 https://m3.material.io/
- 设计 token https://m3.material.io/foundations/design-tokens/overview
- 颜色系统 https://m3.material.io/styles/color/system/overview
- 字体比例 https://m3.material.io/styles/typography/type-scale-tokens
- 形状 https://m3.material.io/styles/shape/shape-scale-tokens
- 高度 https://m3.material.io/styles/elevation/overview
- 动效 https://m3.material.io/styles/motion/overview
- 组件索引 https://m3.material.io/components
- M3 Expressive https://m3.material.io/blog/building-with-m3-expressive

token 权威真值（数值必须从这里取，禁止凭记忆写）
- https://github.com/material-components/material-web/tree/main/tokens
  （_md-sys-color.scss / _md-sys-typescale.scss / _md-sys-shape.scss / _md-sys-elevation.scss / _md-sys-motion.scss，以及 versions/ 下的 JSON）

颜色算法（官方 C++ 实现，可直接移植，不要自己重写色空间转换）
- https://github.com/material-foundation/material-color-utilities
  （HCT / CAM16、TonalPalette、CorePalette、DynamicScheme、MaterialDynamicColors、contrast、blend、dislike、temperature、quantize、score）

组件行为与 API
- https://github.com/material-components/material-web/tree/main/docs/components

Expressive 行为参考
- material-web 已进入 maintenance mode 且未实现 M3 Expressive；Expressive 的组件行为、spring 参数、shape morph 以 Jetpack Compose Material3 源码为准

## 一、项目定位

- 项目名：qt-md3
- 命名空间：md；类前缀：Md（如 MdButton / MdButtonStyle / MdTheme / MdColorRole）
- 绘制方式：QPainter 手绘，不依赖 QSS / QStyleSheet
- 输出物：
  - 静态库或动态库 `qt-md3`（由 BUILD_SHARED_LIBS 控制，默认静态）
  - 示例程序 `qt-md3-example`：组件画廊（每个公开组件一个独立页面 + 左侧导航）+ 一个 Showcase 首页
  - 可安装头文件、CMake package（`find_package(qt-md3 CONFIG REQUIRED)`）、Windows 运行依赖部署
- 交付分两次：阶段一 = MD3 官方 36 族全量覆盖；阶段二 = Qt 常用控件补充（MD3 风格设计）

## 二、工程约定（硬约束）

- CMake：`find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)`，自动识别 Qt6/Qt5
- 最低版本 Qt 6.5.0 / Qt 5.15.2，在源码配置与安装包 consumer 两侧都硬校验并报错
- C++17，`CMAKE_CXX_EXTENSIONS OFF`；AUTOMOC/AUTOUIC/AUTORCC 开启；MSVC 加 `/FS /utf-8`
- 目录结构：
  - `src/core/`    — token、主题、颜色算法、字体、形状、动效、图标、工具、导出宏
  - `src/styles/`  — 全部 `Md*Style` 绘制类
  - `src/widgets/` — 全部公开组件（`Md*.h`）
  - `examples/`    — 示例程序
  - `tests/`       — QTest + CTest
  - `resources/`   — 图标资源、字体、`.qrc`
  - `docs/`        — 状态、审计、规范、待办
- 禁止使用 QSS 实现任何组件外观，并加一条 CTest 门禁扫描 `src/` `examples/` `tests/` `resources/`
- 所有视觉数值必须来自主题 token；禁止在组件里散落硬编码的颜色、圆角、字号、间距、动画时长
- 渲染架构（沿用 qt-ant-design 已验证的分层）：
  - Pattern A（首选）：自定义 QWidget 子类 + `Md<Component>Style`（继承 `MdStyleBase`，用 eventFilter 拦截 Paint 后实现 `drawWidget()`）
  - Pattern B：继承标准 Qt 控件（QPushButton / QCheckBox / QMenu / QToolBar…）时，在 Style 中重写 `drawControl()` / `drawComplexControl()`
  - Pattern C：纯容器或不需要独立 Style 的组件，组件自身 `paintEvent` 自绘
- 组件公共枚举与通用类型统一放 `src/core/MdTypes.h`
- 主题刷新统一走 `MdStyleBase::connectThemeUpdate<T>()`，禁止主题切换时全局扫描所有 widget
- 版本号唯一来源为根目录 `VERSION`，CMake 生成并安装 `core/QtMd3Version.h`
- 导出宏：`core/QtMd3Export.h` 中的 `QT_MD3_EXPORT`
- 子项目友好：作为 `add_subdirectory()` 引入时默认只构建库，示例/测试需显式开启

## 三、阶段一的第 0 批：先搭底座，再写任何组件

这一批不产出任何 UI 组件，但决定了整个库的上限。必须交付并测试：

1. `MdTokens` — 三层 token 体系
   - reference（`md.ref.*`）：完整 tonal palette，primary/secondary/tertiary/neutral/neutral-variant/error 各 0/10/20/…/95/99/100 全部 tone
   - system（`md.sys.*`）：颜色角色、typography 30 个 style、shape 圆角、elevation 0–5、motion easing/duration/spring
   - component（`md.comp.*`）：按 `md.comp.<component>.<part>.<property>` 命名，提供全局 + 单实例两级覆盖 API
2. `MdColorScheme` — 完整色角色枚举 + light/dark 双 scheme（含 fixed 系列角色）
3. `MdDynamicColor` — 移植 material-color-utilities 的 cpp 实现：HCT/CAM16 色空间、TonalPalette、CorePalette、DynamicScheme、MaterialDynamicColors、contrast、blend、dislike、temperature；scheme 变体至少覆盖 tonalSpot / vibrant / expressive / content / fidelity / monochrome / neutral；支持 ColorSpec2025 与 2026、contrast level（standard / medium / high）
4. `MdTheme` — 单例；`themeMode`（Light/Dark）、`seedColor` 动态取色、`contrastLevel`、`density`、`direction`（RTL）；提供 `themeAboutToChange` / `themeChanged` / `themeModeChanged` 生命周期信号
5. `MdTypeScale` — 30 个 type style（15 baseline + 15 emphasized），每个含 font/weight/size/lineHeight/tracking；按语言脚本高度类别（small / medium / large / xlarge）自适应 lineHeight（中文属 medium，比拉丁文高约 7%）；字体族可配置，默认 Roboto / Roboto Flex + 中文搭配
6. `MdShape` — shape scale 全部圆角 token（none / extra-small / small / medium / large / large-increased / extra-large / extra-large-increased / extra-extra-large / full）+ 35 个 Expressive 装饰形状路径 + shape morph（形状之间的路径插值）；提供可复用的"可插值圆角 + 路径 morph"工具
7. `MdMotion` — md.sys.motion 全部 easing / duration token，以及 M3 Expressive spring 物理（spatial / effects × fast / default / slow）的 spring solver（stiffness / damping / settling），提供能直接驱动自绘动画的驱动器
8. `MdStateLayer` — hover / focus / pressed / dragged 各自的叠加不透明度（数值以官方 state layer 文档为准），必须与 ripple 分离
9. `MdRipple` — 水波纹扩散动画，径向扩散并跟随组件当前圆角裁剪
10. `MdElevation` — 用 tonal surface 表达层级（surface tint 叠加 primary + tonal offset），阴影仅在官方明确要求的场景使用
11. `MdFocusRing` — M3 focus indicator（outline + 间隙）
12. `MdIcon` — Material Symbols 可变字体（fill / weight / grade / optical size 四轴）+ 经典 Material Icons SVG 资源基线；提供字符串名 API 与内置清单文档
13. `MdStyleBase` — QProxyStyle 基类：`installPaintFilter<T>` / `removePaintFilter<T>` / `drawWidget` / `connectThemeUpdate<T>` / `onThemeUpdate` / `drawCrispRoundedRect`
14. `MdFont` — 捆绑字体加载与全局应用；`MdDesign::configureHighDpi()` + `MdDesign::initialize(&app)` 作为统一启动入口
15. 资源落地 `resources/` 并配套 `.qrc` 与清单文档
16. 覆盖率矩阵 `docs/md3-coverage.md` 与门禁测试 `tests/TestMd3CoveragePolicy.cmake`（36 个组件族先行占位为未完成）
17. 门禁测试：
    - token 数值与 material-web token 源逐项比对（这是整个库正确性的地基）
    - 全色角色 light/dark 渲染
    - 固定 seed 下动态取色输出与官方实现一致

## 四、阶段一：MD3 官方组件全量覆盖

按 MD3 官方分类顺序推进。**每一族必须做到变体齐全**，不是挑几个主要变体。

### 1.2 Actions
Buttons（5 个颜色 style × 5 个尺寸 × 3 个形状）→ Icon buttons（4 style × 4 size × 形状）→ FABs（4 色 × 3 size）→ Extended FABs → FAB menu → Button groups → Split buttons → Segmented buttons

### 1.3 Communication
Badges → Progress indicators（linear / circular / wavy）→ Loading indicator → Snackbar → Tooltips

### 1.4 Containment
Cards（elevated / filled / outlined + 全部插槽）→ Dialogs（basic / full-screen）→ Bottom sheets（standard / modal）→ Side sheets（standard / modal + 各尺寸）→ Carousel（hero / multi-browse / uncontained / full-screen）→ Divider → Lists / List item（各 density、leading/trailing、展开折叠）

### 1.5 Navigation
App bars（small / medium / large + 各行为）→ Toolbars（docked / floating）→ Navigation bar → Navigation rail（含 expanded 形态）→ Navigation drawer（standard / modal）→ Tabs（primary / secondary）

### 1.6 Selection
Checkbox（含三态）→ Chips（assist / filter / input / suggestion）→ Date pickers（docked / modal input / modal calendar / range）→ Menus（standard / exposed dropdown / cascading）→ Radio button → Sliders（continuous / discrete / range）→ Switch → Time pickers（docked / modal dialogs / modal input）

### 1.7 Text inputs
Text fields（filled / outlined，单行 + 多行）→ Search（search bar / search view）

### 1.8 M3 Expressive 横切能力
emphasized type style 全组件接入 → spring 动效全组件接入 → 35 个装饰形状 + shape morph 全组件接入 → Expressive 变体的组件更新（app bars / carousel / buttons / FAB / icon buttons / navigation bar / navigation rail / progress indicators / sliders）

### 1.9 阶段一收尾
拿官方 36 个组件族清单逐族复核变体与状态，把缺口补完，`docs/md3-coverage.md` 九列全绿。**全绿之前不许进入阶段二。**

## 五、阶段二：Qt 常用控件补充（MD3 风格设计）

阶段一全绿之后才开始，完整清单与设计要点见附录 D。核心约束：

- 命名、token、状态语义、形状、动效**全部复用阶段一的底座**，不得为扩展组件另造一套视觉语言
- 每个扩展组件必须先回答"它在 MD3 体系里对应什么语义"，并在代码注释与文档里写明：
  例如 Table → MD3 的 List/data display 行语义 + surface container 层级；Window → MD3 的 surface + app bar 语义；ScrollBar → MD3 无规范，需自行推导（thumb 用 on-surface-variant 低透明度、hover 提升）
- MD3 已有对应概念的（Dialog / Toolbar / Text area / Navigation drawer 等），**不得重复造**，只能复用阶段一的组件或在其上组合
- 扩展组件同样要完整通过 DoD 全部条目

## 六、阶段三：工程收尾

安装包与外部 consumer 验证、高 DPI、可访问性（键盘、焦点顺序、meta 属性）、RTL 全组件复核

## 七、每个组件的完成定义（DoD，缺一不可）

1. `src/widgets/Md<Component>.h/.cpp` + `src/styles/Md<Component>Style.h/.cpp`，CMake 显式列源文件
2. 公开 API 覆盖官方文档列出的**全部**变体 / 尺寸 / 形状 / 状态 / 插槽，命名与官方语义一一对应
3. 每个可配置项都是 `Q_PROPERTY` 且带 NOTIFY 信号
4. 状态全覆盖：enabled / hovered / focused / pressed / disabled / selected / dragged / loading（按组件实际状态集）
5. 主题全覆盖：light / dark、seed 取色变化、contrast level、density、RTL、字体切换
6. 可访问性：Tab 可达、focus ring 可见、Space/Enter/Esc/方向键行为与官方一致
7. 示例程序新增独立页面 + 左侧导航项；页面内零样式操作（不得出现 QPalette / setFont / setStyleSheet）
8. 测试：属性与信号、生命周期与对象树、meta-property 读写、主题生命周期、渲染烟测（非空白像素校验）
9. 视觉审计：官方页面截图 vs Qt 截图 side-by-side，覆盖 light/dark 与默认/hover/focus/pressed/disabled 状态矩阵，结果写入 `docs/visual-audit.md`
10. 更新 `docs/md3-coverage.md` 中该组件对应行的九列打勾
11. 文档更新：`AGENTS.md`（组件计数与状态）、`README.md`（light/dark 截图画廊）、`docs/project-status.md`
12. 单独一次 commit，信息格式 `feat(<component>): <描述>`

## 八、避坑清单（MD3 与 Ant Design 差异最大、最容易做错的地方）

- 层级用 tonal surface 表达，**不是阴影**。浮层默认不加投影，只有官方明确要求处才有阴影
- state layer 与 ripple 是两套独立机制：hover / focus 是静态叠加层，ripple 只在按下时扩散
- 圆角全部来自 shape token，且必须支持运行时补间（shape morph）。不要把圆角写成常量
- 颜色不能写死：light / dark 是同一个 DynamicScheme 的不同 tone 映射；改 seed 色要能整体重新生成全部色角色
- 行高必须按语言脚本高度类别自适应，中文不能沿用拉丁文的行高
- M3 Expressive 的动效是 spring 物理，不是固定时长的 easing 曲线，两者要能共存
- 图标是 Material Symbols（可变字体四轴），不是静态 SVG 集合，至少保证 fill / weight / grade 可调
- 不要照搬 Ant Design 的语义体系（AntD 的 type / size / status 与 MD3 的 variant / elevation / state layer 不是一回事）

## 九、工作方式

- 先输出「当前进度报告 + 下一步计划」，等我确认后再动手
- 一次只推进一个底座模块或一个组件，DoD 全部条目完成后再进入下一个
- **每次任务开始前先自问：这是阶段一还是阶段二？** 若是阶段二，必须先确认阶段一覆盖率矩阵已全绿；若有缺口，先回去补阶段一
- 阶段一内部严格按官方分类顺序推进，不跳序、不挑容易的先做
- 每次改完只跑该组件对应的 CTest 目标，不要默认跑全量
- 需要对照官方视觉时，先截图再改代码；差异必须先归因到具体组件或容器，不要跨组件乱改
- 官方文档与官方实现不一致时，先记录差异并说明取舍，不要默默选一个
- 扩展组件（阶段二）不得提前创建，门禁测试会拦住

现在开始：完成第 0 步的资料读取，然后输出「项目骨架计划」——
包含目录结构、底座模块清单与依赖顺序、阶段一（MD3 官方 36 族）的完整组件清单与推进次序、
阶段二（Qt 扩展）的占位清单、命名规范（类/属性/信号/token 命名），
以及 `docs/md3-coverage.md` 覆盖率矩阵骨架（36 个组件族全部为未完成状态）。
输出后停下等我确认，不要直接开始写代码。
```

---

## 4. 提示词 B：单组件移植循环（日常主力）

> 底座搭好之后，每移植一个组件用一次。把 `<组件名>` 和官方 URL 填进去。

```text
继续开发 qt-md3。

## 本次任务
移植 / 完善组件：<组件名>
官方文档：<https://m3.material.io/components/xxx/overview>
组件 API 参考：<https://github.com/material-components/material-web/blob/main/docs/components/xxx.md>

## 前置校验（先做这一件事，再进入执行流程）

判断本次任务属于哪个阶段，并把结论写在回复最前面：

- **阶段一（MD3 官方组件）**：核对本次组件是否与 MD3 官方分类顺序一致。
  - 若是按序推进 → 直接进入执行流程。
  - 若是补缺口 → 从 `docs/md3-coverage.md` 指出具体缺的是哪一族、哪一列。
  - 若发现有人想跳序（比如 Containment 还没做完就去做 Selection）→ 停下来先说明，等我确认。
- **阶段二（Qt 扩展）**：必须先给出 `docs/md3-coverage.md` 中阶段一 36 个组件族九列**全部 ✅** 的证据。
  **只要有一个缺口，就停下，先回去补阶段一，不要继续阶段二。**
- 无法判定时：先输出覆盖率摘要，让我决定。

## 执行流程（严格按序，每步输出结果后再进入下一步）

### 第 1 步：规格摘要
读官方文档与参考实现，输出该组件的完整规格：
- 全部 variant / size / shape / 颜色 style（逐一列出，不要只列主要的）
- 全部 `md.comp.<component>.*` token，标出各自引用了哪些 `md.sys.*` token
- 全部状态（enabled / hovered / focused / pressed / disabled / selected / dragged / loading）及其 state layer 不透明度
- 用到的 elevation level、ripple 行为、focus ring 行为
- 交互与动效：easing / duration token，或 Expressive 场景下的 spring 参数
- 可访问性与键盘行为（焦点顺序、快捷键、ARIA 对应的 meta 信息）
- 官方文档与参考实现如有出入，明确指出并说明取舍
- **末尾列一张"变体/属性核对清单"**，作为后面 DoD 自检的依据

### 第 2 步：影响面分析
- 需要新建 / 修改的文件清单
- 需要新增的 token（sys 层 / comp 层）
- 需要复用或扩展的底座模块（shape / state layer / ripple / motion / elevation / type scale）
- 对既有组件或公共 API 的影响

### 第 3 步：设计方案（等我确认后再动手）
- 类结构：组件类 + Style 类的职责划分，选用 Pattern A / B / C 并说明理由
- 完整公开 API 草案：属性（含 Q_PROPERTY）、方法、信号
- 绘制分层与命中测试策略
- 动画驱动方式（timer / QVariantAnimation / spring driver）与帧率策略
- 尺寸与布局策略（sizeHint、minimumSizeHint、Qt Layout 自适应行为）

### 第 4 步：实现
顺序：底座补充（如需）→ 组件类 → Style 类 → 示例页

### 第 5 步：DoD 自检
逐条对照完成定义打勾，并拿第 1 步的"变体/属性核对清单"逐项确认已实现。
无法完成的条目必须说明原因，不要静默跳过。

### 第 6 步：构建与测试
- 构建库 + 示例 + 该组件相关测试目标
- 只跑相关 CTest 目标，不跑全量
- 报告实际命令与结果

### 第 7 步：视觉验证
- 抓官方页面参考图与 Qt 示例页截图（light + dark）
- 生成 side-by-side 对比图
- 逐项归因差异并修复，复测后再报告
- 更新 `docs/visual-audit.md` 矩阵

### 第 8 步：文档与提交
更新 `docs/md3-coverage.md` 对应行、`AGENTS.md`、`README.md`、`docs/project-status.md`，然后单独提交一次。

## 约束
- 不修改与本组件无关的组件行为；发现底座缺陷时先报告，不要顺手改
- 不引入 QSS；不硬编码任何视觉值
- 阶段一期间不得创建阶段二的扩展组件，也不得顺手"多做一点"
- 保持既有公开 API 兼容；破坏式改动需先说明原因并征求确认
- 示例页禁止写 QPalette / setFont / setStyleSheet

现在从「前置校验」开始。
```

---

## 5. 提示词 C：视觉审计

> 用于批量复核，或用户发现视觉问题后定位修复。

```text
对 qt-md3 执行视觉审计，范围：<组件列表 / 全部组件>。

## 审计方法

1. 抓官方参考（不提交，放 `build/ref/`）
   用 Playwright 抓取 m3.material.io 对应组件页，覆盖各状态与尺寸变体：
   npx playwright screenshot --wait-for-timeout=4000 --viewport-size "1280,900" "<url>" build/ref/<component>.png
   注意 m3.material.io 是 JS 渲染站点，需等待渲染完成；必要时用 DOM 抓取替代整页截图。

2. 抓 Qt 侧（不提交，放 `build/qt/`）
   构建示例程序，用截图 helper 抓取对应示例页，输出 `build/qt/<component>-<mode>.png`，覆盖 light / dark 与各状态。

3. 生成 side-by-side 对比图（左官方 / 右 Qt），light 与 dark 各一组。

4. 逐项归因，至少覆盖这些维度：
   token 数值 | 色角色映射 | 圆角（shape token） | 间距与内边距 | 字重字号行高 | state layer 不透明度 |
   tonal elevation | 阴影使用是否越界 | 图标形态与轴参数 | 动效曲线或 spring 参数 | 命中区域 |
   变体缺失（官方有但本仓库没实现的 variant / size / style）

5. 只修组件本体差异；容器、页面留白、示例内容差异归到对应组件审查，不要跨组件乱改。

6. 更新 `docs/visual-audit.md` 矩阵状态：
   Pass = 已截图对比且无组件本体差异
   Needs visual QA = 状态已覆盖但待截图确认
   Needs fix = 仍有组件差异
   Blocked = 无法截图或官方参考缺失

7. 若发现"变体缺失"类问题，同步在 `docs/md3-coverage.md` 把对应列改回未完成——
   覆盖率矩阵不得出现"变体还没做完但记为已完成"的情况。

## 输出格式
- 对比图路径清单
- 差异表：项目 | 官方值 | 当前值 | 归因 | 修复动作
- 修复后的复测结果
- 未解决问题与原因
```

---

## 6. 提示词 D：状态归档与待办同步

> 阶段性收尾或换电脑接手时使用。

```text
更新 qt-md3 的项目状态文档。

1. 扫描 `src/widgets/`、`src/styles/`、`tests/`、`examples/`、`resources/`，统计：
   - 公开组件数、Style 类数、示例覆盖数
   - CTest 条目数与最近一次完整运行结果（数量 + 耗时）
   - 图标资源数量、README 截图画廊数量
   - 当前版本号与依赖的 Qt 版本

2. 与官方组件索引 https://m3.material.io/components 做一次 diff：
   - 上游有、本仓库没有的组件（单独标出 M3 Expressive 新增项）
   - 已有但缺变体 / 缺尺寸 / 缺状态 / 缺键盘行为的组件

3. 更新以下文档：
   - `docs/md3-coverage.md`：逐族核对九列，逐项打勾或标记缺口（**这是阶段判定的唯一依据**）
   - `AGENTS.md`：项目状态、组件状态表、开发规范
   - `docs/project-status.md`：状态总览与最近完成工作
   - `docs/porting-todo.md`：上游移植待办与差距清单
   - `docs/reliability-coverage.md`：逐组件可靠性覆盖矩阵

4. 给出**阶段判定结论**：
   - 阶段一是否已达 100%？（逐族列出还缺什么）
   - 若未达：下一批应该推进哪一族，为什么
   - 若已达：输出阶段二的启动清单（附录 D 中按优先级排序的前 10 项）

5. 输出「当前状态摘要 + 下一步优先级建议」，等我确认后再执行后续动作。
不要自动提交。
```

---

## 附录 A：阶段一范围 —— MD3 组件族全清单 → Qt 映射

官方 36 个组件族（截至 2026-10，含 M3 Expressive 新增的 5 个）。**这 36 族是阶段一的全部范围，全绿之前不启动阶段二。** `★` = Expressive 新增。

| # | MD3 组件（官方分类） | 建议类名 | 建议 Qt 基类 |
| --- | --- | --- | --- |
| 1 | Buttons | `MdButton` | `QPushButton`（Pattern B） |
| 2 | ★ Button groups | `MdButtonGroup` | `QWidget` 容器 |
| 3 | Icon buttons | `MdIconButton` | `QAbstractButton` |
| 4 | FABs | `MdFab` | `QAbstractButton` |
| 5 | Extended FABs | `MdExtendedFab` | `QAbstractButton` |
| 6 | ★ FAB menu | `MdFabMenu` | `MdFab` + 浮层组合 |
| 7 | ★ Split buttons | `MdSplitButton` | `MdButton` + `MdMenu` 组合 |
| 8 | Segmented buttons | `MdSegmentedButton` | `QWidget`（滑动指示器） |
| 9 | Date pickers | `MdDatePicker` | `QWidget` + 弹层 |
| 10 | Time pickers | `MdTimePicker` | `QWidget` + 弹层 |
| 11 | ★ Loading indicator | `MdLoadingIndicator` | `QWidget`（形状 morph 动画） |
| 12 | Progress indicators | `MdProgressIndicator` | `QWidget`（linear / circular / wavy） |
| 13 | Navigation bar | `MdNavigationBar` | `QWidget` |
| 14 | Navigation drawer | `MdNavigationDrawer` | `QWidget` + 浮层 |
| 15 | Navigation rail | `MdNavigationRail` | `QWidget` |
| 16 | Bottom sheets | `MdBottomSheet` | `QWidget` 浮层 |
| 17 | Side sheets | `MdSideSheet` | `QWidget` 浮层 |
| 18 | App bars | `MdTopAppBar` | `QWidget` |
| 19 | Badges | `MdBadge` | `QWidget` |
| 20 | Cards | `MdCard` | `QFrame`（elevated / filled / outlined） |
| 21 | Carousel | `MdCarousel` | `QWidget` |
| 22 | Checkbox | `MdCheckBox` | `QCheckBox`（Pattern B） |
| 23 | Chips | `MdChip` | `QWidget`（assist / filter / input / suggestion） |
| 24 | Dialogs | `MdDialog` | `QDialog` |
| 25 | Divider | `MdDivider` | `QFrame` |
| 26 | Lists | `MdList` / `MdListItem` | `QAbstractItemView` |
| 27 | Menus | `MdMenu` / `MdMenuItem` | `QMenu` |
| 28 | Radio button | `MdRadioButton` | `QRadioButton`（Pattern B） |
| 29 | Search | `MdSearchBar` / `MdSearchView` | `QLineEdit` |
| 30 | Sliders | `MdSlider` | `QSlider`（Pattern B） |
| 31 | Snackbar | `MdSnackbar` | `QWidget` 浮层 |
| 32 | Switch | `MdSwitch` | `QAbstractButton` |
| 33 | Tabs | `MdTabs` / `MdTab` | `QTabBar` 派生 |
| 34 | Text fields | `MdTextField` / `MdTextArea` | `QLineEdit` / `QTextEdit` |
| 35 | ★ Toolbars | `MdToolbar` | `QToolBar`（docked / floating） |
| 36 | Tooltips | `MdTooltip` | `QWidget` 浮层 |

> 官方还有 **Scaffold**（仅 Compose 侧）与 **Window size classes / Canonical layouts**（响应式布局体系，属 `foundations/layout` 而非 `components`）。两者不在 36 族之内，**归入阶段二的布局层扩展**，见附录 D。

---

## 附录 B：需要落地的核心枚举清单

提示词里提到"底座模块"时，可直接引用这些清单：

- **色角色**：primary、on-primary、primary-container、on-primary-container；secondary / tertiary / error 同上四件套；surface、on-surface、surface-variant、on-surface-variant、surface-dim、surface-bright、surface-container-lowest / low /（默认）/ high / highest、inverse-surface、inverse-on-surface、inverse-primary、outline、outline-variant、scrim、shadow、surface-tint；以及 primary/secondary/tertiary 各自的 -fixed、-fixed-dim、on-fixed、on-fixed-variant
- **字体 style**：display / headline / title / body / label × large / medium / small，各 15 个 baseline + 15 个 emphasized
- **形状 token**：none、extra-small、small、medium、large、large-increased、extra-large、extra-large-increased、extra-extra-large、full，外加 35 个装饰形状
- **高度等级**：level 0–5（tonal offset + shadow 组合）
- **动效**：easing（linear、standard、standard-accelerate、standard-decelerate、emphasized、emphasized-accelerate、emphasized-decelerate）、duration（short/medium/long/extra-long 各 4 档）、spring（spatial / effects × fast / default / slow）
- **state layer**：hover / focus / pressed / dragged 四档不透明度
- **scheme 变体**：tonalSpot、vibrant、expressive、content、fidelity、monochrome、neutral、rainbow、fruitSalad
- **contrast level**：standard、medium、high

---

## 附录 C：可直接复制的替换对照表

以 `qt-ant-design` 为参照，新项目的对应关系：

| qt-ant-design | qt-md3 |
| --- | --- |
| 项目名 `qt-ant-design` | `qt-md3` |
| 命名空间 `Ant` | `md` |
| 类前缀 `Ant*` | `Md*` |
| `AntButton` / `AntButtonStyle` | `MdButton` / `MdButtonStyle` |
| `AntTheme` / `AntPalette` / `AntTypes.h` | `MdTheme` / `MdColorScheme` / `MdTypes.h` |
| `AntStyleBase` | `MdStyleBase` |
| `core/QtAntDesignVersion.h` | `core/QtMd3Version.h` |
| `QT_ANT_DESIGN_EXPORT` | `QT_MD3_EXPORT` |
| `QT_ANT_DESIGN_BUILD_*` | `QT_MD3_BUILD_*` |
| `AntConfigProvider`（token 覆盖入口） | `MdConfigProvider` |
| `AntWindow` / `AntFileDialog` / `AntDockManager` 那批 Qt 扩展 | `MdWindow` / `MdFileDialog` / `MdDockManager`（阶段二，见附录 D） |
| `docs/porting-todo.md`、`docs/visual-audit.md`、`docs/reliability-coverage.md` | 同名沿用，内容换成 MD3 |
| — | 新增 `docs/md3-coverage.md`（阶段判定的唯一依据） |

---

## 附录 D：阶段二 Qt 扩展组件清单（MD3 无官方规范）

**启动条件：阶段一 36 族覆盖率九列全绿。** 在此之前不得创建这些组件（门禁测试会拦）。

设计总原则：**用阶段一的 token、状态语义、形状与动效重新设计，不套用 Qt 原生外观，也不套用 Ant Design 的语义。** 每个组件必须先写明它在 MD3 体系里锚定哪个概念。

| 分组 | 建议类名 | Qt 对应 | MD3 语义锚点 | 优先级 |
| --- | --- | --- | --- | --- |
| 窗口与壳体 | `MdWindow` | 无边框主窗口 | Surface + Top app bar；标题栏按钮用 icon button 语义 | P0 |
| | `MdFileDialog` | QFileDialog | Dialog + Navigation drawer（左侧位置）+ List（文件列表）+ Text field | P0 |
| | `MdMessageBox` | QMessageBox | Dialog + Buttons + 语义色（error 用 error 色角色） | P0 |
| | `MdInputDialog` | QInputDialog | Dialog + Text field + Buttons | P1 |
| 应用框架 | `MdMenuBar` | QMenuBar | Menu 语义横向化 | P1 |
| | `MdStatusBar` | QStatusBar | Surface container + label typography | P1 |
| | `MdScrollBar` | QScrollBar | **MD3 无滚动条规范，需自行推导**：thumb 用 on-surface-variant 低透明度，hover / drag 时提升 | P0 |
| | `MdScrollArea` | QScrollArea | 组合 MdScrollBar | P0 |
| | `MdSplitter` | QSplitter | outline-variant 分割线；hover 时显示 handle | P1 |
| | `MdStackedWidget` | QStackedWidget | Surface container 层级 | P1 |
| | `MdDockWidget` / `MdDockManager` | QDockWidget | Side sheet + Surface container；停靠引导用 shape 高亮 | P1 |
| 数据展示 | `MdTable` | QTableView | List 的行语义 + Surface container 表头；**MD3 无数据表格规范，需自行设计** | P0 |
| | `MdTree` / `MdTreeSelect` | QTreeView | List 的展开折叠 + Menu 弹层 | P1 |
| | `MdAvatar` | — | **MD3 无独立 Avatar 规范**（仅作 Chip/List 的 leading 元素）；独立化时用 shape（circle / full）+ surface variant 背景 | P1 |
| | `MdEmptyState` / `MdResult` | — | Display typography 插画 + shape + Text button | P1 |
| | `MdSkeleton` | — | Surface-container-highest 占位 + shimmer | P1 |
| | `MdPagination` | — | Text button + Icon button 组合 | P1 |
| | `MdBreadcrumb` | — | Label typography + 分隔符 | P2 |
| | `MdSteps` / `MdStepper` | — | 用 shape 与连接线表达进度，参照 Progress indicator 语义 | P2 |
| | `MdTimeline` | — | List + 连线 | P2 |
| | `MdDescriptions` | — | 键值列表，label / body typography 对比 | P2 |
| | `MdStatistic` | — | Display / headline typography 展示数值 | P2 |
| | `MdImage`（含全屏预览） | — | Full-screen dialog + 手势 | P2 |
| | `MdQRCode` | — | Surface container + outline 描边（如需） | P2 |
| | `MdWatermark` | — | on-surface 低透明度平铺 | P2 |
| | `MdLog` | — | Surface container + label typography 等宽变体 | P2 |
| 数据录入 | `MdSelect` | QComboBox | **优先用 MD3 exposed dropdown menu 实现**（阶段一 Menus 的变体），若需独立控件则锚定 Menu 语义 | P0 |
| | `MdInputNumber` | QSpinBox / QDoubleSpinBox | Text field + Icon button 步进 | P1 |
| | `MdCascader` | — | Cascading menu 横向展开 | P2 |
| | `MdTransfer` | — | List × 2 + Buttons | P2 |
| | `MdUpload` | — | List + Progress indicator + Icon button | P2 |
| | `MdColorPicker` | QColorDialog | Dialog + Slider（HSV）+ Text field（Hex）；**MD3 无色板规范，需自行设计** | P2 |
| | `MdRate` | — | Icon button + 选中态 | P2 |
| | `MdMentions` | — | Text field + Menu 弹层 | P2 |
| | `MdAutoComplete` | QCompleter | Text field + exposed dropdown | P2 |
| | `MdPlainTextEdit`（编辑器能力） | QPlainTextEdit | 在阶段一 Text field 多行 variant 之上补编辑/缩放能力，不重造外观 | P2 |
| 布局 | `MdFlex` | — | MD3 layout 的 flex / grid 语义 | P1 |
| | `MdGrid` | — | MD3 响应式栅格 + Window size classes | P1 |
| | `MdSpace` | — | MD3 spacing scale | P1 |
| | `MdScaffold` | — | 组合 App bar + Navigation + FAB 的页面骨架 | P1 |
| | `MdMasonry` | — | 同上 | P3 |
| 反馈与浮层 | `MdDrawer` | — | 复用阶段一的 Navigation drawer / Side sheet，不另造 | P1 |
| | `MdPopover` / `MdPopconfirm` | — | 用 MD3 Menu / Dialog 语义收敛，不另造浮层样式 | P2 |
| | `MdNotification`（桌面通知） | — | Snackbar 的持久化变体 | P2 |
| | `MdCollapse` | — | List 的展开折叠语义 | P2 |
| | `MdTour` / `MdCoachMark` | — | Scrim + shape 高亮 + Dialog | P3 |
| | `MdRibbon` | — | Toolbar + Tabs 组合 | P3 |
| | `MdNav` / `MdNavItem` | — | 复用 Navigation rail / drawer 的 item 语义 | P1 |
| | `MdAffix` / `MdAnchor` | — | 滚动吸附与锚点 | P2 |
| | `MdBackTop` | — | FAB 的 small variant | P3 |

> 清单已排除阶段一覆盖的组件（Dialog、Toolbar、Text area 多行、Navigation drawer、Tabs、Snackbar 等），避免重复造。

---

## 附录 E：`docs/md3-coverage.md` 覆盖率矩阵骨架

复制到新仓库后从第一天开始维护。**这是阶段一 ↔ 阶段二切换的唯一依据。**

列含义：
`变体` = 官方全部 variant/size/shape/颜色 style 是否齐全；`状态` = 全部交互状态与 state layer；`属性` = 官方列出的每一个属性都有对应 API；`token` = 全部 `md.comp.*` token 已接入；`动效` = easing/duration 或 spring 参数对齐；`示例页` = 示例程序独立页面；`测试` = DoD 第 8 条；`视觉审计` = side-by-side 已确认为 Pass。

| 组件族 | 组件类 | 变体 | 状态 | 属性 | token | 动效 | 示例页 | 测试 | 视觉审计 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Buttons | MdButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Button groups | MdButtonGroup | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Icon buttons | MdIconButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| FABs | MdFab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Extended FABs | MdExtendedFab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| FAB menu | MdFabMenu | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Split buttons | MdSplitButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Segmented buttons | MdSegmentedButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Badges | MdBadge | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Progress indicators | MdProgressIndicator | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Loading indicator | MdLoadingIndicator | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Snackbar | MdSnackbar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Tooltips | MdTooltip | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Cards | MdCard | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Dialogs | MdDialog | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Bottom sheets | MdBottomSheet | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Side sheets | MdSideSheet | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Carousel | MdCarousel | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Divider | MdDivider | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Lists | MdList / MdListItem | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| App bars | MdTopAppBar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Toolbars | MdToolbar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation bar | MdNavigationBar | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation rail | MdNavigationRail | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Navigation drawer | MdNavigationDrawer | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Tabs | MdTabs / MdTab | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Checkbox | MdCheckBox | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Chips | MdChip | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Date pickers | MdDatePicker | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Menus | MdMenu / MdMenuItem | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Radio button | MdRadioButton | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Sliders | MdSlider | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Switch | MdSwitch | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Time pickers | MdTimePicker | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Text fields | MdTextField / MdTextArea | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |
| Search | MdSearchBar / MdSearchView | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ | ⬜ |

### 横切项（全部组件族都必须勾满才算阶段一完成）

| 横切能力 | 状态 |
| --- | --- |
| emphasized type style 全组件接入 | ⬜ |
| spring 物理动效全组件接入 | ⬜ |
| 35 个装饰形状 + shape morph 接入 | ⬜ |
| Expressive 变体组件更新（app bars / carousel / buttons / FAB / icon buttons / navigation bar / navigation rail / progress indicators / sliders） | ⬜ |
| light / dark × contrast level（standard / medium / high） | ⬜ |
| RTL 全组件 | ⬜ |
| 键盘与焦点顺序全组件 | ⬜ |

### 门禁测试 `tests/TestMd3CoveragePolicy.cmake`

```text
1. 解析 docs/md3-coverage.md，校验 36 个组件族全部存在、无缺行、无重复
2. 阶段一未全绿时：若 src/widgets/ 中出现附录 D 清单里任一阶段二公开组件头文件
   （MdWindow.h / MdTable.h / MdTree.h / MdMenuBar.h / MdStatusBar.h / MdScrollBar.h / ... ），
   测试失败并打印冲突文件路径
3. 阶段一期间允许的少数内部/示例级实现走显式白名单，白名单在脚本内硬编码，不允许隐式放行
4. 阶段一全绿后自动放宽第 2 条，改为校验附录 D 清单的完成进度
```
