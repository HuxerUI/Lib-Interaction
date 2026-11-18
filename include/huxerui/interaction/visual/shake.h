#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <memory>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct ShakeTriggerState;
} // namespace detail

/// Motion configuration for Shaken.
struct ShakeStyle {
  /// Peak offset of the first wobble, in logical pixels. Must be finite and positive.
  float amplitude = 10.0F;
  /// Number of out-and-back wobbles. Must be at least one.
  int wobbles = 3;
  /// Seconds spent moving in one direction of one wobble. Must be finite and positive.
  double duration = 0.06;
  /// Curve applied within each direction of each wobble.
  Easing easing = Easing::EaseInOut;
  /// Axis the shake travels along.
  Axis axis = Axis::Horizontal;

  bool operator==(const ShakeStyle&) const = default;
};

/// Copyable trigger handle created by UseShake.
/// Default-constructed handles are unbound and safe outside an Application runtime. Call Trigger() only from event
/// handlers, semantic actions, or tasks on the owning UI thread — never during composition.
class ShakeTrigger {
public:
  ShakeTrigger() noexcept = default;

  /// Starts one decaying shake sequence on the view bound by Shaken.
  void Trigger() const;

  /// Reports whether the handle was created by UseShake.
  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  bool operator==(const ShakeTrigger&) const = default;

private:
  explicit ShakeTrigger(std::shared_ptr<detail::ShakeTriggerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::ShakeTriggerState> state_;

  friend ShakeTrigger UseShake(ShakeStyle style);
  friend View Shaken(View content, const ShakeTrigger& shake);
};

/// Composable hook: creates a shake trigger remembered in the current composition scope.
/// @throws std::invalid_argument If amplitude or duration is non-finite or not positive, or wobbles is below one.
ShakeTrigger UseShake(ShakeStyle style = {});

/// Composes content that plays the bound trigger's shake with an animated offset.
View Shaken(View content, const ShakeTrigger& shake);

/// Pipeable adaptor for Shaken.
[[nodiscard]] inline ViewAdaptor Shaken(const ShakeTrigger& shake) {
  return ViewAdaptor([shake](View content) { return Shaken(std::move(content), shake); });
}

} // namespace huxerui::interaction
