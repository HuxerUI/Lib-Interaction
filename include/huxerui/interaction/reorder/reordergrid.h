#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

#include <concepts>
#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
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
  /// When true, dragging starts only from a nested ReorderHandle inside each cell rather than the whole cell.
  bool handle_only = false;
  /// Enables Alt + arrow keyboard reordering on a focused cell.
  bool keyboard_reorder = true;
  /// Distance from a viewport edge that drives per-drag-update auto-scroll. Empty disables auto-scroll.
  std::optional<float> auto_scroll_edge = 24.0F;
  /// Scroll delta requested per drag update while near an edge. Must be finite and positive.
  float auto_scroll_step = 24.0F;
  /// Controller bound to the enclosing ScrollView. Auto-scroll is skipped when disconnected.
  ScrollController scroll_controller{};

  bool operator==(const ReorderableGridStyle&) const = default;
};

namespace detail {
View ReorderableGridImpl(std::vector<View> items, std::function<void(std::size_t, std::size_t)> on_reordered,
                         ReorderableGridStyle style);
} // namespace detail

/// Visual-only ReorderableGrid overload.
inline View ReorderableGrid(std::vector<View> items, ReorderableGridStyle style = {}) {
  return detail::ReorderableGridImpl(std::move(items), {}, style);
}

/// Composes fixed-size cells in a wrapping grid with drag-to-reorder and optional Alt+arrow keyboard reordering.
/// @throws std::invalid_argument If cell dimensions, gap, highlight width, or auto-scroll settings are invalid.
template <std::invocable<std::size_t, std::size_t> F>
View ReorderableGrid(std::vector<View> items, F&& on_reordered, ReorderableGridStyle style = {}) {
  return detail::ReorderableGridImpl(
      std::move(items), std::function<void(std::size_t, std::size_t)>(std::forward<F>(on_reordered)), style);
}

} // namespace huxerui::interaction
