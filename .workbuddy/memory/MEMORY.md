# qt-md3 项目长期记忆

## 项目定位
Qt Widgets 手绘复刻 Material Design 3（含 M3 Expressive）的 C++ 组件库。
命名空间 `md`，类前缀 `Md`，导出宏 `QT_MD3_EXPORT`，库目标 `qt-md3`。
禁止 QSS，只用 QPainter / QProxyStyle。

## 两次交付，严格串行（最高约束）
- 阶段一：MD3 官方 36 个组件族全量（全部变体/尺寸/形状/状态/token/动效/键盘）。
- 阶段二：Qt 扩展组件，必须复用阶段一 token 与语义。
- **切换唯一依据：`docs/md3-coverage.md` 九列全绿。** 门禁
  `TestMd3CoveragePolicy` 会在阶段一未全绿时拦截 `src/widgets/` 中的阶段二组件。

## 交互保真度规则（2026-10-08 按压动画审计得出，适用于所有后续组件）
- **焦点环走 `:focus-visible` 语义**：鼠标点击不显示焦点环与 Focused 状态色，仅键盘焦点（Tab/Backtab/快捷键）显示。Qt 按 focusInEvent 的 reason() 分类，控件暴露 hasKeyboardFocus()，样式用它门控环与 Focused 状态行（MdButton/MdIconButton 已实现，后续组件照抄）。
- **涟漪 = pressed 状态层**：涟漪颜色必须取 tokens 按压行的 stateLayer 角色（filled=on-primary 白色涟漪，tonal=on-secondary-container，outlined/text=on-surface），禁止全局 OnSurface。
- **按压不是平的状态层**：平层只画 hover+键盘焦点（strongestActive 传 pressed=false），按压响应完全由扩散的涟漪圆承载。
- 动画验证方法：探针 exe 逐帧截图（~50ms 步进）+ PIL 拼胶片 + 像素采样，与 material-web ripple 源码常量（450/225/375/0.2/10/75/0.35、STANDARD easing）比对。
- **容器摆放「容器」而不是 widget——用 `styles/MdChildBox`，别用 `sizeHint()`**
  （2026-10-09 app bars 得出，MdButtonGroup 先例）：凡能显示焦点环的子控件都把
  环的余量留在自身矩形内，恰好**每侧 7.5px**（`offset 2 + activeWidth 8/2 + width
  3/2`），因为 Qt 把子控件裁剪到自身矩形、而指示环朝外。于是 `MdIconButton` 是
  55×55 的 widget 包 **40×40** 的容器；用 `sizeHint()` 排布会让图标间距、边距全错
  （实测 55 vs 40、23 vs 14）。`MdChildBox::measure(w)`（先 sizing 再取
  `containerRect()`，闭集 `qobject_cast`）→ `geometryOn(box)` 给出 widget 应设的
  geometry。代价：相邻 widget 矩形重叠 `2×7.5=15px`（重叠区透明，刻意）。新组件
  长出 `containerRect()` 时记得加进 `MdChildBox` 的 dispatch 闭集。
- **颜色插值在 Oklab 里做，且缓动先于插值**：Compose 的 `Color.VectorConverter`
  在 Oklab 插值 → `MdColorMath::lerpOklab`。app bars 的滚动过渡 =
  `lerp(container, scrolled, FastOutLinearInEasing(f))`；注意
  `FastOutLinearInEasing`（`cubic-bezier(0.4,0,1,1)`）**中点低于线性**（x=0.5 →
  **0.32481**）。别用 0.5 直接插值，也别在 sRGB 里插。

## 测试与渲染约定（2026-10-09 divider/lists 两族踩坑后固化）
- **`QCOMPARE`/`QVERIFY` 失败 = 立即 return 出整个槽函数**（不是继续跑完）。
  所以 `cleanup()` 必须穷尽重置该槽可能触碰过的**每一个**可变状态（含
  `setEnabled`、`setLayoutDirection`、`selected`、`variant`、segmented）；
  否则一个断言失败会污染后续所有槽，症状是"一串莫名其妙的失败"。这个坑
  到 lists 一族已重演四次。
- **平台差异要在测试里运行期探测，不要硬编码**：Qt5 offscreen 插件无字体库，
  `drawText` 不产生任何墨迹（最暗像素 == 容器色），Qt6 同平台有墨迹；Qt5 下
  `QTest::mouseMove` 不合成 enter 事件（改用 `QEnterEvent` + `sendEvent`，
  TestMd3Card / TestMd3SplitButton 既有手法）。涉及文字的像素断言先探测是否
  真有墨迹，无则跳过；token 表断言不受平台影响。
