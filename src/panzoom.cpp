#include <huxerui/interaction/panzoom.h>

#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace huxerui::interaction {
namespace {

void ValidateStyle(const PanZoomStyle& style) {
  if(!std::isfinite(style.minimum_scale) || style.minimum_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: PanZoom minimum scale must be finite and positive");
  }
  if(!std::isfinite(style.maximum_scale) || style.maximum_scale <= 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: PanZoom maximum scale must be finite and positive");
  }
  if(style.minimum_scale > style.maximum_scale) {
    throw std::invalid_argument("HuxerUI Interaction: PanZoom minimum scale must not exceed maximum scale");
  }
  if(!std::isfinite(style.wheel_sensitivity) || style.wheel_sensitivity < 0.0F) {
    throw std::invalid_argument("HuxerUI Interaction: PanZoom wheel sensitivity must be finite and non-negative");
  }
}

} // namespace

View PanZoom(View content, PanZoomStyle style) {
  ValidateStyle(style);
  return Scope([content = std::move(content), style]() -> View {
    auto pan = UseState(Point{});
    auto scale = UseState(1.0F);
    auto rotation = UseState(0.0F);
    auto active = UseState(false);
    const bool transforming = active.Get();
    const AnimationSpec motion = transforming ? style.follow_motion : style.return_motion;

    View zoomable = content;
    zoomable = std::move(zoomable)
                   .With(TransformGesture{})
                   .With(Scale(AnimateTo(scale.Get(), motion), style.origin));
    if(style.rotation) {
      zoomable = std::move(zoomable).With(Rotation(AnimateTo(rotation.Get(), motion), style.origin));
    }
    zoomable = std::move(zoomable)
                   .With(Offset(AnimateTo(pan.Get(), motion)))
                   .On<TransformEvents::Started>([active](const TransformEvent&) { active = true; })
                   .On<TransformEvents::Changed>(
                       [active, pan, scale, rotation, style](const TransformEvent& event) {
                         if(!active.Get()) return;
                         Point next_pan = pan.Get();
                         next_pan.x += event.pan.x;
                         next_pan.y += event.pan.y;
                         pan = next_pan;
                         scale = std::clamp(scale.Get() * event.scale, style.minimum_scale,
                                            style.maximum_scale);
                         if(style.rotation) rotation = rotation.Get() + event.rotation * (180.0F / 3.14159265358979323846F);
                       })
                   .On<TransformEvents::Ended>([active, pan, scale, rotation, style](const TransformEvent&) {
                     active = false;
                     if(style.spring_back) {
                       pan = Point{};
                       scale = 1.0F;
                       rotation = 0.0F;
                     }
                   })
                   .On<TransformEvents::Canceled>([active, pan, scale, rotation, style](const TransformEvent&) {
                     active = false;
                     if(style.spring_back) {
                       pan = Point{};
                       scale = 1.0F;
                       rotation = 0.0F;
                     }
                   })
                   .On<ViewEvents::ScrollInput>([scale, style](const ScrollInputEvent& event) -> bool {
                     if(!style.wheel_zoom || !event.modifiers.control) return false;
                     scale = std::clamp(scale.Get() * std::exp(-event.delta_y * style.wheel_sensitivity),
                                        style.minimum_scale, style.maximum_scale);
                     return true;
                   });
    return zoomable;
  });
}

} // namespace huxerui::interaction
