#pragma once

#include <huxerui/animation.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/gesture.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <memory>
#include <optional>
#include <utility>

namespace huxerui::interaction {
namespace detail {
struct PanZoomControllerState;
} // namespace detail

/// Copyable handle for reading or programmatically driving a mounted PanZoom or PinchZoom surface.
/// Default-constructed handles are unbound and safe outside an Application runtime; create bound handles with UsePanZoomController().
class PanZoomController {
public:
  PanZoomController() noexcept = default;

  [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(state_); }

  /// Current accumulated pan offset in logical pixels (subscribes active composition).
  [[nodiscard]] Point Pan() const;
  /// Current clamped zoom factor (subscribes active composition).
  [[nodiscard]] float Scale() const;
  /// Current accumulated rotation in degrees (subscribes active composition).
  [[nodiscard]] float Rotation() const;

  /// Clamps and applies `next_scale` using the mounted widget's range.
  void ZoomTo(float next_scale) const;
  /// Multiplies the current scale by `factor` and clamps to the mounted widget's range.
  void ZoomBy(float factor) const;
  /// Applies `next_pan`, clamped to the mounted widget's pan_bounds when configured.
  void PanTo(Point next_pan) const;
  /// Resets pan, scale, and rotation back to rest (`{0, 0}`, `1.0`, `0.0`).
  void Reset() const;

  bool operator==(const PanZoomController&) const = default;

private:
  explicit PanZoomController(std::shared_ptr<detail::PanZoomControllerState> state) : state_(std::move(state)) {}

  std::shared_ptr<detail::PanZoomControllerState> state_;

  friend PanZoomController UsePanZoomController(float initial_scale);
  friend View PanZoom(View content, struct PanZoomStyle style);
};

/// Remembers a PanZoomController in the current composition scope.
/// @throws std::invalid_argument If `initial_scale` is non-finite or not positive.
PanZoomController UsePanZoomController(float initial_scale = 1.0F);

/// Transform configuration for PanZoom.
struct PanZoomStyle {
  /// Smallest allowed accumulated scale. Must be finite and positive.
  float minimum_scale = 0.5F;
  /// Largest allowed accumulated scale. Must be finite, positive, and >= minimum_scale.
  float maximum_scale = 4.0F;
  /// Whether two-pointer translation updates are applied.
  bool pan = true;
  /// Whether two-pointer rotation updates are applied.
  bool rotation = true;
  /// Whether releasing or canceling the gesture springs pan, scale, and rotation back to rest.
  bool spring_back = false;
  /// Optional absolute pan limit on each axis. Components must be finite and positive.
  std::optional<Point> pan_bounds = std::nullopt;
  /// Whether a double tap resets pan, scale, and rotation.
  bool double_tap_reset = false;
  /// Rotation snap step in degrees; zero disables snapping. Must be finite and non-negative.
  float rotation_snap = 0.0F;
  /// Whether Ctrl + wheel/trackpad updates zoom the content.
  bool wheel_zoom = true;
  /// Exponential zoom response to wheel delta. Must be finite and non-negative.
  float wheel_sensitivity = 0.15F;
  /// Pivot used by scale and rotation. Defaults to the content center.
  TransformOrigin origin = {};
  /// Motion used while the transform gesture owns the pointer.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.04, .easing = Easing::Linear};
  /// Motion used for spring_back return and programmatic controller updates.
  AnimationSpec return_motion = SpringSpec{.stiffness = 260.0F, .damping_ratio = 0.85F};
  /// Optional controller connected to this PanZoom instance.
  PanZoomController controller{};

  bool operator==(const PanZoomStyle&) const = default;
};

/// Scale-only preset configuration for PinchZoom (delegates to PanZoom with pan and rotation disabled).
struct PinchZoomStyle {
  /// Smallest allowed zoom factor. Must be finite, positive, and <= maximum_scale.
  float minimum_scale = 0.5F;
  /// Largest allowed zoom factor. Must be finite, positive, and >= minimum_scale.
  float maximum_scale = 3.0F;
  /// Motion used while the gesture updates.
  AnimationSpec follow_motion = TweenSpec{.duration = 0.08, .easing = Easing::EaseOut};
  /// Motion used to return to rest when spring_back is set.
  AnimationSpec return_motion = SpringSpec{.stiffness = 200.0F, .damping_ratio = 0.7F};
  /// Whether releasing the gesture returns the content to its rest scale.
  bool spring_back = false;
  /// Whether holding Ctrl and scrolling zooms the content.
  bool wheel_zoom = true;
  /// Optional controller connected to this PinchZoom instance.
  PanZoomController controller{};

  bool operator==(const PinchZoomStyle&) const = default;
};

/// Applies two-pointer pan, scale, and rotation plus a Ctrl+wheel zoom fallback.
/// @throws std::invalid_argument If scale bounds, wheel sensitivity, pan bounds, or rotation snap are invalid.
View PanZoom(View content, PanZoomStyle style = {});

/// Pipeable adaptor for PanZoom.
[[nodiscard]] inline ViewAdaptor PanZoom(PanZoomStyle style = {}) {
  return ViewAdaptor([style](View content) { return PanZoom(std::move(content), style); });
}

/// Scale-only two-pointer pinch and Ctrl+wheel zoom convenience built on PanZoom.
View PinchZoom(View content, PinchZoomStyle style = {});

/// Pipeable adaptor for PinchZoom.
[[nodiscard]] inline ViewAdaptor PinchZoom(PinchZoomStyle style = {}) {
  return ViewAdaptor([style](View content) { return PinchZoom(std::move(content), style); });
}

} // namespace huxerui::interaction
