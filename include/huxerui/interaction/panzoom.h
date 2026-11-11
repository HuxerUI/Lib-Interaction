#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/gesture.h>
#include <huxerui/view.h>

#include <optional>

namespace huxerui::interaction {

/// Transform configuration for PanZoom.
struct PanZoomStyle {
  /// Smallest allowed accumulated scale. Must be finite and positive.
  float minimum_scale = 0.5F;
  /// Largest allowed accumulated scale. Must be finite, positive, and not smaller than minimum_scale.
  float maximum_scale = 4.0F;
  /// Whether two-pointer rotation updates are applied and painted.
  bool rotation = true;
  /// Whether a completed or canceled gesture returns pan, scale, and rotation to rest.
  bool spring_back = false;
  /// Optional absolute pan limit on each axis. Components must be finite and positive.
  std::optional<Point> pan_bounds = std::nullopt;
  /// Whether a double tap resets pan, scale, and rotation.
  bool double_tap_reset = false;
  /// Rotation snap step in degrees; zero disables snapping. Must be finite and non-negative.
  float rotation_snap = 0.0F;
  /// Whether Ctrl + wheel/trackpad updates zoom the content. Other scroll input passes through.
  bool wheel_zoom = true;
  /// Exponential zoom response to wheel delta, in inverse logical pixels. Must be finite and non-negative.
  float wheel_sensitivity = 0.15F;
  /// Pivot used by scale and rotation. Defaults to the content center.
  TransformOrigin origin = {};
  /// Motion used while the transform gesture owns the pointer.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used for spring_back return.
  AnimationSpec return_motion = SpringSpec{.stiffness = 260.0F, .damping_ratio = 0.85F};
  bool operator==(const PanZoomStyle&) const = default;
};

/// Applies two-pointer pan, scale, and rotation plus a Ctrl+wheel zoom fallback.
///
/// The widget accumulates TransformGesture deltas into authoritative pan/scale/rotation values; scale is clamped to
/// the configured range, and inertia is not added. Wheel updates are consumed only while the control modifier is held.
/// The content follows the gesture with follow_motion and optionally springs back with return_motion.
/// @param content Content to transform. The widget imposes no intrinsic frame.
/// @param style Scale range, rotation, wheel, pivot, and motions.
/// @return A View whose mounted instance owns accumulated transform state.
/// @throws std::invalid_argument If the scale range or wheel sensitivity is invalid.
View PanZoom(View content, PanZoomStyle style = {});

} // namespace huxerui::interaction
