#include <huxerui/interaction/hover/hoverlift.h>

#include "../internal/validate.h"

#include <huxerui/modifier.h>
#include <huxerui/state.h>

#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const HoverLiftStyle& style) {
  internal::RequireFinite(style.lift, "HuxerUI Interaction: HoverLift lift must be finite");
  internal::RequirePositive(style.hovered_scale, "HuxerUI Interaction: HoverLift scale must be finite and positive");
}

} // namespace

View Hoverable(std::function<View(bool hovered)> builder, PointerCursorKind cursor) {
  if(!builder) {
    throw std::invalid_argument("HuxerUI Interaction: Hoverable requires a view builder");
  }
  return Scope([builder = std::move(builder), cursor]() -> View {
    auto hovered = UseState(false);
    View surface = builder(hovered.Get());
    surface = std::move(surface).On<ViewEvents::Hover>([hovered](const HoverEvent& event) {
      hovered = event.type != HoverEventType::Leave;
    });
    if(cursor != PointerCursorKind::Default) {
      surface = std::move(surface).With(PointerCursor{cursor});
    }
    return surface;
  });
}

View HoverLift(View content, HoverLiftStyle style) {
  ValidateStyle(style);
  return Hoverable([content = std::move(content), style](bool hovered) -> View {
    View lifted = content;
    return std::move(lifted)
        .With(Offset(AnimateTo(hovered ? style.lift : Point{}, style.animation)))
        .With(Scale(AnimateTo(hovered ? style.hovered_scale : 1.0F, style.animation), style.origin));
  });
}

} // namespace huxerui::interaction
