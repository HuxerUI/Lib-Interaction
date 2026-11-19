# Lib-Interaction

[English](README.md)

[HuxerUI](https://github.com/HuxerUI/HuxerUI) 的交互式组件与视觉动效库：采用**无头交互状态 / 受控句柄（Layer 1）**与**预设动效组件 / 纯视觉域（Layer 2）**分离架构，基于 C++20 构建，命名空间为 `huxerui::interaction`，CMake 目标为 `HuxerUI::Interaction`（`huxerui_interaction`）。

## 架构概览

- **Layer 1 — 无头交互原语、受控句柄与管道组合**：
  - `InteractiveSurface`、`Hoverable`、`Copyable`、`Popover` 与 `PredictiveBack`：封装主键按压、悬停、键盘激活、剪贴板反馈、浮层显隐、预测式返回进度与焦点环状态机，向构建器输出实时状态快照，不强制绑定单一视觉样式。
  - `SwipeController`（`UseSwipeController`）、`PanZoomController`（`UsePanZoomController`）、`HoverCardController`（`UseHoverCardController`）、`PopoverController`（`UsePopoverController`）、`ScrollHideController`（`UseScrollHideController`）、`ScrollSpyController`（`UseScrollSpyController`）、`RovingFocusController`（`UseRovingFocusController`）、`PredictiveBackController`（`UsePredictiveBackController`）、`MarqueeSelectController`（`UseMarqueeSelectController`）：支持外部读取实时状态与编程式控制。
  - `ReorderHandle`、`RovingFocusItem`、`MarqueeSelectItem` 与 `ScrollSpySection`：通过 HuxerUI `Environment` 与外层 `Reorderable` / `ReorderableGrid`、`RovingFocusGroup`、`MarqueeSelectArea` 或 `ScrollSpyArea` 对接。
  - `ViewAdaptor`（`operator|`）：支持从左到右链式叠加交互装饰器，告别多层函数包裹的“洋葱嵌套”。
- **Layer 2 — 预设交互组件与保留式反馈修饰符**：
  - **按压与悬停（`press/`、`hover/`）**：`Pressable`、`Copyable`、`LongPressable`、`HoldProgressRing`、`MultiClickable`、`DoubleClickable`、`ContextMenuArea`、`SecondaryClickable`、`Ripple`、`HoverLift`、`HoverHighlight`、`HoverShadow`、`Spotlight`、`Magnetic`。
  - **连续手势、重排与拖放（`gesture/`、`reorder/`、`dragdrop/`）**：`DragSpring`、`InertialDrag`、`MarqueeSelectArea`、`MarqueeSelectItem`、`PanZoom`、`PinchZoom`、`SwipeActions`、`Dismissible`、`PressDragHandle`、`ResizeHandle`、`Reorderable`、`ReorderableGrid`、`Draggable<T>`、带谓词过滤的 `DropZone<T>`、`FileDropZone`。
  - **浮层、滚动与无障碍（`overlay/`、`scroll/`、`accessibility/`）**：`HoverCard`、`Popover`、`ScrollProgress`、`ScrollEdgeFade`、`NestedScrollConnection`、`ScrollSpyArea`、`ScrollSpySection`、`KeyboardShortcut`、`ShortcutMap`、`BackHandler`、`PredictiveBack`、`RovingFocusGroup`、`RovingFocusItem`、`SemanticActions`。
- **纯视觉动效子域（`visual/`）**：
  - 将不接收用户输入的纯视觉/数值过渡组件独立归类：`Expandable`、`Appear`、`SharedMorph`（`UseSharedMorph` / `MorphScope` / `MorphElement` / `MorphBounds`）、`Pulsing`、`Shimmer`、`Fireworks`、`RollingNumber`、`UsePop` / `Popped`、`UseShake` / `Shaken`、`UseRevealOnTap`。
- **统一签名与框架时钟**：全库统一 `(View content, [必填参数], callback, Style = {})` 参数顺序（纯视觉重载通过 `std::invocable` 无歧义并存）；所有动画统一走 `AnimateTo`、`MotionController` 与 `TaskScope::Delay`，自动遵循系统减弱动态（Reduced Motion）与帧调度策略。

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
  auto shake = UseShake();
  auto swipe = UseSwipeController();

  return Column {
    // 管道组合：同一节点上先挂载无状态事件装饰器，再挂载带 Scope 的状态组件
    Card()
        .With(Frame{.width = 240.0F, .height = 120.0F}, Ripple{}, Spotlight{})
        | ContextMenuArea([](Point p) { /* 在 p 处弹出菜单 */ })
        | Pressable([clicks] { clicks += 1; },
                    {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}}),

    // 无头 InteractiveSurface：用 {hovered, pressed, focused} 自由驱动任意视觉属性
    InteractiveSurface(
        [](const InteractionSnapshot& s) {
          return Text(s.pressed ? "按压中" : (s.hovered ? "悬停中" : "默认"))
              .With(Frame{.width = 240.0F, .height = 56.0F});
        },
        [clicks] { clicks += 1; }),

    // 受控 SwipeActions：支持进度感知与外部互斥收起
    SwipeActions(
        ListRow(),
        [](SwipeActionContext ctx) {
          return Button("删除").OnClick(ctx.close).With(Opacity(ctx.progress));
        },
        {.controller = swipe}),

    Shaken(FormError(), shake),
    Button("重试").OnClick([shake] { shake.Trigger(); }),
  }.With(Spacing(16.0F));
}
```

非法配置在调用点直接抛出 `std::invalid_argument`。控件不施加固有布局尺寸，请通过父容器或 `.With(Frame{...})` 提供有界布局。分层设计与组件目录见 [docs/design.md](docs/design.md)，仓库结构与依赖规则见 [docs/structure.md](docs/structure.md)，测试矩阵见 [docs/testing.md](docs/testing.md)，设计演进记录见 [docs/open-questions.md](docs/open-questions.md)。

## 构建与测试

需要 CMake 3.20+ 与 C++20，依赖 HuxerUI 源码或 SDK（CI 验证版本为 `56eb5ab225997cffc83ce8f2736d1fd52c530b70`）。

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_INTERACTION_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

演示应用位于 [`examples/preview`](examples/preview/README.md)，支持独立构建：

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

## 许可证

MIT。见 [LICENSE](LICENSE)。
