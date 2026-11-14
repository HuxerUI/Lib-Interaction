#include "demos.h"

namespace interaction_demo {
using namespace huxerui;
using namespace huxerui::interaction;

View DragSection(const DemoState&, const DemoContext& context) {
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
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Stretch));
}

} // namespace interaction_demo
