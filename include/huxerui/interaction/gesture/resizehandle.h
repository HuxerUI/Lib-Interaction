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
///
/// The handle reports incremental drag deltas along `axis`; the caller owns pane sizes and clamping. Place it between
/// panes in a Row or Column whose cross-axis alignment stretches the handle, and set the handle thickness through
/// ResizeHandleStyle. SplitLayout remains the higher-level two-pane solution; this primitive is for table columns,
/// resizable sidebars, and other custom arrangements.
/// @param axis Drag axis. Horizontal reports x deltas; Vertical reports y deltas.
/// @param on_resize Callback with the incremental delta each accepted move.
/// @param style Thickness and colors.
/// @return A fixed-thickness draggable separator.
/// @throws std::invalid_argument If thickness is invalid.
View ResizeHandle(Axis axis, std::function<void(float delta)> on_resize, ResizeHandleStyle style = {});

} // namespace huxerui::interaction
