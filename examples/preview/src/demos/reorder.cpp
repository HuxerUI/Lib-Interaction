#include "demos.h"

#include <string>
#include <utility>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View ReorderSection(const DemoContext& context) {
  return Scope([context]() -> View {
    auto last_reorder = UseState(std::string{"drop to reorder"});
    auto grip_reorder = UseState(std::string{"drag grip to reorder"});

    const auto tile = [&context](const char* label) {
      return Stack { Text(label, TextRole::Label).With(Foreground(context.colors.on_primary_container)) }
          .With(Frame{.width = 240.0F, .height = 48.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.primary_container}, CornerRadius(12.0F));
    };

    const auto grip_row = [&context](const char* label) {
      View grip = Stack { Text("⋮⋮", TextRole::Label).With(Foreground(context.colors.on_primary)) }
                      .With(Frame{.width = 36.0F, .height = 48.0F},
                            Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                            Background{context.colors.primary}, CornerRadius(10.0F));
      return Row {
        std::move(grip) | ReorderHandle(),
        Text(label, TextRole::Label)
            .With(Grow{}, Foreground(context.colors.on_surface)),
      }.With(Frame{.width = 260.0F, .height = 48.0F}, Padding(6.0F), Spacing(10.0F),
             CrossAlign(CrossAxisAlignment::Center),
             Background{context.colors.surface_container_low}, CornerRadius(12.0F));
    };

    return Column {
      SectionLabel(context, "Reorder"),
      Flow {
        Column {
          Reorderable(
              {tile("1 · Drag me"), tile("2 · over a slot"), tile("3 · and drop")},
              [last_reorder](std::size_t from, std::size_t to) {
                last_reorder = "moved slot " + std::to_string(from) + " → " + std::to_string(to);
              })
              .With(Frame{.width = 240.0F})
              .Key("reorder-demo"),
          Text("last: " + last_reorder.Get(), TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant)),
        }.With(Spacing(8.0F)),
        Column {
          Reorderable(
              {grip_row("A · Grip-only row"), grip_row("B · Content stays free"), grip_row("C · Drop slot")},
              [grip_reorder](std::size_t from, std::size_t to) {
                grip_reorder = "grip moved " + std::to_string(from) + " → " + std::to_string(to);
              },
              {.handle_only = true})
              .With(Frame{.width = 260.0F})
              .Key("reorder-handle-demo"),
          Text(grip_reorder.Get(), TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant)),
        }.With(Spacing(8.0F)),
      }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
  });
}

} // namespace interaction_demo
