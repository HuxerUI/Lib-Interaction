#pragma once

#include <huxerui/geometry.h>
#include <huxerui/modifier.h>
#include <huxerui/view.h>

#include <vector>

namespace huxerui::interaction {

/// Configuration for RovingFocus.
struct RovingFocusStyle {
  /// Arrow-key axis.
  Axis axis = Axis::Horizontal;
  /// Whether arrow keys wrap at the ends.
  bool wrap = true;
  /// Gap between items in logical pixels. Must be finite and non-negative.
  float spacing = 0.0F;
  bool operator==(const RovingFocusStyle&) const = default;
};

/// Wraps focusable items in a roving-focus group.
///
/// Every item receives Focusable and a stable FocusRequester; arrow keys on the focus route move focus along the
/// configured axis. The caller should keep item count stable and provide ordinary focusable content. The wrapper
/// imposes no intrinsic frame.
/// @param items Focusable item views.
/// @param style Axis, wrap behavior, and spacing.
/// @return A Row or Column with the roving key binding.
/// @throws std::invalid_argument If spacing is invalid.
View RovingFocus(std::vector<View> items, RovingFocusStyle style = {});

} // namespace huxerui::interaction
