#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Paint configuration for HoverHighlight. Defaults provide a subtle brightening on hover and a light dim on press.
struct HoverHighlightStyle {
  /// Overlay color at full hover. Its alpha channel scales with the animated presence.
  Color hover_color = Color::Rgb(255, 255, 255, 0.08F);
  /// Overlay color while pressed; the overlay blends from hover_color toward this color.
  Color pressed_color = Color::Rgb(0, 0, 0, 0.10F);
  /// Additional overlay color while focus is visible.
  Color focus_color = Color::Transparent();
  /// Motion used when the interaction state changes, including the return to rest.
  AnimationSpec motion = TweenSpec{.duration = 0.15, .easing = Easing::EaseOut};
  /// Clip corner radii applied to the overlay's paint clip.
  CornerRadii corner_radii = {};

  bool operator==(const HoverHighlightStyle&) const = default;
};

/// A retained modifier that paints an interaction-driven tint above the content.
class HoverHighlight {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  explicit HoverHighlight(HoverHighlightStyle style = {});

  HoverHighlightStyle style;

  bool operator==(const HoverHighlight&) const = default;
};

} // namespace huxerui::interaction
