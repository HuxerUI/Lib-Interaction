#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>
#include <huxerui/testing/ui_test.h>

#include "test.h"

#include <chrono>
#include <cmath>
#include <functional>
#include <string>

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
}); }
