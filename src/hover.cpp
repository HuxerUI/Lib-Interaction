#include <huxerui/interaction/hover.h>

#include <huxerui/state.h>

#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const HoverLiftStyle& style) {
  if(!std::isfinite(style.lift.x) || !std::isfinite(style.lift.y)) {
    throw std::invalid_argument("HuxerUI Interaction: HoverLift lift must be finite");
  }
  if(!std::isfinite(style.hovered_scale) || style.hovered_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: HoverLift scale must be finite and positive");
  }
}

} // namespace

View HoverLift(View content, HoverLiftStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style]() -> View {
    auto hovered = UseState(false);
    View lifted = content;
    return std::move(lifted)
        .With(Offset(AnimateTo(hovered ? style.lift : Point{}, style.animation)))
        .With(Scale(AnimateTo(hovered ? style.hovered_scale : 1.0F, style.animation), style.origin))
        .On<ViewEvents::Hover>([hovered](const HoverEvent& event) {
          hovered = event.type != HoverEventType::Leave;
        });
  });
}

} // namespace huxerui::interaction
