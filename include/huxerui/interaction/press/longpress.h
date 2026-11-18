#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <chrono>
#include <concepts>
#include <functional>
#include <optional>
#include <type_traits>
#include <utility>

namespace huxerui::interaction {

/// Paint configuration for HoldProgressRing.
struct HoldProgressStyle {
  /// Foreground arc color; the background track uses this color at reduced alpha.
  Color color = Color::Rgb(255, 255, 255, 0.9F);
  /// Seconds required for a primary pointer hold to fill the full circle. Must be finite and positive.
  double duration = 0.5;
  /// Ring stroke width in logical pixels. Must be finite and positive.
  float stroke_width = 3.0F;
  /// Radial inset from the half-min-dimension of the host node bounds. Must be finite and non-negative.
  float inset = 8.0F;

  bool operator==(const HoldProgressStyle&) const = default;
};

/// A retained modifier that paints a circular hold-progress ring above node content without consuming pointer events.
class HoldProgressRing {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the progress ring configuration.
  /// @throws std::invalid_argument If duration or stroke_width is not positive, or inset is negative.
  explicit HoldProgressRing(HoldProgressStyle style = {});

  HoldProgressStyle style;

  bool operator==(const HoldProgressRing&) const = default;
};

/// Presentation configuration for LongPressable.
struct LongPressableStyle {
  /// Time the pointer must remain within movement tolerance before recognition. Empty uses platform default.
  std::optional<std::chrono::duration<double>> minimum_duration = std::nullopt;
  /// Node-local movement tolerance override in logical pixels. Empty uses platform pointer slop.
  std::optional<float> maximum_movement = std::nullopt;
  /// Scale applied while a recognized long press owns the pointer. Rest state is 1.0. Must be finite and positive.
  float held_scale = 0.97F;
  /// Motion used when the scale target changes, including the return to rest.
  AnimationSpec animation = TweenSpec{.duration = 0.16, .easing = Easing::EaseOut};
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};
  /// Cursor shown over the content.
  PointerCursorKind cursor = PointerCursorKind::Default;
  /// Whether to attach a HoldProgressRing while the hold runs.
  bool show_progress = false;
  /// Ring color used when `show_progress` is true.
  Color progress_color = Color::Rgb(255, 255, 255, 0.9F);
  /// Ring stroke width used when `show_progress` is true. Must be finite and positive.
  float progress_stroke_width = 3.0F;
  /// Ring inset used when `show_progress` is true. Must be finite and non-negative.
  float progress_inset = 8.0F;

  bool operator==(const LongPressableStyle&) const = default;
};

namespace detail {
View LongPressableImpl(View content, std::function<void()> on_long_press, LongPressableStyle style,
                       std::function<void(bool)> on_holding_changed = {});
} // namespace detail

/// Visual-only LongPressable overload.
inline View LongPressable(View content, LongPressableStyle style = {}) {
  return detail::LongPressableImpl(std::move(content), {}, style);
}

/// Composes content that recognizes a press-and-hold gesture and invokes `on_long_press` once at recognition.
template <std::invocable F>
View LongPressable(View content, F&& on_long_press, LongPressableStyle style = {},
                   std::function<void(bool)> on_holding_changed = {}) {
  return detail::LongPressableImpl(std::move(content), std::function<void()>(std::forward<F>(on_long_press)), style,
                                   std::move(on_holding_changed));
}

/// Pipeable adaptor for visual-only LongPressable.
[[nodiscard]] inline ViewAdaptor LongPressable(LongPressableStyle style = {}) {
  return ViewAdaptor([style](View content) { return detail::LongPressableImpl(std::move(content), {}, style); });
}

/// Pipeable adaptor for actionable LongPressable.
template <std::invocable F>
  requires(!std::convertible_to<std::remove_cvref_t<F>, View>)
[[nodiscard]] ViewAdaptor LongPressable(F&& on_long_press, LongPressableStyle style = {},
                                        std::function<void(bool)> on_holding_changed = {}) {
  std::function<void()> callback(std::forward<F>(on_long_press));
  return ViewAdaptor(
      [callback = std::move(callback), style, on_holding = std::move(on_holding_changed)](View content) {
        return detail::LongPressableImpl(std::move(content), callback, style, on_holding);
      });
}

} // namespace huxerui::interaction
