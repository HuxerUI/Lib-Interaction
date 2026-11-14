#include "demos.h"

#include <algorithm>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View LayoutSection(const DemoState& state, const DemoContext& context) {
  const auto card_label = Foreground(context.colors.on_primary_container);
  const auto grid_tile = [&context](const char* label) {
    return Stack { Text(label, TextRole::Label)
                       .With(Foreground(context.colors.on_primary_container)) }
        .With(Frame{.width = 96.0F, .height = 64.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(12.0F));
  };

  return Column {
    SectionLabel(context, "Layout"),
    Flow {
      Column {
        ReorderableGrid(
            {grid_tile("A"), grid_tile("B"), grid_tile("C"), grid_tile("D"), grid_tile("E"), grid_tile("F")},
            {.cell_width = 96.0F, .cell_height = 64.0F, .spacing = 12.0F},
            [last = state.grid_last](std::size_t from, std::size_t to) {
              last = "grid " + std::to_string(from) + " → " + std::to_string(to);
            })
            .With(Frame{.width = 320.0F})
            .Key("grid-demo"),
        Text("last: " + state.grid_last.Get(), TextRole::Label)
            .With(Foreground(context.colors.on_surface_variant))
            .Key("grid-last"),
      }.With(Spacing(8.0F)),
      Column {
        Row {
          Stack { Text("Sidebar", TextRole::Label).With(card_label) }.With(
              Frame{.width = state.pane_width.Get(), .height = 140.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.surface_container_low}, CornerRadius(12.0F)),
          ResizeHandle(Axis::Horizontal, [width = state.pane_width](float delta) {
            width = std::clamp(width.Get() + delta, 96.0F, 280.0F);
          }).Key("resize-demo"),
          Stack { Text("Fill", TextRole::Label).With(card_label) }.With(
              Grow{}, Frame{.height = 140.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.surface_container_low}, CornerRadius(12.0F)),
        }.With(CrossAlign(CrossAxisAlignment::Stretch), Frame{.width = 360.0F, .height = 140.0F}),
        Text("pane: " + std::to_string(static_cast<int>(state.pane_width.Get())) + " px", TextRole::Label)
            .With(Foreground(context.colors.on_surface_variant))
            .Key("resize-width"),
      }.With(Spacing(8.0F)),
    }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
