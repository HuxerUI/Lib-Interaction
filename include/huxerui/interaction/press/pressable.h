#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <concepts>
#include <functional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// Motion configuration for Pressable. Defaults produce a subtle hover emphasis and a light press-in.
struct PressableStyle {
  /// Scale applied while a hover-capable pointer targets the content. Rest state is 1.0.
  float hovered_scale = 1.03F;
  /// Scale applied while a primary pointer press or focused Space press is active. Rest state is 1.0.
  float pressed_scale = 0.97F;
  /// Motion used when the effective scale target changes, including the return to rest.
  AnimationSpec animation = TweenSpec{.duration = 0.16, .easing = Easing::EaseOut};
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};
  /// Cursor shown over the content. Default shows the hand cursor.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// Whether an actionable surface participates in keyboard focus and Space/Enter activation.
  bool keyboard_focusable = true;

  bool operator==(const PressableStyle&) const = default;
};

namespace detail {
View PressableImpl(View content, std::function<void()> on_click, PressableStyle style);
} // namespace detail

/// Composes content that responds to hover and primary press with an animated scale around one pivot (visual-only).
/// @throws std::invalid_argument If either scale is non-finite or not positive.
inline View Pressable(View content, PressableStyle style = {}) {
  return detail::PressableImpl(std::move(content), {}, style);
}

/// Composes an actionable pressable surface with click, keyboard Enter/Space activation, and Button semantics.
/// @param content Content displayed inside the interactive surface. The view imposes no intrinsic frame.
/// @param on_click Activation callback invoked on primary click or keyboard Enter/Space.
/// @param style Scale targets, motion, cursor, and focus configuration.
/// @return A View whose mounted instance owns the hover and press presentation state.
/// @throws std::invalid_argument If either scale is non-finite or not positive.
template <std::invocable F>
View Pressable(View content, F&& on_click, PressableStyle style = {}) {
  return detail::PressableImpl(std::move(content), std::function<void()>(std::forward<F>(on_click)), style);
}

/// Pipeable adaptor for visual-only Pressable.
[[nodiscard]] inline ViewAdaptor Pressable(PressableStyle style = {}) {
  return ViewAdaptor([style](View content) { return detail::PressableImpl(std::move(content), {}, style); });
}

/// Pipeable adaptor for actionable Pressable.
template <std::invocable F>
  requires(!std::convertible_to<std::remove_cvref_t<F>, View>)
[[nodiscard]] ViewAdaptor Pressable(F&& on_click, PressableStyle style = {}) {
  std::function<void()> callback(std::forward<F>(on_click));
  return ViewAdaptor([callback = std::move(callback), style](View content) {
    return detail::PressableImpl(std::move(content), callback, style);
  });
}

} // namespace huxerui::interaction
