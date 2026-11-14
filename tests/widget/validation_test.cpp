#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cstddef>
#include <limits>

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
