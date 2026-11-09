#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

#include <functional>
#include <vector>

namespace huxerui::interaction {

/// Presentation configuration for Reorderable. Layout never changes while a drag runs: the dragged item follows
/// the pointer, the items between ease one slot aside, and the model reorders only at drop.
struct ReorderableStyle {
  /// Space between consecutive slots, in logical pixels. Must be finite and non-negative.
  float spacing = 12.0F;
  /// Motion used by displaced items easing to their preview slots and by the dragged item settling back on drop.
  AnimationSpec settle_motion = TweenSpec{.duration = 0.18, .easing = Easing::EaseOut};
  /// Motion used by the dragged item following the pointer.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Outline color drawn over the preview target slot while a drag runs. A fully transparent color disables the
  /// highlight.
  Color target_highlight = Color::Rgb(31, 111, 235, 0.9F);
  /// Outline width of the preview target highlight. Must be finite and non-negative.
  float target_highlight_width = 2.0F;
  bool operator==(const ReorderableStyle&) const = default;
};

/// Composes items in one column that supports drag-to-reorder.
///
/// While a drag runs, the layout stays in the original order — the dragged item follows the pointer from its own
/// slot, the items between it and the pointer's closest slot ease one position aside, and the closest slot draws
/// the target highlight. Dropping fires the callback once with the source and destination indices; equal indices
/// mean the item returned to its own slot. The items vector is display input: after the callback, recompose with
/// the model's new order and the drag session is already reset.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted drag session by scope position. Slot geometry is recorded during layout preparation, so
/// preview targeting works while displaced items are still animating. The widget imposes no intrinsic frame;
/// supply bounds through a parent or Frame.
/// @param items One view per slot, in display order. The widget never mutates the vector.
/// @param style Spacing, motions, and preview highlight. Must satisfy the documented ranges.
/// @param on_reordered Callback invoked on drop with the original and previewed slot indices, or an empty
///     function to compose a purely visual reorder surface.
/// @return A View whose mounted instance owns the drag session and slot geometry.
/// @throws std::invalid_argument If spacing or the highlight width is non-finite or negative.
/// @code
/// auto order = UseState<std::vector<int>>({1, 2, 3});
/// return Reorderable(BuildTiles(order.Get()), {}, [order](std::size_t from, std::size_t to) {
///   auto next = order.Get();
///   Rotate(next, from, to);
///   order = std::move(next);
/// }).With(Frame{.width = 320.0F});
/// @endcode
View Reorderable(std::vector<View> items, ReorderableStyle style = {},
                 std::function<void(std::size_t from, std::size_t to)> on_reordered = {});

} // namespace huxerui::interaction
