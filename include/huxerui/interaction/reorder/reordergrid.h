#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>
#include <vector>

namespace huxerui::interaction {

/// Configuration for ReorderableGrid.
struct ReorderableGridStyle {
  /// Cell width in logical pixels. Must be finite and positive.
  float cell_width = 160.0F;
  /// Cell height in logical pixels. Must be finite and positive.
  float cell_height = 120.0F;
  /// Gap between cells in logical pixels. Must be finite and non-negative.
  float spacing = 12.0F;
  /// Motion used by displaced cells easing to their preview cells.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.18, .easing = Easing::EaseOut};
  /// Motion used by the dragged cell following the pointer.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Outline color drawn over the preview target cell.
  Color target_highlight = Color::Rgb(31, 111, 235, 0.9F);
  /// Outline width of the preview target highlight. Must be finite and non-negative.
  float target_highlight_width = 2.0F;
  bool operator==(const ReorderableGridStyle&) const = default;
};

/// Composes fixed-size cells in a wrapping grid with drag-to-reorder.
///
/// Cells keep layout order while a drag runs: the dragged cell follows the pointer, cells between source and target
/// ease one linear position aside, and the closest slot draws the target outline. Dropping calls on_reordered with
/// source and destination linear indices. The caller recomposes with the model's new order. The widget imposes no
/// intrinsic frame beyond its cells.
/// @param items One view per cell in display order.
/// @param style Cell size, gap, motions, and target highlight.
/// @param on_reordered Callback invoked on drop with original and target linear indices.
/// @return A Flow of fixed-size cells.
/// @throws std::invalid_argument If a cell dimension, gap, or highlight width is invalid.
View ReorderableGrid(std::vector<View> items, ReorderableGridStyle style = {},
                     std::function<void(std::size_t from, std::size_t to)> on_reordered = {});

} // namespace huxerui::interaction
