# Lib-Interaction

[简体中文](README.zh-CN.md)

Interactive primitives, headless state builders, controllers, retained feedback modifiers, and visual motion effects for [HuxerUI](https://github.com/HuxerUI/HuxerUI) — built in C++20 under namespace `huxerui::interaction` and CMake target `HuxerUI::Interaction` (`huxerui_interaction`).

## Architecture highlights

- **Headless state & controllers (Layer 1)**:
  - `InteractiveSurface`, `Hoverable`, `Copyable`, `Popover`, and `PredictiveBack` run hover, primary-button press, keyboard activation, clipboard feedback, popup visibility, predictive back progress, and focus tracking while delegating visuals to caller builders.
  - `SwipeController` (`UseSwipeController`), `PanZoomController` (`UsePanZoomController`), `HoverCardController` (`UseHoverCardController`), `PopoverController` (`UsePopoverController`), `ScrollHideController` (`UseScrollHideController`), `ScrollSpyController` (`UseScrollSpyController`), `RovingFocusController` (`UseRovingFocusController`), `PredictiveBackController` (`UsePredictiveBackController`), and `MarqueeSelectController` (`UseMarqueeSelectController`) support external observation and programmatic control.
  - `ReorderHandle`, `RovingFocusItem`, `MarqueeSelectItem`, and `ScrollSpySection` connect to enclosing containers via `Environment` (`ProvideEnvironment` / `UseEnvironment`).
  - `ViewAdaptor` (`operator|`) chains decorators left-to-right without deeply nested wrapper calls.
- **Styled interaction presets & retained modifiers (Layer 2)**:
  - **Press & Hover**: `Pressable`, `Copyable`, `LongPressable`, `HoldProgressRing`, `MultiClickable`, `DoubleClickable`, `ContextMenuArea`, `SecondaryClickable`, `Ripple`, `HoverLift`, `HoverHighlight`, `HoverShadow`, `Spotlight`, `Magnetic`.
  - **Gesture, Reorder & Drag/Drop**: `DragSpring`, `InertialDrag`, `MarqueeSelectArea`, `MarqueeSelectItem`, `PanZoom`, `PinchZoom`, `SwipeActions`, `Dismissible`, `PressDragHandle`, `ResizeHandle`, `Reorderable`, `ReorderableGrid`, `Draggable<T>`, predicate-filtered `DropZone<T>`, `FileDropZone`.
  - **Overlay, Scroll & Accessibility**: `HoverCard`, `Popover`, `ScrollProgress`, `ScrollEdgeFade`, `NestedScrollConnection`, `ScrollSpyArea`, `ScrollSpySection`, `KeyboardShortcut`, `ShortcutMap`, `BackHandler`, `PredictiveBack`, `RovingFocusGroup`, `RovingFocusItem`, `SemanticActions`.
- **Visual motion domain (`visual/`)**:
  - Non-interactive and value-driven effects isolated from input recognizers: `Expandable`, `Appear`, `SharedMorph` (`UseSharedMorph`, `MorphScope`, `MorphElement`, `MorphBounds`), `Pulsing`, `Shimmer`, `Fireworks`, `RollingNumber`, `UsePop` / `Popped`, `UseShake` / `Shaken`, `UseRevealOnTap`.
- **Framework-native motion**: Every animation runs through `AnimateTo`, `MotionController`, or `TaskScope::Delay`, automatically honoring system reduced-motion preferences and frame scheduling.

## Add to a HuxerUI app

```cmake
# After huxerui_add_app(your_app ...):
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
    // Pipeline syntax: stateless decorators precede stateful Scope wrappers on the same node
    Card()
        .With(Frame{.width = 240.0F, .height = 120.0F}, Ripple{}, Spotlight{})
        | ContextMenuArea([](Point p) { /* open menu at p */ })
        | Pressable([clicks] { clicks += 1; },
                    {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}}),

    // Headless InteractiveSurface driving custom visuals from {hovered, pressed, focused}
    InteractiveSurface(
        [](const InteractionSnapshot& s) {
          return Text(s.pressed ? "Pressed" : (s.hovered ? "Hovered" : "Idle"))
              .With(Frame{.width = 240.0F, .height = 56.0F});
        },
        [clicks] { clicks += 1; }),

    // Controlled SwipeActions
    SwipeActions(
        ListRow(),
        [](SwipeActionContext ctx) {
          return Button("Delete").OnClick(ctx.close).With(Opacity(ctx.progress));
        },
        {.controller = swipe}),

    Shaken(FormError(), shake),
    Button("Retry").OnClick([shake] { shake.Trigger(); }),
  }.With(Spacing(16.0F));
}
```

Invalid configuration throws `std::invalid_argument` at the call site. Views impose no intrinsic frame; supply bounded layout through a parent or `.With(Frame{...})`. See [docs/design.md](docs/design.md) for the layered architecture and component catalog, [docs/structure.md](docs/structure.md) for repository boundaries, [docs/testing.md](docs/testing.md) for the test suite, and [docs/open-questions.md](docs/open-questions.md) for design history.

## Build and test

Requires CMake 3.20+ and C++20, plus HuxerUI source or SDK (`56eb5ab225997cffc83ce8f2736d1fd52c530b70` verified in CI).

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_INTERACTION_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The preview gallery in [`examples/preview`](examples/preview/README.md) can also be built standalone:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

## License

MIT. See [LICENSE](LICENSE).
