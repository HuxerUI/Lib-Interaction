#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>

namespace huxerui::interaction {

/// Configuration for ResizeHandle.
struct ResizeHandleStyle {
  /// Handle thickness along the cross axis. Must be finite and positive.
  float thickness = 8.0F;
  /// Resting handle color.
  Color color = Color::Rgb(31, 111, 235, 0.24F);
  /// Handle color while pressed.
  Color pressed_color = Color::Rgb(31, 111, 235, 0.72F);

  bool operator==(const ResizeHandleStyle&) const = default;
};

/// A draggable separator for custom row/column layouts.
/// @throws std::invalid_argument If thickness is non-finite or not positive.
View ResizeHandle(Axis axis, std::function<void(float delta)> on_resize, ResizeHandleStyle style = {});

} // namespace huxerui::interaction
