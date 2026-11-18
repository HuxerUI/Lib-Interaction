#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <functional>
#include <utility>

namespace huxerui::interaction {

/// Headless hover state primitive that tracks pointer hover presence and delegates visuals to `builder`.
/// @param builder Invoked during composition with the current `hovered` state. Must not be empty.
/// @param cursor Optional cursor shown over the hovered surface.
/// @throws std::invalid_argument If `builder` is empty.
View Hoverable(std::function<View(bool hovered)> builder,
               PointerCursorKind cursor = PointerCursorKind::Default);

/// Motion configuration for HoverLift. Defaults produce a small upward drift with a gentle grow.
struct HoverLiftStyle {
  /// Offset applied while hovered, relative to the rest position. Positive values move down.
  Point lift = {0.0F, -4.0F};
  /// Scale applied while hovered. Rest state is 1.0.
  float hovered_scale = 1.02F;
  /// Motion used when the hovered presentation changes, including the return to rest.
  AnimationSpec animation = TweenSpec{.duration = 0.22, .easing = Easing::EaseOut};
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};

  bool operator==(const HoverLiftStyle&) const = default;
};

/// Composes content that drifts by `lift` and grows to `hovered_scale` while a hover-capable pointer targets it.
/// @throws std::invalid_argument If lift or scale is non-finite, or the scale is not positive.
View HoverLift(View content, HoverLiftStyle style = {});

/// Pipeable adaptor for HoverLift.
[[nodiscard]] inline ViewAdaptor HoverLift(HoverLiftStyle style = {}) {
  return ViewAdaptor([style](View content) { return HoverLift(std::move(content), style); });
}

} // namespace huxerui::interaction
