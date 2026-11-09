#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/state.h>
#include <huxerui/task.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Motion configuration for Popped. One pop is an out-and-back scale sequence: the content grows to its peak and
/// settles back to rest with one tween per direction.
struct PopStyle {
  /// Peak scale above rest; the peak target is 1.0 + amplitude. Must be finite and positive.
  float amplitude = 0.12F;
  /// Seconds spent moving in one direction of the sequence. Must be finite and positive.
  double duration = 0.12;
  /// Curve applied within each direction.
  Easing easing = Easing::EaseOut;
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};
  bool operator==(const PopStyle&) const = default;
};

/// Copyable trigger handle created by UsePop. Triggering writes composition state and launches a timed task, so
/// call Trigger() only from event handlers, semantic actions, or tasks on the owning UI thread — never during
/// composition. Triggering while a pop runs restarts it at full amplitude.
class PopTrigger {
public:
  /// Creates an unbound handle; Trigger() does nothing and the handle is false.
  PopTrigger() noexcept = default;

  /// Starts one out-and-back pop on the view bound by Popped.
  void Trigger() const;

  /// Reports whether the handle was created by UsePop.
  [[nodiscard]] explicit operator bool() const noexcept {
    return scale_.IsValid() && run_.IsValid();
  }

  bool operator==(const PopTrigger&) const = default;

private:
  PopTrigger(State<float> scale, State<std::uint64_t> run, TaskScope tasks, PopStyle style)
      : scale_(std::move(scale)), run_(std::move(run)), tasks_(std::move(tasks)), style_(style) {}

  State<float> scale_;
  State<std::uint64_t> run_;
  TaskScope tasks_;
  PopStyle style_;

  friend PopTrigger UsePop(PopStyle style);
  friend View Popped(View content, const PopTrigger& pop);
};

/// Composable hook: creates a pop trigger and binds it to the current composition scope.
///
/// Call during composition. The returned handle is cheap to copy and can be captured by event callbacks.
/// @param style Pop amplitude, pacing, curve, and pivot. Must satisfy the documented ranges.
/// @return A trigger handle bound to the current composition.
/// @throws std::invalid_argument If amplitude or duration is non-finite or not positive.
/// @code
/// auto pop = UsePop();
/// return Popped(Badge(), pop).With(...);
/// // later, from a callback: pop.Trigger();
/// @endcode
PopTrigger UsePop(PopStyle style = {});

/// Composes content that plays the bound trigger's pop with an animated scale.
///
/// Call during composition, with a trigger created by UsePop. The composed view animates between the trigger's
/// poses using one tween per direction; reduced motion snaps between poses, and every sequence ends at rest.
/// @param content Content the pop scales. The view imposes no intrinsic frame.
/// @param pop Trigger created by UsePop. An unbound handle composes the content unchanged.
/// @return A View whose scale follows the trigger's sequence.
/// @code
/// auto pop = UsePop({.amplitude = 0.2F});
/// return Popped(LikeIcon(), pop).With(Frame{.width = 32.0F, .height = 32.0F});
/// @endcode
View Popped(View content, const PopTrigger& pop);

} // namespace huxerui::interaction
