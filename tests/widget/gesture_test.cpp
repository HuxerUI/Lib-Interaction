#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;
#endif

using namespace huxerui;
using namespace huxerui::interaction;

#ifdef INTERACTION_HAS_UI_TESTING

View GestureGallery() {
  auto holds = UseState(0);
  auto doubles = UseState(0);
  auto order = UseState(std::vector<int>{1, 2, 3});
  auto reorder_event = UseState(std::string("none"));

  std::vector<View> slots;
  for(int value : order.Get()) {
    slots.push_back(Text("slot " + std::to_string(value))
                        .With(Frame{.width = 200.0F, .height = 48.0F},
                              Background{Color::Rgb(80, 110, 160)}, CornerRadius(8.0F))
                        .Key("slot-" + std::to_string(value)));
  }

  return Column {
    PinchZoom(Text("zoom target")
                  .With(Frame{.width = 100.0F, .height = 50.0F}, Background{Color::Rgb(90, 130, 90)})
                  .Key("zoom-content"),
              {.minimum_scale = 1.0F, .maximum_scale = 3.0F, .spring_back = false})
        .Key("pinch"),
    Reorderable(
        std::move(slots), {},
        [order, reorder_event](std::size_t from, std::size_t to) {
          auto next = order.Get();
          if(from >= next.size() || to >= next.size()) return;
          const int value = next[from];
          next.erase(next.begin() + static_cast<std::ptrdiff_t>(from));
          next.insert(next.begin() + static_cast<std::ptrdiff_t>(to), value);
          order = std::move(next);
          reorder_event = std::to_string(from) + "->" + std::to_string(to);
        })
        .Key("reorder"),
    LongPressable(Text("hold target")
                      .With(Frame{.width = 120.0F, .height = 48.0F}, Background{Color::Rgb(150, 110, 70)}),
                  {.minimum_duration = std::chrono::duration<double>(0.2)}, [holds] { holds += 1; })
        .Key("hold"),
    DoubleClickable(Text("double target")
                        .With(Frame{.width = 140.0F, .height = 48.0F},
                              Background{Color::Rgb(120, 90, 150)}),
                    {.maximum_interval = std::chrono::duration<double>(0.25)},
                    [doubles](const MultiTapEvent&) { doubles += 1; })
        .Key("double"),
    Text("holds: " + std::to_string(holds.Get())).Key("holds-count"),
    Text("doubles: " + std::to_string(doubles.Get())).Key("doubles-count"),
    Text("reorder: " + reorder_event.Get()).Key("reorder-event"),
  }.With(Padding(16.0F), Spacing(12.0F));
}

void CheckGestureContracts() {
  Application app(GestureGallery, {.window = {.initial_size = {620, 620}}, .show_debug_overlay = false});
  UiTest ui(app, {.viewport = {620, 620}});

  // PinchZoom: plain wheel passes through, Ctrl+wheel clamps to the configured range.
  const Rect initial_zoom = ui.Find(UiSelector::Key("zoom-content")).One().bounds;
  const Point zoom_center = Center(initial_zoom);
  const Point plain = ui.SendScroll({.position = zoom_center, .delta_y = 120.0F});
  ui.PumpAndSettle();
  CHECK(plain.y == 0.0F);
  CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

  const Point consumed = ui.SendScroll(
      {.position = zoom_center, .delta_y = -1000.0F, .modifiers = KeyModifiers{.control = true}});
  ui.PumpAndSettle();
  CHECK(consumed.y != 0.0F);
  CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width * 3.0F));

  const Point plain_at_max = ui.SendScroll({.position = zoom_center, .delta_y = 120.0F});
  ui.PumpAndSettle();
  CHECK(plain_at_max.y == 0.0F);
  CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width * 3.0F));

  ui.SendScroll({.position = zoom_center, .delta_y = 1000.0F, .modifiers = KeyModifiers{.control = true}});
  ui.PumpAndSettle();
  CHECK(Near(ui.Find(UiSelector::Key("zoom-content")).One().bounds.width, initial_zoom.width));

  // LongPressable: a short press does not fire; holding past the minimum duration fires once.
  const Point hold_center = Center(ui.Find(UiSelector::Key("hold")).One().bounds);
  PointerEvent press{.type = PointerEventType::Down,
                     .pointer_id = 7,
                     .position = hold_center,
                     .device_kind = PointerDeviceKind::Touch,
                     .changed_button = PointerButton::Primary,
                     .pressed_buttons = PointerButton::Primary};
  ui.SendPointer(press);
  ui.Pump(std::chrono::milliseconds(50));
  press.type = PointerEventType::Up;
  press.changed_button = PointerButton::Primary;
  press.pressed_buttons = PointerButton::None;
  ui.SendPointer(press);
  ui.Pump();
  CHECK(ui.Find(UiSelector::Text("holds: 0")).Exists());

  press.type = PointerEventType::Down;
  press.changed_button = PointerButton::Primary;
  press.pressed_buttons = PointerButton::Primary;
  ui.SendPointer(press);
  ui.Pump(std::chrono::milliseconds(250));
  ui.Pump();
  CHECK(ui.Find(UiSelector::Text("holds: 1")).Exists());
  press.type = PointerEventType::Up;
  press.changed_button = PointerButton::Primary;
  press.pressed_buttons = PointerButton::None;
  ui.SendPointer(press);
  ui.Pump();

  // DoubleClickable: two taps inside the interval fire once; a later single tap stays single.
  ui.Find(UiSelector::Key("double")).Tap();
  ui.Pump(std::chrono::milliseconds(100));
  ui.Find(UiSelector::Key("double")).Tap();
  ui.Pump();
  CHECK(ui.Find(UiSelector::Text("doubles: 1")).Exists());

  ui.Pump(std::chrono::milliseconds(300));
  ui.Find(UiSelector::Key("double")).Tap();
  ui.Pump();
  CHECK(ui.Find(UiSelector::Text("doubles: 1")).Exists());

  // Reorderable: dragging the first slot onto the third reports (0, 2) and moves the item.
  ui.Drag(Center(ui.Find(UiSelector::Key("slot-1")).One().bounds),
          Center(ui.Find(UiSelector::Key("slot-3")).One().bounds),
          {.duration = std::chrono::duration<double>(0.3), .segments = 12});
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("reorder: 0->2")).Exists());
  const Rect moved_1 = ui.Find(UiSelector::Key("slot-1")).One().bounds;
  const Rect moved_2 = ui.Find(UiSelector::Key("slot-2")).One().bounds;
  const Rect moved_3 = ui.Find(UiSelector::Key("slot-3")).One().bounds;
  CHECK(moved_2.y < moved_3.y);
  CHECK(moved_3.y < moved_1.y);
}

#endif
