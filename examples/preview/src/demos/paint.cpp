#include "demos.h"

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View PaintSection(const DemoContext& context) {
  const auto card_label = Foreground(context.colors.on_primary_container);
  const auto card = Frame{.width = 240.0F, .height = 96.0F};

  Color aura = context.colors.primary;
  aura.alpha = context.dark ? 0.42F : 0.30F;
  Color glow_rim = context.colors.primary;
  glow_rim.alpha = context.dark ? 0.85F : 0.65F;
  Color soft_rim = context.colors.primary;
  soft_rim.alpha = context.dark ? 0.72F : 0.48F;

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
      HoverShadow(
          Stack {
            Column {
              Text("Hover glow", TextRole::Label).With(card_label),
              Text("Perimeter aura follows cursor", TextRole::Label).With(card_label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
          }.With(card, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                 Background{context.colors.primary_container}, CornerRadius(14.0F)),
          {.color = aura,
           .hover_offset = {},
           .max_pointer_offset = {6.0F, 5.0F},
           .blur_radius = 20.0F,
           .spread = 1.0F,
           .border_glow_color = glow_rim,
           .border_glow_width = 1.75F,
           .border_glow_diameter = 180.0F,
           .corner_radii = CornerRadii{14.0F}})
          .Key("hovershadow-glow-demo"),
      HoverShadow(
          Stack {
            Column {
              Text("Hover shadow", TextRole::Label).With(card_label),
              Text("Soft elevation with edge glint", TextRole::Label).With(card_label),
            }.With(Spacing(4.0F), CrossAlign(CrossAxisAlignment::Center)),
          }.With(card, Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                 Background{context.colors.primary_container}, CornerRadius(14.0F)),
          {.color = Color::Rgb(0, 0, 0, context.dark ? 0.44F : 0.18F),
           .hover_offset = {0.0F, 6.0F},
           .max_pointer_offset = {3.0F, 2.0F},
           .blur_radius = 20.0F,
           .spread = 0.0F,
           .border_glow_color = soft_rim,
           .border_glow_width = 1.25F,
           .border_glow_diameter = 150.0F,
           .corner_radii = CornerRadii{14.0F}})
          .Key("hovershadow-shadow-demo"),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
