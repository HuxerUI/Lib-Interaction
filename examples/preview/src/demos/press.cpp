#include "demos.h"

#include <utility>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View PressSection(const DemoState& state, const DemoContext& context) {
  return Scope([state, context]() -> View {
    auto shake = UseShake();
    const auto tile = Frame{.width = 240.0F, .height = 120.0F};
    const auto card_label = Foreground(context.colors.on_primary_container);

    View press_tile = Stack {
      Column {
        Text("Press me", TextRole::Label).With(card_label),
        Text("Spring scale on hover and press", TextRole::Label).With(card_label),
      }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
    }.With(tile, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
           Background{context.colors.primary_container}, CornerRadius(14.0F));

    View shake_tile = Stack { Text("Shake me", TextRole::Label).With(card_label) }.With(
        Frame{.width = 240.0F, .height = 72.0F},
        Align(HorizontalAlignment::Center, VerticalAlignment::Center),
        Background{context.colors.primary_container}, CornerRadius(14.0F));

    return Column {
      SectionLabel(context, "Press and hover"),
      Flow {
        (std::move(press_tile)
         | Pressable([clicks = state.clicks] { clicks += 1; },
                     {.animation = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.7F}}))
            .Key("pressable-demo"),
        HoverLift(
            Stack {
              Column {
                Text("Hover me", TextRole::Label).With(card_label),
                Text("Lift on hover", TextRole::Label).With(card_label),
              }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
            }.With(tile, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                   Background{context.colors.primary_container}, CornerRadius(14.0F)),
            {.lift = {0.0F, -6.0F}, .animation = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut}})
            .Key("lift-demo"),
        (std::move(shake_tile) | Shaken(shake))
            .OnClick([shake] { shake.Trigger(); })
            .With(PointerCursor{PointerCursorKind::Hand})
            .Key("shake-demo"),
        (Stack {
          Column {
            Text("Magnetic", TextRole::Label).With(card_label),
            Text("Follows pointer within bounds", TextRole::Label).With(card_label),
          }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
        }
             .With(tile, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                   Background{context.colors.primary_container}, CornerRadius(14.0F))
         | Magnetic({.maximum_offset = {10.0F, 8.0F}, .hovered_scale = 1.04F, .cursor = PointerCursorKind::Hand}))
            .Key("magnetic-demo"),
        Copyable(
            [context, card_label](bool copied) {
              return Stack {
                Column {
                  Text(copied ? "Copied!" : "Copy snippet", TextRole::Label).With(card_label),
                  Text("HuxerUI::Interaction", TextRole::Label).With(card_label),
                }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
              }.With(Frame{.width = 240.0F, .height = 72.0F},
                     Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                     Background{copied ? context.colors.secondary_container : context.colors.primary_container},
                     CornerRadius(14.0F));
            },
            "HuxerUI::Interaction")
            .Key("copyable-demo"),
      }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
  });
}

} // namespace interaction_demo
