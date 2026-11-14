#include "demos.h"

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View GestureSection(const DemoState& state, const DemoContext& context) {
  const auto card_label = Foreground(context.colors.on_primary_container);
  const auto card = Frame{.width = 240.0F, .height = 96.0F};

  return Column {
    SectionLabel(context, "Gesture"),
    Flow {
      LongPressable(
          Stack { Text("Hold me", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(14.0F)),
          {.show_progress = true},
          [holds = state.holds] { holds += 1; })
          .Key("longpress-demo"),
      DoubleClickable(
          Stack { Text("Double-click me", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(14.0F)),
          {},
          [doubles = state.doubles, burst = state.burst](const MultiTapEvent& event) {
            doubles += 1;
            burst = FireworksBurst{.id = burst.Get().id + 1, .local_position = event.position};
          })
          .With(Fireworks{FireworksStyle{.corner_radii = 14.0F}, state.burst.Get()})
          .Key("doubleclick-demo"),
      Popped(
          Stack { Text("Pop", TextRole::Label).With(Foreground(context.colors.on_primary)) }.With(
              Frame{.width = 96.0F, .height = 40.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary}, CornerRadius(20.0F)),
          state.pop)
          .OnClick([pop = state.pop] { pop.Trigger(); })
          .With(PointerCursor{PointerCursorKind::Hand})
          .Key("pop-demo"),
      Stack { Text("Loading…", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(14.0F),
              Shimmer{ShimmerStyle{.corner_radii = 14.0F}})
          .Key("shimmer-demo"),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
