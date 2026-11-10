#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "test.h"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>
#ifdef INTERACTION_HAS_UI_TESTING
#include <huxerui/testing/ui_test.h>
using namespace huxerui::testing;
#endif

using namespace huxerui;
using namespace huxerui::interaction;

void CheckValidation() {
  // Valid compositions construct without a composition context.
  (void)Pressable(Text("ok"));
  (void)Pressable(Text("ok"), {.hovered_scale = 1.0F, .pressed_scale = 0.9F, .animation = SpringSpec{}});
  (void)Pressable(Text("ok"), {}, [] {});
  (void)Pressable(Text("ok"), {.keyboard_focusable = false}, [] {});
  (void)HoverLift(Text("ok"));
  (void)HoverLift(Text("ok"), {.lift = {6.0F, -6.0F}, .hovered_scale = 1.0F});
  (void)Pulsing(Text("ok"));
  (void)Pulsing(Text("ok"), {.scale_amplitude = 0.0F, .minimum_opacity = 0.0F, .animate_opacity = true});
  (void)DragSpring(Text("ok"));
  (void)DragSpring(Text("ok"), {.axis = Axis::Horizontal});
  (void)Text("ok").With(Ripple{});
  (void)Text("ok").With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.1F), .duration = 0.2}});
  (void)Text("ok").With(Spotlight{});
  (void)Text("ok").With(Spotlight{SpotlightStyle{.diameter = 160.0F, .fade = 0.0}});
  (void)Text("ok").With(HoverHighlight{});
  (void)Text("ok").With(HoverHighlight{HoverHighlightStyle{.corner_radii = 10.0F}});
  (void)Text("ok").With(Shimmer{});
  (void)Text("ok").With(Shimmer{ShimmerStyle{.period = 2.0, .band_width = 0.5F}});
  (void)Text("ok").With(Fireworks{});
  (void)Text("ok").With(
      Fireworks{FireworksStyle{.particle_count = 8}, FireworksBurst{.id = 1, .local_position = {10.0F, 10.0F}}});
  (void)Shaken(Text("ok"), ShakeTrigger{});
  (void)Text("ok").With(Ripple{RippleStyle{.center_origin = true}});
  (void)PinchZoom(Text("ok"));
  (void)PinchZoom(Text("ok"), {.minimum_scale = 1.0F, .maximum_scale = 2.0F, .spring_back = true});
  (void)PinchZoom(Text("ok"), {.wheel_zoom = false});
  // Reorderable compositions.
  (void)Reorderable({Text("a"), Text("b")});
  (void)Reorderable({Text("a")}, {.spacing = 8.0F}, [](std::size_t, std::size_t) {});

  // Pressable scale validation.
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.pressed_scale = -0.5F}); });
  Throws<std::invalid_argument>([] {
    (void)Pressable(Text("x"), {.hovered_scale = std::numeric_limits<float>::infinity()});
  });
  // HoverLift validation.
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.lift = {0.0F, std::nanf("")}}); });
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.hovered_scale = 0.0F}); });
  // Pulsing validation.
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.scale_amplitude = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.minimum_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.duration = 0.0}); });
  Throws<std::invalid_argument>([] {
    (void)Pulsing(Text("x"), {.animate_scale = false, .animate_opacity = false});
  });
  // Ripple validation.
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = -0.4}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.maximum_opacity = 1.5F}}); });
  // Spotlight validation.
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.diameter = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.diameter = -8.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.fade = -0.1}}); });
  // Shake validation through the composition hook.
  Throws<std::invalid_argument>([] { (void)UseShake({.amplitude = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)UseShake({.wobbles = 0}); });
  Throws<std::invalid_argument>([] { (void)UseShake({.duration = 0.0}); });
  // LongPressable and DoubleClickable validation.
  (void)LongPressable(Text("ok"));
  (void)LongPressable(
      Text("ok"), {.minimum_duration = std::chrono::duration<double>(0.3), .held_scale = 0.9F}, [] {});
  (void)LongPressable(Text("ok"), {.show_progress = true});
  (void)DoubleClickable(Text("ok"));
  (void)DoubleClickable(
      Text("ok"), {.maximum_interval = std::chrono::duration<double>(0.25), .maximum_movement = 24.0F},
      [](const MultiTapEvent&) {});
  (void)Popped(Text("ok"), PopTrigger{});
  Throws<std::invalid_argument>([] {
    (void)LongPressable(Text("x"), {.minimum_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)LongPressable(Text("x"), {.maximum_movement = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)LongPressable(Text("x"), {.held_scale = 0.0F}); });
  Throws<std::invalid_argument>([] {
    (void)DoubleClickable(Text("x"), {.maximum_interval = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)DoubleClickable(Text("x"), {.maximum_movement = 0.0F}); });
  // Pop validation through the composition hook.
  Throws<std::invalid_argument>([] { (void)UsePop({.amplitude = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)UsePop({.duration = 0.0}); });
  // Shimmer validation.
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.period = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.band_width = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.band_width = 1.5F}}); });
  // Fireworks validation.
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.palette = {}}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.particle_count = 0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.particle_count = 65}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.duration = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.spread_radius = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.gravity = -1.0F}}); });
  // Reorderable validation.
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("x")}, {.spacing = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("x")}, {.target_highlight_width = -2.0F}); });
  // PinchZoom validation.
  Throws<std::invalid_argument>([] { (void)PinchZoom(Text("x"), {.minimum_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)PinchZoom(Text("x"), {.maximum_scale = -1.0F}); });
  Throws<std::invalid_argument>([] {
    (void)PinchZoom(Text("x"), {.minimum_scale = 2.0F, .maximum_scale = 1.0F});
  });
}

