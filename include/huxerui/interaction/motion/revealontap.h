#pragma once

#include <huxerui/animation.h>

#include <functional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for RevealOnTap.
struct RevealOnTapStyle {
  /// One-shot scene effect used for the mutation.
  TransitionSpec transition{CircularRevealTransition{}, TweenSpec{.duration = 0.36, .easing = Easing::EaseInOut}};
  bool operator==(const RevealOnTapStyle&) const = default;
};

/// Runs a synchronous mutation with a circular-reveal scene transition from the current interaction origin.
///
/// Call from a synchronous click, pointer, keyboard, or semantic callback. The handle is retained by the calling
/// composition scope, so repeated activations reuse the same scene service and reduced-motion policy.
class RevealOnTapHandle {
public:
  /// Runs the mutation through SceneTransitionHandle::RunFromCurrentInteraction.
  /// @param mutation Synchronous authoritative state update; must not be empty.
  /// @throws std::invalid_argument If mutation is empty.
  /// @throws std::logic_error If no synchronous interaction origin exists or the service is disconnected.
  void Run(std::function<void()> mutation) const;

private:
  RevealOnTapHandle(SceneTransitionHandle handle, TransitionSpec transition);

  SceneTransitionHandle handle_;
  TransitionSpec transition_;

  friend RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style);
};

/// Retains a reveal handle in the current composition scope.
/// @param style Transition effect and timing.
/// @return A copyable handle for synchronous activation callbacks.
RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style = {});

} // namespace huxerui::interaction
