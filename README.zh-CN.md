# qt-md3

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6%20%7C%205-green.svg)](https://www.qt.io)
[![CMake](https://img.shields.io/badge/CMake-3.16+-blue.svg)](https://cmake.org)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com)
[![Release](https://img.shields.io/github/v/release/sorrowfeng/qt-md3)](https://github.com/sorrowfeng/qt-md3/releases)
[![GitHub Stars](https://img.shields.io/github/stars/sorrowfeng/qt-md3?style=social)](https://github.com/sorrowfeng/qt-md3/stargazers)
[![Last Commit](https://img.shields.io/github/last-commit/sorrowfeng/qt-md3)](https://github.com/sorrowfeng/qt-md3/commits/main)

[English](README.md) | 简体中文

`qt-md3` 是一个用 C++ 编写、以 **Qt Widgets** 原生手绘复刻 **Material Design 3**（含
**M3 Expressive**）的组件库。全部外观由 `QPainter` / `QProxyStyle` 绘制，**不使用 QSS，
不使用 `QStyleSheet`**。

目标不是"Material 风格"的近似模仿，而是逐项对齐官方设计系统的 token 数值、组件行为、
状态与动效：

- 设计规范：<https://m3.material.io/>
- token 权威真值（数值一律以此为准，禁止凭记忆写）：
  <https://github.com/material-components/material-web/tree/main/tokens>
- 颜色算法：<https://github.com/material-foundation/material-color-utilities>

## 当前状态

> **仓库处于起步阶段（Bootstrap）。** 版本 `0.1.0`。构建系统、示例壳体与门禁测试已就绪，
> 组件尚未开始移植。

| 指标 | 数值 |
| --- | --- |
| 阶段一 MD3 组件族完成度 | `0 / 36` |
| 公开组件数 | `0` |
| 阶段二（Qt 扩展） | 未开始 —— 阶段一全绿前被门禁拦住 |

进度以 [docs/md3-coverage.md](docs/md3-coverage.md) 为准，它是阶段一 ↔ 阶段二切换的唯一
依据。项目状态总览见 [docs/project-status.md](docs/project-status.md)。

## 两次交付，严格串行

1. **阶段一 —— MD3 官方组件。** 移植 <https://m3.material.io/components> 上全部 **36** 个
   组件族，及其全部属性、变体、尺寸、形状、状态、token、动效与键盘行为。
2. **阶段二 —— Qt 桌面扩展。** 补充 MD3 规范里没有、但 Qt 常用控件，一律用阶段一的 token
   与语义重新设计。

阶段一 36 族九列未全绿之前，不启动阶段二。CTest 门禁 `TestMd3CoveragePolicy` 会在阶段二
公开组件提前出现时直接让构建失败。

<details>
<summary>阶段一 36 个组件族</summary>

Buttons · Button groups · Icon buttons · FABs · Extended FABs · FAB menu ·
Split buttons · Segmented buttons · Badges · Progress indicators · Loading
indicator · Snackbar · Tooltips · Cards · Dialogs · Bottom sheets · Side sheets ·
Carousel · Divider · Lists · App bars · Toolbars · Navigation bar · Navigation
rail · Navigation drawer · Tabs · Checkbox · Chips · Date pickers · Menus ·
Radio button · Sliders · Switch · Time pickers · Text fields · Search

</details>

## 设计原则

- **token 优先，拒绝散落硬编码。** 颜色、圆角、字号、间距、动画时长全部来自主题 token。
- **层级用 tonal surface 表达，不是阴影。** 仅官方明确要求的场景使用阴影。
- **state layer 与 ripple 分离。** hover / focus 是静态叠加层；ripple 仅在按下时扩散并按
  当前圆角裁剪。
- **形状可补间。** 圆角全部来自 shape token，并支持运行时 shape morph。
- **动态取色。** light / dark 是同一个 `DynamicScheme` 的不同 tone 映射；改 seed 色会整体
  重新生成全部色角色。
- **中文行高自适应。** 行高按语言脚本高度类别调整，中文比拉丁文更高。
- **Expressive 动效。** spring 物理与固定时长 easing 曲线共存。
- **零 QSS。** 只用 `QPainter` 绘制，由门禁测试强制。

## 环境要求

- Qt **6.5.0+** 或 Qt **5.15.2+**（`Core`、`Widgets`、`Svg`；测试需要 `Test`）。
- CMake **3.16+**。
- 支持 C++17 的编译器。

## 构建

```bash
cmake --preset windows-msvc-qt6-debug
cmake --build --preset qt6-debug
ctest --preset qt6-debug
```

或手动配置：

```bash
cmake -S . -B build -DQT_MD3_BUILD_EXAMPLES=ON -DQT_MD3_BUILD_TESTS=ON
cmake --build build --config Debug --parallel
ctest --test-dir build -C Debug --output-on-failure
```

默认构建静态库；需要动态库时加 `-DBUILD_SHARED_LIBS=ON`。Windows 上安装示例程序时会自动
执行 `windeployqt` 部署运行依赖。

## 安装与消费

```bash
cmake --install build --config Release
```

```cmake
find_package(qt-md3 CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE qt-md3::qt-md3)
```

安装包在消费方一侧会再次硬校验最低 Qt 版本。

## 目录结构

```text
src/core/       token、主题、颜色算法、字体比例、形状、动效、图标
src/styles/     全部 Md*Style 绘制类
src/widgets/    全部公开组件（Md*.h）
src/private/    内部 helper（不导出）
examples/       组件画廊示例程序
tests/          QTest + CTest，含门禁测试
resources/      图标、字体、.qrc
docs/           状态、审计、规范、待办
```

## 门禁测试

| 门禁 | 约束 |
| --- | --- |
| `TestMd3NoQss` | `src/`、`examples/`、`tests/`、`resources/` 中不得出现任何 QSS |
| `TestMd3CoveragePolicy` | 36 族必须齐全；阶段一未全绿时不得出现阶段二组件 |

## 参与贡献

开发流程、组件完成定义（DoD）与构建/测试命令见 [CONTRIBUTING.md](CONTRIBUTING.md)，工作
约定见 [AGENTS.md](AGENTS.md)，同时请遵守[行为准则](CODE_OF_CONDUCT.md)。

## 许可证

[MIT](LICENSE)。移植或捆绑的第三方资源（Material Design token、Material Color Utilities、
Material Symbols、Roboto）保留各自许可证，详见 [NOTICE.md](NOTICE.md)。本项目为独立的社区
实现，与 Google LLC 及 The Qt Company Ltd. 无隶属或背书关系。
