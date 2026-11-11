#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>

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
  /// Surface revealed behind the content while it moves.
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
///
/// The content follows the pointer with follow_motion; below threshold it settles back, otherwise it exits by
/// exit_distance and invokes on_dismissed after exit_duration. The wrapper paints style.background behind the content
/// and clips descendants. Cancellation returns to rest. The caller removes or replaces the content after on_dismissed.
/// @param content Content to dismiss.
/// @param on_dismissed Callback invoked after the exit travel.
/// @param style Thresholds, background, and motions.
/// @return A View whose mounted instance owns the drag and dismissal state.
/// @throws std::invalid_argument If threshold, distance, velocity, or exit duration is invalid.
View Dismissible(View content, std::function<void()> on_dismissed, DismissibleStyle style = {});

} // namespace huxerui::interaction
