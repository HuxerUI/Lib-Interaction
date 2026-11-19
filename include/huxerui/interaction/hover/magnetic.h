#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <utility>

namespace huxerui::interaction {

/// Configuration for pointer-attracted Magnetic hover motion.
struct MagneticStyle {
  /// Maximum translation along each axis at the outer edge of the view, in logical pixels. Must be finite and >= 0.
  Point maximum_offset{8.0F, 8.0F};
  /// Uniform scale applied while hovered. Must be finite and positive.
  float hovered_scale = 1.03F;
  /// Transform pivot for hover scaling. Must have finite coordinates.
  TransformOrigin origin{};
  /// Animation specification while tracking pointer movement inside the hover zone.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.09, .easing = Easing::EaseOut};
  /// Animation specification when springing back to rest on hover leave.
  AnimationSpec return_motion = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.72F};
  /// Optional pointer cursor applied on hover over the stationary container.
  PointerCursorKind cursor = PointerCursorKind::Default;

  bool operator==(const MagneticStyle&) const = default;
};

/// Wraps `content` in a stationary hover probe container that pulls the inner content toward the cursor.
/// @throws std::invalid_argument If any field of `style` is invalid.
View Magnetic(View content, MagneticStyle style = {});

/// Pipeable adaptor for Magnetic.
[[nodiscard]] inline ViewAdaptor Magnetic(MagneticStyle style = {}) {
  return ViewAdaptor([style](View content) { return Magnetic(std::move(content), style); });
}

} // namespace huxerui::interaction
