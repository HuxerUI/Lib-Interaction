#include <huxerui/interaction/gesture/transform.h>

#include "../internal/validate.h"

#include <huxerui/state.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace huxerui::interaction {

namespace detail {
struct PanZoomControllerState {
  State<Point> pan;
  State<float> scale;
  State<float> rotation;
  float minimum_scale = 0.5F;
  float maximum_scale = 4.0F;
  std::optional<Point> pan_bounds;
};
} // namespace detail

namespace {

void ValidatePanZoomStyle(const PanZoomStyle& style) {
  internal::RequireScaleBounds(style.minimum_scale, style.maximum_scale,
                               "HuxerUI Interaction: PanZoom minimum scale must be finite and positive",
                               "HuxerUI Interaction: PanZoom maximum scale must be finite and positive",
                               "HuxerUI Interaction: PanZoom minimum scale must not exceed maximum scale");
  internal::RequireNonNegative(
      style.wheel_sensitivity, "HuxerUI Interaction: PanZoom wheel sensitivity must be finite and non-negative");
  if(style.pan_bounds.has_value()) {
    internal::RequirePositive(style.pan_bounds->x,
                              "HuxerUI Interaction: PanZoom pan bounds must be finite and positive");
    internal::RequirePositive(style.pan_bounds->y,
                              "HuxerUI Interaction: PanZoom pan bounds must be finite and positive");
  }
  internal::RequireNonNegative(
      style.rotation_snap, "HuxerUI Interaction: PanZoom rotation snap must be finite and non-negative");
}

Point ClampPan(Point value, const std::optional<Point>& bounds) {
  if(!bounds.has_value()) return value;
  return {
      std::clamp(value.x, -bounds->x, bounds->x),
      std::clamp(value.y, -bounds->y, bounds->y),
  };
}

} // namespace

Point PanZoomController::Pan() const {
  if(!state_ || !state_->pan.IsValid()) return {};
  return state_->pan.Get();
}

float PanZoomController::Scale() const {
  if(!state_ || !state_->scale.IsValid()) return 1.0F;
  return state_->scale.Get();
}

float PanZoomController::Rotation() const {
  if(!state_ || !state_->rotation.IsValid()) return 0.0F;
  return state_->rotation.Get();
}

void PanZoomController::ZoomTo(float next_scale) const {
  if(!state_ || !state_->scale.IsValid() || !std::isfinite(next_scale)) return;
  state_->scale = std::clamp(next_scale, state_->minimum_scale, state_->maximum_scale);
}

void PanZoomController::ZoomBy(float factor) const {
  if(!state_ || !state_->scale.IsValid() || !std::isfinite(factor) || factor <= 0.0F) return;
  state_->scale = std::clamp(state_->scale.Get() * factor, state_->minimum_scale, state_->maximum_scale);
}

void PanZoomController::PanTo(Point next_pan) const {
  if(!state_ || !state_->pan.IsValid() || !std::isfinite(next_pan.x) || !std::isfinite(next_pan.y)) return;
  state_->pan = ClampPan(next_pan, state_->pan_bounds);
}

void PanZoomController::Reset() const {
  if(!state_) return;
  if(state_->pan.IsValid()) state_->pan = Point{};
  if(state_->scale.IsValid()) state_->scale = std::clamp(1.0F, state_->minimum_scale, state_->maximum_scale);
  if(state_->rotation.IsValid()) state_->rotation = 0.0F;
}

PanZoomController UsePanZoomController(float initial_scale) {
  internal::RequirePositive(initial_scale,
                            "HuxerUI Interaction: UsePanZoomController initial scale must be finite and positive");
  auto pan = UseState(Point{});
  auto scale = UseState(initial_scale);
  auto rotation = UseState(0.0F);
  auto holder = UseState(std::make_shared<detail::PanZoomControllerState>());
  auto shared = holder.Get();
  shared->pan = pan;
  shared->scale = scale;
  shared->rotation = rotation;
  return PanZoomController(std::move(shared));
}

