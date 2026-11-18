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

/// Compiles the umbrella header, links one symbol from every family, and exercises pipeable adaptors.
int main() {
  // Default-constructed controllers and triggers must be safe and equal outside an Application runtime.
  if(!(PopTrigger{} == PopTrigger{})) return 1;
  if(!(ShakeTrigger{} == ShakeTrigger{})) return 1;
  if(!(SwipeController{} == SwipeController{})) return 1;
  if(!(PanZoomController{} == PanZoomController{})) return 1;
  if(!(HoverCardController{} == HoverCardController{})) return 1;

  View probe = Text("probe")
                   .With(Ripple{}, Spotlight{}, HoverHighlight{})
               | Pressable([] {})
               | DoubleClickable([](const MultiTapEvent&) {})
               | ContextMenuArea([](Point) {})
               | KeyboardShortcut({.key = Key::K}, [] {});

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

  return probe ? 0 : 1;
}
