#include "demos.h"

namespace interaction_demo {
using namespace huxerui;

View StatusSection(const DemoState& state, const DemoContext& context) {
  return Flow {
    Text("Tap anywhere on the tiles", TextRole::Label)
        .With(Foreground(context.colors.on_surface_variant)),
    Text("Tab focuses, Enter or Space activates", TextRole::Label)
        .With(Foreground(context.colors.on_surface_variant)),
    Text("Ctrl + wheel zooms the pinch tile", TextRole::Label)
        .With(Foreground(context.colors.on_surface_variant)),
    Text("clicks: " + std::to_string(state.clicks.Get()), TextRole::Label).Key("click-count"),
    Text("holds: " + std::to_string(state.holds.Get()), TextRole::Label).Key("hold-count"),
    Text("double-clicks: " + std::to_string(state.doubles.Get()), TextRole::Label).Key("double-count"),
    Text("Motion respects the system reduced-motion preference.", TextRole::Label)
        .With(Foreground(context.colors.on_surface_variant)),
  }.With(Spacing(20.0F), CrossAlign(CrossAxisAlignment::Center));
}

} // namespace interaction_demo
