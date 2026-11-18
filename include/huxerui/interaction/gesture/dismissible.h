#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for Dismissible.
struct DismissibleStyle {
  /// Drag axis.
  Axis axis = Axis::Horizontal;
  /// Absolute displacement required to dismiss. Must be finite and positive.
  float dismiss_threshold = 120.0F;
  /// Release velocity required to dismiss below the distance threshold. Must be finite and non-negative.
  float minimum_velocity = 600.0F;
  /// Travel distance used for the exit animation. Must be finite and positive.
  float exit_distance = 480.0F;
  /// Time before on_dismissed runs after the exit target is selected. Must be finite and non-negative.
  double exit_duration = 0.24;
  /// Surface revealed behind the content while it moves (used when no custom background builder is supplied).
  Color background = Color::Rgb(0, 0, 0, 0.12F);
  /// Motion while the pointer drags the content.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion when the content returns below threshold.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.2, .easing = Easing::EaseOut};
  /// Motion for the exit travel.
  AnimationSpec exit_motion = TweenSpec{.duration = 0.24, .easing = Easing::EaseIn};

  bool operator==(const DismissibleStyle&) const = default;
};

/// Drags content along one axis and dismisses it past a threshold or velocity.
/// @throws std::invalid_argument If threshold, distance, velocity, or exit duration is invalid.
View Dismissible(View content, std::function<void()> on_dismissed, DismissibleStyle style = {});

/// Headless-background overload supplying `progress` in [0, 1] and signed `direction` (-1, 0, or +1) to `background_builder`.
/// @throws std::invalid_argument If `background_builder` is empty or `style` is invalid.
View Dismissible(View content, std::function<View(float progress, float direction)> background_builder,
                 std::function<void()> on_dismissed, DismissibleStyle style = {});

/// Pipeable adaptor for Dismissible.
[[nodiscard]] inline ViewAdaptor Dismissible(std::function<void()> on_dismissed, DismissibleStyle style = {}) {
  return ViewAdaptor([on_dismissed = std::move(on_dismissed), style](View content) {
    return Dismissible(std::move(content), on_dismissed, style);
  });
}

} // namespace huxerui::interaction
