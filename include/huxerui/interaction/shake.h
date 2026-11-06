#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/state.h>
#include <huxerui/task.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Motion configuration for Shaken. One shake is a sequence of alternating tweens between the rest position and a
/// decaying horizontal peak; duration is the time of one direction of one wobble.
struct ShakeStyle {
  /// Peak horizontal offset of the first wobble, in logical pixels. Must be finite and positive.
  float amplitude = 10.0F;
  /// Number of out-and-back wobbles. Must be at least one.
  int wobbles = 3;
  /// Seconds spent moving in one direction of one wobble. Must be finite and positive.
  double duration = 0.06;
  /// Curve applied within each direction of each wobble.
  Easing easing = Easing::EaseInOut;
  bool operator==(const ShakeStyle&) const = default;
};

/// Copyable trigger handle created by UseShake. Starting a shake writes composition state and launches a timed
/// task, so call Trigger() only from event handlers, semantic actions, or tasks on the owning UI thread — never
/// during composition. Triggering while a shake runs replaces it with a new full-amplitude sequence.
class ShakeTrigger {
public:
  /// Creates an unbound handle; Trigger() does nothing and the handle is false.
  ShakeTrigger() noexcept = default;

  /// Starts one decaying shake sequence on the view bound by Shaken.
  void Trigger() const;

  /// Reports whether the handle was created by UseShake.
  [[nodiscard]] explicit operator bool() const noexcept {
    return offset_.IsValid() && run_.IsValid();
  }

  bool operator==(const ShakeTrigger&) const = default;

private:
  ShakeTrigger(State<Point> offset, State<std::uint64_t> run, TaskScope tasks, ShakeStyle style)
      : offset_(std::move(offset)), run_(std::move(run)), tasks_(std::move(tasks)), style_(style) {}

  State<Point> offset_;
  State<std::uint64_t> run_;
  TaskScope tasks_;
  ShakeStyle style_;

  friend ShakeTrigger UseShake(ShakeStyle style);
  friend View Shaken(View content, const ShakeTrigger& shake);
};

/// Composable hook: creates a shake trigger and binds it to the current composition scope.
///
/// Call during composition. The returned handle is cheap to copy and can be captured by event callbacks.
/// @param style Shake amplitude, wobble count, pacing, and curve. Must satisfy the documented ranges.
/// @return A trigger handle bound to the current composition.
/// @throws std::invalid_argument If amplitude or duration is non-finite or not positive, or wobbles is below one.
/// @code
/// auto shake = UseShake();
/// return Shaken(Field(), shake).With(...);
/// // later, from a callback: shake.Trigger();
/// @endcode
ShakeTrigger UseShake(ShakeStyle style = {});

/// Composes content that plays the bound trigger's shake with an animated horizontal offset.
///
/// Call during composition, with a trigger created by UseShake. The composed view animates between the trigger's
/// poses using one tween per direction; reduced motion snaps between poses, and every sequence ends at rest.
/// @param content Content the shake moves. The view imposes no intrinsic frame.
/// @param shake Trigger created by UseShake. An unbound handle composes the content unchanged.
/// @return A View whose offset follows the trigger's sequence.
/// @code
/// auto shake = UseShake({.amplitude = 8.0F});
/// return Shaken(Card(), shake).With(Frame{.width = 280.0F, .height = 120.0F});
/// @endcode
View Shaken(View content, const ShakeTrigger& shake);

} // namespace huxerui::interaction
