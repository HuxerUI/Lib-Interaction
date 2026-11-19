#pragma once

#include <huxerui/animation.h>
#include <huxerui/geometry.h>
#include <huxerui/interaction/pipe.h>
#include <huxerui/view.h>

#include <utility>

namespace huxerui::interaction {

/// Configuration for layout-driven Expandable transitions.
struct ExpandableStyle {
  /// Axis whose measured extent collapses toward zero (`Vertical` animates height; `Horizontal` animates width).
  Axis axis = Axis::Vertical;
  /// Timing curve or spring used when `expanded` changes.
  AnimationSpec motion = TweenSpec{.duration = 0.22, .easing = Easing::EaseInOut};
  /// Cross-fades child opacity proportionally to layout expansion progress when true.
  bool fade = true;
  /// Excludes the collapsed child from layout participation (hit-testing, focus, semantics, paint) once settled closed.
  bool exclude_collapsed_input = true;

  bool operator==(const ExpandableStyle&) const = default;
};

/// Smoothly animates a View's measured extent between collapsed zero size and its natural measured size, clipping
/// overflow and excluding collapsed children from input and layout participation when settled closed.
View Expandable(View content, bool expanded, ExpandableStyle style = {});

/// Pipeable adaptor for Expandable.
[[nodiscard]] inline ViewAdaptor Expandable(bool expanded, ExpandableStyle style = {}) {
  return ViewAdaptor([expanded, style](View content) { return Expandable(std::move(content), expanded, style); });
}

} // namespace huxerui::interaction
