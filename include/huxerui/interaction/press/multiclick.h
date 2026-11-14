#pragma once

#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/view.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

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
  bool operator==(const MultiClickableStyle&) const = default;
};

/// Recognizes an arbitrary repeated-tap count without replacing ordinary Click handling.
///
/// This is the generalized form of DoubleClickable. The callback receives the final MultiTapEvent once per completed
/// sequence; ordinary Click and sibling gestures remain available. The widget imposes no intrinsic frame.
/// @param content Content displayed in the interactive surface.
/// @param style Tap count, timing, movement, and cursor.
/// @param on_recognized Callback invoked once per completed sequence, or an empty function for a visual-only wrapper.
/// @return A View carrying the MultiTapGesture binding.
/// @throws std::invalid_argument If count is below two, the interval is non-positive, or movement is invalid.
View MultiClickable(View content, MultiClickableStyle style = {},
                    std::function<void(const MultiTapEvent&)> on_recognized = {});

} // namespace huxerui::interaction
