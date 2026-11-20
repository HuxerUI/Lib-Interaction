#include <huxerui/interaction.h>
#include <huxerui/huxerui.h>

#include <concepts>
#include <cstddef>
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
static_assert(std::equality_comparable<ScrollJumpBehavior>);
static_assert(std::equality_comparable<ScrollSpyStyle>);
static_assert(std::equality_comparable<ScrollSpyController>);
static_assert(std::equality_comparable<StickyHeaderSnapshot>);
static_assert(std::equality_comparable<StickyHeaderStyle>);
static_assert(std::equality_comparable<StickyHeaderController>);
static_assert(std::equality_comparable<ScrollParallaxSnapshot>);
static_assert(std::equality_comparable<ScrollParallaxAreaStyle>);
static_assert(std::equality_comparable<ScrollParallaxStyle>);
static_assert(std::equality_comparable<ScrollRevealSnapshot>);
static_assert(std::equality_comparable<ScrollRevealAreaStyle>);
static_assert(std::equality_comparable<ScrollRevealStyle>);
static_assert(std::equality_comparable<ScrollToTopContext>);
static_assert(std::equality_comparable<ScrollToTopController>);
static_assert(std::equality_comparable<ScrollToTopStyle>);
static_assert(std::equality_comparable<ExpandableStyle>);
static_assert(std::equality_comparable<AppearStyle>);
static_assert(std::equality_comparable<MagneticStyle>);
static_assert(std::equality_comparable<HoverShadowStyle>);
static_assert(std::equality_comparable<MarqueeSelectStyle>);
static_assert(std::equality_comparable<MarqueeSelectController>);
static_assert(std::equality_comparable<CompareSliderSnapshot>);
static_assert(std::equality_comparable<CompareSliderStyle>);
static_assert(std::equality_comparable<CompareSliderController>);
static_assert(std::equality_comparable<ScrubPrecision>);
static_assert(std::equality_comparable<ScrubValueSnapshot>);
static_assert(std::equality_comparable<ScrubValueStyle>);
static_assert(std::equality_comparable<ScrubValueController>);
static_assert(std::equality_comparable<RadialDialMode>);
static_assert(std::equality_comparable<RadialDialSnapshot>);
static_assert(std::equality_comparable<RadialDialStyle>);
static_assert(std::equality_comparable<RadialDialController>);
static_assert(std::equality_comparable<SnapMode>);
static_assert(std::equality_comparable<SnapDragSnapshot>);
static_assert(std::equality_comparable<SnapDragStyle>);
static_assert(std::equality_comparable<SnapDragController>);
static_assert(std::equality_comparable<SwipeDirection>);
static_assert(std::equality_comparable<SwipeDirectionMask>);
static_assert(std::equality_comparable<SwipeDeckCardContext>);
static_assert(std::equality_comparable<SwipeDeckStyle>);
static_assert(std::equality_comparable<SwipeDeckController>);
static_assert(std::equality_comparable<HoldRepeatSnapshot>);
static_assert(std::equality_comparable<HoldRepeatStyle>);
static_assert(std::equality_comparable<HoldRepeatController>);
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
  if(!(ScrollSpyController{} == ScrollSpyController{}) || static_cast<bool>(ScrollSpyController{})) return 1;
  if(!(ScrollSpyStyle{} == ScrollSpyStyle{})) return 1;
  if(!(StickyHeaderController{} == StickyHeaderController{}) || static_cast<bool>(StickyHeaderController{})) return 1;
  if(!(StickyHeaderStyle{} == StickyHeaderStyle{})) return 1;
  if(!(ScrollParallaxAreaStyle{} == ScrollParallaxAreaStyle{})) return 1;
  if(!(ScrollParallaxStyle{} == ScrollParallaxStyle{})) return 1;
  if(!(ScrollRevealAreaStyle{} == ScrollRevealAreaStyle{})) return 1;
  if(!(ScrollRevealStyle{} == ScrollRevealStyle{})) return 1;
  if(!(ScrollToTopContext{} == ScrollToTopContext{})) return 1;
  if(!(ScrollToTopStyle{} == ScrollToTopStyle{})) return 1;
  if(!(ScrollToTopController{} == ScrollToTopController{}) || static_cast<bool>(ScrollToTopController{})) return 1;
  if(!(CompareSliderSnapshot{} == CompareSliderSnapshot{})) return 1;
  if(!(CompareSliderStyle{} == CompareSliderStyle{})) return 1;
  if(!(CompareSliderController{} == CompareSliderController{}) || static_cast<bool>(CompareSliderController{}))
    return 1;
  if(!(ScrubValueSnapshot{} == ScrubValueSnapshot{})) return 1;
  if(!(ScrubValueStyle{} == ScrubValueStyle{})) return 1;
  if(!(ScrubValueController{} == ScrubValueController{}) || static_cast<bool>(ScrubValueController{})) return 1;
  if(!(RadialDialSnapshot{} == RadialDialSnapshot{})) return 1;
  if(!(RadialDialStyle{} == RadialDialStyle{})) return 1;
  if(!(RadialDialController{} == RadialDialController{}) || static_cast<bool>(RadialDialController{})) return 1;
  if(!(SnapDragController{} == SnapDragController{}) || static_cast<bool>(SnapDragController{})) return 1;
  if(!(SnapDragStyle{} == SnapDragStyle{})) return 1;
  if(!(SwipeDeckController{} == SwipeDeckController{}) || static_cast<bool>(SwipeDeckController{})) return 1;
  if(!(SwipeDeckStyle{} == SwipeDeckStyle{})) return 1;
  if(!(HoldRepeatController{} == HoldRepeatController{}) || static_cast<bool>(HoldRepeatController{})) return 1;
  if(!(HoldRepeatStyle{} == HoldRepeatStyle{})) return 1;

  View probe = Text("probe")
                   .With(Ripple{}, Spotlight{}, HoverHighlight{}, NestedScrollConnection{})
               | Pressable([] {})
               | HoldRepeat([] {})
               | DoubleClickable([](const MultiTapEvent&) {})
               | ContextMenuArea([](Point) {})
               | KeyboardShortcut({.key = Key::K}, [] {})
               | ShortcutMap({{.chord = {.key = Key::S}, .on_trigger = [] {}}})
               | BackHandler([] {})
               | PredictiveBack([] {})
               | ScrollSpySection(0)
               | ScrollSpyArea()
               | StickyHeaderSection(0)
               | StickyHeaderArea([](StickyHeaderSnapshot) { return Text("hdr"); })
               | ScrollParallax()
               | ScrollParallaxArea()
               | ScrollReveal()
               | ScrollRevealArea()
               | ScrollToTop(Text("top"))
               | ScrollToTop([](ScrollToTopContext) { return Text("top"); })
               | ScrollToTopTrigger()
               | ScrubValue()
               | SnapDrag()
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
  (void)HoldRepeat(Text("hr"));
  (void)HoldRepeat([](HoldRepeatSnapshot) { return Text("hr"); }, [](std::size_t) {});
  (void)Hoverable([](bool) { return Text("hoverable"); });
  (void)HoverLift(Text("hover"));
  (void)HoldProgressRing{};
  (void)Pulsing(Text("pulse"));
  (void)Shimmer{};
  (void)Fireworks{};
  (void)RollingNumber(42);
  (void)DragSpring(Text("drag"));
  (void)InertialDrag(Text("inertia"));
  (void)CompareSlider(Text("before"), Text("after"));
  (void)CompareSlider(Text("before"), Text("after"), [](CompareSliderSnapshot) { return Text("h"); });
  (void)SnapDrag([](SnapDragSnapshot) { return Text("sd"); });
  (void)PanZoom(Text("panzoom"));
  (void)PinchZoom(Text("pinch"));
  (void)SwipeActions(Text("row"), [](SwipeActionContext) { return Text("act"); });
  (void)SwipeDeck(2, [](SwipeDeckCardContext) { return Text("card"); });
  (void)SwipeDeck(std::vector<View>{Text("c1"), Text("c2")});
  (void)Dismissible(Text("dismiss"), [] {});
  (void)PressDragHandle(Text("grip"));
  (void)ResizeHandle(Axis::Horizontal, [](float) {});
  (void)ScrubValue(Text("v"));
  (void)ScrubValue([](ScrubValueSnapshot) { return Text("sv"); });
  (void)RadialDial();
  (void)RadialDial([](RadialDialSnapshot) { return Text("rd"); });
  (void)Reorderable({Text("a"), Text("b")});
  (void)ReorderableGrid({Text("a")});
  (void)ReorderHandle(Text("grip"));
  (void)Draggable(Text("src"), 7);
  (void)DropZone<int>(Text("dst"));
  (void)FileDropZone(Text("files"), {});
  (void)HoverCard(Text("hover"), [](PopupContext) { return Text("card"); });
  (void)ScrollProgress(ScrollController{});
  (void)ScrollEdgeFade(ScrollController{});
  (void)ScrollSpyStyle{.jump_behavior = ScrollJumpBehavior::Instant};
  (void)ScrollSpyController{}.ScrollToSection(0, ScrollJumpBehavior::Smooth);
  (void)StickyHeaderList({});
  (void)StickyHeaderController{}.ScrollToSection(0, ScrollJumpBehavior::Smooth);
  (void)ScrollParallax([](ScrollParallaxSnapshot) { return Text("p"); });
  (void)ScrollReveal([](ScrollRevealSnapshot) { return Text("r"); });
  (void)ScrollToTop(Text("c"), [](ScrollToTopContext) { return Text("t"); });
  (void)ScrollToTop(Text("c"), Text("t"));
  (void)ScrollToTopTrigger(Text("t"));
  (void)SemanticActions{Semantics{}, SemanticActionMask(SemanticActionKind::Activate)};
  (void)PredictiveBack([](PredictiveBackSnapshot) { return Text("pb"); }, [] {});

  return probe ? 0 : 1;
}