- **原位组件的自绘阴影/外溢描边会被 Qt 裁掉**：容器即整个 widget 矩形的组件
  （列表项、以及后续 navigation bar / toolbar 的项）无法自绘矩形外的阴影，
  父级代画又会被相邻项的不透明容器盖住。Compose 的做法是**浮层**渲染。遇此
  情况按项目规矩「承载 token、不绘制、写进 porting-todo」，不要写一个画出来
  看不见的实现充数。
- **库内没有颜色覆盖机制**（`MdCompTokenParse.h` 只有 length/shape，全库无
  parseColor）。画廊若要给组件换容器色是做不到的，演示必须靠真实状态
  （selected / dragged / disabled）让形状/颜色可见。
- **画廊截图钩子两件套**（2026-10-09 app bars 得出）：① 动画落定前 `processEvents()`
  一次会在弹簧**起点**抓帧 → 用 `settleAnimations(400ms)`（8ms 步进泵事件循环，
  Qt 定时器按墙钟）；② `examples/main.cpp` 的 **`--language <tag>`**——本机
  offscreen 字体中文全是豆腐块、英文可读，视觉审计两种语言都能跑。**画廊文案
  必须 `L(zh,en)` 双语且两个半边都写**（页 28 曾只填中文半边，英文模式画出豆腐块）。
- **`MdTheme::isRightToLeft()` 是全库级缺口**：目前只被四个按钮族读取，其余
  组件都不镜像 RTL。新组件别谎称「已镜像」，如实写进 porting-todo。

## 移植参考来源（用户明确要求）
每个组件移植时参考两处：① 官方规范 m3.material.io/components/<component>（结构/行为，JS 渲染需浏览器抓取）；② material-web 仓库（tokens/versions/latest/sass 的全部数值 + docs/components 行为说明，maintenance mode 未实现 Expressive，Expressive 行为以 Compose M3 为准）。两者冲突时记录分歧并说明取舍，不默默选一个。已固化为 AGENTS.md 与 docs/porting-todo.md 的守则。

## 关键约定
- 目录：`src/core`（token/主题/颜色算法/字体/形状/动效/图标）、`src/styles`
  （`Md*Style`）、`src/widgets`（`Md*` 公开组件）、`examples`、`tests`、`resources`、`docs`。
- 渲染分层：Pattern A（自定义 QWidget + `Md<Component>Style : MdStyleBase`）首选；
  Pattern B（`QProxyStyle::drawControl/drawComplexControl`）；Pattern C（容器自绘）。
- 所有视觉数值必须来自 token，禁止散落硬编码；主题刷新走
  `MdStyleBase::connectThemeUpdate<T>()`。
- 版本唯一来源为根 `VERSION`；CMake 生成 `core/QtMd3Version.h`。
- 组件 DoD 共 12 条，见 `CONTRIBUTING.md` / `AGENTS.md`；单组件一次 commit，
  格式 `feat(<component>): <desc>`。
- 每次任务先自问「阶段一还是阶段二」，并先输出「进度报告 + 下一步计划」等确认。

## 参考项目
- `D:/Project/GitProject/qt-ant-design`：已验证的工程惯例来源（命名、CI、
  install/consumer、门禁思路）。本项目多数脚手架文件由其对应文件改写而来。

## 本机构建要点
- MSVC 路线在本机受限：CMake 4.0.1 无 VS18 生成器，且沙箱屏蔽 `reg.exe`，
  `vcvars64.bat` 无法定位 Windows SDK。
- 可行的本地验证路线：`C:/Qt/Tools/mingw1310_64` 的 GCC 13.1 + Qt 6.9.1
  `mingw_64` + `C:/Qt/Tools/Ninja`，用 `-DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/mingw_64`。
- 运行测试/示例需把 `C:/Qt/Tools/mingw1310_64/bin` 与 `C:/Qt/6.9.1/mingw_64/bin`
  加入 PATH（缺 MinGW 运行库会加载失败）。
- 代理会漂移，别记死：2026-10-09 时 shell 侧为 `127.0.0.1:9426`（用 `env`
  确认），git config 为 `127.0.0.1:7889`。**推送 main/dev 直接绕过代理最稳**：
  `git -c http.proxy= -c https.proxy= push`（已连续三次成功）。curl 走
  raw.githubusercontent.com 常 404/被挡，取 material-web 源码一律用
  `gh api repos/.../contents/<path> --jq .content | base64 -d`。偶发
  `schannel handshake` 失败时，改用 `gh api` 验证远程状态。直连可达
  `m3.material.io`，但 `lh3.googleusercontent.com` / `raw.githubusercontent.com`
  被挡（app bars 时确认）。
