#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Motion configuration for DragSpring. The return motion also serves as the press-follow smoothing policy swap:
/// while dragging, the widget retargets with a short linear tween, and releasing swaps in the configured spring.
struct DragSpringStyle {
  /// Constrained drag axis, or an empty value for free two-dimensional movement.
  std::optional<Axis> axis = std::nullopt;
  /// Motion used to return the content to its rest position after the drag ends or cancels.
  AnimationSpec return_motion = SpringSpec{.stiffness = 180.0F, .damping_ratio = 0.75F};
  /// Cursor shown over the content at rest. An active drag always shows the grabbing cursor.
  PointerCursorKind cursor = PointerCursorKind::Grab;
  bool operator==(const DragSpringStyle&) const = default;
};

/// Composes content that follows a recognized drag gesture and springs back to its rest position on release.
///
/// While the drag runs, the offset target tracks the gesture translation with a short linear tween, so movement
/// feels attached to the pointer; when the sequence ends or cancels, the target returns to the origin with the
/// configured spring. Terminal drag velocity is not carried into the spring in this version; the spring starts
/// from the released position at rest velocity.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted drag state by scope position. The widget imposes no intrinsic frame.
/// @param content Content that the drag moves. Hit testing follows the content's own bounds.
/// @param style Drag axis and return motion.
/// @return A View whose mounted instance owns the drag and offset state.
/// @code
/// DragSpring(Chip(), {.axis = Axis::Horizontal}).With(Frame{.width = 240.0F, .height = 56.0F});
/// @endcode
View DragSpring(View content, DragSpringStyle style = {});

} // namespace huxerui::interaction
