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
  /// Motion used when the interaction state changes, including the return to rest.
  AnimationSpec motion = TweenSpec{.duration = 0.15, .easing = Easing::EaseOut};
  /// Clip corner radii applied to the overlay's paint clip. Defaults to the rectangular node bounds.
  CornerRadii corner_radii = {};
  bool operator==(const HoverHighlightStyle&) const = default;
};

/// A retained modifier that paints an interaction-driven tint above the content.
///
/// Apply through View::With(). The modifier observes the node's InteractionState: the overlay fades to
/// hover_color while hovered, blends toward pressed_color while pressed, and fades out at rest or when the node
/// is disabled. Reduced motion resolves the overlay immediately. The overlay is clipped to the node bounds and
/// HoverHighlightStyle::corner_radii.
///
/// The modifier makes the node an input target so inert views can still respond; it does not consume pointer
/// events, and it does not change layout or semantics.
/// @code
/// ListRow{...}.With(HoverHighlight{HoverHighlightStyle{.corner_radii = 10.0F}});
/// @endcode
class HoverHighlight {
public:
  /// Retained extension created by the framework when the modifier is attached through With(). Defined in the
  /// library; consumers never construct it directly.
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  /// @return The framework modifier descriptor; attach the value through With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @param style Highlight paint configuration.
  explicit HoverHighlight(HoverHighlightStyle style = {});

  /// The paint configuration applied by the retained extension.
  HoverHighlightStyle style;
};

} // namespace huxerui::interaction