#ifdef INTERACTION_HAS_UI_TESTING

View InteractionGallery() {
  auto clicks = UseState(0);
  auto holds = UseState(0);
  auto doubles = UseState(0);
  auto shake = UseShake();
  auto pop = UsePop();
  return Column {
    Row {
      Pressable(Text("Pressable card").With(Frame{.width = 200.0F, .height = 64.0F},
                                            Background{Color::Rgb(64, 108, 178)}, CornerRadius(10.0F)),
                {}, [clicks] { clicks += 1; })
          .Key("pressable"),
      HoverLift(Text("Hover lift").With(Frame{.width = 160.0F, .height = 48.0F},
                                        Background{Color::Rgb(88, 150, 110)}))
          .Key("lift"),
      Pulsing(Text("Pulse").With(Frame{.width = 80.0F, .height = 48.0F},
                                 Background{Color::Rgb(190, 120, 60)}),
              {.duration = 0.3})
          .Key("pulse"),
      Text("Ripple").With(Frame{.width = 160.0F, .height = 48.0F},
                          Background{Color::Rgb(120, 96, 170)},
                          Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.25F)}})
          .Key("ripple"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      Shaken(Text("Shake").With(Frame{.width = 120.0F, .height = 48.0F},
                                Background{Color::Rgb(178, 88, 88)}), shake)
          .Key("shake"),
      Button("Trigger").OnClick([shake] { shake.Trigger(); }).Key("shake-trigger"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      Text("Spotlight").With(Frame{.width = 160.0F, .height = 48.0F},
                             Background{Color::Rgb(60, 70, 90)},
                             Spotlight{SpotlightStyle{.diameter = 120.0F}})
          .Key("spotlight"),
      Text("Highlight").With(Frame{.width = 160.0F, .height = 48.0F},
                             Background{Color::Rgb(90, 100, 70)},
                             HoverHighlight{HoverHighlightStyle{}})
          .Key("highlight"),
      DragSpring(Text("Drag").With(Frame{.width = 100.0F, .height = 40.0F},
                                   Background{Color::Rgb(70, 110, 130)}))
          .With(Frame{.width = 320.0F, .height = 56.0F})
          .Key("dragspring"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Row {
      LongPressable(Text("Hold").With(Frame{.width = 120.0F, .height = 48.0F},
                                      Background{Color::Rgb(96, 110, 160)}, CornerRadius(10.0F)),
                    {}, [holds] { holds += 1; })
          .Key("hold"),
      DoubleClickable(Text("Double-click").With(Frame{.width = 140.0F, .height = 48.0F},
                                                 Background{Color::Rgb(120, 140, 90)}, CornerRadius(10.0F)),
                      {}, [doubles](const MultiTapEvent&) { doubles += 1; })
          .Key("double-click"),
      Popped(Text("Pop").With(Frame{.width = 80.0F, .height = 48.0F},
                             Background{Color::Rgb(160, 96, 140)}, CornerRadius(10.0F)), pop)
          .Key("pop"),
      Button("Pop trigger").OnClick([pop] { pop.Trigger(); }).Key("pop-trigger"),
    }.With(Spacing(16.0F), CrossAlign(CrossAxisAlignment::Center)),
    Text("clicks: " + std::to_string(clicks.Get())).Key("count"),
  }.With(Padding(16.0F), Spacing(16.0F));
}

Point Center(const Rect& bounds) { return {bounds.x + bounds.width * 0.5F, bounds.y + bounds.height * 0.5F}; }

bool Near(float actual, float expected) { return std::abs(actual - expected) <= 0.5F; }

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

void CheckInteractionContracts() {
  Application app(InteractionGallery, {.window = {.initial_size = {980, 480}}, .show_debug_overlay = false});
  UiTest ui(app);
  CHECK(ui.Find(UiSelector::Key("pressable")).One().size == (Size{200, 64}));
  CHECK(ui.Find(UiSelector::Key("lift")).One().size == (Size{160, 48}));
  CHECK(ui.Find(UiSelector::Key("pulse")).One().size == (Size{80, 48}));
  CHECK(ui.Find(UiSelector::Key("ripple")).Exists());
  CHECK(ui.Find(UiSelector::Key("spotlight")).Exists());
  CHECK(ui.Find(UiSelector::Key("highlight")).Exists());
  CHECK(ui.Find(UiSelector::Key("dragspring")).One().size == (Size{320, 56}));
  CHECK(ui.Find(UiSelector::Key("hold")).Exists());
  CHECK(ui.Find(UiSelector::Key("double-click")).Exists());
  CHECK(ui.Find(UiSelector::Key("pop")).Exists());
  CHECK(ui.Find(UiSelector::Text("clicks: 0")).Exists());
  ui.Find(UiSelector::Key("pressable")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("ripple")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Text("clicks: 1")).Exists());
  ui.Find(UiSelector::Key("shake-trigger")).Tap();
  ui.PumpAndSettle();
  ui.Find(UiSelector::Key("highlight")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("shake")).Exists());
  ui.Find(UiSelector::Key("pop-trigger")).Tap();
  ui.PumpAndSettle();
  CHECK(ui.Find(UiSelector::Key("pop")).Exists());
}

#endif

int main() { return Run([] {
  CheckValidation();
#ifdef INTERACTION_HAS_UI_TESTING
  CheckInteractionContracts();
  CheckGestureContracts();
#endif
}); }
