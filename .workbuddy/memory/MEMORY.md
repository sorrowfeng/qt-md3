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
  看不见的实现充数。**2026-10-09 floating toolbar 是最干净的一例**：不带动作
  按钮时控件矩形 == pill，level3 阴影探针读到的就是页面底色（diff=0）；带 FAB
  时控件 80 宽而 pill 64，上下各 8px 余量，阴影才落地。**行继续画**（有地就落）
  是正确做法，但必须把「哪里看不见」写进 porting-todo。
- **「发布了但没人读」的 token 行，token 测试永远抓不到**（2026-10-09 toolbars
  最重要的教训）：`floating.container.between-space` 被解析、被 `floatingTokenTable`
  断言为 4.0、被头文件注释描述成「项间距」，**而布局一个字符都没读它**。症状只有
  像素能看见（三项 pill 量到 136，导出算式要求 144）。因此：① 新增「本库读这条行」
  的断言时，必须走 **override → `sizeHint()`/布局** 的路径，不能只断言结构体字段；
  ② 每条 token 行落码时自问「谁读它」，答案是「没人」就要么接上、要么明确写成
  「承载不读」并列进 porting-todo。**Compose 同族也有同样形态**（同一条行在
  `FloatingToolbar.kt` 里引用 0 次，因为它把排列交给调用方的 `Row`）——所以
  「Compose 没读」**不等于**「我们也不该读」：本库控件自己摆子控件，就得自己读。
- **QtTest 在本机（Windows offscreen）有两个坑**：① 测试 exe **不写 stdout**，
  必须 `-o <file>,txt` 才能读到断言（直接跑还会看到 `exit=127` / ctest 报
  0xc0000374 的假象，断言其实在文件里）；② **QWidget 析构会删掉子控件**，栈上
  被 `addWidget()` 收养的探针若声明在容器**之前**，容器先析构 → 探针二次析构 →
  堆损坏。规律：**先声明容器，再声明探针**（`TestMd3Toolbar::verticalIsTheTranspose`
  栽过一次，写进注释了）。
- **画廊页不得拉伸「贴内容」的控件**：`MdFloatingToolbar` 是
  `QSizePolicy::Fixed`，画廊放置时若一律铺满 band，pill 仍在首端（长度来自内容）
  但 `End` 位按钮会被甩到 band 最右。规律：**按 `sizePolicy().horizontalPolicy()
  == QSizePolicy::Expanding` 决定「用 band 还是用 sizeHint」**。
- **`MdChildBox::resizedGeometryOn()`**（toolbars 新增）：`geometryOn()` 的姊妹，
  给「尺寸不归自己」的那一个子控件用 —— 把 widget 调到 `box + 2*margin` 并
  **对齐中心**（不是左上角）。这是唯一同时适配「容器 = widget − margin」与
  「容器是固定 token 尺寸、居中在更大 widget 里」（正是 `MdFab`）的规则。
  `MdFab` 的容器恒为 `MdFabTokens::containerWidth/Height`，所以它在 80px 框里
  仍是 56px 的圆 —— 那是 FAB 族的尺寸集缺口，工具栏几何是对的。
- **`MdIconButton` 的尺寸阶梯是 Expressive 的 32 / 40 / 56 / 96 / 136，没有 48**：
  所以规范里 `8 + 48 + 8 = 64` 的组合在本库复现不出来（工具栏也没有 item 尺寸行，
  项宽完全由子控件决定）→ pill 每项窄 8px。要补的是 icon-button，不是工具栏。
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
- 代理会漂移，别记死：2026-10-09 多轮内依次见过 `9426` → `12574`（`env` 确认），
  git config 为 `127.0.0.1:7889`。**推送 main/dev 直接绕过代理最稳**：
  `git -c http.proxy= -c https.proxy= push`（已连续三次成功）。curl 走
  raw.githubusercontent.com 常 404/被挡，取 material-web 源码一律用
  `gh api repos/.../contents/<path> --jq .content | base64 -d`。偶发
  `schannel handshake` 失败时，改用 `gh api` 验证远程状态。直连
  （`curl --noproxy '*'`）可达 `m3.material.io`，但 `lh3.googleusercontent.com` /
  `raw.githubusercontent.com` 被挡。
- **抓 m3.material.io 的正确姿势（2026-10-09 toolbars 时摸清）**：该站是纯客户端
  Angular SPA（`<mio-root>`，`<noscript>` 提示需要 JS），curl/WebFetch 只拿到空壳。
  内容接口 `/guide-page-content` 是**无参 GET 且只返回根页**（"Get Started"），
  对 path/query/Referer/cookie 一律无视——别在这上面浪费时间。唯一可行路径是
  截图：`msedge.exe --headless --disable-gpu --hide-scrollbars
  --window-size=1440,<高> --screenshot="C:/绝对/路径.png" --virtual-time-budget=16000
  --proxy-server="direct://"`。**三个硬约束**：① 输出与 `--user-data-dir` 都必须是
  **Windows 绝对路径**（Edge 是原生二进制，`/tmp/...` 认不了）；② `--dump-dom`
  在本机**必被 SIGTERM**，只能用 `--screenshot`；③ 一次 Bash 调用**只跑一个 Edge
  实例**（循环/重试里跑第二个会被杀），且约四次里成一次——失败就单独重试。
  页面下方是懒加载，给高视口能多渲一部分，抓不全时可 `--virtual-time-budget` 加大
  或换一个 `--user-data-dir` 重试。
