#pragma once

#include <huxerui/animation.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct SharedMorphState;
} // namespace detail

/// Timing configuration for local shared-element transitions.
struct SharedMorphStyle {
  /// One-shot animation driving matched shared elements (`SpringSpec` requires `damping_ratio > 0`).
  AnimationSpec motion = SpringSpec{.stiffness = 320.0F, .damping_ratio = 0.85F};

  bool operator==(const SharedMorphStyle&) const = default;
};

/// Copyable handle driving local shared-element / shared-bounds transitions within an attached `MorphScope`.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with UseSharedMorph().
class SharedMorphHandle {
public:
  SharedMorphHandle() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Returns the retained scope modifier to attach on the container enclosing morphing elements.
  /// @throws std::logic_error If unbound.
  [[nodiscard]] SharedTransitionScope Scope() const;

  /// Runs `mutation` inside a shared transition using this handle's configured default motion.
  /// @throws std::invalid_argument If `mutation` is empty.
  /// @throws std::logic_error If unbound or the container is unmounted.
  void Run(std::function<void()> mutation) const;

  /// Runs `mutation` inside a shared transition using an explicit one-shot `motion`.
  /// @throws std::invalid_argument If `motion` is invalid or `mutation` is empty.
  /// @throws std::logic_error If unbound or the container is unmounted.
  void Run(AnimationSpec motion, std::function<void()> mutation) const;

  bool operator==(const SharedMorphHandle&) const = default;

private:
  explicit SharedMorphHandle(std::shared_ptr<detail::SharedMorphState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::SharedMorphState> state_;

  friend SharedMorphHandle UseSharedMorph(SharedMorphStyle style);
};

/// Remembers a SharedMorphHandle in the current composition scope.
/// @throws std::invalid_argument If `style.motion` is not valid one-shot timing.
SharedMorphHandle UseSharedMorph(SharedMorphStyle style = {});

/// Pipeable adaptor attaching `handle.Scope()` to a container View.
[[nodiscard]] ViewAdaptor MorphScope(SharedMorphHandle handle);

/// Pipeable adaptor marking a View as a `SharedElement` under `key`.
/// @throws std::invalid_argument If `key` is empty.
[[nodiscard]] ViewAdaptor MorphElement(std::string key);

/// Pipeable adaptor marking a View as a `SharedBounds` region under `key`.
/// @throws std::invalid_argument If `key` is empty.
[[nodiscard]] ViewAdaptor MorphBounds(std::string key);

} // namespace huxerui::interaction
