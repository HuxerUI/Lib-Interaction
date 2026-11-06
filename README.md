# Lib-Interaction

[简体中文](README.zh-CN.md)

Interactive animation widgets for [HuxerUI](https://github.com/HuxerUI/HuxerUI): pressable surfaces, hover lifts, continuous pulses, and press ripples, composed from the framework's retained motion primitives.

- C++20; namespace `huxerui::interaction`, CMake target `HuxerUI::Interaction` (`huxerui_interaction`).
- `Pressable` animates hover and press scale around one pivot and forwards activation through OnClick.
- `HoverLift` drifts and grows content while a hover-capable pointer targets it.
- `Pulsing` oscillates scale or opacity with unbounded reverse repetition and no application state.
- `Ripple` is a retained modifier painting an expanding, fading circle from each primary press without consuming clicks.
- All motion flows through the HuxerUI animation system (`AnimateTo`, `MotionController`), so reduced motion, frame scheduling, and background throttling follow the framework policy.

The widgets use no platform-specific APIs. Windows builds and tests are verified locally; CI covers native widget and preview tests on Windows. Other HuxerUI backends require their normal SDK/toolchain and have not been verified here.

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
  return Column {
    Pressable(Card(),
              {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}},
              [clicks] { clicks += 1; })
        .With(Frame{.width = 240.0F, .height = 120.0F}),
    HoverLift(Card(), {.lift = {0.0F, -6.0F}}).With(Frame{.width = 240.0F, .height = 120.0F}),
    Pulsing(StatusDot(), {.scale_amplitude = 0.08F, .animate_opacity = true, .minimum_opacity = 0.4F})
        .With(Frame{.width = 12.0F, .height = 12.0F}),
    Card().With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.14F)}}),
  }.With(Spacing(16.0F));
}
```

`Pressable` and `HoverLift` are composition widgets: each call creates an independent state scope, and recomposition with equal arguments preserves the mounted state by scope position. They impose no intrinsic frame; supply bounded layout through a parent or `.With(Frame{...})`. `Pulsing` is stateless declarative motion. `Ripple` is a modifier applied through `View::With()`; it paints above content, clips to the node bounds and resolved corner radii, and observes pointer sequences with `PointerResult::Observe` so sibling gestures and OnClick continue to work.

Invalid configuration throws `std::invalid_argument` at the call site: non-finite or non-positive scales, non-finite lifts, negative amplitudes, opacities outside [0, 1], non-positive durations, a `Pulsing` style with no participating channel, or a `Ripple` with an invalid duration or opacity multiplier.

Press tracking follows pointer down, up, and cancel events for the primary button. Keyboard activation, focus-visible press feedback, and disabled-state recovery are not provided in this version; combine `OnClick` and `Enabled` at the call site. See [docs/design.md](docs/design.md) for the component catalog and motion conventions, and [docs/open-questions.md](docs/open-questions.md) for reviewed design decisions.

## Build and test

Requires CMake 3.20+ and C++20. The widgets use the HuxerUI source or SDK; the verified framework revision is `56eb5ab225997cffc83ce8f2736d1fd52c530b70`.

```sh
cmake -S . -B build -DHUXERUI_HOME=/path/to/HuxerUI \
  -DHUXERUI_INTERACTION_BUILD_TESTS=ON \
  -DHUXERUI_BUILD_TESTING_LIBRARY=ON \
  -DHUXERUI_INTERACTION_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

Run `example_interaction` to try the widgets: pressable cards with spring and tween motion, a hover-lift tile, a pulsing status tile, and a ripple card, with a light/dark theme toggle. Like the other Lib projects, the demo lives in [`examples/preview`](examples/preview/README.md) and can also be configured independently:

```sh
cmake -S examples/preview -B build-preview -DHUXERUI_HOME=/path/to/HuxerUI
cmake --build build-preview --config Release --parallel
```

The library requires the HuxerUI view system; there is no standalone core in this version. HuxerUI library-graph and project-plan discovery do not compile widget sources. There is no standalone install/export package in this version; consume it through `add_subdirectory` or `huxerui_use_library`.

## License

MIT. See [LICENSE](LICENSE).
