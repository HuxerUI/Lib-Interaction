# Lib-Interaction

[English](README.md)

[HuxerUI](https://github.com/HuxerUI/HuxerUI) 的交互动画控件扩展：可按压表面、悬停抬升、持续脉冲和按压涟漪，全部基于框架的保留式动画原语构建。使用 C++20，命名空间为 `huxerui::interaction`，CMake 目标为 `HuxerUI::Interaction`（`huxerui_interaction`）。

- `Pressable`：悬停与按压时围绕一个轴点缩放，并通过 OnClick 转发激活。
- `HoverLift`：悬停时内容上浮并轻微放大。
- `Pulsing`：以无限往复播放振荡缩放或透明度，不需要任何应用状态。
- `Ripple`：保留式修饰符，从每次主键按压位置绘制扩散、淡出的圆形，且不消费点击事件。
- 所有动画都经过 HuxerUI 动画系统（`AnimateTo`、`MotionController`），因此减弱动态、帧调度与后台节流均遵循框架策略。

控件不使用任何平台专属 API。已在本地验证 Windows 构建与测试；CI 在 Windows 上覆盖原生控件与预览测试。其他 HuxerUI 后端需要各自的 SDK/工具链，尚未在此验证。

## 接入项目

```cmake
# 在 huxerui_add_app(your_app ...) 之后调用：
huxerui_use_library(your_app
    TARGET HuxerUI::Interaction
    PATH "/path/to/Lib-Interaction")
```

```cpp
#include <huxerui/huxerui.h>
#include <huxerui/interaction.h>

using namespace huxerui;
using namespace huxerui::interaction;

View Gallery() {
  auto clicks = UseState(0);
  return Column {
    Pressable(Card(), {},
              [clicks] { clicks += 1; })
        .With(Frame{.width = 240.0F, .height = 120.0F}),
    HoverLift(Card(), {.lift = {0.0F, -6.0F}}).With(Frame{.width = 240.0F, .height = 120.0F}),
    Pulsing(StatusDot(), {.scale_amplitude = 0.08F, .animate_opacity = true, .minimum_opacity = 0.4F})
        .With(Frame{.width = 12.0F, .height = 12.0F}),
    Card().With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.14F)}}),
  }.With(Spacing(16.0F));
}
```

`Pressable` 与 `HoverLift` 是组合期控件：每次调用创建独立的状态作用域，参数不变的重组合会按作用域位置保留已挂载的状态。它们不施加固有尺寸；请通过父容器或 `.With(Frame{...})` 提供有界布局。`Pulsing` 是无状态的声明式动画。`Ripple` 是通过 `View::With()` 应用的修饰符：它绘制在内容之上，按节点边界与解析后的圆角裁剪，并以 `PointerResult::Observe` 观察指针序列，因此兄弟手势识别器与 OnClick 不受影响。

无效配置在调用点抛出 `std::invalid_argument`：非有限或非正的缩放、非有限的位移、负的振幅、[0, 1] 之外的透明度、非正的时长、没有任何参与通道的 `Pulsing` 样式，或时长／透明度乘数无效的 `Ripple`。

按压跟踪遵循主键的按下、抬起与取消事件。键盘激活、焦点可见的按压反馈与禁用状态恢复在本版本中尚未提供；请在调用处按需组合 `OnClick` 与 `Enabled`。组件目录与动效约定见 [docs/design.md](docs/design.md)，设计决策记录见 [docs/open-questions.md](docs/open-questions.md)。

## 构建与测试

需要 CMake 3.20+ 与 C++20。控件依赖 HuxerUI 源码或 SDK；已验证的框架版本为 `42dc57f10d2713f40272be393bafb6200005ca83`。

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_INTERACTION_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

运行 `example_interaction` 体验控件：弹簧／补间按压卡片、悬停抬升、脉冲状态与涟漪卡片，并支持亮暗主题切换。与其他 Lib 项目一样，演示位于 [`examples/preview`](examples/preview/README.md)，也可以独立配置：

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

控件依赖 HuxerUI 视图系统；本版本没有独立于 HuxerUI 的核心层。HuxerUI 的 library-graph 与 project-plan 发现不会编译控件源码。本版本不提供独立的安装／导出包；请通过 `add_subdirectory` 或 `huxerui_use_library` 消费。

## 许可证

MIT。见 [LICENSE](LICENSE)。
