#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Motion configuration for DragSpring.
struct DragSpringStyle {
  /// Constrained drag axis, or empty for free two-dimensional movement.
  std::optional<Axis> axis = std::nullopt;
  /// Motion used while the pointer drags the content.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used to return the content to its rest position after the drag ends or cancels.
  AnimationSpec return_motion = SpringSpec{.stiffness = 180.0F, .damping_ratio = 0.75F};
  /// Cursor shown over the content at rest. An active drag always shows the grabbing cursor.
  PointerCursorKind cursor = PointerCursorKind::Grab;

  bool operator==(const DragSpringStyle&) const = default;
};

/// Composes content that follows a recognized drag gesture and springs back to rest on release.
View DragSpring(View content, DragSpringStyle style = {});

/// Pipeable adaptor for DragSpring.
[[nodiscard]] inline ViewAdaptor DragSpring(DragSpringStyle style = {}) {
  return ViewAdaptor([style](View content) { return DragSpring(std::move(content), style); });
}

} // namespace huxerui::interaction
