#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>
#include <huxerui/testing/ui_test.h>

#include "test.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

using namespace huxerui;
using namespace huxerui::interaction;
using namespace huxerui::testing;

namespace {

bool Near(float actual, float expected) { return std::abs(actual - expected) <= 0.5F; }

Point Center(const Rect& bounds) { return {bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F}; }

View StateGallery() {
  auto clicks = UseState(0);
  View ripple = Text("ripple surface")
                    .With(Frame{.width = 200.0F, .height = 64.0F}, Background{Color::Rgb(80, 110, 160)},
                          CornerRadius(10.0F));
  ripple = std::move(ripple).OnClick([] {}).With(Focusable{true}, Ripple{RippleStyle{.duration = 0.4}});
  return Column {
    Pressable(Text("keyboard press")
                  .With(Frame{.width = 200.0F, .height = 64.0F}, Background{Color::Rgb(64, 108, 178)},
                        CornerRadius(10.0F)),
              {}, [clicks] { clicks += 1; })
        .Key("pressable"),
    std::move(ripple).Key("ripple"),
    Text("clicks: " + std::to_string(clicks.Get())).Key("clicks"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View SwipeGallery() {
  auto deleted = UseState(0);
  return Column {
    SwipeActions(
        Text("swipe row")
            .With(Frame{.width = 300.0F, .height = 64.0F}, Background{Color::Rgb(80, 110, 160)},
                  CornerRadius(10.0F))
            .Key("swipe-content"),
        [deleted](std::function<void()> close) {
          return Button("Delete")
              .OnClick([deleted, close] {
                deleted += 1;
                close();
              })
              .Key("delete");
        },
        {.action_extent = 96.0F, .activation_ratio = 0.4F})
        .Key("swipe"),
    Text("deleted: " + std::to_string(deleted.Get())).Key("deleted-count"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View ContextGallery() {
  auto requests = UseState(0);
  auto last_position = UseState(Point{});
  return Column {
    ContextMenuArea(Text("context target")
                        .With(Frame{.width = 220.0F, .height = 64.0F}, Background{Color::Rgb(110, 90, 150)},
                              CornerRadius(10.0F)),
                    [requests, last_position](Point position) {
                      requests += 1;
                      last_position = position;
                    })
        .Key("context"),
    Text("requests: " + std::to_string(requests.Get())).Key("request-count"),
    Text("last x: " + std::to_string(static_cast<int>(last_position.Get().x))).Key("last-x"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View DragDropGallery() {
  auto drops = UseState(0);
  auto payload = UseState(0);
  return Column {
    Draggable(Text("drag source")
                  .With(Frame{.width = 160.0F, .height = 56.0F}, Background{Color::Rgb(70, 120, 160)},
                        CornerRadius(10.0F)),
              42)
        .Key("drag-source"),
    DropZone<int>(Text("drop zone")
                      .With(Frame{.width = 220.0F, .height = 80.0F}, Background{Color::Rgb(90, 130, 100)},
                            CornerRadius(10.0F)),
                  [drops, payload](const int& value, const DropEvent&) {
                    drops += 1;
                    payload = value;
                  })
        .Key("drop-zone"),
    Text("drops: " + std::to_string(drops.Get())).Key("drop-count"),
    Text("payload: " + std::to_string(payload.Get())).Key("drop-payload"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View FileGallery() {
  return Column {
    FileDropZone(Text("file zone")
                     .With(Frame{.width = 220.0F, .height = 80.0F}, Background{Color::Rgb(120, 90, 150)},
                           CornerRadius(10.0F)),
                 FileDropOptions{.extensions = {"png"}})
        .Key("file-zone"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View TransformGallery() {
  return Column {
    PanZoom(Text("transform target")
                .With(Frame{.width = 160.0F, .height = 80.0F}, Background{Color::Rgb(80, 120, 160)},
                      CornerRadius(10.0F))
                .Key("transform-content"),
            {.minimum_scale = 0.5F, .maximum_scale = 3.0F})
        .Key("panzoom"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View InertiaGallery() {
  return Column {
    InertialDrag(Text("inertial target")
                     .With(Frame{.width = 160.0F, .height = 60.0F}, Background{Color::Rgb(150, 100, 70)},
                           CornerRadius(10.0F))
                     .Key("inertial-content"),
                 {.projection_time = 0.2F, .maximum_projection = 300.0F})
        .Key("inertial"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View ScrollGallery() {
  auto controller = UseScrollController();
  std::vector<View> rows;
  for(int index = 0; index < 24; ++index) {
    rows.push_back(Text("scroll row " + std::to_string(index))
                       .With(Frame{.height = 32.0F})
                       .Key("scroll-row-" + std::to_string(index)));
  }
  return ScrollView(Column(std::move(rows)).With(Spacing(8.0F)))
      .Controller(controller)
      .With(ScrollProgress{controller, ScrollProgressStyle{}})
      .With(ScrollEdgeFade{controller, ScrollEdgeFadeStyle{.color = Color::White()}})
      .Key("scroll");
}

View RollingGallery() {
  auto value = UseState(0);
  return Column {
    RollingNumber(value.Get(), {.motion = TweenSpec{.duration = 0.2}}).Key("rolling"),
    Button("Increment").OnClick([value] { value += 1; }).Key("increment"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View SemanticsGallery() {
  auto count = UseState(0);
  View target = Text("semantic target")
                    .With(Frame{.width = 220.0F, .height = 48.0F}, Background{Color::Rgb(80, 110, 160)},
                          CornerRadius(8.0F));
  target = std::move(target)
               .With(Semantics{.role = SemanticRole::Button, .label = "Semantic target"},
                     SemanticActions{Semantics{},
                                     SemanticActionMask(SemanticActionKind::Activate),
                                     [count](SemanticActionKind) { count += 1; }});
  return Column {
    std::move(target).Key("semantic"),
    Text("semantic count: " + std::to_string(count.Get())).Key("semantic-count"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View KeyboardGallery() {
  auto count = UseState(0);
  View root = Column {
    Button("Focus me").Key("focus-target"),
    Text("shortcut count: " + std::to_string(count.Get())).Key("shortcut-count"),
  }.With(Padding(16.0F), Spacing(16.0F));
  return KeyboardShortcut(std::move(root), {.key = Key::K, .modifiers = KeyModifiers{.control = true}},
                          [count] { count += 1; });
}

View MultiGallery() {
  auto count = UseState(0);
  return Column {
    MultiClickable(Text("triple target")
                       .With(Frame{.width = 200.0F, .height = 48.0F}, Background{Color::Rgb(120, 90, 150)},
                             CornerRadius(8.0F)),
                   {.count = 3}, [count](const MultiTapEvent&) { count += 1; })
        .Key("triple"),
    Text("triples: " + std::to_string(count.Get())).Key("triples"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

View SecondaryGallery() {
  auto count = UseState(0);
  return Column {
    SecondaryClickable(Text("secondary target")
                           .With(Frame{.width = 220.0F, .height = 48.0F}, Background{Color::Rgb(90, 130, 100)},
                                 CornerRadius(8.0F)),
                       [count](Point) { count += 1; })
        .Key("secondary"),
    Text("secondaries: " + std::to_string(count.Get())).Key("secondaries"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

} // namespace

int main() { return Run([] {
  Application app(StateGallery, {.window = {.initial_size = {420, 360}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {420, 360}});

  const Rect initial = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  ui.PressKey(Key::Tab);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pressable")).Find(UiSelector::Focused(true)).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());

  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.PumpAndSettle();
  const Rect held = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  CHECK(held.width < initial.width);

  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
  const Rect released = ui.Find(UiSelector::Text("keyboard press")).One().bounds;
  CHECK(Near(released.width, initial.width));
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());

  // Focus loss clears the held Space scale even if the key release is never delivered to the widget.
  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("keyboard press")).One().bounds.width < initial.width);
  ui.PressKey(Key::Tab);
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("ripple")).One().focused);
  CHECK(Near(ui.Find(UiSelector::Text("keyboard press")).One().bounds.width, initial.width));
  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());

  // Keyboard activation drives the same InteractionEvent path as pointer presses.
  ui.SendKey({.type = KeyEventType::Down, .key = Key::Space});
  ui.Pump(std::chrono::milliseconds(100));
  CHECK(ui.Find(UiSelector::Key("ripple")).One().focused);
  ui.SendKey({.type = KeyEventType::Up, .key = Key::Space});
  ui.PumpAndSettle();

  {
    Application swipe_app(SwipeGallery, {.window = {.initial_size = {420, 240}}, .show_debug_overlay = false});
    UiTest swipe(swipe_app, {.viewport = {420, 240}});
    const Rect initial = swipe.Find(UiSelector::Key("swipe-content")).One().bounds;
    const Point start{initial.x + initial.width * 0.5F, initial.y + initial.height * 0.5F};
    swipe.Drag(start, {start.x - 120.0F, start.y},
               {.duration = std::chrono::duration<double>(0.3), .segments = 12});
    swipe.PumpAndSettle();
    const Rect open = swipe.Find(UiSelector::Key("swipe-content")).One().bounds;
    CHECK(open.x < initial.x - 40.0F);
    const UiNodeInfo action_node = swipe.Find(UiSelector::Key("delete")).One();
    CHECK(action_node.in_viewport);
    CHECK(Near(action_node.bounds.height, initial.height));
    swipe.Find(UiSelector::Key("delete")).Tap();
    swipe.PumpAndSettle();
    CHECK(swipe.Find(UiSelector::Text("deleted: 1")).Exists());
    CHECK(Near(swipe.Find(UiSelector::Key("swipe-content")).One().bounds.x, initial.x));
  }

  {
    Application context_app(ContextGallery, {.window = {.initial_size = {420, 240}}, .show_debug_overlay = false});
    UiTest context(context_app, {.viewport = {420, 240}});
    const Point target_point = Center(context.Find(UiSelector::Key("context")).One().bounds);
    PointerEvent secondary{.type = PointerEventType::Down,
                           .pointer_id = 9,
                           .position = target_point,
                           .device_kind = PointerDeviceKind::Mouse,
                           .changed_button = PointerButton::Secondary,
                           .pressed_buttons = PointerButton::Secondary};
    context.SendPointer(secondary);
    secondary.type = PointerEventType::Up;
    secondary.changed_button = PointerButton::Secondary;
    secondary.pressed_buttons = PointerButton::None;
    context.SendPointer(secondary);
    context.PumpAndSettle();
    CHECK(context.Find(UiSelector::Text("requests: 1")).Exists());

    PointerEvent press{.type = PointerEventType::Down,
                       .pointer_id = 10,
                       .position = target_point,
                       .device_kind = PointerDeviceKind::Touch,
                       .changed_button = PointerButton::Primary,
                       .pressed_buttons = PointerButton::Primary};
    context.SendPointer(press);
    context.Pump(std::chrono::milliseconds(600));
    context.Pump();
    press.type = PointerEventType::Up;
    press.changed_button = PointerButton::Primary;
    press.pressed_buttons = PointerButton::None;
    context.SendPointer(press);
    context.PumpAndSettle();
    CHECK(context.Find(UiSelector::Text("requests: 2")).Exists());
    CHECK(context.Find(UiSelector::Text("last x: " + std::to_string(static_cast<int>(target_point.x)))).Exists());
  }

  {
    Application drag_app(DragDropGallery, {.window = {.initial_size = {420, 320}}, .show_debug_overlay = false});
    UiTest drag(drag_app, {.viewport = {420, 320}});
    const Rect source = drag.Find(UiSelector::Key("drag-source")).One().bounds;
    const Rect target = drag.Find(UiSelector::Key("drop-zone")).One().bounds;
    drag.Drag(Center(source), Center(target), {.duration = std::chrono::duration<double>(0.3), .segments = 12});
    drag.PumpAndSettle();
    CHECK(drag.Find(UiSelector::Text("drops: 1")).Exists());
    CHECK(drag.Find(UiSelector::Text("payload: 42")).Exists());
  }

  {
    Application file_app(FileGallery, {.window = {.initial_size = {420, 240}}, .show_debug_overlay = false});
    UiTest file(file_app, {.viewport = {420, 240}});
    CHECK(file.Find(UiSelector::Key("file-zone")).Exists());
  }

  {
    Application transform_app(TransformGallery, {.window = {.initial_size = {420, 320}}, .show_debug_overlay = false});
    UiTest transform(transform_app, {.viewport = {420, 320}});
    const Rect initial = transform.Find(UiSelector::Key("transform-content")).One().bounds;
    const Point center = Center(initial);
    const Point left{center.x - 30.0F, center.y};
    const Point right{center.x + 30.0F, center.y};
    PointerEvent first{.type = PointerEventType::Down,
                       .pointer_id = 21,
                       .position = left,
                       .device_kind = PointerDeviceKind::Touch,
                       .changed_button = PointerButton::Primary,
                       .pressed_buttons = PointerButton::Primary};
    PointerEvent second = first;
    second.pointer_id = 22;
    second.position = right;
    transform.SendPointer(first);
    transform.SendPointer(second);
    transform.Pump();
    first.type = PointerEventType::Move;
    first.position = {center.x - 60.0F, center.y};
    second.type = PointerEventType::Move;
    second.position = {center.x + 60.0F, center.y};
    transform.SendPointer(first);
    transform.SendPointer(second);
    transform.PumpAndSettle();
    const Rect scaled = transform.Find(UiSelector::Key("transform-content")).One().bounds;
    CHECK(scaled.width > initial.width * 1.2F);
    first.type = PointerEventType::Up;
    first.changed_button = PointerButton::Primary;
    first.pressed_buttons = PointerButton::None;
    second.type = PointerEventType::Up;
    second.changed_button = PointerButton::Primary;
    second.pressed_buttons = PointerButton::None;
    transform.SendPointer(first);
    transform.SendPointer(second);
    transform.PumpAndSettle();

    // Pan with a fresh two-pointer sequence.
    PointerEvent pan_first{.type = PointerEventType::Down,
                           .pointer_id = 31,
                           .position = left,
                           .device_kind = PointerDeviceKind::Touch,
                           .changed_button = PointerButton::Primary,
                           .pressed_buttons = PointerButton::Primary};
    PointerEvent pan_second = pan_first;
    pan_second.pointer_id = 32;
    pan_second.position = right;
    transform.SendPointer(pan_first);
    transform.SendPointer(pan_second);
    transform.Pump();
    pan_first.type = PointerEventType::Move;
    pan_first.position = {left.x + 40.0F, left.y};
    pan_second.type = PointerEventType::Move;
    pan_second.position = {right.x + 40.0F, right.y};
    transform.SendPointer(pan_first);
    transform.SendPointer(pan_second);
    transform.PumpAndSettle();
    const Rect panned = transform.Find(UiSelector::Key("transform-content")).One().bounds;
    CHECK(panned.x > scaled.x + 10.0F);
    pan_first.type = PointerEventType::Up;
    pan_first.changed_button = PointerButton::Primary;
    pan_first.pressed_buttons = PointerButton::None;
    pan_second.type = PointerEventType::Up;
    pan_second.changed_button = PointerButton::Primary;
    pan_second.pressed_buttons = PointerButton::None;
    transform.SendPointer(pan_first);
    transform.SendPointer(pan_second);
    transform.PumpAndSettle();

    // Rotate the accumulated transform by moving two pointers from horizontal to vertical.
    const Point rotated_center = Center(panned);
    PointerEvent rotate_first{.type = PointerEventType::Down,
                              .pointer_id = 41,
                              .position = {rotated_center.x - 60.0F, rotated_center.y},
                              .device_kind = PointerDeviceKind::Touch,
                              .changed_button = PointerButton::Primary,
                              .pressed_buttons = PointerButton::Primary};
    PointerEvent rotate_second = rotate_first;
    rotate_second.pointer_id = 42;
    rotate_second.position = {rotated_center.x + 60.0F, rotated_center.y};
    transform.SendPointer(rotate_first);
    transform.SendPointer(rotate_second);
    transform.Pump();
    rotate_first.type = PointerEventType::Move;
    rotate_first.position = {rotated_center.x, rotated_center.y - 60.0F};
    rotate_second.type = PointerEventType::Move;
    rotate_second.position = {rotated_center.x, rotated_center.y + 60.0F};
    transform.SendPointer(rotate_first);
    transform.SendPointer(rotate_second);
    transform.PumpAndSettle();
    const Rect rotated = transform.Find(UiSelector::Key("transform-content")).One().bounds;
    CHECK(rotated.height > panned.height + 20.0F);
    rotate_first.type = PointerEventType::Up;
    rotate_first.changed_button = PointerButton::Primary;
    rotate_first.pressed_buttons = PointerButton::None;
    rotate_second.type = PointerEventType::Up;
    rotate_second.changed_button = PointerButton::Primary;
    rotate_second.pressed_buttons = PointerButton::None;
    transform.SendPointer(rotate_first);
    transform.SendPointer(rotate_second);
    transform.PumpAndSettle();
  }

  {
    Application inertia_app(InertiaGallery, {.window = {.initial_size = {420, 240}}, .show_debug_overlay = false});
    UiTest inertia(inertia_app, {.viewport = {420, 240}});
    const Rect initial = inertia.Find(UiSelector::Key("inertial-content")).One().bounds;
    const Point start = Center(initial);
    inertia.Drag(start, {start.x + 60.0F, start.y},
                 {.duration = std::chrono::duration<double>(0.1), .segments = 5});
    inertia.PumpAndSettle();
    const Rect settled = inertia.Find(UiSelector::Key("inertial-content")).One().bounds;
    CHECK(settled.x > initial.x + 60.0F);
  }

  {
    Application scroll_app(ScrollGallery, {.window = {.initial_size = {320, 240}}, .show_debug_overlay = false});
    UiTest scroll(scroll_app, {.viewport = {320, 240}});
    const float row_before = scroll.Find(UiSelector::Key("scroll-row-0")).One().bounds.y;
    scroll.SendScroll({.position = {160.0F, 120.0F}, .delta_y = 160.0F});
    scroll.PumpAndSettle();
    CHECK(scroll.Find(UiSelector::Key("scroll-row-0")).One().bounds.y < row_before);
  }

  {
    Application rolling_app(RollingGallery, {.window = {.initial_size = {320, 240}}, .show_debug_overlay = false});
    UiTest rolling(rolling_app, {.viewport = {320, 240}});
    const float zero_before = rolling.Find(UiSelector::Key("rolling")).Find(UiSelector::Text("0")).One().bounds.y;
    rolling.Find(UiSelector::Key("increment")).Tap();
    rolling.PumpAndSettle();
    const float zero_after = rolling.Find(UiSelector::Key("rolling")).Find(UiSelector::Text("0")).One().bounds.y;
    CHECK(zero_after < zero_before - 1.0F);
  }

  {
    Application semantic_app(SemanticsGallery, {.window = {.initial_size = {360, 240}}, .show_debug_overlay = false});
    UiTest semantic(semantic_app, {.viewport = {360, 240}});
    semantic.FindSemantics(UiSemanticSelector::Label("Semantic target"))
        .PerformSemanticAction({SemanticActionKind::Activate, std::monostate{}});
    semantic.PumpAndSettle();
    CHECK(semantic.Find(UiSelector::Text("semantic count: 1")).Exists());
  }

  {
    Application keyboard_app(KeyboardGallery, {.window = {.initial_size = {360, 240}}, .show_debug_overlay = false});
    UiTest keyboard(keyboard_app, {.viewport = {360, 240}});
    keyboard.Find(UiSelector::Key("focus-target")).Tap();
    keyboard.PumpAndSettle();
    keyboard.PressKey(Key::K, KeyModifiers{.control = true});
    keyboard.PumpAndSettle();
    CHECK(keyboard.Find(UiSelector::Text("shortcut count: 1")).Exists());
  }

  {
    Application multi_app(MultiGallery, {.window = {.initial_size = {360, 240}}, .show_debug_overlay = false});
    UiTest multi(multi_app, {.viewport = {360, 240}});
    for(int tap = 0; tap < 3; ++tap) {
      multi.Find(UiSelector::Key("triple")).Tap();
      multi.Pump(std::chrono::milliseconds(50));
    }
    multi.PumpAndSettle();
    CHECK(multi.Find(UiSelector::Text("triples: 1")).Exists());
  }

  {
    Application secondary_app(SecondaryGallery, {.window = {.initial_size = {360, 240}}, .show_debug_overlay = false});
    UiTest secondary(secondary_app, {.viewport = {360, 240}});
    const Point point = Center(secondary.Find(UiSelector::Key("secondary")).One().bounds);
    PointerEvent click{.type = PointerEventType::Down,
                       .pointer_id = 51,
                       .position = point,
                       .device_kind = PointerDeviceKind::Mouse,
                       .changed_button = PointerButton::Secondary,
                       .pressed_buttons = PointerButton::Secondary};
    secondary.SendPointer(click);
    click.type = PointerEventType::Up;
    click.changed_button = PointerButton::Secondary;
    click.pressed_buttons = PointerButton::None;
    secondary.SendPointer(click);
    secondary.PumpAndSettle();
    CHECK(secondary.Find(UiSelector::Text("secondaries: 1")).Exists());
  }

  Throws<std::invalid_argument>([] { (void)SwipeActions(Text("x"), {}, {}); });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](std::function<void()>) { return Text("action"); },
                       {.action_extent = 0.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](std::function<void()>) { return Text("action"); },
                       {.activation_ratio = 1.5F});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](std::function<void()>) { return Text("action"); },
                       {.minimum_velocity = -1.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)ContextMenuArea(Text("x"), {}, {.long_press_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)Draggable(Text("x"), 1, {.dragging_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)DropZone<int>(Text("x"), {}, {.border_width = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)FileDropZone(Text("x"), {.extensions = {"bad/name"}}); });
  Throws<std::invalid_argument>([] {
    (void)FileDropZone(Text("x"), {}, {}, {.border_width = -1.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)PanZoom(Text("x"), {.minimum_scale = 2.0F, .maximum_scale = 1.0F});
  });
  Throws<std::invalid_argument>([] { (void)PanZoom(Text("x"), {.wheel_sensitivity = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)InertialDrag(Text("x"), {.projection_time = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)InertialDrag(Text("x"), {.maximum_projection = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollProgress(ScrollController{}, {.thickness = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollEdgeFade(ScrollController{}, {.extent = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)RollingNumber(0, {.digit_gap = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)RollingNumber(0, {.minimum_digits = -1}); });
  Throws<std::invalid_argument>([] { (void)MultiClickable(Text("x"), {.count = 1}); });
  Throws<std::invalid_argument>([] { (void)KeyboardShortcut(Text("x"), {}, [] {}); });
  Throws<std::invalid_argument>([] {
    (void)SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Custom)};
  });
}); }
