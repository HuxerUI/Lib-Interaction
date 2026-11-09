#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>
#include <functional>

namespace huxerui::interaction {

/// Motion configuration for Pressable. Defaults produce a subtle hover emphasis and a light press-in.
/// The style controls only presentation; activation is configured separately through the on_click callback.
struct PressableStyle {
  /// Scale applied while a hover-capable pointer targets the content. Rest state is 1.0.
  float hovered_scale = 1.03F;
  /// Scale applied while a primary pointer press is owned by the content. Rest state is 1.0.
  float pressed_scale = 0.97F;
  /// Motion used when the effective scale target changes, including the return to rest.
  AnimationSpec animation = TweenSpec{.duration = 0.16, .easing = Easing::EaseOut};
  /// Pivot used by the scale transform. Defaults to the center of the content.
  TransformOrigin origin = {};
  /// Cursor shown over the content. Default keeps the platform cursor.
  PointerCursorKind cursor = PointerCursorKind::Hand;
  bool operator==(const PressableStyle&) const = default;
};

/// Composes content that responds to hover and press with an animated scale around one pivot.
///
/// Hover is tracked through ViewEvents::Hover; press is tracked through ViewEvents::Pointer down, up, and cancel
/// phases. The effective scale target is 1.0 at rest, hovered_scale while hovered, and pressed_scale while pressed;
/// target changes animate with the configured motion without recomposing the content. Keyboard activation and
/// disabled-state recovery are not provided in this version; combine OnClick and Enabled at the call site as needed.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted hover and press state by scope position.
/// @param content Content displayed inside the interactive surface. The view imposes no intrinsic frame.
/// @param style Scale targets, motion, and pivot. Must satisfy the documented ranges.
/// @param on_click Optional activation callback invoked by the built-in Click event, or an empty function to
///     compose a purely visual pressable surface.
/// @return A View whose mounted instance owns the hover and press presentation state.
/// @throws std::invalid_argument If either scale is non-finite or not positive.
/// @code
/// Pressable(Card{...}, {.hovered_scale = 1.02F, .animation = SpringSpec{}})
///     .With(Frame{.width = 280.0F, .height = 160.0F});
/// @endcode
View Pressable(View content, PressableStyle style = {}, std::function<void()> on_click = {});

} // namespace huxerui::interaction
