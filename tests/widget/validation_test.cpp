#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include "../support/test.h"

#include <chrono>
#include <cstddef>
#include <limits>

using namespace huxerui;
using namespace huxerui::interaction;

void CheckValidation() {
  // Valid compositions construct without an active composition context.
  (void)InteractiveSurface([](const InteractionSnapshot&) { return Text("ok"); });
  (void)Pressable(Text("ok"));
  (void)Pressable(Text("ok"), {.hovered_scale = 1.0F, .pressed_scale = 0.9F, .animation = SpringSpec{}});
  (void)Pressable(Text("ok"), [] {});
  (void)Pressable(Text("ok"), [] {}, {.keyboard_focusable = false});
  (void)Hoverable([](bool) { return Text("ok"); });
  (void)HoverLift(Text("ok"));
  (void)HoverLift(Text("ok"), {.lift = {6.0F, -6.0F}, .hovered_scale = 1.0F});
  (void)Pulsing(Text("ok"));
  (void)Pulsing(Text("ok"), {.scale_amplitude = 0.0F, .minimum_opacity = 0.0F, .animate_opacity = true});
  (void)DragSpring(Text("ok"));
  (void)DragSpring(Text("ok"), {.axis = Axis::Horizontal});
  (void)Text("ok").With(Ripple{});
  (void)Text("ok").With(Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.1F), .duration = 0.2}});
  (void)Text("ok").With(Ripple{RippleStyle{.center_origin = true}});
  (void)Text("ok").With(HoldProgressRing{});
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
  (void)Popped(Text("ok"), PopTrigger{});
  (void)PinchZoom(Text("ok"));
  (void)PinchZoom(Text("ok"), {.minimum_scale = 1.0F, .maximum_scale = 2.0F, .spring_back = true});
  (void)PinchZoom(Text("ok"), {.wheel_zoom = false});
  (void)PanZoom(Text("ok"));
  (void)Reorderable({Text("a"), Text("b")});
  (void)Reorderable({Text("a")}, [](std::size_t, std::size_t) {}, {.spacing = 8.0F});
  (void)ReorderableGrid({Text("a")});
  (void)ReorderHandle(Text("grip"));
  (void)LongPressable(Text("ok"));
  (void)LongPressable(Text("ok"), [] {}, {.minimum_duration = std::chrono::duration<double>(0.3), .held_scale = 0.9F});
  (void)LongPressable(Text("ok"), {.show_progress = true});
  (void)HoldRepeat(Text("ok"));
  (void)HoldRepeat(Text("ok"), [] {});
  (void)HoldRepeat(Text("ok"), [](std::size_t) {},
                   {.initial_delay = std::chrono::duration<double>(0.3),
                    .repeat_interval = std::chrono::duration<double>(0.1),
                    .minimum_interval = std::chrono::duration<double>(0.04),
                    .acceleration = 0.8F});
  (void)HoldRepeat([](HoldRepeatSnapshot) { return Text("ok"); });
  (void)DoubleClickable(Text("ok"));
  (void)DoubleClickable(
      Text("ok"), [](const MultiTapEvent&) {},
      {.maximum_interval = std::chrono::duration<double>(0.25), .maximum_movement = 24.0F});

  // Press family validation.
  Throws<std::invalid_argument>([] { (void)InteractiveSurface({}); });
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Pressable(Text("x"), {.pressed_scale = -0.5F}); });
  Throws<std::invalid_argument>([] {
    (void)Pressable(Text("x"), {.hovered_scale = std::numeric_limits<float>::infinity()});
  });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.duration = -0.4}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Ripple{RippleStyle{.maximum_opacity = 1.5F}}); });
  Throws<std::invalid_argument>([] {
    (void)Text("x").With(HoldProgressRing{HoldProgressStyle{.duration = 0.0}});
  });
  Throws<std::invalid_argument>([] {
    (void)Text("x").With(HoldProgressRing{HoldProgressStyle{.stroke_width = 0.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)Text("x").With(HoldProgressRing{HoldProgressStyle{.inset = -1.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)LongPressable(Text("x"), {.minimum_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] {
    (void)LongPressable(Text("x"), {.minimum_duration = std::chrono::duration<double>(std::nan(""))});
  });
  Throws<std::invalid_argument>([] { (void)LongPressable(Text("x"), {.maximum_movement = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)LongPressable(Text("x"), {.held_scale = 0.0F}); });
  Throws<std::invalid_argument>([] {
    (void)DoubleClickable(Text("x"), {.maximum_interval = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)DoubleClickable(Text("x"), {.maximum_movement = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)MultiClickable(Text("x"), {.count = 1}); });
  Throws<std::invalid_argument>([] {
    (void)ContextMenuArea(Text("x"), {}, {.long_press_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] {
    (void)ContextMenuArea(Text("x"), {}, {.long_press_duration = std::chrono::duration<double>(std::nan(""))});
  });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(std::function<View(HoldRepeatSnapshot)>{}); });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.initial_delay = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.initial_delay = std::chrono::duration<double>(std::nan(""))});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.repeat_interval = std::chrono::duration<double>(-0.1)});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.minimum_interval = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.repeat_interval = std::chrono::duration<double>(0.1),
                                 .minimum_interval = std::chrono::duration<double>(0.2)});
  });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(Text("x"), {.acceleration = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(Text("x"), {.acceleration = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(Text("x"), {.acceleration = std::nanf("")}); });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)HoldRepeat(Text("x"), {.pressed_scale = -1.0F}); });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.origin = {std::numeric_limits<float>::infinity(), 0.5F}});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.animation = TweenSpec{.duration = -0.1}});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.animation = SpringSpec{.stiffness = 0.0F, .damping_ratio = 1.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)HoldRepeat(Text("x"), {.animation = SpringSpec{.stiffness = 100.0F, .damping_ratio = 0.0F}});
  });

  // Hover & Overlay validation.
  Throws<std::invalid_argument>([] { (void)Hoverable({}); });
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.lift = {0.0F, std::nanf("")}}); });
  Throws<std::invalid_argument>([] { (void)HoverLift(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.diameter = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.diameter = -8.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Spotlight{SpotlightStyle{.fade = -0.1}}); });
  Throws<std::invalid_argument>([] { (void)HoverCard(Text("x"), {}, {}); });
  Throws<std::invalid_argument>([] {
    (void)HoverCard(Text("x"), [](PopupContext) { return Text("ok"); },
                    {.show_delay = std::chrono::duration<double>(-0.1)});
  });
  Throws<std::invalid_argument>([] {
    (void)HoverCard(Text("x"), [](PopupContext) { return Text("ok"); }, {.gap = -1.0F});
  });

  // Gesture validation.
  Throws<std::invalid_argument>([] { (void)SwipeActions(Text("x"), {}, {}); });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](SwipeActionContext) { return Text("action"); }, {.action_extent = 0.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](SwipeActionContext) { return Text("action"); }, {.activation_ratio = 1.5F});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeActions(Text("x"), [](SwipeActionContext) { return Text("action"); }, {.minimum_velocity = -1.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)PanZoom(Text("x"), {.minimum_scale = 2.0F, .maximum_scale = 1.0F});
  });
  Throws<std::invalid_argument>([] { (void)PanZoom(Text("x"), {.wheel_sensitivity = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)PanZoom(Text("x"), {.pan_bounds = Point{0.0F, 10.0F}}); });
  Throws<std::invalid_argument>([] { (void)PanZoom(Text("x"), {.rotation_snap = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)UsePanZoomController(0.0F); });
  Throws<std::invalid_argument>([] { (void)PinchZoom(Text("x"), {.minimum_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)PinchZoom(Text("x"), {.maximum_scale = -1.0F}); });
  Throws<std::invalid_argument>([] {
    (void)PinchZoom(Text("x"), {.minimum_scale = 2.0F, .maximum_scale = 1.0F});
  });
  Throws<std::invalid_argument>([] { (void)InertialDrag(Text("x"), {.projection_time = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)InertialDrag(Text("x"), {.maximum_projection = -1.0F}); });
  Throws<std::invalid_argument>([] {
    (void)PressDragHandle(Text("x"), {.press_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)Dismissible(Text("x"), {}, {.dismiss_threshold = 0.0F}); });
  Throws<std::invalid_argument>([] {
    (void)Dismissible(Text("x"), std::function<View(float, float)>{}, {});
  });
  Throws<std::invalid_argument>([] { (void)ResizeHandle(Axis::Horizontal, {}, {.thickness = 0.0F}); });

  // Reorder & DragDrop validation.
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("x")}, {.spacing = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("x")}, {.target_highlight_width = -2.0F}); });
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("a")}, {.auto_scroll_edge = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Reorderable({Text("a")}, {.auto_scroll_step = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)ReorderableGrid({Text("a")}, {.cell_width = 0.0F}); });
  Throws<std::invalid_argument>([] {
    (void)ReorderHandle(Text("g"), {.minimum_press_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] { (void)Draggable(Text("x"), 1, {.dragging_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)DropZone<int>(Text("x"), {}, {.border_width = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)DropZone<int>(std::function<View(bool)>{}); });
  Throws<std::invalid_argument>([] { (void)FileDropZone(Text("x"), {.extensions = {"bad/name"}}); });
  Throws<std::invalid_argument>([] { (void)FileDropZone(Text("x"), {}, {}, {.border_width = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)FileDropZone(std::function<View(bool)>{}, {}); });

  // Scroll, Accessibility & Visual validation.
  Throws<std::invalid_argument>([] { (void)ScrollProgress(ScrollController{}, {.thickness = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollEdgeFade(ScrollController{}, {.extent = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)KeyboardShortcut(Text("x"), {}, [] {}); });
  Throws<std::invalid_argument>([] {
    (void)SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Custom)};
  });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.scale_amplitude = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.minimum_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)Pulsing(Text("x"), {.duration = 0.0}); });
  Throws<std::invalid_argument>([] {
    (void)Pulsing(Text("x"), {.animate_scale = false, .animate_opacity = false});
  });
  Throws<std::invalid_argument>([] { (void)UseShake({.amplitude = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)UseShake({.wobbles = 0}); });
  Throws<std::invalid_argument>([] { (void)UseShake({.duration = 0.0}); });
  Throws<std::invalid_argument>([] { (void)UsePop({.amplitude = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)UsePop({.duration = 0.0}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.period = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.band_width = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Shimmer{ShimmerStyle{.band_width = 1.5F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.palette = {}}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.particle_count = 0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.particle_count = 65}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.duration = 0.0}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.spread_radius = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Text("x").With(Fireworks{FireworksStyle{.gravity = -1.0F}}); });
  Throws<std::invalid_argument>([] { (void)RollingNumber(0, {.digit_gap = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)RollingNumber(0, {.minimum_digits = -1}); });

  // Batch 1 upgrades validation.
  Throws<std::invalid_argument>([] { (void)UseScrollHideController({.extent = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)UseScrollHideController({.extent = -5.0F}); });
  Throws<std::invalid_argument>([] { (void)Expandable(Text("x"), true, {.motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>([] { (void)Expandable(Text("x"), true, {.motion = SpringSpec{.stiffness = 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Appear(Text("x"), {.initial_opacity = 1.2F}); });
  Throws<std::invalid_argument>([] { (void)Appear(Text("x"), {.initial_offset = {std::nanf(""), 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)Appear(Text("x"), {.initial_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Appear(Text("x"), {.delay = -0.1}); });
  Throws<std::invalid_argument>([] { (void)Magnetic(Text("x"), {.maximum_offset = {-1.0F, 4.0F}}); });
  Throws<std::invalid_argument>([] { (void)Magnetic(Text("x"), {.hovered_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)Magnetic(Text("x"), {.follow_motion = TweenSpec{.duration = -0.1}}); });

  // Batch 2 upgrades validation.
  Throws<std::invalid_argument>([] { (void)ShortcutMap(Text("x"), {{.chord = {.key = Key::Unknown}}}); });
  Throws<std::invalid_argument>([] {
    (void)Popover(std::function<View(PopoverTriggerContext)>{}, [](PopupContext) { return Text("ok"); });
  });
  Throws<std::invalid_argument>([] { (void)Popover(Text("x"), std::function<View(PopupContext)>{}); });
  Throws<std::invalid_argument>([] {
    (void)Popover(Text("x"), [](PopupContext) { return Text("ok"); }, {.gap = -1.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)Popover(Text("x"), [](PopupContext) { return Text("ok"); }, {.offset = {std::nanf(""), 0.0F}});
  });
  Throws<std::invalid_argument>([] { (void)Copyable(std::function<View(bool)>{}, "text"); });
  Throws<std::invalid_argument>([] {
    (void)Copyable(Text("x"), "text", {.feedback_duration = std::chrono::duration<double>(0.0)});
  });
  Throws<std::invalid_argument>([] {
    (void)UseSharedMorph({.motion = TweenSpec{.duration = -0.1}});
  });
  Throws<std::invalid_argument>([] {
    (void)UseSharedMorph({.motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });
  Throws<std::invalid_argument>([] { (void)MorphElement(""); });
  Throws<std::invalid_argument>([] { (void)MorphBounds(""); });
  Throws<std::logic_error>([] { (void)MorphScope(SharedMorphHandle{}); });

  // HoverShadow validation.
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.rest_blur = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.max_pointer_offset = {-1.0F, 2.0F}}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.blur_radius = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.border_glow_width = -0.5F}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.border_glow_diameter = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.fade = -0.1}); });
  Throws<std::invalid_argument>([] { (void)HoverShadow(Text("x"), {.corner_radii = CornerRadii{-1.0F}}); });

  // MarqueeSelect validation.
  Throws<std::invalid_argument>([] { (void)MarqueeSelectArea(Text("x"), {.border_width = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)MarqueeSelectArea(Text("x"), {.minimum_distance = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)MarqueeSelectArea(Text("x"), {.corner_radii = CornerRadii{-1.0F}}); });

  // PredictiveBack validation.
  Throws<std::invalid_argument>(
      [] { (void)PredictiveBack(std::function<View(PredictiveBackSnapshot)>{}, [] {}); });
  Throws<std::invalid_argument>([] { (void)PredictiveBack(Text("x"), {}, {.maximum_offset = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)PredictiveBack(Text("x"), {}, {.minimum_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)PredictiveBack(Text("x"), {}, {.minimum_scale = 1.2F}); });

  // ScrollSpy validation.
  Throws<std::invalid_argument>([] { (void)ScrollSpyArea(Text("x"), {.activation_offset = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollSpyArea(Text("x"), {.activation_offset = std::nanf("")}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollSpyArea(Text("x"), {.scroll_motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollSpyArea(Text("x"), {.scroll_motion = SpringSpec{.stiffness = 0.0F}}); });
  Throws<std::invalid_argument>([] {
    (void)ScrollSpyArea(Text("x"), {.scroll_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });

  // StickyHeader validation.
  (void)StickyHeaderSection(Text("sec"), 0);
  (void)StickyHeaderArea(Text("ok"), [](StickyHeaderSnapshot) { return Text("h"); });
  (void)StickyHeaderList({});
  (void)StickyHeaderList({StickyHeaderGroup{.header = [](StickyHeaderSnapshot) { return Text("h"); }, .body = Text("b")}});
  Throws<std::invalid_argument>(
      [] { (void)StickyHeaderArea(Text("x"), std::function<View(StickyHeaderSnapshot)>{}); });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderList({StickyHeaderGroup{.header = {}, .body = Text("b")}});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderArea(Text("x"), [](StickyHeaderSnapshot) { return Text("h"); }, {.sticky_offset = -1.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderArea(Text("x"), [](StickyHeaderSnapshot) { return Text("h"); },
                           {.sticky_offset = std::nanf("")});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderList({}, {.section_spacing = -2.0F});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderArea(Text("x"), [](StickyHeaderSnapshot) { return Text("h"); },
                           {.scroll_motion = TweenSpec{.duration = -0.1}});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderArea(Text("x"), [](StickyHeaderSnapshot) { return Text("h"); },
                           {.scroll_motion = SpringSpec{.stiffness = 0.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)StickyHeaderArea(Text("x"), [](StickyHeaderSnapshot) { return Text("h"); },
                           {.scroll_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });

  // ScrollParallax validation.
  (void)ScrollParallaxArea(Text("sc"));
  (void)ScrollParallax(Text("p"));
  (void)ScrollParallax([](ScrollParallaxSnapshot) { return Text("p"); },
                       {.speed = -0.5F, .maximum_offset = 0.0F, .edge_opacity = 0.5F});
  Throws<std::invalid_argument>([] { (void)ScrollParallax(std::function<View(ScrollParallaxSnapshot)>{}); });
  Throws<std::invalid_argument>([] { (void)ScrollParallax(Text("x"), {.speed = std::nanf("")}); });
  Throws<std::invalid_argument>([] { (void)ScrollParallax(Text("x"), {.maximum_offset = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollParallax(Text("x"), {.center_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollParallax(Text("x"), {.edge_opacity = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)ScrollParallax(Text("x"), {.edge_opacity = 1.2F}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollParallax(Text("x"), {.origin = {std::numeric_limits<float>::infinity(), 0.5F}}); });

  // ScrollReveal validation.
  (void)ScrollRevealArea(Text("sc"));
  (void)ScrollReveal(Text("r"));
  (void)ScrollReveal([](ScrollRevealSnapshot) { return Text("r"); },
                     {.threshold = 0.0F, .once = false, .delay = 0.1, .initial_opacity = 0.2F});
  Throws<std::invalid_argument>([] { (void)ScrollRevealArea(Text("x"), {.viewport_margin = std::nanf("")}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(std::function<View(ScrollRevealSnapshot)>{}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.threshold = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.threshold = 1.2F}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.delay = -0.1}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.initial_opacity = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.initial_opacity = 1.5F}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.initial_offset = {std::nanf(""), 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.initial_scale = 0.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollReveal(Text("x"), {.origin = {std::numeric_limits<float>::infinity(), 0.0F}}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>([] { (void)ScrollReveal(Text("x"), {.motion = SpringSpec{.stiffness = 0.0F}}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollReveal(Text("x"), {.motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}}); });

  // SnapDrag validation.
  Throws<std::invalid_argument>([] { (void)SnapDrag(std::function<View(SnapDragSnapshot)>{}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.margin = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.projection_time = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.maximum_velocity = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.drag_scale = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.mode = SnapMode::Custom, .anchors = {}}); });
  Throws<std::invalid_argument>(
      [] { (void)SnapDrag(Text("x"), {.mode = SnapMode::Custom, .anchors = {{1.5F, 0.5F}}}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.snap_motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>([] { (void)SnapDrag(Text("x"), {.snap_motion = SpringSpec{.stiffness = 0.0F}}); });
  Throws<std::invalid_argument>(
      [] { (void)SnapDrag(Text("x"), {.snap_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}}); });

  // SwipeDeck validation.
  Throws<std::invalid_argument>([] { (void)SwipeDeck(2, std::function<View(SwipeDeckCardContext)>{}); });
  Throws<std::invalid_argument>([] { (void)SwipeDeck(std::vector<View>{View{}}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.visible_cards = 0}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.allowed_directions = SwipeDirectionMask::None}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.swipe_threshold = 0.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.minimum_velocity = -1.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.exit_distance = 0.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.maximum_rotation = -1.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.depth_scale_step = 0.5F}); });
  Throws<std::invalid_argument>(
      [] { (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.depth_opacity_step = 1.2F}); });
  Throws<std::invalid_argument>([] {
    (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.return_motion = SpringSpec{.stiffness = 0.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeDeck(std::vector<View>{Text("a")}, {}, {.exit_motion = TweenSpec{.duration = -0.1}});
  });
  Throws<std::invalid_argument>([] {
    (void)SwipeDeck(std::vector<View>{Text("a")}, {},
                         {.rewind_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });

  // ScrollToTop validation.
  (void)ScrollToTop(Text("x"), Text("top"));
  (void)ScrollToTop(Text("x"), [](ScrollToTopContext) { return Text("top"); });
  (void)ScrollToTopTrigger(Text("top"));
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), std::function<View(ScrollToTopContext)>{}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.threshold = -1.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), Text("top"), {.threshold = std::numeric_limits<float>::quiet_NaN()}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.target_offset = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.scroll_up_delta = -0.5F}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.margin = {.top = -1.0F}}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.margin = {.right = -1.0F}}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.margin = {.bottom = -1.0F}}); });
  Throws<std::invalid_argument>([] { (void)ScrollToTop(Text("x"), Text("top"), {.margin = {.left = -1.0F}}); });
  Throws<std::invalid_argument>([] {
    (void)ScrollToTop(Text("x"), Text("top"), {.margin = {.top = std::numeric_limits<float>::quiet_NaN()}});
  });
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), Text("top"), {.scroll_motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>([] {
    (void)ScrollToTop(Text("x"), Text("top"), {.scroll_motion = SpringSpec{.stiffness = 0.0F, .damping_ratio = 1.0F}});
  });
  Throws<std::invalid_argument>([] {
    (void)ScrollToTop(Text("x"), Text("top"),
                      {.scroll_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), Text("top"), {.reveal_motion = {.initial_opacity = -0.1F}}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), Text("top"), {.reveal_motion = {.initial_scale = 0.0F}}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrollToTop(Text("x"), Text("top"), {.reveal_motion = {.delay = -0.1}}); });

  // CompareSlider validation.
  (void)CompareSlider(Text("b"), Text("a"));
  (void)CompareSlider(Text("b"), Text("a"), [](CompareSliderSnapshot) { return Text("h"); });
  Throws<std::invalid_argument>([] { (void)UseCompareSliderController(-0.1F); });
  Throws<std::invalid_argument>([] { (void)UseCompareSliderController(1.2F); });
  Throws<std::invalid_argument>(
      [] { (void)UseCompareSliderController(std::numeric_limits<float>::quiet_NaN()); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), std::function<View(CompareSliderSnapshot)>{}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.min_progress = -0.1F}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.max_progress = 1.2F}); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), {.initial_progress = 0.5F, .min_progress = 0.6F, .max_progress = 0.4F}); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), {.initial_progress = 0.1F, .min_progress = 0.2F, .max_progress = 0.8F}); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), {.handle_hit_extent = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.keyboard_step = 0.0F}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.keyboard_step = 1.1F}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.divider_thickness = -1.0F}); });
  Throws<std::invalid_argument>([] { (void)CompareSlider(Text("b"), Text("a"), {.handle_size = -1.0F}); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), {.motion = TweenSpec{.duration = -0.1}}); });
  Throws<std::invalid_argument>(
      [] { (void)CompareSlider(Text("b"), Text("a"), {.motion = SpringSpec{.stiffness = 0.0F}}); });
  Throws<std::invalid_argument>([] {
    (void)CompareSlider(Text("b"), Text("a"), {.motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.0F}});
  });

  // ScrubValue validation.
  (void)ScrubValue(Text("s"));
  (void)ScrubValue([](ScrubValueSnapshot) { return Text("s"); });
  Throws<std::invalid_argument>([] { (void)UseScrubValueController(std::numeric_limits<double>::quiet_NaN()); });
  Throws<std::invalid_argument>([] { (void)UseScrubValueController(std::numeric_limits<double>::infinity()); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(std::function<View(ScrubValueSnapshot)>{}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.min_value = std::numeric_limits<double>::quiet_NaN()}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.min_value = std::numeric_limits<double>::infinity()}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.max_value = -std::numeric_limits<double>::infinity()}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.initial_value = 5.0, .min_value = 10.0, .max_value = 0.0}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.initial_value = -1.0, .min_value = 0.0, .max_value = 10.0}); });
  Throws<std::invalid_argument>(
      [] { (void)ScrubValue(Text("s"), {.initial_value = 5.0, .default_value = 11.0, .min_value = 0.0, .max_value = 10.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.sensitivity = 0.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.fine_multiplier = 0.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.coarse_multiplier = -1.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.step = 0.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.keyboard_step = 0.0}); });
  Throws<std::invalid_argument>([] { (void)ScrubValue(Text("s"), {.drag_slop = -1.0F}); });
}
