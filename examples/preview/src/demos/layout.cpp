#include "demos.h"

#include <algorithm>
#include <string>

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View LayoutSection(const DemoContext& context) {
  return Scope([context]() -> View {
    auto grid_last = UseState(std::string{"drop a cell"});
    auto pane_width = UseState(164.0F);
    auto details_open = UseState(false);
    auto morph_swapped = UseState(false);
    auto morph = UseSharedMorph();
    const auto card_label = Foreground(context.colors.on_primary_container);
    const auto grid_tile = [&context](const char* label) {
      return Stack { Text(label, TextRole::Label).With(Foreground(context.colors.on_primary_container)) }
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
              [grid_last](std::size_t from, std::size_t to) {
                grid_last = "grid " + std::to_string(from) + " → " + std::to_string(to);
              },
              {.cell_width = 96.0F, .cell_height = 64.0F, .spacing = 12.0F})
              .With(Frame{.width = 320.0F})
              .Key("grid-demo"),
          Text("last: " + grid_last.Get(), TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant))
              .Key("grid-last"),
        }.With(Spacing(8.0F)),
        Column {
          Row {
            Stack { Text("Sidebar", TextRole::Label).With(card_label) }.With(
                Frame{.width = pane_width.Get(), .height = 140.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.surface_container_low}, CornerRadius(12.0F)),
            ResizeHandle(Axis::Horizontal, [pane_width](float delta) {
              pane_width = std::clamp(pane_width.Get() + delta, 96.0F, 280.0F);
            }).Key("resize-demo"),
            Stack { Text("Fill", TextRole::Label).With(card_label) }.With(
                Grow{}, Frame{.height = 140.0F},
                Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                Background{context.colors.surface_container_low}, CornerRadius(12.0F)),
          }.With(CrossAlign(CrossAxisAlignment::Stretch), Frame{.width = 360.0F, .height = 140.0F}),
          Text("pane: " + std::to_string(static_cast<int>(pane_width.Get())) + " px", TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant))
              .Key("resize-width"),
        }.With(Spacing(8.0F)),
        Column {
          Row {
            Button(details_open.Get() ? "Collapse details" : "Expand details")
                .OnClick([details_open] { details_open = !details_open.Get(); })
                .Key("expandable-toggle"),
            (Stack { Text("Appear", TextRole::Label).With(card_label) }
                 .With(Padding(EdgeInsets::Symmetric(14.0F, 8.0F)),
                       Background{context.colors.primary_container}, CornerRadius(10.0F))
             | Appear())
                .Key("appear-demo"),
          }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
          (Stack {
            Text("Layout-driven accordion content clips smoothly while collapsing.", TextRole::Body)
                .With(Foreground(context.colors.on_surface)),
          }
               .With(Padding(12.0F), Frame{.width = 300.0F},
                     Background{context.colors.surface_container_low}, CornerRadius(12.0F))
           | Expandable(details_open.Get()))
              .Key("expandable-demo"),
          Row {
            Popover(
                Button("Open Popover"),
                [context](PopupContext ctx) {
                  return Column {
                    Text("Click-triggered anchored popover", TextRole::Body)
                        .With(Foreground(context.colors.on_surface)),
                    Button("Close").OnClick([ctx] { ctx.Dismiss(); }),
                  }
                      .With(Padding(12.0F), Spacing(8.0F), Background{context.colors.surface_container_high},
                            CornerRadius(10.0F));
                })
                .Key("popover-demo"),
            (Row {
              Button("Prev") | RovingFocusItem(0),
              Button("Current") | RovingFocusItem(1),
              Button("Next") | RovingFocusItem(2),
            }.With(Spacing(6.0F))
             | RovingFocusGroup({.orientation = RovingFocusOrientation::Horizontal}))
                .Key("roving-demo"),
          }.With(Spacing(12.0F), CrossAlign(CrossAxisAlignment::Center)),
          (Column {
            Row {
              morph_swapped.Get()
                  ? View(Stack{}.With(Frame{.width = 96.0F, .height = 28.0F}))
                  : (Stack { Text("Morph", TextRole::Label).With(card_label) }
                         .With(Frame{.width = 96.0F, .height = 28.0F},
                               Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                               Background{context.colors.primary_container}, CornerRadius(8.0F))
                     | MorphElement("preview-morph-pill")),
              Button("Swap Morph").OnClick([morph, morph_swapped] {
                morph.Run([morph_swapped] { morph_swapped = !morph_swapped.Get(); });
              }),
              morph_swapped.Get()
                  ? (Stack { Text("Morph", TextRole::Label).With(card_label) }
                         .With(Frame{.width = 96.0F, .height = 28.0F},
                               Align(HorizontalAlignment::Center, VerticalAlignment::Center),
                               Background{context.colors.primary_container}, CornerRadius(8.0F))
                     | MorphElement("preview-morph-pill"))
                  : View(Stack{}.With(Frame{.width = 96.0F, .height = 28.0F})),
            }.With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Center)),
          }.With(Padding(8.0F), Background{context.colors.surface_container_low}, CornerRadius(12.0F))
           | MorphScope(morph))
              .Key("morph-demo"),
        }.With(Spacing(10.0F), CrossAlign(CrossAxisAlignment::Start)),
      }.With(Spacing(24.0F), CrossAlign(CrossAxisAlignment::Start)),
    }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
  });
}

} // namespace interaction_demo