View PanZoom(View content, PanZoomStyle style) {
  ValidatePanZoomStyle(style);
  return Scope([content = std::move(content), style]() -> View {
    auto local_pan = UseState(Point{});
    auto local_scale = UseState(1.0F);
    auto local_rotation = UseState(0.0F);
    auto active = UseState(false);

    const bool has_controller = style.controller.state_ && style.controller.state_->scale.IsValid();
    const State<Point> pan = has_controller ? style.controller.state_->pan : local_pan;
    const State<float> scale = has_controller ? style.controller.state_->scale : local_scale;
    const State<float> rotation = has_controller ? style.controller.state_->rotation : local_rotation;

    if(style.controller.state_) {
      style.controller.state_->minimum_scale = style.minimum_scale;
      style.controller.state_->maximum_scale = style.maximum_scale;
      style.controller.state_->pan_bounds = style.pan_bounds;
    }

    const bool transforming = active.Get();
    const float current_scale = std::clamp(scale.Get(), style.minimum_scale, style.maximum_scale);
    const AnimationSpec motion = transforming ? style.follow_motion : style.return_motion;

    View zoomable = content;
    zoomable = std::move(zoomable)
                   .With(TransformGesture{})
                   .With(Scale(AnimateTo(current_scale, motion), style.origin));
    if(style.rotation) {
      zoomable = std::move(zoomable).With(Rotation(AnimateTo(rotation.Get(), motion), style.origin));
    }
    if(style.pan) {
      zoomable = std::move(zoomable).With(Offset(AnimateTo(pan.Get(), motion)));
    }
    zoomable = std::move(zoomable)
                   .On<TransformEvents::Started>([active](const TransformEvent&) { active = true; })
                   .On<TransformEvents::Changed>(
                       [active, pan, scale, rotation, style](const TransformEvent& event) {
                         if(!active.Get()) return;
                         if(style.pan) {
                           Point next_pan = pan.Get();
                           next_pan.x += event.pan.x;
                           next_pan.y += event.pan.y;
                           pan = ClampPan(next_pan, style.pan_bounds);
                         }
                         scale = std::clamp(scale.Get() * event.scale, style.minimum_scale, style.maximum_scale);
                         if(style.rotation) {
                           rotation = rotation.Get() + event.rotation * (180.0F / 3.14159265358979323846F);
                         }
                       })
                   .On<TransformEvents::Ended>([active, pan, scale, rotation, style](const TransformEvent&) {
                     active = false;
                     if(style.spring_back) {
                       pan = Point{};
                       scale = std::clamp(1.0F, style.minimum_scale, style.maximum_scale);
                       rotation = 0.0F;
                     } else if(style.rotation && style.rotation_snap > 0.0F) {
                       rotation = std::round(rotation.Get() / style.rotation_snap) * style.rotation_snap;
                     }
                   })
                   .On<TransformEvents::Canceled>([active, pan, scale, rotation, style](const TransformEvent&) {
                     active = false;
                     if(style.spring_back) {
                       pan = Point{};
                       scale = std::clamp(1.0F, style.minimum_scale, style.maximum_scale);
                       rotation = 0.0F;
                     }
                   })
                   .On<ViewEvents::ScrollInput>([scale, style](const ScrollInputEvent& event) -> bool {
                     if(!style.wheel_zoom || !event.modifiers.control) return false;
                     scale = std::clamp(scale.Get() * std::exp(-event.delta_y * style.wheel_sensitivity),
                                        style.minimum_scale, style.maximum_scale);
                     return true;
                   });
    if(style.double_tap_reset) {
      zoomable = std::move(zoomable)
                     .With(MultiTapGesture{
                         .count = 2,
                         .maximum_interval = std::nullopt,
                         .maximum_movement = std::nullopt,
                     })
                     .On<MultiTapEvents::Recognized>([pan, scale, rotation, style](const MultiTapEvent&) {
                       pan = Point{};
                       scale = std::clamp(1.0F, style.minimum_scale, style.maximum_scale);
                       rotation = 0.0F;
                     });
    }
    return zoomable;
  });
}

View PinchZoom(View content, PinchZoomStyle style) {
  internal::RequireScaleBounds(style.minimum_scale, style.maximum_scale,
                               "HuxerUI Interaction: PinchZoom minimum scale must be finite and positive",
                               "HuxerUI Interaction: PinchZoom maximum scale must be finite and positive",
                               "HuxerUI Interaction: PinchZoom minimum scale must not exceed maximum scale");
  return PanZoom(
      std::move(content),
      PanZoomStyle{
          .minimum_scale = style.minimum_scale,
          .maximum_scale = style.maximum_scale,
          .pan = false,
          .rotation = false,
          .spring_back = style.spring_back,
          .wheel_zoom = style.wheel_zoom,
          .follow_motion = style.follow_motion,
          .return_motion = style.return_motion,
          .controller = style.controller,
      });
}

} // namespace huxerui::interaction
