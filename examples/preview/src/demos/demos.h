#pragma once

#include <huxerui/huxerui.h>
#include <huxerui/interaction.h>

#include <string>

namespace interaction_demo {

/// State and triggers shared by the gallery sections so cross-demo counters stay in one place.
struct DemoState {
  huxerui::State<int> clicks;
  huxerui::State<int> holds;
  huxerui::State<int> doubles;
  huxerui::State<huxerui::interaction::FireworksBurst> burst;
  huxerui::State<std::string> last_reorder;
  huxerui::State<std::string> grid_last;
  huxerui::State<float> pane_width;
  huxerui::State<int> boops;
  huxerui::State<bool> paw_hovered;
  huxerui::State<huxerui::interaction::FireworksBurst> paw_burst;
  huxerui::interaction::ShakeTrigger shake;
  huxerui::interaction::PopTrigger pop;
  huxerui::interaction::PopTrigger paw_pop;
};

/// Read-only theme and viewport values resolved once by the gallery.
struct DemoContext {
  const huxerui::ColorScheme& colors;
  bool compact = false;
  bool expanded = false;
  bool dark = false;
};

/// Section label shared by the family demos.
inline huxerui::View SectionLabel(const DemoContext& context, const char* text) {
  return huxerui::Text(text, huxerui::TextRole::Label)
      .With(huxerui::Foreground(context.colors.on_surface_variant));
}

huxerui::View BloomSection(const DemoState& state, const DemoContext& context);
huxerui::View PressSection(const DemoState& state, const DemoContext& context);
huxerui::View PaintSection(const DemoState& state, const DemoContext& context);
huxerui::View GestureSection(const DemoState& state, const DemoContext& context);
huxerui::View ReorderSection(const DemoState& state, const DemoContext& context);
huxerui::View LayoutSection(const DemoState& state, const DemoContext& context);
huxerui::View DragSection(const DemoState& state, const DemoContext& context);
huxerui::View StatusSection(const DemoState& state, const DemoContext& context);

} // namespace interaction_demo
