#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <utility>

namespace huxerui::interaction {

/// Configuration for synchronized mount entrance and visibility transitions.
struct AppearStyle {
  /// Starting opacity when hidden / before entrance completes. Must be in `[0, 1]`.
  float initial_opacity = 0.0F;
  /// Starting translation relative to final resting layout position. Must be finite.
  Point initial_offset{0.0F, 12.0F};
  /// Starting uniform scale factor when hidden / before entrance completes. Must be finite and positive.
  float initial_scale = 0.96F;
  /// Transform pivot used by the scale track. Must have finite coordinates.
  TransformOrigin origin{};
  /// Animation timing or spring curve driving the transition.
  AnimationSpec motion = TweenSpec{.duration = 0.24, .easing = Easing::EaseOut};
  /// Entrance stagger delay in seconds before animating visible. Must be finite and non-negative.
  double delay = 0.0;
  /// Animates from initial state on initial mount when true; settles immediately on mount when false.
  bool animate_on_mount = true;

  bool operator==(const AppearStyle&) const = default;
};

/// Applies a synchronized opacity, offset, and scale entrance/visibility transition controlled by `visible`.
/// @throws std::invalid_argument If any field of `style` is invalid.
View Appear(View content, bool visible, AppearStyle style = {});

/// Applies a mount entrance transition to `content` (`visible = true`).
/// @throws std::invalid_argument If any field of `style` is invalid.
View Appear(View content, AppearStyle style = {});

/// Pipeable adaptor for mount entrance Appear (`visible = true`).
[[nodiscard]] inline ViewAdaptor Appear(AppearStyle style = {}) {
  return ViewAdaptor([style](View content) { return Appear(std::move(content), style); });
}

/// Pipeable adaptor for visibility-driven Appear.
[[nodiscard]] inline ViewAdaptor Appear(bool visible, AppearStyle style = {}) {
  return ViewAdaptor([visible, style](View content) { return Appear(std::move(content), visible, style); });
}

} // namespace huxerui::interaction
