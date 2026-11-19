#include "demos.h"

#include <string>
#include <vector>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View DragSection(const DemoContext& context) {
  return Column {
    SectionLabel(context, "Drag"),
    DragSpring(
        Stack { Text("Drag me", TextRole::Label).With(Foreground(context.colors.on_primary)) }.With(
            Frame{.width = 140.0F, .height = 48.0F},
            Align(HorizontalAlignment::Center, VerticalAlignment::Center),
            Background{context.colors.primary}, CornerRadius(12.0F)))
        .With(Frame{.width = context.expanded ? 560.0F : (context.compact ? 240.0F : 400.0F), .height = 72.0F},
              Background{context.colors.surface_container_low}, CornerRadius(14.0F))
        .Key("dragspring-demo"),
    PinchZoom(
        Stack { Text("Pinch me", TextRole::Label).With(Foreground(context.colors.on_primary_container)) }.With(
            Frame{.width = 240.0F, .height = 72.0F},
            Align(HorizontalAlignment::Center, VerticalAlignment::Center),
            Background{context.colors.primary_container}, CornerRadius(14.0F)))
        .Key("pinch-demo"),
    Scope([context]() -> View {
      auto marquee = UseMarqueeSelectController();
      const std::vector<std::size_t> selected = marquee.Selection();
      std::vector<View> tiles;
      for(std::size_t i = 0; i < 6; ++i) {
        const bool is_selected = marquee.IsSelected(i);
        tiles.push_back(
            Stack {
              Text("Tile " + std::to_string(i + 1), TextRole::Label)
                  .With(Foreground(is_selected ? context.colors.on_primary : context.colors.on_primary_container)),
            }
                .With(Frame{.width = 96.0F, .height = 56.0F},
                      Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                      Background{is_selected ? context.colors.primary : context.colors.primary_container},
                      CornerRadius(10.0F))
                .OnClick([marquee, i] { marquee.Toggle(i); })
            | MarqueeSelectItem(i));
      }

      Color box_fill = context.colors.primary;
      box_fill.alpha = 0.16F;
      Color box_border = context.colors.primary;
      box_border.alpha = 0.85F;

      return Column {
        Row {
          Text("Box select (drag across tiles): " + std::to_string(selected.size()) + " selected", TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant)),
          Button("Clear").OnClick([marquee] { marquee.Clear(); }),
        }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
        (Flow(std::move(tiles))
             .With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Center), Padding(12.0F),
                   Frame{.width = context.expanded ? 560.0F : (context.compact ? 260.0F : 360.0F)},
                   Background{context.colors.surface_container_low}, CornerRadius(14.0F))
         | MarqueeSelectArea({
               .fill_color = box_fill,
               .border_color = box_border,
               .controller = marquee,
           })),
      }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Start));
    }).Key("marquee-demo"),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
