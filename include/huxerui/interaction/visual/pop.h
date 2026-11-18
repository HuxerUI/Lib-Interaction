#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct PopTriggerState;
} // namespace detail

/// Motion configuration for Popped.
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

/// Copyable trigger handle created by UsePop.
/// Default-constructed handles are unbound and safe outside an Application runtime. Call Trigger() only from event
/// handlers, semantic actions, or tasks on the owning UI thread — never during composition.
class PopTrigger {
public:
  PopTrigger() noexcept = default;

  /// Starts one out-and-back pop on the view bound by Popped.
  void Trigger() const;

  /// Reports whether the handle was created by UsePop.
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  bool operator==(const PopTrigger&) const = default;

private:
  explicit PopTrigger(std::shared_ptr<detail::PopTriggerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::PopTriggerState> state_;

  friend PopTrigger UsePop(PopStyle style);
  friend View Popped(View content, const PopTrigger& pop);
};

/// Composable hook: creates a pop trigger remembered in the current composition scope.
/// @throws std::invalid_argument If amplitude or duration is non-finite or not positive.
PopTrigger UsePop(PopStyle style = {});

/// Composes content that plays the bound trigger's pop with an animated scale.
View Popped(View content, const PopTrigger& pop);

/// Pipeable adaptor for Popped.
[[nodiscard]] inline ViewAdaptor Popped(const PopTrigger& pop) {
  return ViewAdaptor([pop](View content) { return Popped(std::move(content), pop); });
}

} // namespace huxerui::interaction
