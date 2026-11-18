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

/// Presentation and interaction configuration for Reorderable.
struct ReorderableStyle {
  /// Layout and drag axis. Vertical composes Column; Horizontal composes Row.
  Axis axis = Axis::Vertical;
  /// Space between consecutive slots, in logical pixels. Must be finite and non-negative.
  float spacing = 12.0F;
  /// Motion used by displaced items easing to their preview slots and by the dragged item settling back on drop.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.18, .easing = Easing::EaseOut};
  /// Motion used by the dragged item following the pointer.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Outline color drawn over the preview target slot while a drag runs.
  Color target_highlight = Color::Rgb(31, 111, 235, 0.9F);
  /// Outline width of the preview target highlight. Must be finite and non-negative.
  float target_highlight_width = 2.0F;
  /// When true, dragging starts only from a nested ReorderHandle inside each item rather than the whole slot.
  bool handle_only = false;
  /// Enables Alt + arrow keyboard reordering on a focused slot.
  bool keyboard_reorder = true;
  /// Distance from a viewport edge that drives per-drag-update auto-scroll. Empty disables auto-scroll.
  std::optional<float> auto_scroll_edge = 24.0F;
  /// Scroll delta requested per drag update while near an edge. Must be finite and positive.
  float auto_scroll_step = 24.0F;
  /// Controller bound to the enclosing ScrollView. Auto-scroll is skipped when disconnected.
  ScrollController scroll_controller{};

  bool operator==(const ReorderableStyle&) const = default;
};

namespace detail {
View ReorderableImpl(std::vector<View> items, std::function<void(std::size_t, std::size_t)> on_reordered,
                     ReorderableStyle style);
} // namespace detail

/// Visual-only Reorderable overload.
inline View Reorderable(std::vector<View> items, ReorderableStyle style = {}) {
  return detail::ReorderableImpl(std::move(items), {}, style);
}

/// Composes items in a row or column that supports drag-to-reorder and optional Alt+arrow keyboard reordering.
/// @throws std::invalid_argument If spacing, highlight width, auto-scroll edge, or auto-scroll step is invalid.
template <std::invocable<std::size_t, std::size_t> F>
View Reorderable(std::vector<View> items, F&& on_reordered, ReorderableStyle style = {}) {
  return detail::ReorderableImpl(
      std::move(items), std::function<void(std::size_t, std::size_t)>(std::forward<F>(on_reordered)), style);
}

} // namespace huxerui::interaction
