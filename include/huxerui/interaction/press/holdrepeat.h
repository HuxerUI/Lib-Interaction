#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/semantics.h>
#include <huxerui/view.h>

#include <chrono>
#include <concepts>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// Live hold and repeat state exposed to headless HoldRepeat builders.
struct HoldRepeatSnapshot {
  /// True while a hover-capable pointer is inside the host bounds.
  bool hovered = false;
  /// True while an active pointer or keyboard hold owns the control.
  bool holding = false;
  /// True while the control holds keyboard focus.
  bool focused = false;
  /// True once the initial delay has elapsed and repeating ticks are firing.
  bool repeating = false;
  /// 1-based number of triggers emitted in the current hold session; resets to 0 on release or cancel.
  std::size_t trigger_count = 0;

  bool operator==(const HoldRepeatSnapshot&) const = default;
};

struct HoldRepeatStyle;

namespace detail {
struct HoldRepeatControllerState;

View HoldRepeatImpl(std::function<View(HoldRepeatSnapshot)> builder,
                    std::function<void(std::size_t)> on_trigger, HoldRepeatStyle style,
                    std::function<void(bool)> on_holding_changed);
} // namespace detail

/// Copyable handle for observing or canceling an active HoldRepeat session.
/// Default-constructed handles are unbound and safe outside an Application runtime; bind one with
/// UseHoldRepeatController().
class HoldRepeatController {
public:
  HoldRepeatController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// True while a hold session is active (subscribes active composition).
  [[nodiscard]] bool IsHolding() const;

  /// True while repeating timer ticks are running after the initial delay (subscribes active composition).
  [[nodiscard]] bool IsRepeating() const;

  /// 1-based trigger count during the current hold session, or 0 when idle (subscribes active composition).
  [[nodiscard]] std::size_t TriggerCount() const;

  /// Stops any active hold and resets controller state without firing further triggers.
  void Cancel() const;

  bool operator==(const HoldRepeatController&) const = default;

private:
  explicit HoldRepeatController(std::shared_ptr<detail::HoldRepeatControllerState> state)
      : state_(std::move(state)) {}

  std::shared_ptr<detail::HoldRepeatControllerState> state_;

  friend HoldRepeatController UseHoldRepeatController();
  friend View detail::HoldRepeatImpl(std::function<View(HoldRepeatSnapshot)> builder,
                                     std::function<void(std::size_t)> on_trigger, HoldRepeatStyle style,
                                     std::function<void(bool)> on_holding_changed);
};

/// Remembers a HoldRepeatController in the current composition scope.
HoldRepeatController UseHoldRepeatController();

/// Timing, acceleration, input, and default wrapper scale configuration for HoldRepeat.
struct HoldRepeatStyle {
  /// Delay before the first repeating trigger fires while holding. Must be finite and positive.
  std::chrono::duration<double> initial_delay{0.4};
  /// Initial delay between repeat triggers once repeating begins. Must be finite and positive.
  std::chrono::duration<double> repeat_interval{0.12};
  /// Lower bound when `acceleration < 1.0`. Must be finite, positive, and <= `repeat_interval` when set.
  /// Unset defaults to `std::min(repeat_interval, 0.03s)` when accelerating.
  std::optional<std::chrono::duration<double>> minimum_interval = std::nullopt;
  /// Geometric interval multiplier applied after each repeat step. Must be finite and in `(0, 1]`.
  float acceleration = 1.0F;
  /// Whether pressing down emits trigger #1 immediately before `initial_delay`.
  bool fire_on_press = true;
  /// Whether moving a captured pointer outside host bounds stops the active hold.
  bool cancel_on_leave = true;
  /// Scale applied while hovered (wrapped overload). Must be finite and positive.
  float hovered_scale = 1.03F;
  /// Scale applied while holding (wrapped overload). Must be finite and positive.
  float pressed_scale = 0.96F;
  /// Scale transition curve used by the wrapped overload.
  AnimationSpec animation = TweenSpec{.duration = 0.14, .easing = Easing::EaseOut};
  /// Pivot used by the wrapped scale transform. Defaults to content center.
  TransformOrigin origin = {};
  /// Cursor shown over the host surface.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// Whether Space and Enter repeat triggers while focused.
  bool keyboard_focusable = true;
  /// Semantic role published when actionable (`role` non-empty).
  std::optional<SemanticRole> role = SemanticRole::Button;
  /// Optional external controller bound to this HoldRepeat instance.
  HoldRepeatController controller{};

