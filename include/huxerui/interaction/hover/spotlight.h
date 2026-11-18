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
  /// Clip corner radii applied to the glow's paint clip.
  CornerRadii corner_radii = {};

  bool operator==(const SpotlightStyle&) const = default;
};

/// A retained modifier that paints a soft radial glow following the hover-capable pointer.
class Spotlight {
public:
  class Extension;

  static const huxerui::detail::ModifierDescriptor& Descriptor();

  /// Validates and stores the paint configuration.
  /// @throws std::invalid_argument If diameter is non-finite or not positive, or fade is non-finite or negative.
  explicit Spotlight(SpotlightStyle style = {});

  SpotlightStyle style;

  bool operator==(const Spotlight&) const = default;
};

} // namespace huxerui::interaction
