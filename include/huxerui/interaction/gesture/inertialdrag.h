#pragma once

#include <huxerui/animation.h>
#include <huxerui/gesture.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <optional>

namespace huxerui::interaction {

/// Configuration for InertialDrag.
struct InertialDragStyle {
  /// Optional axis constraint. An empty value allows two-dimensional movement.
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
///
/// This is a velocity-projected target, not a velocity-continuous retained simulation: the terminal DragEvent
/// velocity extends the target by projection_time seconds of travel, clamped to maximum_projection, and one
/// AnimateTo settles there. Cancellation returns to rest. The widget imposes no intrinsic frame.
/// @param content Content to drag. The widget imposes no intrinsic frame.
/// @param style Axis, projection, and motions.
/// @return A View whose mounted instance owns the drag offset.
/// @throws std::invalid_argument If projection time or maximum projection is invalid.
View InertialDrag(View content, InertialDragStyle style = {});

} // namespace huxerui::interaction
