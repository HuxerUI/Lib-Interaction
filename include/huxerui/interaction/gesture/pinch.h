#pragma once

#include <huxerui/animation.h>
#include <huxerui/gesture.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Presentation configuration for PinchZoom. The gesture recognizes two or more pointers; translation and
/// rotation updates are ignored, and the content zooms around its center pivot.
struct PinchZoomStyle {
  /// Smallest allowed zoom factor. Must be finite, positive, and at most maximum_scale.
  float minimum_scale = 0.5F;
  /// Largest allowed zoom factor. Must be finite, positive, and at least minimum_scale.
  float maximum_scale = 3.0F;
  /// Motion used while the gesture updates and, without spring back, after it ends.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.08, .easing = Easing::EaseOut};
  /// Motion used to return to rest when spring_back is set.
  AnimationSpec return_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.7F};
  /// Whether releasing the gesture returns the content to its rest scale.
  bool spring_back = false;
  /// Whether holding the keyboard control modifier and scrolling zooms the content on wheel and trackpad hosts.
  /// The zoom step follows the scroll delta multiplicatively; scrolling without the modifier passes through to
  /// the surrounding scroll container untouched.
  bool wheel_zoom = true;
  bool operator==(const PinchZoomStyle&) const = default;
};

/// Composes content that zooms with a two-pointer pinch gesture.
///
/// Each scale update multiplies the current zoom by the gesture's incremental spread change, clamped to the
/// configured range, and the effective target animates without recomposing the content. The zoom persists at
/// the released value unless PinchZoomStyle::spring_back is set. On wheel and trackpad hosts, holding the
/// control modifier and scrolling zooms through the same clamped range (PinchZoomStyle::wheel_zoom), consuming
/// the scroll event only while the modifier is held; pan and rotation updates are ignored in this version
/// because the declarative presentation set has no rotation modifier.
///
/// Call during composition. Each call creates an independent state scope; recomposition with equal arguments
/// preserves the mounted zoom and gesture state by scope position. The widget imposes no intrinsic frame.
/// @param content Content the pinch zooms. The view imposes no intrinsic frame.
/// @param style Zoom range and motions. Must satisfy the documented ranges.
/// @return A View whose mounted instance owns the zoom and gesture state.
/// @throws std::invalid_argument If either scale bound is non-finite or not positive, or minimum_scale exceeds
///     maximum_scale.
/// @code
/// PinchZoom(Photo(), {.minimum_scale = 1.0F, .maximum_scale = 4.0F})
///     .With(Frame{.width = 280.0F, .height = 180.0F});
/// @endcode
View PinchZoom(View content, PinchZoomStyle style = {});

} // namespace huxerui::interaction
