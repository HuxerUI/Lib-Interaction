#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/view.h>

#include <chrono>
#include <functional>
#include <optional>

namespace huxerui::interaction {

/// Configuration for DoubleClickable. Recognition follows the platform GestureSettings unless overridden per
/// widget; the gesture recognizes consecutive taps without replacing ordinary Click handling.
struct DoubleClickableStyle {
  /// Maximum elapsed time between the two taps. An empty value uses the platform multi-tap interval. Must be
  /// positive when set.
  std::optional<std::chrono::duration<double>> maximum_interval = std::nullopt;
  /// Window-space movement tolerance between the two taps in logical pixels. An empty value uses the platform
  /// multi-tap slop. Must be finite and positive when set.
  std::optional<float> maximum_movement = std::nullopt;
  /// Cursor shown over the content. Default keeps the platform cursor.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  bool operator==(const DoubleClickableStyle&) const = default;
};

/// Composes content that recognizes a double-tap sequence and reports it through the callback.
///
/// Recognition uses the platform multi-tap recognizer, so ordinary Click events and sibling gestures remain
/// unaffected; the callback fires once per completed two-tap sequence. There is no built-in press presentation;
/// compose Pressable at the call site when double-click targets also need hover and press feedback.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted gesture binding by scope position. The widget imposes no intrinsic frame.
/// @param content Content displayed inside the interactive surface.
/// @param style Recognition overrides and cursor. Must satisfy the documented ranges.
/// @param on_double_click Callback invoked once when the two-tap sequence completes with the final tap event, or
///     an empty function to compose a purely visual surface. The event position is node-local to the composed
///     surface.
/// @return A View whose mounted instance owns the multi-tap binding.
/// @throws std::invalid_argument If the interval override is not positive or the movement override is not finite
///     and positive.
/// @code
/// DoubleClickable(Card(), {}, [](const MultiTapEvent& event) { OpenAt(event.position); })
///     .With(Frame{.width = 200.0F, .height = 140.0F});
/// @endcode
View DoubleClickable(View content, DoubleClickableStyle style = {},
                     std::function<void(const MultiTapEvent&)> on_double_click = {});

} // namespace huxerui::interaction
