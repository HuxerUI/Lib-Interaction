#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

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

/// Composes content that drifts by lift and grows to hovered_scale while a hover-capable pointer targets it.
///
/// Offset and scale retarget independently but share one hover state and one motion description; use Pressable when
/// press feedback is also required. Pointer-only touch hosts without hover-capable presence keep the rest state.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted hover state by scope position.
/// @param content Content displayed inside the hover surface. The view imposes no intrinsic frame.
/// @param style Lift offset, scale, motion, and pivot. Must satisfy the documented ranges.
/// @return A View whose mounted instance owns the hover presentation state.
/// @throws std::invalid_argument If lift or scale is non-finite, or the scale is not positive.
/// @code
/// HoverLift(Card{...}, {.lift = {0.0F, -6.0F}}).With(Frame{.width = 240.0F, .height = 140.0F});
/// @endcode
View HoverLift(View content, HoverLiftStyle style = {});

} // namespace huxerui::interaction
