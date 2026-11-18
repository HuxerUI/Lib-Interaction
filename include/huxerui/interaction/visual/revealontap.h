#pragma once

#include <huxerui/animation.h>

#include <functional>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Configuration for RevealOnTap.
struct RevealOnTapStyle {
  /// One-shot scene effect used for the mutation.
  TransitionSpec transition{CircularRevealTransition{}, TweenSpec{.duration = 0.36, .easing = Easing::EaseInOut}};

  bool operator==(const RevealOnTapStyle&) const = default;
};

/// Runs a synchronous mutation with a circular-reveal scene transition from the current interaction origin.
class RevealOnTapHandle {
public:
  RevealOnTapHandle() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return state_.has_value(); }

  /// Runs the mutation through SceneTransitionHandle::RunFromCurrentInteraction.
  /// @throws std::invalid_argument If `mutation` is empty.
  /// @throws std::logic_error If unbound, no synchronous interaction origin exists, or the service is disconnected.
  void Run(std::function<void()> mutation) const;

private:
  struct BoundState {
    SceneTransitionHandle handle;
    TransitionSpec transition;
  };

  RevealOnTapHandle(SceneTransitionHandle handle, TransitionSpec transition);

  std::optional<BoundState> state_;

  friend RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style);
};

/// Retains a reveal handle in the current composition scope.
RevealOnTapHandle UseRevealOnTap(RevealOnTapStyle style = {});

} // namespace huxerui::interaction
