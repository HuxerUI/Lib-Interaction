#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/scroll.h>
#include <huxerui/view.h>

#include <functional>
#include <optional>
#include <utility>

namespace huxerui::interaction {

/// Live viewport/scroll parallax telemetry passed to headless ScrollParallax builders.
struct ScrollParallaxSnapshot {
  /// Normalized position along the scroll axis in `[-1, 1]` (`-1` exiting leading edge, `0` centered,
  /// `+1` entering trailing edge). In standalone ScrollController mode, equals the clamped shift ratio in `[-1, 1]`.
  float progress = 0.0F;
  /// Viewport intersection ratio along the scroll axis in `[0, 1]` (`1.0` in standalone mode).
  float visibility = 1.0F;
  /// Computed parallax translation in logical pixels (zeroed when `respect_reduced_motion && reduced_motion`).
  Point offset{};
  /// Computed scale multiplier centered around rest `1.0` (`1.0` when suppressed by reduced motion).
  float scale = 1.0F;
  /// Computed opacity multiplier in `[edge_opacity, 1.0]` (`1.0` when suppressed by reduced motion).
  float opacity = 1.0F;
  /// True when the active window reports `FrameInfo::reduced_motion`.
  bool reduced_motion = false;

  bool operator==(const ScrollParallaxSnapshot&) const = default;
};

/// Viewport tracking configuration for ScrollParallaxArea.
struct ScrollParallaxAreaStyle {
  /// Optional ScrollController bound to the scrollable viewport for zero-lag scroll updates.
  std::optional<ScrollController> scroll_controller{};
  /// Axis along which descendant ScrollParallax items compute normalized viewport progress.
  Axis axis = Axis::Vertical;

  bool operator==(const ScrollParallaxAreaStyle&) const = default;
};

/// Motion and clamping configuration for ScrollParallax.
struct ScrollParallaxStyle {
  /// Optional ScrollController used in standalone mode outside a ScrollParallaxArea.
  std::optional<ScrollController> scroll_controller{};
  /// Scroll axis for standalone mode (overridden by an enclosing ScrollParallaxArea's axis when mounted in one).
  Axis axis = Axis::Vertical;
  /// Parallax multiplier applied to normalized progress (or scroll offset in standalone mode). Must be finite.
  float speed = 0.35F;
  /// Maximum translation magnitude along the scroll axis in logical pixels. Must be finite and >= 0.
  float maximum_offset = 32.0F;
  /// Scale multiplier peaked at viewport center (`progress == 0`), easing back to `1.0` at edges. Must be > 0.
  float center_scale = 1.0F;
  /// Minimum opacity applied at viewport edges (`|progress| == 1`), easing to `1.0` at center. Must be in `[0, 1]`.
  float edge_opacity = 1.0F;
  /// Transform pivot used by the wrapped scale overload. Defaults to content center.
  TransformOrigin origin = {};
  /// When true, clips translated/scaled child content to the outer stationary layout bounds.
  bool clip_to_bounds = false;
  /// When true, honors `FrameInfo::reduced_motion` by suppressing parallax offset, scale, and opacity fades.
  bool respect_reduced_motion = true;

  bool operator==(const ScrollParallaxStyle&) const = default;
};

/// Registers a scrollable viewport ancestor so descendant ScrollParallax items compute normalized viewport progress
/// and visibility relative to the viewport bounds without geometry feedback loops.
View ScrollParallaxArea(View scrollable_content, ScrollParallaxAreaStyle style = {});

/// Pipeable adaptor for ScrollParallaxArea.
[[nodiscard]] inline ViewAdaptor ScrollParallaxArea(ScrollParallaxAreaStyle style = {}) {
  return ViewAdaptor([style = std::move(style)](View scrollable_content) {
    return ScrollParallaxArea(std::move(scrollable_content), style);
  });
}

/// Headless scroll-driven parallax builder receiving live ScrollParallaxSnapshot telemetry.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds.
View ScrollParallax(std::function<View(ScrollParallaxSnapshot)> builder, ScrollParallaxStyle style = {});

/// Wraps `content` in a stationary outer layout node while applying scroll parallax Offset, Scale, and Opacity
/// inside it (optionally clipped when `style.clip_to_bounds` is true).
/// @throws std::invalid_argument If `style` contains invalid bounds.
View ScrollParallax(View content, ScrollParallaxStyle style = {});

/// Pipeable adaptor for ScrollParallax.
[[nodiscard]] inline ViewAdaptor ScrollParallax(ScrollParallaxStyle style = {}) {
  return ViewAdaptor([style = std::move(style)](View content) {
    return ScrollParallax(std::move(content), style);
  });
}

} // namespace huxerui::interaction
