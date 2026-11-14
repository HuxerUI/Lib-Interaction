#include "demos.h"

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View ReorderSection(const DemoState& state, const DemoContext& context) {
  const auto tile = [&context](const char* label) {
    return Stack { Text(label, TextRole::Label)
                       .With(Foreground(context.colors.on_primary_container)) }
        .With(Frame{.width = 240.0F, .height = 48.0F},
              Align(HorizontalAlignment::Center, VerticalAlignment::Center),
              Background{context.colors.primary_container}, CornerRadius(12.0F));
  };

  return Column {
    SectionLabel(context, "Reorder"),
    Flow {
      Reorderable(
          {tile("1 · Drag me"), tile("2 · over a slot"), tile("3 · and drop")},
          {},
          [last = state.last_reorder](std::size_t from, std::size_t to) {
            last = "moved slot " + std::to_string(from) + " → " + std::to_string(to);
          })
          .With(Frame{.width = 240.0F})
          .Key("reorder-demo"),
      Text("last: " + state.last_reorder.Get(), TextRole::Label)
          .With(Foreground(context.colors.on_surface_variant)),
    }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
