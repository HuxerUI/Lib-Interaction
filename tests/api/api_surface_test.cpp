#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include <concepts>
#include <utility>

using namespace huxerui;
using namespace huxerui::interaction;

// Static contract checks: all retained modifiers, styles, triggers, and controllers must be equality-comparable
// so HuxerUI can short-circuit unchanged modifier and environment updates.
static_assert(std::equality_comparable<Ripple>);
static_assert(std::equality_comparable<HoldProgressRing>);
static_assert(std::equality_comparable<HoverHighlight>);
static_assert(std::equality_comparable<Spotlight>);
static_assert(std::equality_comparable<Shimmer>);
static_assert(std::equality_comparable<Fireworks>);
static_assert(std::equality_comparable<ScrollProgress>);
static_assert(std::equality_comparable<ScrollEdgeFade>);
static_assert(std::equality_comparable<HoverCardStyle>);
static_assert(std::equality_comparable<PopTrigger>);
static_assert(std::equality_comparable<ShakeTrigger>);
static_assert(std::equality_comparable<SwipeController>);
static_assert(std::equality_comparable<PanZoomController>);
static_assert(std::equality_comparable<HoverCardController>);
static_assert(std::equality_comparable<NestedScrollConnection>);
static_assert(std::equality_comparable<ScrollHideStyle>);
static_assert(std::equality_comparable<ScrollHideController>);
static_assert(std::equality_comparable<ExpandableStyle>);
static_assert(std::equality_comparable<AppearStyle>);
static_assert(std::equality_comparable<MagneticStyle>);
static_assert(std::equality_comparable<HoverShadowStyle>);
static_assert(std::equality_comparable<MarqueeSelectStyle>);
static_assert(std::equality_comparable<MarqueeSelectController>);
static_assert(std::equality_comparable<ShortcutMapStyle>);
static_assert(std::equality_comparable<RovingFocusStyle>);
static_assert(std::equality_comparable<RovingFocusItemStyle>);
static_assert(std::equality_comparable<RovingFocusController>);
static_assert(std::equality_comparable<PopoverStyle>);
static_assert(std::equality_comparable<PopoverController>);
static_assert(std::equality_comparable<CopyableStyle>);
static_assert(std::equality_comparable<SharedMorphStyle>);
static_assert(std::equality_comparable<SharedMorphHandle>);
static_assert(std::equality_comparable<BackHandlerStyle>);
static_assert(std::equality_comparable<PredictiveBackSnapshot>);
static_assert(std::equality_comparable<PredictiveBackStyle>);
static_assert(std::equality_comparable<PredictiveBackController>);

/// Compiles the umbrella header, links one symbol from every family, and exercises pipeable adaptors.
int main() {
  // Default-constructed controllers and triggers must be safe and equal outside an Application runtime.
  if(!(PopTrigger{} == PopTrigger{})) return 1;
  if(!(ShakeTrigger{} == ShakeTrigger{})) return 1;
  if(!(SwipeController{} == SwipeController{})) return 1;
  if(!(PanZoomController{} == PanZoomController{})) return 1;
  if(!(HoverCardController{} == HoverCardController{})) return 1;
  if(!(ScrollHideController{} == ScrollHideController{}) || static_cast<bool>(ScrollHideController{})) return 1;
  if(!(NestedScrollConnection{} == NestedScrollConnection{})) return 1;
  if(!(RovingFocusController{} == RovingFocusController{}) || static_cast<bool>(RovingFocusController{})) return 1;
  if(!(PopoverController{} == PopoverController{}) || static_cast<bool>(PopoverController{})) return 1;
  if(!(SharedMorphHandle{} == SharedMorphHandle{}) || static_cast<bool>(SharedMorphHandle{})) return 1;
  if(!(MarqueeSelectController{} == MarqueeSelectController{}) || static_cast<bool>(MarqueeSelectController{})) return 1;
  if(!(PredictiveBackController{} == PredictiveBackController{}) || static_cast<bool>(PredictiveBackController{})) {
    return 1;
  }

  View probe = Text("probe")
                   .With(Ripple{}, Spotlight{}, HoverHighlight{}, NestedScrollConnection{})
               | Pressable([] {})
               | DoubleClickable([](const MultiTapEvent&) {})
               | ContextMenuArea([](Point) {})
               | KeyboardShortcut({.key = Key::K}, [] {})
               | ShortcutMap({{.chord = {.key = Key::S}, .on_trigger = [] {}}})
               | BackHandler([] {})
               | PredictiveBack([] {})
               | RovingFocusItem(0)
               | RovingFocusGroup()
               | Copyable("probe")
               | Popover([](PopupContext) { return Text("pop"); })
               | MorphElement("item")
               | MorphBounds("bounds")
               | MarqueeSelectItem(0)
               | MarqueeSelectArea()
               | Expandable(true)
               | Appear()
               | Magnetic()
               | HoverShadow()
               | DropZone<int>([](int value) { return value > 0; });

  (void)InteractiveSurface([](const InteractionSnapshot&) { return Text("surface"); });
  (void)Hoverable([](bool) { return Text("hoverable"); });
  (void)HoverLift(Text("hover"));
  (void)HoldProgressRing{};
  (void)Pulsing(Text("pulse"));
  (void)Shimmer{};
  (void)Fireworks{};
  (void)RollingNumber(42);
  (void)DragSpring(Text("drag"));
  (void)InertialDrag(Text("inertia"));
  (void)PanZoom(Text("panzoom"));
  (void)PinchZoom(Text("pinch"));
  (void)SwipeActions(Text("row"), [](SwipeActionContext) { return Text("act"); });
  (void)Dismissible(Text("dismiss"), [] {});
  (void)PressDragHandle(Text("grip"));
  (void)ResizeHandle(Axis::Horizontal, [](float) {});
  (void)Reorderable({Text("a"), Text("b")});
  (void)ReorderableGrid({Text("a")});
  (void)ReorderHandle(Text("grip"));
  (void)Draggable(Text("src"), 7);
  (void)DropZone<int>(Text("dst"));
  (void)FileDropZone(Text("files"), {});
  (void)HoverCard(Text("hover"), [](PopupContext) { return Text("card"); });
  (void)ScrollProgress(ScrollController{});
  (void)ScrollEdgeFade(ScrollController{});
  (void)SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Activate)};
  (void)PredictiveBack([](PredictiveBackSnapshot) { return Text("pb"); }, [] {});

  return probe ? 0 : 1;
}