  bool operator==(const HoldRepeatStyle&) const = default;
};

namespace detail {

template <class F>
concept HoldRepeatAction =
    !std::convertible_to<std::remove_cvref_t<F>, View> &&
    ((std::invocable<std::remove_cvref_t<F>&> &&
      !std::convertible_to<std::invoke_result_t<std::remove_cvref_t<F>&>, View>) ||
     (std::invocable<std::remove_cvref_t<F>&, std::size_t> &&
      !std::convertible_to<std::invoke_result_t<std::remove_cvref_t<F>&, std::size_t>, View>));

template <class F>
std::function<void(std::size_t)> AdaptHoldRepeatAction(F&& action) {
  using Decayed = std::decay_t<F>;
  Decayed fn = std::forward<F>(action);
  if constexpr(requires(const Decayed& f) {
                 { f == nullptr } -> std::convertible_to<bool>;
               }) {
    if(fn == nullptr) {
      return {};
    }
  }
  if constexpr(std::invocable<Decayed&, std::size_t>) {
    return std::function<void(std::size_t)>(std::move(fn));
  } else {
    return [cb = std::move(fn)](std::size_t) mutable { cb(); };
  }
}

View HoldRepeatWrapImpl(View content, std::function<void(std::size_t)> on_trigger, HoldRepeatStyle style,
                        std::function<void(bool)> on_holding_changed);

} // namespace detail

/// Visual-only headless HoldRepeat overload delegating presentation to `builder(HoldRepeatSnapshot)`.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds.
inline View HoldRepeat(std::function<View(HoldRepeatSnapshot)> builder, HoldRepeatStyle style = {}) {
  return detail::HoldRepeatImpl(std::move(builder), {}, std::move(style), {});
}

/// Headless press-and-hold repeating trigger delegating visuals to `builder(HoldRepeatSnapshot)`.
/// Accepts either `void()` or `void(std::size_t trigger_count)` trigger callbacks.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds.
template <detail::HoldRepeatAction F>
View HoldRepeat(std::function<View(HoldRepeatSnapshot)> builder, F&& on_trigger, HoldRepeatStyle style = {},
                std::function<void(bool)> on_holding_changed = {}) {
  return detail::HoldRepeatImpl(std::move(builder), detail::AdaptHoldRepeatAction(std::forward<F>(on_trigger)),
                                std::move(style), std::move(on_holding_changed));
}

/// Visual-only wrapped HoldRepeat overload.
/// @throws std::invalid_argument If `style` contains invalid bounds.
inline View HoldRepeat(View content, HoldRepeatStyle style = {}) {
  return detail::HoldRepeatWrapImpl(std::move(content), {}, std::move(style), {});
}

/// Wraps `content` in a scale-animated hold-repeat trigger invoking `on_trigger` on press and repeat ticks.
/// Accepts either `void()` or `void(std::size_t trigger_count)` trigger callbacks.
/// @throws std::invalid_argument If `style` contains invalid bounds.
template <detail::HoldRepeatAction F>
View HoldRepeat(View content, F&& on_trigger, HoldRepeatStyle style = {},
                std::function<void(bool)> on_holding_changed = {}) {
  return detail::HoldRepeatWrapImpl(std::move(content), detail::AdaptHoldRepeatAction(std::forward<F>(on_trigger)),
                                    std::move(style), std::move(on_holding_changed));
}

/// Pipeable adaptor for visual-only HoldRepeat.
[[nodiscard]] inline ViewAdaptor HoldRepeat(HoldRepeatStyle style = {}) {
  return ViewAdaptor([style = std::move(style)](View content) {
    return detail::HoldRepeatWrapImpl(std::move(content), {}, style, {});
  });
}

/// Pipeable adaptor for actionable HoldRepeat (`void()` or `void(std::size_t)`).
template <detail::HoldRepeatAction F>
[[nodiscard]] ViewAdaptor HoldRepeat(F&& on_trigger, HoldRepeatStyle style = {},
                                     std::function<void(bool)> on_holding_changed = {}) {
  std::function<void(std::size_t)> adapted = detail::AdaptHoldRepeatAction(std::forward<F>(on_trigger));
  return ViewAdaptor(
      [adapted = std::move(adapted), style = std::move(style), on_holding = std::move(on_holding_changed)](
          View content) { return detail::HoldRepeatWrapImpl(std::move(content), adapted, style, on_holding); });
}

} // namespace huxerui::interaction
