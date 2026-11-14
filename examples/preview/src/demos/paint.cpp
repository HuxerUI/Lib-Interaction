#include "demos.h"

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View PaintSection(const DemoState&, const DemoContext& context) {
  const auto card_label = Foreground(context.colors.on_primary_container);
  const auto card = Frame{.width = 240.0F, .height = 96.0F};

  return Column {
    SectionLabel(context, "Paint and pulse"),
    Flow {
      Pulsing(
          Stack { Text("Live", TextRole::Label).With(card_label) }.With(
              Frame{.width = 96.0F, .height = 40.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(20.0F)),
          {.scale_amplitude = 0.05F, .duration = 0.9})
          .Key("pulse-demo"),
      Stack { Text("Ripple card", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container},
              Ripple{RippleStyle{.color = Color::Rgb(255, 255, 255, 0.22F), .corner_radii = 14.0F}},
              CornerRadius(14.0F))
          .Key("ripple-demo"),
      Stack { Text("Spotlight", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(14.0F),
              Spotlight{SpotlightStyle{.corner_radii = 14.0F}})
          .Key("spotlight-demo"),
      Stack { Text("Hover row", TextRole::Label).With(card_label) }.With(
              card,
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(14.0F),
              HoverHighlight{HoverHighlightStyle{.corner_radii = 14.0F}}),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
