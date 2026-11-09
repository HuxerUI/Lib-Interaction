#include <huxerui/interaction/pinch.h>

#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const PinchZoomStyle& style) {
  if(!std::isfinite(style.minimum_scale) || style.minimum_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: PinchZoom minimum scale must be finite and positive");
  }
  if(!std::isfinite(style.maximum_scale) || style.maximum_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: PinchZoom maximum scale must be finite and positive");
  }
  if(style.minimum_scale > style.maximum_scale) {
    throw std::invalid_argument(
        "HuxerUI Interaction: PinchZoom minimum scale must not exceed maximum scale");
  }
}

} // namespace

View PinchZoom(View content, PinchZoomStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style]() -> View {
    auto zoom = UseState(1.0F);
    auto active = UseState(false);
    const float target = (active.Get() || !style.spring_back) ? zoom.Get() : 1.0F;
    const AnimationSpec motion = active.Get() ? style.follow_motion : style.return_motion;
    View zoomable = content;
    return std::move(zoomable)
        .With(TransformGesture{})
        .With(Scale(AnimateTo(target, motion)))
        .On<TransformEvents::Started>(
            [zoom, active, style](const TransformEvent&) {
              if(style.spring_back) zoom = 1.0F;
              active = true;
            })
        .On<TransformEvents::Changed>(
            [zoom, active, style](const TransformEvent& event) {
              if(!active.Get()) return;
              zoom = std::clamp(zoom.Get() * event.scale, style.minimum_scale, style.maximum_scale);
            })
        .On<TransformEvents::Ended>([active](const TransformEvent&) { active = false; })
        .On<TransformEvents::Canceled>([active](const TransformEvent&) { active = false; })
        .On<ViewEvents::ScrollInput>(
            [zoom, style](const ScrollInputEvent& event) -> bool {
              if(!style.wheel_zoom || !event.modifiers.control) return false;
              zoom = std::clamp(zoom.Get() * std::exp(-event.delta_y * 0.15F), style.minimum_scale,
                                style.maximum_scale);
              return true;
            });
  });
}

} // namespace huxerui::interaction
