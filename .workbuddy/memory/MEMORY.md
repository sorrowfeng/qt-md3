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
- shell 侧代理与 git 代理不同：git 走 `http://127.0.0.1:7890`。偶发
  `schannel handshake` 失败时，改用 `gh api` 验证远程状态。
