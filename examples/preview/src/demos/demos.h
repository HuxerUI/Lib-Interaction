#pragma once

#include <huxerui/huxerui.h>
#include <huxerui/interaction.h>

namespace interaction_demo {

/// Counters genuinely shared across gallery sections and displayed in the footer StatusSection.
/// Section-local state lives inside each section's own Scope so local interactions do not recompose the whole gallery.
struct DemoState {
  huxerui::State<int> clicks;
  huxerui::State<int> holds;
  huxerui::State<int> doubles;
};

/// Value-owned theme and viewport snapshot safe to capture into section-local composition scopes.
struct DemoContext {
  huxerui::ColorScheme colors;
  bool compact = false;
  bool expanded = false;
  bool dark = false;
};

/// Section label shared by the family demos.
inline huxerui::View SectionLabel(const DemoContext& context, const char* text) {
  return huxerui::Text(text, huxerui::TextRole::Label)
      .With(huxerui::Foreground(context.colors.on_surface_variant));
}

huxerui::View BloomSection(const DemoContext& context);
huxerui::View PressSection(const DemoState& state, const DemoContext& context);
huxerui::View PaintSection(const DemoContext& context);
huxerui::View GestureSection(const DemoState& state, const DemoContext& context);
huxerui::View ReorderSection(const DemoContext& context);
huxerui::View LayoutSection(const DemoContext& context);
huxerui::View DragSection(const DemoContext& context);
huxerui::View StatusSection(const DemoState& state, const DemoContext& context);

} // namespace interaction_demo
