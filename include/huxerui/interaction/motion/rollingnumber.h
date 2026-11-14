#pragma once

#include <huxerui/animation.h>
#include <huxerui/text.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Configuration for RollingNumber.
struct RollingNumberStyle {
  /// Typography used for every digit and sign.
  TextStyle text_style = TextStyle::Default();
  /// Motion used when a digit column changes target.
  AnimationSpec motion = TweenSpec{.duration = 0.35, .easing = Easing::EaseOut};
  /// Gap between digit windows in logical pixels. Must be finite and non-negative.
  float digit_gap = 0.0F;
  /// Minimum number of digit windows before the sign. Zero keeps the natural width. Must be non-negative.
  int minimum_digits = 0;
  bool operator==(const RollingNumberStyle&) const = default;
};

/// Renders an integer through vertically rolling digit windows.
///
/// Each digit is a clipped column of zero through nine; changing the value retargets the column offset and the
/// existing animation eases from the currently visible digit. The widget measures digit advances and line height
/// through UseTextMeasurer, so no frame metric must be supplied by the caller. The widget imposes no intrinsic frame.
/// @param value Integer to display, including a leading minus sign when negative.
/// @param style Typography, motion, spacing, and minimum digits.
/// @return A Row of clipped digit windows.
/// @throws std::invalid_argument If digit_gap or minimum_digits is invalid.
/// @code
/// RollingNumber(count, {.motion = TweenSpec{.duration = 0.24}});
/// @endcode
View RollingNumber(int value, RollingNumberStyle style = {});

} // namespace huxerui::interaction
