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

/// Live viewport reveal telemetry passed to headless ScrollReveal builders.
struct ScrollRevealSnapshot {
  /// Effective revealed flag driving animations or content visibility (`latched true` when `once == true`).
  bool visible = false;
  /// True while the item's live intersection ratio with the area viewport meets `threshold`.
  bool in_viewport = false;
  /// Current intersection ratio in `[0, 1]` along the scroll axis.
  float visibility_ratio = 0.0F;

  bool operator==(const ScrollRevealSnapshot&) const = default;
};

/// Viewport tracking configuration for ScrollRevealArea.
struct ScrollRevealAreaStyle {
  /// Optional ScrollController bound to the scrollable container for zero-lag offset awareness.
  std::optional<ScrollController> scroll_controller{};
  /// Scroll axis along which descendant ScrollReveal items compute intersection ratios.
  Axis axis = Axis::Vertical;
  /// Expansion (positive) or inset (negative) around viewport bounds in logical pixels. Must be finite.
  float viewport_margin = 0.0F;

  bool operator==(const ScrollRevealAreaStyle&) const = default;
};

/// Reveal threshold, stagger delay, and transition configuration for ScrollReveal.
struct ScrollRevealStyle {
  /// Minimum intersection ratio in `[0, 1]` required inside the viewport before triggering reveal.
  /// Zero reveals as soon as any positive overlap exists.
  float threshold = 0.15F;
  /// When true, stays `visible == true` once revealed even after scrolling back out of the viewport.
  bool once = true;
  /// Stagger delay in seconds after crossing `threshold` before setting `visible = true`. Must be finite and >= 0.
  double delay = 0.0;
  /// Starting opacity while hidden (wrapped overload). Must be in `[0, 1]`.
  float initial_opacity = 0.0F;
  /// Starting translation offset while hidden (wrapped overload). Must be finite.
  Point initial_offset{0.0F, 16.0F};
  /// Starting uniform scale while hidden (wrapped overload). Must be finite and positive.
  float initial_scale = 0.96F;
  /// Scale pivot used by the wrapped overload. Must have finite coordinates.
  TransformOrigin origin{};
  /// Animation timing or spring curve driving wrapped Opacity, Offset, and Scale reveals.
  AnimationSpec motion = TweenSpec{.duration = 0.28, .easing = Easing::EaseOut};

  bool operator==(const ScrollRevealStyle&) const = default;
};

/// Registers a scrollable viewport ancestor so descendant ScrollReveal views trigger when scrolled into view.
/// @throws std::invalid_argument If `style.viewport_margin` is non-finite.
View ScrollRevealArea(View scrollable_content, ScrollRevealAreaStyle style = {});

/// Pipeable adaptor for ScrollRevealArea.
[[nodiscard]] inline ViewAdaptor ScrollRevealArea(ScrollRevealAreaStyle style = {}) {
  return ViewAdaptor([style = std::move(style)](View scrollable_content) {
    return ScrollRevealArea(std::move(scrollable_content), style);
  });
}

/// Headless scroll-reveal primitive delegating presentation to `builder(ScrollRevealSnapshot)`.
/// @throws std::invalid_argument If `builder` is empty or `style` contains invalid bounds.
View ScrollReveal(std::function<View(ScrollRevealSnapshot)> builder, ScrollRevealStyle style = {},
                  std::function<void(bool visible)> on_visibility_changed = {});

/// Wraps `content` in a stationary layout probe that animates Opacity, Offset, and Scale when scrolled into view.
/// @throws std::invalid_argument If `style` contains invalid bounds.
View ScrollReveal(View content, ScrollRevealStyle style = {},
                  std::function<void(bool visible)> on_visibility_changed = {});

/// Pipeable adaptor for ScrollReveal.
[[nodiscard]] inline ViewAdaptor ScrollReveal(ScrollRevealStyle style = {},
                                              std::function<void(bool visible)> on_visibility_changed = {}) {
  return ViewAdaptor([style = std::move(style), on_visibility_changed = std::move(on_visibility_changed)](
                         View content) { return ScrollReveal(std::move(content), style, on_visibility_changed); });
}

} // namespace huxerui::interaction
