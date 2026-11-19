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
    Scope([context]() -> View {
      auto snap = UseSnapDragController(3);
      auto snap_mode = UseState(SnapMode::Corners);
      const bool corners = snap_mode.Get() == SnapMode::Corners;
      const float stage_w = context.expanded ? 560.0F : (context.compact ? 260.0F : 360.0F);
      static constexpr const char* kCornerNames[] = {"TL", "TR", "BL", "BR"};
      const auto anchor_label = [corners](std::size_t idx) -> std::string {
        if(corners) {
          return idx < 4 ? kCornerNames[idx] : "?";
        }
        return idx == 0 ? "Left" : "Right";
      };

      std::vector<View> quick_btns;
      const std::size_t btn_count = corners ? 4U : 2U;
      for(std::size_t i = 0; i < btn_count; ++i) {
        const bool is_cur = snap.ActiveAnchor() == i;
        quick_btns.push_back(
            Stack {
              Text(anchor_label(i), TextRole::Label)
                  .With(Foreground(is_cur ? context.colors.on_primary : context.colors.on_surface)),
            }
                .With(Padding(EdgeInsets::Symmetric(8.0F, 5.0F)),
                      Background{is_cur ? context.colors.primary : context.colors.surface_container_high},
                      CornerRadius(8.0F), PointerCursor(PointerCursorKind::Hand))
                .OnClick([snap, i] { snap.SnapToAnchor(i); }));
      }
      quick_btns.push_back(
          Stack {
            Text(corners ? "Corners" : "Edges", TextRole::Label).With(Foreground(context.colors.on_surface)),
          }
              .With(Padding(EdgeInsets::Symmetric(8.0F, 5.0F)),
                    Background{context.colors.surface_container_high},
                    Border{.color = context.colors.outline, .width = 1.0F}, CornerRadius(8.0F),
                    PointerCursor(PointerCursorKind::Hand))
              .OnClick([snap_mode] {
                snap_mode = snap_mode.Get() == SnapMode::Corners ? SnapMode::HorizontalEdges : SnapMode::Corners;
              }));

      return Column {
        Flow {
          Text("SnapDrag (drag & flick PiP):", TextRole::Label)
              .With(Foreground(context.colors.on_surface_variant)),
          Row(std::move(quick_btns)).With(Spacing(6.0F)),
        }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Center)),
        SnapDrag(
            [context, anchor_label](SnapDragSnapshot state) -> View {
              const Color fg = state.dragging ? context.colors.on_primary : context.colors.on_primary_container;
              const Color bg = state.dragging ? context.colors.primary : context.colors.primary_container;
              return Column {
                Text("PiP · " + anchor_label(state.active_anchor), TextRole::Label).With(Foreground(fg)),
                Text(state.dragging ? "Dragging" : "Drag me", TextRole::Body).With(Foreground(fg)),
              }
                  .With(Frame{.width = 104.0F, .height = 52.0F}, Padding(EdgeInsets::Symmetric(10.0F, 8.0F)),
                        Spacing(2.0F), Background{bg}, CornerRadius(12.0F),
                        Shadow{.color = Color::Rgb(0, 0, 0, 0.16F), .offset = {0.0F, 4.0F}, .blur_radius = 12.0F});
            },
            {.mode = snap_mode.Get(), .initial_anchor = 3, .margin = 10.0F, .controller = snap})
            .With(Frame{.width = stage_w, .height = 156.0F},
                  Background{context.colors.surface_container_low}, CornerRadius(14.0F)),
      }.With(Spacing(8.0F), CrossAlign(CrossAxisAlignment::Start));
    }).Key("snapdrag-demo"),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
