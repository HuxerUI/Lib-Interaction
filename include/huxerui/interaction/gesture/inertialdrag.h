#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for InertialDrag.
struct InertialDragStyle {
  /// Optional axis constraint. Empty allows two-dimensional movement.
  std::optional<Axis> axis = std::nullopt;
  /// Seconds of terminal velocity projected into the release target. Must be finite and non-negative.
  float projection_time = 0.18F;
  /// Largest absolute projected offset on either axis, in logical pixels. Must be finite and non-negative.
  float maximum_projection = 240.0F;
  /// Motion used while the pointer drags the content.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used to settle at the velocity-projected release offset or back to rest on cancel.
  AnimationSpec settle_motion = SpringSpec{.stiffness = 180.0F, .damping_ratio = 0.9F};

  bool operator==(const InertialDragStyle&) const = default;
};

/// Drags content and settles at a velocity-projected offset when the gesture ends.
/// @throws std::invalid_argument If projection_time or maximum_projection is negative or non-finite.
View InertialDrag(View content, InertialDragStyle style = {},
                  std::function<void(Point)> on_settled = {});

/// Pipeable adaptor for InertialDrag.
[[nodiscard]] inline ViewAdaptor InertialDrag(InertialDragStyle style = {},
                                              std::function<void(Point)> on_settled = {}) {
  return ViewAdaptor([style, on_settled = std::move(on_settled)](View content) {
    return InertialDrag(std::move(content), style, on_settled);
  });
}

} // namespace huxerui::interaction
