#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "test.h"

#include <chrono>
#include <cmath>
#include <limits>
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
#endif
}); }
