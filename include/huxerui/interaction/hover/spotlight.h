#pragma once

#include <huxerui/animation.h>
#include <huxerui/color.h>
#include <huxerui/event.h>
#include <huxerui/geometry.h>
#include <huxerui/view.h>

namespace huxerui::interaction {

/// Paint configuration for Spotlight. The glow is a pointer-following radial gradient drawn behind the content.
struct SpotlightStyle {
  /// Glow color at the gradient center. Its alpha channel is scaled by the animated presence.
  Color color = Color::Rgb(96, 144, 255, 0.22F);
  /// Glow diameter in logical pixels. Must be finite and positive.
  float diameter = 240.0F;
  /// Seconds of the presence fade on hover enter and leave. Must be finite and non-negative.
  double fade = 0.18;
  /// Clip corner radii applied to the glow's paint clip. Defaults to the rectangular node bounds.
  CornerRadii corner_radii = {};
  bool operator==(const SpotlightStyle&) const = default;
};

/// A retained modifier that paints a soft radial glow following the hover-capable pointer.
///
/// Apply through View::With(). The modifier tracks hover movement through the extension's hover hit-test and
/// repaints only the sequence behind node content; no recomposition, layout, or application state is involved.
/// Presence fades in on enter and out on leave using the configured fade duration; reduced motion resolves the
/// fade immediately. The glow is clipped to the node bounds and SpotlightStyle::corner_radii.
///
/// The modifier paints only; it does not consume pointer events or change hit testing, layout, or semantics.
/// @code
/// Card{...}.With(Spotlight{SpotlightStyle{.diameter = 180.0F}});
/// @endcode
class Spotlight {
public:
  /// Retained extension created by the framework when the modifier is attached through With(). Defined in the
  /// library; consumers never construct it directly.
  class Extension;

  /// Returns the retained-modifier descriptor used by View::With().
  /// @return The framework modifier descriptor; attach the value through With().
  static const detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @param style Spotlight paint configuration.
  /// @throws std::invalid_argument If diameter is non-finite or not positive, or fade is non-finite or negative.
  explicit Spotlight(SpotlightStyle style = {});

  /// The paint configuration applied by the retained extension.
  SpotlightStyle style;
};

} // namespace huxerui::interaction
