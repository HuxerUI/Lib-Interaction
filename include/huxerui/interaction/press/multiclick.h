#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// Configuration for MultiClickable.
struct MultiClickableStyle {
  /// Required tap count. Must be at least two.
  std::uint32_t count = 3;
  /// Per-gesture interval override; empty uses the platform multi-tap interval.
  std::optional<std::chrono::duration<double>> maximum_interval = std::nullopt;
  /// Per-gesture movement tolerance override; empty uses the platform multi-tap slop.
  std::optional<float> maximum_movement = std::nullopt;
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// Whether providing a callback publishes SemanticRole::Button by default.
  bool button_semantics = true;

  bool operator==(const MultiClickableStyle&) const = default;
};

/// Configuration for DoubleClickable (fixed two-tap convenience over MultiClickable).
struct DoubleClickableStyle {
  /// Maximum elapsed time between the two taps. Empty uses the platform multi-tap interval.
  std::optional<std::chrono::duration<double>> maximum_interval = std::nullopt;
  /// Movement tolerance between the two taps in logical pixels. Empty uses the platform multi-tap slop.
  std::optional<float> maximum_movement = std::nullopt;
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  /// Whether providing a callback publishes SemanticRole::Button by default.
  bool button_semantics = true;

  bool operator==(const DoubleClickableStyle&) const = default;
};

namespace detail {
View MultiClickableImpl(View content, std::function<void(const MultiTapEvent&)> on_recognized,
                        MultiClickableStyle style);
View DoubleClickableImpl(View content, std::function<void(const MultiTapEvent&)> on_double_click,
                         DoubleClickableStyle style);
} // namespace detail

/// Visual-only MultiClickable overload.
inline View MultiClickable(View content, MultiClickableStyle style = {}) {
  return detail::MultiClickableImpl(std::move(content), {}, style);
}

/// Recognizes an arbitrary repeated-tap count (>= 2) without replacing ordinary Click handling.
template <std::invocable<const MultiTapEvent&> F>
View MultiClickable(View content, F&& on_recognized, MultiClickableStyle style = {}) {
  return detail::MultiClickableImpl(
      std::move(content), std::function<void(const MultiTapEvent&)>(std::forward<F>(on_recognized)), style);
}

/// Pipeable adaptor for visual-only MultiClickable.
[[nodiscard]] inline ViewAdaptor MultiClickable(MultiClickableStyle style = {}) {
  return ViewAdaptor([style](View content) { return detail::MultiClickableImpl(std::move(content), {}, style); });
}

/// Pipeable adaptor for actionable MultiClickable.
template <std::invocable<const MultiTapEvent&> F>
  requires(!std::convertible_to<std::remove_cvref_t<F>, View>)
[[nodiscard]] ViewAdaptor MultiClickable(F&& on_recognized, MultiClickableStyle style = {}) {
  std::function<void(const MultiTapEvent&)> callback(std::forward<F>(on_recognized));
  return ViewAdaptor([callback = std::move(callback), style](View content) {
    return detail::MultiClickableImpl(std::move(content), callback, style);
  });
}

/// Visual-only DoubleClickable overload.
inline View DoubleClickable(View content, DoubleClickableStyle style = {}) {
  return detail::DoubleClickableImpl(std::move(content), {}, style);
}

/// Recognizes a two-tap sequence without replacing ordinary Click handling.
template <std::invocable<const MultiTapEvent&> F>
View DoubleClickable(View content, F&& on_double_click, DoubleClickableStyle style = {}) {
  return detail::DoubleClickableImpl(
      std::move(content), std::function<void(const MultiTapEvent&)>(std::forward<F>(on_double_click)), style);
}

/// Pipeable adaptor for visual-only DoubleClickable.
[[nodiscard]] inline ViewAdaptor DoubleClickable(DoubleClickableStyle style = {}) {
  return ViewAdaptor([style](View content) { return detail::DoubleClickableImpl(std::move(content), {}, style); });
}

/// Pipeable adaptor for actionable DoubleClickable.
template <std::invocable<const MultiTapEvent&> F>
  requires(!std::convertible_to<std::remove_cvref_t<F>, View>)
[[nodiscard]] ViewAdaptor DoubleClickable(F&& on_double_click, DoubleClickableStyle style = {}) {
  std::function<void(const MultiTapEvent&)> callback(std::forward<F>(on_double_click));
  return ViewAdaptor([callback = std::move(callback), style](View content) {
    return detail::DoubleClickableImpl(std::move(content), callback, style);
  });
}

} // namespace huxerui::interaction
